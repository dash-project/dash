/** @file dart_initialization.c
 *  @date 25 Aug 2014
 *  @brief Implementations of the dart init and exit operations.
 */

#include <stdio.h>
#include <mpi.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_translation.h>
#include <dash/dart/mpi/dart_globmem_priv.h>

#define DART_BUDDY_ORDER 24

/* Global objects for dart memory management */

/* Point to the base address of memory region for local allocation. */
char* dart_mempool_localalloc;
#ifdef SHAREDMEM_ENABLE
char**dart_sharedmem_local_baseptr_set;
#endif
/* Help to do memory management work for local allocation/free */
struct dart_buddy* dart_localpool;
int init_by_dart     = 0;
int dart_initialized = 0;

dart_ret_t dart_init(
  int*    argc,
  char*** argv)
{
  if (dart_initialized) {
    ERROR("dart_init(): DART is already initialized");
    return DART_ERR_OTHER;
  }
  LOG("dart_init()");
  dart_initialized = 1;

	int initialized;
	if (MPI_Initialized(&initialized) != MPI_SUCCESS) {
    ERROR("dart_init(): MPI_Initialized failed");
    return DART_ERR_OTHER;
  }
	if (!initialized) {
		init_by_dart = 1;
    DEBUG("dart_init: MPI_Init");
		MPI_Init(argc, argv);
	}

	int rank, size;
	uint16_t index;
	MPI_Win win;
	
#ifdef SHAREDMEM_ENABLE
  DEBUG("dart_init: Shared memory enabled");
	MPI_Info win_info;
	MPI_Info_create (&win_info);
	MPI_Info_set (win_info, "alloc_shared_noncontig", "true");
#endif
	
	/* Initialize the teamlist. */
	dart_adapt_teamlist_init();

	dart_next_availteamid = DART_TEAM_ALL;
	dart_memid = 1;
	dart_registermemid = -1;

	int result = dart_adapt_teamlist_alloc(
                 DART_TEAM_ALL,
                 &index);
	if (result == -1) {
    ERROR("dart_adapt_teamlist_alloc failed");
		return DART_ERR_OTHER;
	}
	dart_teams[index] = MPI_COMM_WORLD;
	
	dart_next_availteamid++;

	/* Create a global translation table for all
   * the collective global memory */
	dart_adapt_transtable_create();

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);	
	dart_localpool = dart_buddy_new (DART_BUDDY_ORDER);
