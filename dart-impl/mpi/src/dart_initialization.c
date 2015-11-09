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
char**        dart_sharedmem_local_baseptr_set;
MPI_Datatype  data_info_type;
MPI_Comm      user_comm_world;
int           top;
#endif
/* Help to do memory management work for local allocation/free */
struct dart_buddy   * dart_localpool;
int                   _init_by_dart     = 0;
int                   _dart_initialized = 0;

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

	int rank, size;
	uint16_t index;
	MPI_Win win;
	
#ifdef SHAREDMEM_ENABLE
  DART_LOG_DEBUG("dart_init: Shared memory enabled");
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
    DART_LOG_ERROR("dart_adapt_teamlist_alloc failed");
		return DART_ERR_OTHER;
	}
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	dart_realteams[index] = MPI_COMM_WORLD;
#endif
#else
	dart_teams[index] = MPI_COMM_WORLD;
#endif
	
	dart_next_availteamid++;

	/* Create a global translation table for all
   * the collective global memory */
	dart_adapt_transtable_create();

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);	
//	dart_localpool = dart_buddy_new (DART_BUDDY_ORDER);
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
#ifdef PROGRESS_ENABLE
	MPI_Group user_group;
	
	if (sharedmem_comm!=MPI_COMM_NUL){
		int *progress_recv = (int*)malloc (sizeof(int)*size);
		int progress, iter;
		MPI_Group new_group[PROGRESS_NUM], progress_group;
		int *progresshead_node, progress_count, *progress_node;
		progresshead_node = (int*)malloc (sizeof(int) * size);
		progress_node = (int*)malloc (sizeof(int) * size);

		for (iter = 0; iter < PROGRESS_NUM; iter++){
			int progress_rel = PROGRESS_UNIT+iter;
			MPI_Group_translate_ranks (sharedmem_group, 1, &progress_rel, group_all, &progress);
			MPI_Allgather (&progress, 1, MPI_INT, progress_recv, 1, MPI_INT, MPI_COMM_WORLD);

			for (i = 0; i < size; i++) progresshead_node[i]=0;
			for (i = 0; i < size; i++){
				int pp;
				int j;
				pp = progress_recv[i];

				for (j = 0; j <= i; j++)
				{
					if (pp == progress_recv[j]) break;
				}
				progresshead_node[pp] = 1;
			}
			progress_count = 0;
			for (i = 0; i < size; i++){
				if (progresshead_node[i]){
					progress_node[progress_count] = i;
					progress_count ++;
				}
			}
			MPI_Group_incl (group_all, progress_count, progress_node, &new_group[iter]);
		}
		MPI_Group_union (new_group[PROGRESS_UNIT], new_group[PROGRESS_UNIT + 1], &progress_group);
		int subgroup_size;
		MPI_Group_size (progress_group, &subgroup_size);
		MPI_Group_difference (group_all, progress_group, &user_group);

		user_comm_world = MPI_COMM_NULL;
		MPI_Comm_create (MPI_COMM_WOLRD, user_group, &user_comm_world);
		dart_teams[index] = user_comm_world;
		free (progresshead_node);
		free (progress_node);
		free (progress_recv);

		if (user_comm_world != MPI_COMM_NULL){
			top = sharedmem_rank % PROGRESS_NUM;
			MPI_Win_allocate_shared (DART_MAX_LENGHT, sizeof (char), MPI_INFO_NULL, sharedmem_comm,
					&(dart_mempool_localalloc), &dart_sharedmem_win_local_alloc);
		}else{
			MPI_Win_allocate_shared (0, sizeof(char), MPI_INFO_NULL, sharedmem_comm,
					&(dart_mempool_localalloc), &dart_sharedmem_win_local_alloc);
		}
	}
	if (user_comm_world != MPI_COMM_NULL){
		MPI_Win_create (dart_mempool_localalloc, DART_MAX_LENGHT, sizeof(char), MPI_INFO_NULL,
					MPI_COMM_WOLRD, &dart_win_local_alloc);
	}else{
		MPI_Win_create (dart_mempool_localalloc, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &dart_win_local_alloc);
	}
#else

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
      &dart_sharedmem_win_local_alloc);}
