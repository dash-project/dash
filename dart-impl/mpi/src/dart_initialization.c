/**
 * \file dart_initialization.c
 *
 *  Implementations of the dart init and exit operations.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mpi.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/mpi/dart_mpi_util.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/mpi/dart_locality_priv.h>
#include <dash/dart/mpi/dart_segment.h>

#define DART_LOCAL_ALLOC_SIZE (1024UL*1024*16)

/* Point to the base address of memory region for local allocation. */
static int _init_by_dart = 0;
static int _dart_initialized = 0;

static
dart_ret_t create_local_alloc(dart_team_data_t *team_data)
{
  dart_localpool = dart_buddy_new(DART_LOCAL_ALLOC_SIZE);
  MPI_Win dart_sharedmem_win_local_alloc = MPI_WIN_NULL;
  char* *dart_sharedmem_local_baseptr_set = NULL;
  MPI_Info win_info;
  MPI_Info_create(&win_info);
  MPI_Info_set(win_info, "same_size", "true");
  MPI_Info_set(win_info, "same_disp_unit", "true");

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

  DART_LOG_DEBUG("dart_init: Shared memory enabled");
  dart_allocate_shared_comm(team_data);
  MPI_Comm sharedmem_comm = team_data->sharedmem_comm;

  if (sharedmem_comm != MPI_COMM_NULL) {
    DART_LOG_DEBUG("dart_init: MPI_Win_allocate_shared(nbytes:%d)",
                   DART_LOCAL_ALLOC_SIZE);
    MPI_Info_set(win_info, "alloc_shared_noncontig", "true");
    /* Reserve a free shared memory block for non-collective
     * global memory allocation. */
    int ret = MPI_Win_allocate_shared(
                DART_LOCAL_ALLOC_SIZE,
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


    DART_LOG_DEBUG("dart_init: MPI_Win_allocate_shared completed");

    int sharedmem_unitid;
    MPI_Comm_size(sharedmem_comm,
      &(team_data->sharedmem_nodesize));
    MPI_Comm_rank(sharedmem_comm, &sharedmem_unitid);
    dart_sharedmem_local_baseptr_set = malloc(
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
  /* Create a single global win object for dart local
   * allocation based on the above allocated shared memory.
   *
   * Return in dart_win_local_alloc. */
  MPI_Win_create(
    dart_mempool_localalloc,
    DART_LOCAL_ALLOC_SIZE,
    sizeof(char),
    win_info,
    DART_COMM_WORLD,
    &dart_win_local_alloc);
#else
  MPI_Win_allocate(
    DART_LOCAL_ALLOC_SIZE, sizeof(char),
    win_info, DART_COMM_WORLD,
    &dart_mempool_localalloc,
    &dart_win_local_alloc);
#endif

  MPI_Info_free(&win_info);

  /* Start an access epoch on dart_win_local_alloc, and later
   * on all the units can access the memory region allocated
   * by the local allocation function through
   * dart_win_local_alloc.
   *
   * NOTE: We use MPI_MODE_NOCHECK since there will be no
   * conflicting locks at all
   */
  MPI_Win_lock_all(MPI_MODE_NOCHECK, dart_win_local_alloc);


  /* put the localalloc in the segment table */
  dart_segment_info_t *segment = dart_segment_alloc(
                                &team_data->segdata, DART_SEGMENT_LOCAL_ALLOC);
  segment->flags       = 1;
  segment->segid       = 0;
  segment->size        = DART_LOCAL_ALLOC_SIZE;
  segment->baseptr     = dart_sharedmem_local_baseptr_set;
  segment->win         = dart_win_local_alloc;
  segment->shmwin      = dart_sharedmem_win_local_alloc;
  segment->selfbaseptr = dart_mempool_localalloc;
  // addressing in this window is relative, no need to exchange displacements
  segment->disp        = calloc(team_data->size, sizeof(MPI_Aint));
  segment->is_dynamic  = false;

  dart__mpi__check_memory_model(segment);

  return DART_OK;
}

static
dart_ret_t do_init()
{
  /* Initialize the teamlist. */
  dart_adapt_teamlist_init();

  dart_next_availteamid = DART_TEAM_ALL;

  if (MPI_Comm_dup(MPI_COMM_WORLD, &dart_comm_world) != MPI_SUCCESS) {
    DART_LOG_ERROR("Failed to duplicate MPI_COMM_WORLD");
    return DART_ERR_OTHER;
  }

  dart_ret_t ret = dart_adapt_teamlist_alloc(DART_TEAM_ALL);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_adapt_teamlist_alloc failed");
    return DART_ERR_OTHER;
  }

  if (dart__mpi__datatype_init() != DART_OK) {
    return DART_ERR_OTHER;
  }

  if (dart__mpi__op_init() != DART_OK) {
    return DART_ERR_OTHER;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(DART_TEAM_ALL);

  /* Create a global translation table for all
   * the collective global memory segments */
  dart_segment_init(&team_data->segdata, DART_TEAM_ALL);

  dart_next_availteamid++;

  team_data->comm = DART_COMM_WORLD;

  MPI_Comm_rank(team_data->comm, &team_data->unitid);
  MPI_Comm_size(team_data->comm, &team_data->size);

  ret = create_local_alloc(team_data);
  if (ret != DART_OK) {
    return ret;
  }

  /* Create a dynamic win object for all the dart collective
   * allocation based on MPI_COMM_WORLD. Return in win. */
  MPI_Win win;
  MPI_Win_create_dynamic(
    MPI_INFO_NULL, DART_COMM_WORLD, &win);
  team_data->window = win;

  /* Start an access epoch on win, and later on all the units
   * can access the attached memory region allocated by the
   * collective allocation function through win.
   *
   * NOTE: We use MPI_MODE_NOCHECK since there will be no
   * conflicting locks at all
   */
  MPI_Win_lock_all(MPI_MODE_NOCHECK, win);

  DART_LOG_DEBUG("dart_init: communication backend initialization finished");

  _dart_initialized = 1;

  dart__mpi__locality_init();

  _dart_initialized = 2;

  DART_LOG_DEBUG("dart_init > initialization finished");
  return DART_OK;
}

dart_ret_t dart_init(
  int*    argc,
  char*** argv)
{
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
    MPI_Init(argc, argv);
  }

  return do_init();
}


dart_ret_t dart_init_thread(
  int*                  argc,
  char***               argv,
  dart_thread_support_level_t * provided)
{
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

  int thread_provided = MPI_THREAD_SINGLE;
  if (!mpi_initialized) {
    _init_by_dart = 1;
    DART_LOG_DEBUG("dart_init: MPI_Init");
#if defined(DART_ENABLE_THREADSUPPORT)
    int thread_required = MPI_THREAD_MULTIPLE;
    MPI_Init_thread(argc, argv, thread_required, &thread_provided);
    DART_LOG_DEBUG("MPI_Init_thread provided = %i", thread_provided);
  } else {
    MPI_Query_thread(&thread_provided);
    DART_LOG_DEBUG("MPI_Query_thread provided = %i", thread_provided);
  }
#else
    MPI_Init(argc, argv);
  }
#endif // DART_ENABLE_THREADSUPPORT

  *provided = (thread_provided == MPI_THREAD_MULTIPLE) ?
                DART_THREAD_MULTIPLE :
                DART_THREAD_SINGLE;
  DART_LOG_DEBUG("dart_init_thread >> thread support enabled: %s",
            (*provided == DART_THREAD_MULTIPLE) ? "yes" : "no");

  return do_init();
}


dart_ret_t dart_exit()
{
  if (!_dart_initialized) {
    DART_LOG_ERROR("dart_exit(): DART has not been initialized");
    return DART_ERR_OTHER;
  }
  dart_global_unit_t unitid;
  dart_myid(&unitid);

  dart__mpi__locality_finalize();

  _dart_initialized = 0;

  DART_LOG_DEBUG("%2d: dart_exit()", unitid.id);
  dart_team_data_t *team_data = dart_adapt_teamlist_get(DART_TEAM_ALL);
  if (team_data == NULL) {
    DART_LOG_ERROR("%2d: dart_exit: dart_adapt_teamlist_convert failed",
                   unitid.id);
    return DART_ERR_OTHER;
  }

  dart_segment_info_t *seginfo = dart_segment_get_info(&team_data->segdata, 0);

  if (MPI_Win_unlock_all(team_data->window) != MPI_SUCCESS) {
    DART_LOG_ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid.id);
    return DART_ERR_OTHER;
  }
  /* End the shared access epoch in dart_win_local_alloc. */
  if (MPI_Win_unlock_all(seginfo->win) != MPI_SUCCESS) {
    DART_LOG_ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid.id);
    return DART_ERR_OTHER;
  }

  /* -- Free up all the resources for dart programme -- */
  MPI_Win_free(&seginfo->win);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  /* Has MPI shared windows: */
  MPI_Win_free(&seginfo->shmwin);
  MPI_Comm_free(&(team_data->sharedmem_comm));
#endif
  MPI_Win_free(&team_data->window);

  dart_segment_fini(&team_data->segdata);
  dart_buddy_delete(dart_localpool);
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
//  free(team_data->sharedmem_tab);
//  free(dart_sharedmem_local_baseptr_set);
#endif

  dart_adapt_teamlist_destroy();

  MPI_Comm_free(&dart_comm_world);

  dart__mpi__datatype_fini();

  dart__mpi__op_fini();

  if (_init_by_dart) {
    DART_LOG_DEBUG("%2d: dart_exit: MPI_Finalize", unitid.id);
    MPI_Finalize();
  }

  DART_LOG_DEBUG("%2d: dart_exit: finalization finished", unitid.id);

  return DART_OK;
}

bool dart_initialized()
{
  return (_dart_initialized > 0);
}


void dart_abort(int errorcode)
{
  DART_LOG_INFO("dart_abort: aborting DART run with error code %i", errorcode);
  MPI_Abort(MPI_COMM_WORLD, errorcode);
  /* just in case MPI_Abort does not abort */
  abort();
}