#ifdef SHAREDMEM_ENABLE
	int i;

	/* Generate separated intra-node communicators and
   * reserve necessary resources for dart programm */
	MPI_Comm sharedmem_comm;

	/* Splits the communicator into subcommunicators,
   * each of which can create a shared memory region */
	MPI_Comm_split_type(
    MPI_COMM_WORLD,
    MPI_COMM_TYPE_SHARED,
    1,
    MPI_INFO_NULL,
    &sharedmem_comm);
	
	dart_sharedmem_comm_list[index] = sharedmem_comm;

	MPI_Group group_all, sharedmem_group;
	char *baseptr;
	int flag;

	if (sharedmem_comm != MPI_COMM_NULL) {
		int sharedmem_unitid;
		/* Reserve a free shared memory block for non-collective
     * global memory allocation. */	
		MPI_Win_allocate_shared(
      DART_MAX_LENGTH,
      sizeof(char),
      win_info,
      sharedmem_comm,
			&(dart_mempool_localalloc),
      &dart_sharedmem_win_local_alloc);

		MPI_Comm_size(
      sharedmem_comm,
      &(dart_sharedmemnode_size[index]));
		MPI_Comm_rank(
      sharedmem_comm,
      &sharedmem_unitid);
		dart_sharedmem_local_baseptr_set = (char**)malloc(
        sizeof(char*) * dart_sharedmemnode_size[index]);
		MPI_Aint winseg_size;

		int disp_unit;
		for(i = 0; i < dart_sharedmemnode_size[index]; i++) {
			if (sharedmem_unitid != i){
				MPI_Win_shared_query(
          dart_sharedmem_win_local_alloc,
          i,
          &winseg_size,
          &disp_unit,
          &baseptr);
				dart_sharedmem_local_baseptr_set[i] = baseptr;
      }
			else { 
        dart_sharedmem_local_baseptr_set[i] =
        dart_mempool_localalloc;
      }
		}

		MPI_Comm_group(sharedmem_comm, &sharedmem_group);
		MPI_Comm_group(MPI_COMM_WORLD, &group_all);

		/* The length of this table is set to be the size
     * of DART_TEAM_ALL. */
		dart_sharedmem_table[index] = (int *)malloc (sizeof (int) * size);

		int* dart_unit_mapping = (int *)malloc(
      sizeof (int) * dart_sharedmemnode_size[index]);
		int* sharedmem_ranks   = (int *)malloc(
      sizeof (int) * dart_sharedmemnode_size[index]);

		for (i = 0; i < dart_sharedmemnode_size[index]; i++) {
			sharedmem_ranks[i] = i;
		}
		for (i = 0; i < size; i++) {
			dart_sharedmem_table[index][i] = -1;
		}

		/* Generate the set (dart_unit_mapping) of units with absolute IDs, 
		 * which are located in the same node 
		 */
		MPI_Group_translate_ranks(
      sharedmem_group,
      dart_sharedmemnode_size[index],
			sharedmem_ranks,
      group_all,
      dart_unit_mapping);
	
		/* The non-negative elements in the array 
     * 'dart_sharedmem_table[index]' consist of a serial of units,
     * which are in the same node and they can communicate via
     * shared memory and i is the relative position in the node
     * for unit dart_unit_mapping[i].
		 */
		for (i = 0; i < dart_sharedmemnode_size[index]; i++) {
			dart_sharedmem_table[index][dart_unit_mapping[i]] = i;
		}
		free(sharedmem_ranks);
		free(dart_unit_mapping);
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
		MPI_COMM_WORLD,
    &dart_win_local_alloc);

	/* Create a dynamic win object for all the dart collective
   * allocation based on MPI_COMM_WORLD. Return in win. */
	MPI_Win_create_dynamic(
    MPI_INFO_NULL, MPI_COMM_WORLD, &win);
	dart_win_lists[index] = win;

	/* Start an access epoch on dart_win_local_alloc, and later
   * on all the units can access the memory region allocated
   * by the local allocation function through
   * dart_win_local_alloc. */	
	MPI_Win_lock_all(0, dart_win_local_alloc);

	/* Start an access epoch on win, and later on all the units
   * can access the attached memory region allocated by the
   * collective allocation function through win. */
	MPI_Win_lock_all(0, win);

#ifdef SHAREDMEM_ENABLE
	MPI_Info_free(&win_info);
#endif
	DEBUG("%2d: dart_init: Initialization finished", rank);

	return DART_OK;
}

dart_ret_t dart_exit()
{
  if (!dart_initialized) {
    ERROR("dart_exit(): DART has not been initialized");
    return DART_ERR_OTHER;
  }
  dart_initialized = 1;

	int finalized;
	uint16_t index;
	dart_unit_t unitid;
	dart_myid(&unitid);
 	
	LOG("%2d: dart_exit()", unitid);
	if (dart_adapt_teamlist_convert(DART_TEAM_ALL, &index) == -1) {
    ERROR("%2d: dart_exit: dart_adapt_teamlist_convert failed", unitid);
    return DART_ERR_OTHER;
  }

	if (MPI_Win_unlock_all(dart_win_lists[index]) != MPI_SUCCESS) {
    ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid);
    return DART_ERR_OTHER;
  }
	/* End the shared access epoch in dart_win_local_alloc. */
	if (MPI_Win_unlock_all(dart_win_local_alloc) != MPI_SUCCESS) {
    ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid);
    return DART_ERR_OTHER;
  }
	/* -- Free up all the resources for dart programme -- */
	MPI_Win_free(&dart_win_local_alloc);
#ifdef SHAREDMEM_ENABLE
	MPI_Win_free(&dart_sharedmem_win_local_alloc);
#endif
	MPI_Win_free(&dart_win_lists[index]);
	
	dart_adapt_transtable_destroy();
	dart_buddy_delete(dart_localpool);
#ifdef SHAREDMEM_ENABLE	
	free(dart_sharedmem_table[index]);
	free(dart_sharedmem_local_baseptr_set);
#endif
	dart_adapt_teamlist_destroy ();

	if (init_by_dart) {
    DEBUG("%2d: dart_exit: MPI_Finalize", unitid);
		MPI_Finalize();
  }
	DEBUG("%2d: dart_exit: Finalization finished", unitid);
	return DART_OK;
}