#endif
		MPI_Comm_size(
      sharedmem_comm,
      &(dart_sharedmemnode_size[index]));
		MPI_Comm_rank(
      sharedmem_comm,
      &sharedmem_unitid);
		dart_sharedmem_local_baseptr_set = (char**)malloc(
        sizeof(char*) * dart_sharedmemnode_size[index]);
		if (sharedmem_comm!=MPI_COMM_NULL){
		MPI_Aint winseg_size;

		int disp_unit;
#ifdef PROGRESS_ENABLE
		for (i = PROGRESS_NUM; i < dart_sharedmemnode_size[index]; i++)
#else
		for(i = 0; i < dart_sharedmemnode_size[index]; i++) 
#endif
		{
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
		
#ifdef PROGRESS_EANBLE
		if (user_comm_world != MPI_COMM_NULL)
		{
#endif
		int *dart_unit_mapping, *sharedmem_ranks;
#ifdef PROGRESS_ENABLE
		int user_size;
		MPI_Comm_size (user_comm_world, &user_size);
		dart_sharedmem_table[index] = (int) malloc (sizeof(int) * user_size);
		dart_unit_mapping = (int*)malloc (sizeof(int) * (dart_sharedmemnode_size[index]-PROGRESS_NUM));
		sharedmem_ranks = (int*)malloc (sizeof(int) * (dart_sharedmemnode_size[index]-PROGRESS_NUM));
#else
		MPI_Comm_group(sharedmem_comm, &sharedmem_group);
		MPI_Comm_group(MPI_COMM_WORLD, &group_all);

		/* The length of this table is set to be the size
     * of DART_TEAM_ALL. */
		dart_sharedmem_table[index] = (int *)malloc (sizeof (int) * size);

		dart_unit_mapping = (int *)malloc(
      sizeof (int) * dart_sharedmemnode_size[index]);
		sharedmem_ranks   = (int *)malloc(
      sizeof (int) * dart_sharedmemnode_size[index]);
#endif
#ifdef PROGRESS_ENABLE
		for (i = 0; i < dart_sharedmemnode_size[index] - PROGRESS_NUM; i++)
		{
			sharedmem_ranks[i] = i+PROGRESS_NUM;
		}
#else
		for (i = 0; i < dart_sharedmemnode_size[index]; i++) {
			sharedmem_ranks[i] = i;
		}
#endif
#ifdef PROGRESS_ENABLE
		for (i = 0; i < user_size; i++)
#else
		for (i = 0; i < size; i++) 
#endif
		{
			dart_sharedmem_table[index][i] = -1;
		}

		/* Generate the set (dart_unit_mapping) of units with absolute IDs, 
		 * which are located in the same node 
		 */
#ifdef PROGRESS_ENABLE
		MPI_Group_translate_ranks (sharedmem_group, dart_sharedmemnode_size[index]-PROGRESS_NUM, sharedmem_ranks, user_group, dart_unit_mapping);
		for (i = 0; i < dart_sharedmemnode_size[index] - PROGRESS_NUM; i++)
		{
			dart_sharedmem_table[index][dart_unit_mapping[i]] = i+PROGRESS_NUM;
		}
#else
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
#endif
		free(sharedmem_ranks);
		free(dart_unit_mapping);
#ifdef PROGRESS_ENABLE
		}
#endif
	
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
#ifdef SHAREDMEM_EANBLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULl)
		dart_localpool = dart_buddy_new (DART_BUDDY_ORDER);
#endif
#else
	dart_localpool = dart_buddy_new (DART_BUDDY_ORDER);
#endif

	/* Start an access epoch on dart_win_local_alloc, and later
   * on all the units can access the memory region allocated
   * by the local allocation function through
   * dart_win_local_alloc. */	
	MPI_Win_lock_all(0, dart_sharedmem_win_local_alloc);
	MPI_Win_lock_all(0, dart_win_local_alloc);

	/* Start an access epoch on win, and later on all the units
   * can access the attached memory region allocated by the
   * collective allocation function through win. */
	MPI_Win_lock_all(0, win);

#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	struct datastruct data_info;
	MPI_Datatype type[7] = {MPI_INT32_T, MPI_UINT16_T, MPI_AINT, MPI_AINT, MPI_INT, MPI_INT16_T, MPI_SHORT};
	int blocklen[7] = {1, 1, 1, 1, 1, 1, 1};
	MPI_Aint member_disp[7];
	MPI_Aint data_info_addr, dest_addr, index_addr, origin_offset_addr, target_offset_addr, data_size_addr, segid_addr, is_sharedmem_addr;

	MPI_Get_address (&data_info, &data_info_addr);
	MPI_Get_address (&data_info.dest, &dest_addr);
	MPI_Get_address (&data_info.index, &index_addr);
	MPI_Get_address (&data_info.origin_offset, &origin_offset_addr);
	MPI_Get_address (&data_info.target_offset, &target_offset_addr);
	MPI_Get_address (&data_info.data_size, &data_size_addr);
	MPI_Get_address (&data_info.segid, &segid_addr);
	MPI_Get_address (&data_info.is_sharedmem, &is_sharedmem_addr);

	member_disp[0] = dest_addr - data_info_addr;
	member_disp[1] = index_addr - data_info_addr;
	member_disp[2] = origin_offset_addr - data_info_addr;
	member_disp[3] = target_offset_addr - data_info_addr;
	member_disp[4] = data_size_addr - data_info_addr;
	member_disp[5] = segid_addr - data_info_addr;
	member_disp[6] = is_sharedmem_addr - data_info_addr;

	MPI_Type_create_struct (7, blocklen, member_disp, type, &data_info_type);
	MPI_Type_commit (&data_info_type);
	MPI_Info_free(&win_info);
#endif
	DART_LOG_DEBUG("%2d: dart_init: Initialization finished", rank);

  _dart_initialized = 1;

	return DART_OK;
}

dart_ret_t dart_exit()
{
  if (!_dart_initialized) {
    DART_LOG_ERROR("dart_exit(): DART has not been initialized");
    return DART_ERR_OTHER;
  }
  _dart_initialized = 0;

	int finalized;
	uint16_t index;
	dart_unit_t unitid;
	dart_myid(&unitid);
 	
	DART_LOG_DEBUG("%2d: dart_exit()", unitid);
	if (dart_adapt_teamlist_convert(DART_TEAM_ALL, &index) == -1) {
    DART_LOG_ERROR("%2d: dart_exit: dart_adapt_teamlist_convert failed", unitid);
    return DART_ERR_OTHER;
  }

	if (MPI_Win_unlock_all(dart_win_lists[index]) != MPI_SUCCESS) {
    DART_LOG_ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid);
    return DART_ERR_OTHER;
  }
	/* End the shared access epoch in dart_win_local_alloc. */
	if (MPI_Win_unlock_all(dart_win_local_alloc) != MPI_SUCCESS) {
    DART_LOG_ERROR("%2d: dart_exit: MPI_Win_unlock_all failed", unitid);
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

	if (_init_by_dart) {
    DART_LOG_DEBUG("%2d: dart_exit: MPI_Finalize", unitid);
		MPI_Finalize();
  }
	DART_LOG_DEBUG("%2d: dart_exit: Finalization finished", unitid);

	return DART_OK;
}

char dart_initialized()
{
  return _dart_initialized;
}
