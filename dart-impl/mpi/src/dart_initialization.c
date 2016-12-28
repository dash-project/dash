/**
 * \file dart_initialization.c
 *
 *  Implementations of the dart init and exit operations.
 */
#include <stdio.h>
#include <mpi.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/mpi/dart_mpi_util.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_locality_priv.h>
#include <dash/dart/mpi/dart_segment.h>

#define DART_BUDDY_ORDER 24

/* Global objects for dart memory management */

/* Point to the base address of memory region for local allocation. */
char* dart_mempool_localalloc;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
char**dart_sharedmem_local_baseptr_set;
#endif
/* Help to do memory management work for local allocation/free */
struct dart_buddy  *  dart_localpool;
static int _init_by_dart = 0;
static int _dart_initialized = 0;
static int _provided_thread_support = 0;

dart_ret_t dart_init(
  int*    argc,
  char*** argv)
{
  int      rank;
  int      size;
  uint16_t index;

  if (_dart_initialized) {
    DART_LOG_ERROR("dart_init(): DART is already initialized");
    return DART_ERR_OTHER;
  }
  DART_LOG_DEBUG("dart_init()");

	int mpi_initialized;
	if (MPI_Initialized(&mpi_initialized) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_init(): MPI_Initialized failed");
    return DART_ERR_OTHER;
  }
	if (!mpi_initialized) {
		_init_by_dart = 1;
    DART_LOG_DEBUG("dart_init: MPI_Init");
    // TODO: Introduce DART_THREAD_SUPPORT_LEVEL
		MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &_provided_thread_support);
	}

  /* Initialize the teamlist. */
  dart_adapt_teamlist_init();
  dart_segment_init();

  dart_next_availteamid = DART_TEAM_ALL;

  if (MPI_Comm_dup(MPI_COMM_WORLD, &dart_comm_world) != MPI_SUCCESS) {
    DART_LOG_ERROR("Failed to duplicate MPI_COMM_WORLD");
    return DART_ERR_OTHER;
  }

	int ret = dart_adapt_teamlist_alloc(
                 DART_TEAM_ALL,
                 &index);
	if (ret == -1) {
    DART_LOG_ERROR("dart_adapt_teamlist_alloc failed");
    return DART_ERR_OTHER;
  }

  dart_team_data_t *team_data = &dart_team_data[index];

  /* Create a global translation table for all
   * the collective global memory segments */
  dart_segment_init();
  // Segment ID zero is reserved for non-global memory allocations
  dart_segment_alloc(0, DART_TEAM_ALL);

  DART_LOG_DEBUG("dart_init: dart_adapt_teamlist_alloc completed, index:%d",
                 index);
	dart_next_availteamid++;

  team_data->comm = DART_COMM_WORLD;

  MPI_Comm_rank(DART_COMM_WORLD, &rank);
  MPI_Comm_size(DART_COMM_WORLD, &size);
  dart_localpool = dart_buddy_new(DART_BUDDY_ORDER);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

  DART_LOG_DEBUG("dart_init: Shared memory enabled");
  dart_allocate_shared_comm(team_data);
  MPI_Comm sharedmem_comm = team_data->sharedmem_comm;

	if (sharedmem_comm != MPI_COMM_NULL) {
    DART_LOG_DEBUG("dart_init: MPI_Win_allocate_shared(nbytes:%d)",
                   DART_MAX_LENGTH);
    MPI_Info win_info;
    MPI_Info_create(&win_info);
    MPI_Info_set(win_info, "alloc_shared_noncontig", "true");
		/* Reserve a free shared memory block for non-collective
     * global memory allocation. */
		ret = MPI_Win_allocate_shared(
                DART_MAX_LENGTH,
                sizeof(char),
                win_info,
                sharedmem_comm,
                &(dart_mempool_localalloc),
                &dart_sharedmem_win_local_alloc);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_init: "
                     "MPI_Win_allocate_shared failed, error %d (%s)",
                     ret, DART__MPI__ERROR_STR(ret));
      return DART_ERR_OTHER;
    }

    MPI_Info_free(&win_info);

    DART_LOG_DEBUG("dart_init: MPI_Win_allocate_shared completed");

		int sharedmem_unitid;
		MPI_Comm_size(
      sharedmem_comm,
      &(team_data->sharedmem_nodesize));
    MPI_Comm_rank(
      sharedmem_comm,
      &sharedmem_unitid);
    dart_sharedmem_local_baseptr_set =
      (char **)malloc(
        sizeof(char *) * team_data->sharedmem_nodesize);

    for (int i = 0; i < team_data->sharedmem_nodesize; i++) {
      if (sharedmem_unitid != i) {
        int        disp_unit;
        char     * baseptr;
        MPI_Aint   winseg_size;
        MPI_Win_shared_query(
          dart_sharedmem_win_local_alloc,
          i,
          &winseg_size,
          &disp_unit,
          &baseptr);
				dart_sharedmem_local_baseptr_set[i] = baseptr;
      }
			else {
        dart_sharedmem_local_baseptr_set[i] = dart_mempool_localalloc;
      }
		}
  }
