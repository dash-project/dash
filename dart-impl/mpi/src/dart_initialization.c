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

	        for (i = 0; i < PROGRESS_NUM;){
			if (i == 0){
				MPI_Group_union (new_group[i], new_group[i+1], &progress_group);
				i += 2;
			}else{
				MPI_Group_union (progress_group, new_group[i], &progress_group);
				i++;
			}
		}
	//	MPI_Group_union (new_group[PROGRESS_UNIT], new_group[PROGRESS_UNIT + 1], &progress_group);
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
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world == MPI_COMM_NULL)
	{
	 	struct rmareq_node
		{
			int source;
			data_unit_t dest;
			MPI_Win win;
			int direction;
			struct rmareq_node* next;
		};
		int16_t segid;
		MPI_Win sharedmem_win, win;
		char *baseptr, *destptr;
		int result, src;
		dart_unit_t dest;
		int nbytes;
		int is_sharedmem;
		struct datastruct recv_data;
		MPI_Aint maximum_size;
		int disp_unit;

		MPI_Aint origin_offset, target_offset;
		MPI_Request mpi_req;
		MPI_Status mpi_sta, mpi_status;

		MPI_Info win_info;
		MPI_Info_create (&win_info);
		MPI_Info_set (win_info, "alloc_shared_noncontig", "trure");

		dart_team_t teamid;
		struct rmareq_node* header = NULL;
		struct rmareq_node* tail = NULL;
		int rmareq_num = 0;

		while (1)
		{
			int flag;
			MPI_Iprobe (MPI_ANY_SOURCE, MPI_ANY_TAG, dart_sharedmem_comm_list[0], &flag, &mpi_status);

			if (flag)
			{
				if (mpi_status.MPI_TAG == MEMALLOC){
					MPI_Recv (&teamid, 1, MPI_INT32_T, mpi_status.MPI_SOURCE, 
							MEMALLOC, dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
					result = dart_adapt_teamlist_convert (teamid, &index);
					if (result == -1)
					{
						return DART_ERR_INVAL;
					}
					char* sub_mem;

					MPI_Win_allocate_shared (0, sizeof(char), MPI_INFO_NULL, dart_sharedmem_comm_list[index],
							&sub_mem, &sharedmem_win);
					MPI_Aint winseg_size;
					char** baseptr_set;
					char *baseptr;
					int disp_unit, i;
					baseptr_set = (char**)malloc (sizeof (char*) * dart_sharedmemnode_size[index]);

					for (i = PROGRESS_NUM; i < dart_sharedmemnode_size[index]; i++)
					{
						MPI_Win_shared_query (sharedmem_win, i, &winseg_size,
								&disp_unit, &baseptr);
						baseptr_set[i] = baseptr;
					}
					info_t item;
					item.seg_id = dart_memid;
					item.size = 0;
					item.disp = NULL;
					item.win = sharedmem_win;
					item.baseptr = baseptr_set;
					item.selfbaseptr = NULL;
					dart_adapt_transtable_add (item);
					dart_memid ++;
					MPI_Win_lock_all (0, sharedmem_win);
				}
				if (mpi_status.MPI_TAG == MEMFREE){
					MPI_Recv (&segid, 1, MPI_INT16_T, mpi_status.MPI_SOURCE,
							MEMFREE, dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
					if (dart_adapt_transtable_get_win (segid, &sharedmem_win) == -1)
						return DART_ERR_INVAL;

					MPI_Win_unlock_all (sharedmem_win);
					MPI_Win_free (&sharedmem_win);
					if (dart_adapt_transtable_remove (segid) == -1)
						return DART_ERR_INVAL;
				}
				if (mpi_status.MPI_TAG == EXIT){
					MPI_Recv (NULL, 0, MPI_UINT16_T, mpi_status.MPI_SOURCE, 
							EXIT, dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
					MPI_Win_unlock_all (dart_sharedmem_win_local_alloc);
					MPI_Win_unlock_all (dart_win_lists[0]);
					MPI_Win_unlock_all (dart_win_lock_alloc);

					MPI_Win_free (&dart_win_lists[0]);
					MPI_Win_free (&dart_win_local_alloc);
					MPI_Win_free (&dart_sharedmem_win_local_alloc);

					dart_adapt_teamlist_destroy ();
					dart_adapt_transtable_destroy ();
					free (dart_sharedmem_table[0]);
					free (dart_sharedmem_local_baseptr_set);
					MPI_Type_free (&data_info_type);
					break;
				}
if (mpi_status.MPI_TAG == TEAMCREATE){

			//	printf ("inside the TEAMCREATE\n");

				int count, size;
				int i, j;
				MPI_Comm newcomm;
				
				MPI_Comm_size (MPI_COMM_WORLD, &size);
				MPI_Get_count (&mpi_status, MPI_INT32_T, &count);
				dart_unit_t* unitids = (dart_unit_t*)malloc (sizeof (dart_unit_t) * count);
				MPI_Recv (unitids, count, MPI_INT32_T, mpi_status.MPI_SOURCE,
						TEAMCREATE, dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);

				result = dart_adapt_teamlist_convert (unitids[count-2], &index);
				if (result == -1)
					return DART_ERR_INVAL;

				int progress, success = 0, mark = -1;
				int *progress_recv= (int*)malloc (sizeof (int) * size);
				for (i = 0; i < count - 2; i++)
				{
					for (j = 0; j <= unitids[i]; j++)
					{
						if (dart_sharedmem_table[index][j] != -1){
							if (mark == -1) mark = j;
								progress = j;}
					}
					if (progress == unitids[i]) 
					{
						success = 1;
						break;
					}
					else mark = -1;	
				}
				if (success)
				{
					progress = mark;
				}
				else
				{
					progress = -1;
				}
				MPI_Allgather (&progress, 1, MPI_INT, progress_recv, 1, MPI_INT, dart_realteams[index]);
				
				int* progresshead_node, progress_count, *progress_node;
				progresshead_node = (int*)malloc (sizeof (int) * size);
				progress_node = (int *)malloc (sizeof (int) * size);

				for(i=0;i<size;i++) progresshead_node[i]=0;
				for(i=0;i<size;i++){
					int pp;
					pp = progress_recv[i];
					if (pp != -1){
						for (j = 0; j <= i; j++){
							if (pp == progress_recv[j]) break;}
						progresshead_node[j] = 1;}
				}
				progress_count = 0;
				for(i=0;i<size;i++){
					if (progresshead_node[i]){
						progress_node[progress_count] = i;
						progress_count ++;}
				}
				MPI_Group recvgroup, newgroup, progress_group, group_all;

							MPI_Comm_group (MPI_COMM_WORLD, &group_all);
				MPI_Group_incl (group_all, progress_count, progress_node, &progress_group);
				MPI_Group_incl (group_all, count - 2, unitids, &recvgroup);
				MPI_Group_union (progress_group, recvgroup, &newgroup);
				free (progress_node);
				free (progresshead_node);
				free (progress_recv);
/*
				for (i = 0; i < count; i++)
				{
					printf ("unitids[%d] = %d\n", i, unitids[i]);
				}
*/

			//	if (dart_realteams[index] == MPI_COMM_NULL) printf ("yesyesyesyes\n");
				MPI_Comm_create (dart_realteams[index], newgroup, &newcomm);

				if (newcomm != MPI_COMM_NULL){		
				MPI_Win_create_dynamic (MPI_INFO_NULL, newcomm, &win);
			

				result = dart_adapt_teamlist_alloc (unitids[count-1], &index);
				if (result == -1)
				{
					return DART_ERR_OTHER;
				}
		//		printf ("unitids[count-1] = %d, index is %d\n", unitids[count-1], index);
				dart_win_lists[index] = win;
				dart_realteams[index] = newcomm;

				MPI_Comm sharedmem_comm;
				MPI_Group sharedmem_group;
				MPI_Comm_split_type (newcomm, MPI_COMM_TYPE_SHARED, 1, MPI_INFO_NULL, &sharedmem_comm);
				dart_sharedmem_comm_list[index] = sharedmem_comm;

				MPI_Comm_size (sharedmem_comm, &(dart_sharedmemnode_size[index]));
				MPI_Comm_group (sharedmem_comm, &sharedmem_group);
				
				int* dart_unit_mapping = (int*)malloc (dart_sharedmemnode_size[index] * sizeof(int));
				int* sharedmem_ranks = (int*)malloc (dart_sharedmemnode_size[index] * sizeof(int));

				dart_sharedmem_table[index] = (int*)malloc (size * sizeof (int));
				for (i = 0; i < dart_sharedmemnode_size[index]; i++)
				{
					sharedmem_ranks[i] = i;
				}	
				MPI_Group_translate_ranks (sharedmem_group, dart_sharedmemnode_size[index], 
						sharedmem_ranks, group_all, dart_unit_mapping);

				for (i = 0; i < size; i++)
				{
					dart_sharedmem_table[index][i] = -1;
				}
				for (i = 0; i < dart_sharedmemnode_size[index]; i++)
				{
					dart_sharedmem_table[index][dart_unit_mapping[i]] = i;
				}

				free (sharedmem_ranks);
				free (dart_unit_mapping);
				free (unitids);
				MPI_Win_lock_all (0, win);}
			}
			if (mpi_status.MPI_TAG == TEAMDESTROY){
				MPI_Recv (&teamid, 1, MPI_INT32_T, mpi_status.MPI_SOURCE, TEAMDESTROY, 
						dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);

				result = dart_adapt_teamlist_convert (teamid, &index);
				if (result == -1) return DART_ERR_INVAL;

				free (dart_sharedmem_table[index]);
				win = dart_win_lists[index];
				MPI_Win_unlock_all (win);
				MPI_Win_free (&win);
				MPI_Comm_free (&dart_realteams[index]);
				dart_adapt_teamlist_recycle (index, result);
			}
			if (mpi_status.MPI_TAG == PUT){
				MPI_Recv (&recv_data, 1, data_info_type, mpi_status.MPI_SOURCE, PUT, 
						dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
				dest = recv_data.dest;
				index = recv_data.index;
				origin_offset = recv_data.origin_offset;
				target_offset = recv_data.target_offset;
				nbytes = recv_data.data_size;
				segid = recv_data.segid;
				is_sharedmem = recv_data.is_sharedmem;

			

				if (segid){
					if (is_sharedmem)
						dart_adapt_transtable_get_win (segid, &win);
					else win = dart_win_lists[index];
					if (index != 0){
						MPI_Group sharedmem_group_all, sharedmem_group;
						MPI_Comm_group (dart_sharedmem_comm_list[0], &sharedmem_group_all);
						MPI_Comm_group (dart_sharedmem_comm_list[index], &sharedmem_group);
						MPI_Group_translate_ranks (sharedmem_group_all, 1, &mpi_status.MPI_SOURCE,
								sharedmem_group, &src);}
					else src = mpi_status.MPI_SOURCE;
				//	dart_adapt_transtable_get_win (segid, &sharedmem_win);
				//	MPI_Win_shared_query (sharedmem_win, src, &maximum_size, &disp_unit, &baseptr);
					if (dart_adapt_transtable_get_baseptr (segid, src, &baseptr) == -1)
							return DART_ERR_INVAL;
					
					}else{
						if (is_sharedmem) win = dart_sharedmem_win_local_alloc;
						else win = dart_win_local_alloc;
					baseptr = dart_sharedmem_local_baseptr_set[mpi_status.MPI_SOURCE];}

				{
					MPI_Put (baseptr+origin_offset, nbytes, MPI_BYTE, dest, target_offset, nbytes, MPI_BYTE, win);
				//	MPI_Rput (baseptr+origin_offset, nbytes, MPI_BYTE, dest, target_offset, nbytes, MPI_BYTE, win, &mpi_req);
				//	if (nbytes >= PUT_THRESH){
				//		MPI_Win_flush (dest, win);
				//		MPI_Send (NULL, 0, MPI_INT, mpi_status.MPI_SOURCE, PUT, dart_sharedmem_comm_list[0]);
				//	}else
					{
					struct rmareq_node* p = (struct rmareq_node*)malloc (sizeof(struct rmareq_node));
					p -> dest = dest;
					p -> source = mpi_status.MPI_SOURCE;
					p -> direction = PUT;
					p -> win = win;
					p -> next = NULL;
					
					if(!header) {header = p; tail = header;}
					else {tail -> next = p; tail=p;}}
/*
					if (rmareq_num >= 128){
						while(header){
							MPI_Win_flush (header -> dest, header -> win);
							MPI_Send (NULL, 0, MPI_INT, header->source, header->direction, 
									dart_sharedmem_comm_list[0]);
							struct rmareq_node* p;
							p = header;
							header = header -> next;
							free (p);}
						rmareq_num = 0;
					}*/

					//	MPI_Wait (&mpi_req, &mpi_sta);
				}

			//	MPI_Send (NULL, 0, MPI_INT, mpi_status.MPI_SOURCE, PUT, dart_sharedmem_comm_list[0]);
			}
			if (mpi_status.MPI_TAG == GET){
			
				MPI_Recv (&recv_data, 1, data_info_type, mpi_status.MPI_SOURCE, GET, 
						dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
			
				dest = recv_data.dest;
				index = recv_data.index;
				origin_offset = recv_data.origin_offset;
				target_offset = recv_data.target_offset;
				nbytes = recv_data.data_size;
				segid = recv_data.segid;
				is_sharedmem = recv_data.is_sharedmem;
			//	MPI_Aint maximum_size;
			//	int disp_unit;
			        if (segid){
					if (is_sharedmem) dart_adapt_transtable_get_win (segid, &win);
					else win = dart_win_lists[index];
					if (index != 0){
						MPI_Group sharedmem_group_all, sharedmem_group;
						MPI_Comm_group (dart_sharedmem_comm_list[0], &sharedmem_group_all);
						MPI_Comm_group (dart_sharedmem_comm_list[index], &sharedmem_group);
						MPI_Group_translate_ranks (sharedmem_group_all, 1, &mpi_status.MPI_SOURCE,
								sharedmem_group, &src);}
					else src = mpi_status.MPI_SOURCE;
					if (dart_adapt_transtable_get_baseptr (segid, src, &baseptr) == -1)
						return DART_ERR_INVAL;
				}else{
					if (is_sharedmem) win = dart_sharedmem_win_local_alloc;
					else win = dart_win_local_alloc;
					baseptr = dart_sharedmem_local_baseptr_set[src];
				}

			
				{

				MPI_Get (baseptr+origin_offset, nbytes, MPI_BYTE, dest, target_offset, nbytes, MPI_BYTE, win);
			//	MPI_Rget (baseptr+origin_offset, nbytes, MPI_BYTE, dest, target_offset, nbytes, MPI_BYTE, win, &mpi_req);

			//	if (nbytes >= GET_THRESH){
			////		MPI_Win_flush (dest, win);
					MPI_Send (NULL, 0, MPI_INT, mpi_status.MPI_SOURCE, GET, dart_sharedmem_comm_list[0]);
			//	}else
				{
			        struct rmareq_node* p = (struct rmareq_node*) malloc (sizeof(struct rmareq_node));
				p -> dest = dest;
				p -> source = mpi_status.MPI_SOURCE;
				p -> direction = GET;
				p -> win = win;
				p -> next = NULL;
				if(!header) {header= p; tail=header;}
				else	{tail-> next = p; tail = p;}}
			        }
			
			//	MPI_Send (NULL, 0, MPI_INT, mpi_status.MPI_SOURCE, GET, dart_sharedmem_comm_list[0]);
			}
			if (mpi_status.MPI_TAG == WAIT)
			{
				MPI_Recv (NULL, 0, MPI_UINT16_T, mpi_status.MPI_SOURCE, WAIT,
						dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
				
				while (header){
					MPI_Win_flush (header -> dest, header -> win);	
			//		MPI_Send (NULL, 0, MPI_INT, header -> source, header -> direction, 
			//				dart_sharedmem_comm_list[0]);
					struct rmareq_node* p;
					p = header;
					header = header -> next;
					free (p);	
				        }
				MPI_Send (NULL, 0, MPI_INT, mpi_status.MPI_SOURCE, WAIT, 
						dart_sharedmem_comm_list[0], MPI_STATUS_IGNORE);
			}}
			else{
		
				while (header){
					MPI_Win_flush (header->dest, header->win);
			//		MPI_Send (NULL, 0, MPI_INT, header->source, header->direction,
			//			       	dart_sharedmem_comm_list[0]);
					
					struct rmareq_node* p;
					p = header;
					header = header -> next;
					free (p);}
			}

		}
		MPI_Info_free (&win_info);
		return MPI_Finalize();

	}
	dart_unit_t unitid;
	MPI_Comm_rank (dart_sharedmem_comm_list[0], &unitid);

	int i;
	if (unitid == PROGRESS_NUM){
		for (i = 0; i < PROGRESS_NUMM; i++){
		MPI_Send (NULL, 0, MPI_UNIT16_T, PROGRESS_UNIT+i, EXIT, dart_sharedmem_comm_list[0]);}
	}
	int result = dart_adapt_teamlist_convert (DART_TEAM_ALL, &index);

	MPI_Win_unlock_all (dart_sharedmem_win_local_alloc);
	MPI_Win_unlock_all (dart_win_lists[index]);
	MPI_Win_unlock_all (dart_win_local_alloc);
	MPI_Win_free (&dart_win_lists[index]);
	MPI_Win_free (&dart_win_local_alloc);
	MPI_Win_free (&dart_sharedmem_win_local_alloc);

	dart_adapt_transtable_destroy ();
	buddy_delete (dart_localpool);

	free (dart_sharedmem_table[index]);
	free (dart_sharedmem_local_baseptr_set);

	dart_adapt_teamlist_destroy ();

	MPI_Comm_free (&user_comm_world);
	MPI_Type_free (&data_info_type);
#endif
#endif
#ifndef PROGRESS_ENABLE
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
#endif
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