#else
	MPI_Alloc_mem(
    DART_MAX_LENGTH,
    MPI_INFO_NULL,
    &dart_mempool_localalloc);
#endif
	/* Create a single global win object for dart local
   * allocation based on the aboved allocated shared memory.
	 *
	 * Return in dart_win_local_alloc. */
	MPI_Win_create(
    dart_mempool_localalloc,
    DART_MAX_LENGTH,
    sizeof(char),
    MPI_INFO_NULL,
		DART_COMM_WORLD,
    &dart_win_local_alloc);

	/* Create a dynamic win object for all the dart collective
   * allocation based on MPI_COMM_WORLD. Return in win. */
  MPI_Win win;
	MPI_Win_create_dynamic(
    MPI_INFO_NULL, DART_COMM_WORLD, &win);
  team_data->window = win;

	/* Start an access epoch on dart_win_local_alloc, and later
   * on all the units can access the memory region allocated
   * by the local allocation function through
   * dart_win_local_alloc. */
	MPI_Win_lock_all(0, dart_win_local_alloc);

	/* Start an access epoch on win, and later on all the units
   * can access the attached memory region allocated by the
   * collective allocation function through win. */
	MPI_Win_lock_all(0, win);

	DART_LOG_DEBUG("dart_init: communication backend initialization finished");

  _dart_initialized = 1;

  dart__mpi__locality_init();

  _dart_initialized = 2;

	DART_LOG_DEBUG("dart_init > initialization finished");
	return DART_OK;
}

dart_ret_t dart_exit()
{
  if (!_dart_initialized) {
    DART_LOG_ERROR("dart_exit(): DART has not been initialized");
    return DART_ERR_OTHER;
  }
	uint16_t    index;
	dart_global_unit_t unitid;
	dart_myid(&unitid);

  dart__mpi__locality_finalize();

  _dart_initialized = 0;

	DART_LOG_DEBUG("%2d: dart_exit()", unitid);
	if (dart_adapt_teamlist_convert(DART_TEAM_ALL, &index) == -1) {
    DART_LOG_ERROR("%2d: dart_exit: dart_adapt_teamlist_convert failed",
                   unitid.id);
    return DART_ERR_OTHER;
  }

  dart_team_data_t *team_data = &dart_team_data[index];

  if (MPI_Win_unlock_all(team_data->window) != MPI_SUCCESS) {
    DART_LOG_ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid.id);
    return DART_ERR_OTHER;
  }
	/* End the shared access epoch in dart_win_local_alloc. */
	if (MPI_Win_unlock_all(dart_win_local_alloc) != MPI_SUCCESS) {
    DART_LOG_ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid.id);
    return DART_ERR_OTHER;
  }

	/* -- Free up all the resources for dart programme -- */
	MPI_Win_free(&dart_win_local_alloc);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  /* Has MPI shared windows: */
	MPI_Win_free(&dart_sharedmem_win_local_alloc);
	MPI_Comm_free(&(team_data->sharedmem_comm));
#else
  /* No MPI shared windows: */
  if (dart_mempool_localalloc) {
    MPI_Free_mem(dart_mempool_localalloc);
  }
#endif
  MPI_Win_free(&team_data->window);

  dart_buddy_delete(dart_localpool);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  free(team_data->sharedmem_tab);
  free(dart_sharedmem_local_baseptr_set);
#endif

	dart_adapt_teamlist_destroy();

  dart_segment_fini();

  MPI_Comm_free(&dart_comm_world);

  if (_init_by_dart) {
    DART_LOG_DEBUG("%2d: dart_exit: MPI_Finalize", unitid.id);
		MPI_Finalize();
  }

	DART_LOG_DEBUG("%2d: dart_exit: finalization finished", unitid.id);

	return DART_OK;
}

char dart_initialized()
{
  return _dart_initialized;
}
