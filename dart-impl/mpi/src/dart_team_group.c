/** 
 *  \file dart_team_group.c
 *  
 *  Implementation of dart operations on team_group.
 */


#include <mpi.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_translation.h>
#include <dash/dart/mpi/dart_group_priv.h>
#include <dash/dart/mpi/dart_communication_priv.h>
/*
#ifndef SHAREDMEM_ENABLE
#define SHAREDMEM_ENABLE
#endif
*/
dart_ret_t dart_group_init(
  dart_group_t *group)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	group -> mpi_group = MPI_GROUP_EMPTY;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_fini(
  dart_group_t *group)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	group -> mpi_group = MPI_GROUP_NULL;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_copy(
  const dart_group_t *gin,
  dart_group_t *gout)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	gout -> mpi_group = gin -> mpi_group;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

#if 0
dart_ret_t dart_adapt_group_union(
  const dart_group_t *g1,
  const dart_group_t *g2,
  dart_group_t *gout)
{
	return MPI_Group_union(
    g1 -> mpi_group,
    g2 -> mpi_group,
    &(gout -> mpi_group));
}
#endif 

dart_ret_t dart_group_union(
  const dart_group_t *g1,
  const dart_group_t *g2,
  dart_group_t *gout)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	/* g1 and g2 are both ordered groups. */
	int ret = MPI_Group_union(
              g1->mpi_group,
              g2->mpi_group,
              &(gout -> mpi_group));
	if (ret == MPI_SUCCESS) {
		int i, j, k, size_in, size_out;
		dart_unit_t *pre_unitidsout, *post_unitidsout;;
	
		MPI_Group group_all;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
		MPI_Comm_group (user_comm_world, &group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
		MPI_Comm_group(MPI_COMM_WORLD, &group_all);
#endif
		MPI_Group_size(gout->mpi_group, &size_out);
		if (size_out > 1) {
			MPI_Group_size(g1->mpi_group, &size_in);
			pre_unitidsout  = (dart_unit_t *)malloc(
                          size_out * sizeof (dart_unit_t));
			post_unitidsout = (dart_unit_t *)malloc(
                          size_out * sizeof (dart_unit_t));
			dart_group_getmembers (gout, pre_unitidsout);

			/* Sort gout by the method of 'merge sort'. */
			i = k = 0;
			j = size_in;
	
			while ((i <= size_in - 1) && (j <= size_out - 1)) {
				post_unitidsout[k++] = 
					(pre_unitidsout[i] <= pre_unitidsout[j])
          ? pre_unitidsout[i++]
          : pre_unitidsout[j++];
			}
			while (i <= size_in -1) {
				post_unitidsout[k++] = pre_unitidsout[i++];
			}
			while (j <= size_out -1) {
				post_unitidsout[k++] = pre_unitidsout[j++];
			}
			gout -> mpi_group = MPI_GROUP_EMPTY;
			MPI_Group_incl(
        group_all,
        size_out,
        post_unitidsout,
        &(gout->mpi_group));
			free (pre_unitidsout);
			free (post_unitidsout);
		}
		ret = DART_OK;
	}
	return ret;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}else return DART_OK;
#endif
#endif

}

dart_ret_t dart_group_intersect(
  const dart_group_t *g1,
  const dart_group_t *g2,
  dart_group_t *gout)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	return MPI_Group_intersection(
           g1 -> mpi_group,
           g2 -> mpi_group,
           &(gout -> mpi_group));
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}else return DART_OK;
#endif
#endif
}

dart_ret_t dart_group_addmember(
  dart_group_t *g,
  dart_unit_t unitid)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	int array[1];
	dart_group_t* group_copy, *group;
	MPI_Group     newgroup, group_all;
	/* Group_all comprises all the running units. */
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	MPI_Comm_group (user_comm_world, &group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
	MPI_Comm_group(MPI_COMM_WORLD, &group_all);
#endif
	group_copy = (dart_group_t*)malloc(sizeof(dart_group_t));
	group      = (dart_group_t*)malloc(sizeof(dart_group_t));
	dart_group_copy(g, group_copy);
	array[0]   = unitid;
	MPI_Group_incl(group_all, 1, array, &newgroup);
	group->mpi_group = newgroup;
	/* Make the new group being an ordered group. */
	dart_group_union (group_copy, group, g);
	free (group_copy);
	free (group);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_delmember(
  dart_group_t *g,
  dart_unit_t unitid)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	int array[1];
	MPI_Group newgroup, group_all;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	MPI_Comm_group (user_comm_world, &group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
	MPI_Comm_group(MPI_COMM_WORLD, &group_all);
#endif
	array[0] = unitid;
	MPI_Group_incl(
    group_all,
    1,
    array,
    &newgroup);
	MPI_Group_difference(
    g -> mpi_group,
    newgroup,
    &(g -> mpi_group));
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_size(
  const dart_group_t *g,
  size_t *size)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	int s;
	MPI_Group_size (g -> mpi_group, &s);
	(*size) = s;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_getmembers(
  const dart_group_t *g,
  dart_unit_t *unitids)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	int size, i;
	int *array;
	MPI_Group group_all;
	MPI_Group_size(g -> mpi_group, &size);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	MPI_Comm_group (user_comm_world, &group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
	MPI_Comm_group(MPI_COMM_WORLD, &group_all);
#endif
	array = (int*) malloc (sizeof (int) * size);
	for (i = 0; i < size; i++) {
		array[i] = i;
	}
	MPI_Group_translate_ranks(
    g->mpi_group,
    size,
    array,
    group_all,
    unitids);
	free (array);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_split(
  const dart_group_t *g,
  size_t n,
  dart_group_t **gout)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	MPI_Group grouptem;
	int size, length, i, ranges[1][3];

	MPI_Group_size (g -> mpi_group, &size);

	/* Ceiling division. */
	length = (size+(int)n-1)/(int)n;
        
	/* Note: split the group into chunks of subgroups. */
	for (i = 0; i < (int)n; i++)
	{
		if (i * length < size)
		{
			ranges[0][0] = i * length;

			if (i * length + length <= size)
			{
				ranges[0][1] = i * length + length -1;
			}
			else 
			{
				ranges[0][1] = size - 1;
			}

			ranges[0][2] = 1;
			MPI_Group_range_incl(
        g -> mpi_group,
        1,
        ranges,
        &grouptem);
			(*(gout + i))->mpi_group = grouptem;
		}
		else 
		{
			(*(gout + i))->mpi_group = MPI_GROUP_EMPTY;
		}
	}
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_sizeof (size_t *size)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	*size = sizeof (dart_group_t);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_group_ismember(
  const dart_group_t *g,
  dart_unit_t unitid,
  int32_t *ismember)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	dart_unit_t id;
	dart_myid (&id);

	int           i, size;
  dart_unit_t*  ranks;
	
	MPI_Group_size(g->mpi_group, &size);
	ranks = (dart_unit_t *)malloc(size * sizeof(dart_unit_t));
	dart_group_getmembers (g, ranks);
	for (i = 0; i < size; i++) {
		if (ranks[i] == unitid) {
			break;
		}
	}
	*ismember = (i!=size);
	DART_LOG_DEBUG("%2d: GROUP_ISMEMBER - %s", unitid, (*ismember) ? "yes" : "no");
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_team_get_group(
  dart_team_t teamid,
  dart_group_t *group)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	MPI_Comm comm;
	uint16_t index;
	
	int result = dart_adapt_teamlist_convert(teamid, &index);
	if (result == -1) {
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];
	MPI_Comm_group (comm, & (group->mpi_group));
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

/** 
 *  TODO: Differentiate units belonging to team_id and that not 
 *  belonging to team_id
 *  within the function or outside it?
 *  FIX: Outside it.
 *
 *  The teamid stands for a superteam related to the new generated 
 *  newteam.
 */
dart_ret_t dart_team_create(
  dart_team_t teamid,
  const dart_group_t* group,
  dart_team_t *newteam)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
     if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	MPI_Comm comm;	
	MPI_Comm subcomm;
	MPI_Win win;
	uint16_t index, unique_id;
	dart_team_t max_teamid = -1;
	dart_unit_t sub_unit, unit;

	dart_myid (&unit);
//	dart_size (&size);
	dart_team_myid (teamid, &sub_unit);
	
	int result = dart_adapt_teamlist_convert (teamid, &unique_id);
	if (result == -1) {
		return DART_ERR_INVAL;	
	}
	comm = dart_teams[unique_id];
	subcomm = MPI_COMM_NULL;

	MPI_Comm_create (comm, group -> mpi_group, &subcomm);
	
	*newteam = DART_TEAM_NULL;

	/* Get the maximum next_availteamid among all the units belonging to
   * the parent team specified by 'teamid'. */
	MPI_Allreduce(
    &dart_next_availteamid,
    &max_teamid,
    1,
    MPI_INT32_T,
    MPI_MAX,
    comm);
	dart_next_availteamid = max_teamid + 1;

	if (subcomm != MPI_COMM_NULL) {
		int result = dart_adapt_teamlist_alloc(max_teamid, &index);
		if (result == -1) {
			return DART_ERR_OTHER;
		}
		/* max_teamid is thought to be the new created team ID. */
		*newteam = max_teamid;
		dart_teams[index] = subcomm;
//		MPI_Win_create_dynamic(MPI_INFO_NULL, subcomm, &win);
//		dart_win_lists[index] = win;
	}
#if 0
	/* Another way of generating the available teamID for the newly crated team. */
	if (subcomm != MPI_COMM_NULL)
	{
		/* Get the maximum next_availteamid among all the units belonging to the 
     * created sub-communicator. */
		MPI_Allreduce (&next_availteamid, &max_teamid, 1, MPI_INT, MPI_MAX, subcomm);
		int result = dart_adapt_teamlist_alloc (max_teamid, &index);

		if (result == -1)
		{
			return DART_ERR_OTHER;
		}
			
		*newteam = max_teamid;
		teams[index] = subcomm;
		MPI_Comm_rank (subcomm, &rank);
		
		if (rank == 0)
		{
			root = sub_unit;
			if (sub_unit != 0)
			{
				MPI_Send (&root, 1, MPI_INT, 0, 0, comm);
			}
		}
		
		next_availteamid = max_teamid + 1;
	}

	if (sub_unit == 0)
	{
		if (root == -1)
		{
			MPI_Recv (&root, 1, MPI_INT, MPI_ANY_SOURCE, 0, comm, MPI_STATUS_IGNORE);
		}
	}

	MPI_Bcast (&root, 1, MPI_INT, 0, comm);
	
	/* Broadcast the calculated max_teamid to all the units not belonging to the
   * sub-communicator. */
	MPI_Bcast (&max_teamid, 1, MPI_INT, root, comm);
	if (subcomm == MPI_COMM_NULL)
	{
		/* 'Next_availteamid' is changed iff it is smaller than 'max_teamid + 1' */
		if (max_teamid + 1 > next_availteamid)
		{
			next_availteamid = max_teamid + 1;
		}
	}
#endif	
	MPI_Group group_all;
	MPI_Comm_group (MPI_COMM_WORLD, &group_all);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	dart_unit_t unitid;
	int parent_unitid, sub_unitid = 0;

	int size, sub_size;
	int i, j, iter;
	MPI_Comm real_comm, real_subcomm;
	MPI_Group parent_group;
	MPI_Comm_group (comm, &parent_group);
	real_comm = dart_realteams[unique_id];
	MPI_Comm_size (real_comm, &size);
	MPI_Group_size (group->mpi_group, &sub_size);
	MPI_Comm_rank (dart_sharedmem_comm_list[unique_id], &unitid);

	MPI_Group user_group;
	MPI_Comm_group (user_comm_world, &user_group);
	
	dart_unit_t* unitids = (dart_unit_t*) malloc (sizeof(dart_unit_t) * (sub_size+1));
	dart_unit_t* abso_unitids = (dart_unit_t*)malloc (sizeof(dart_unit_t) * (sub_size+1));
	dart_group_getmembers (group, unitids);
	MPI_Group_translate_ranks (user_group, sub_size, unitids, group_all, abso_unitids);
	MPI_Group_translate_ranks (group->mpi_group, 1, &sub_unitid, parent_group, &parent_unitid);
//	MPI_Bcast (&index, 1, MPI_UINT16_T, parent_unitid, comm);

	if (unitid == PROGRESS_NUM){
		abso_unitids[sub_size] = dart_progress_index[unique_id];
		for (iter = 0; iter < PROGRESS_NUM; iter++){
			MPI_Send (abso_unitids, sub_size+1, MPI_INT32_T, PROGRESS_UNIT+iter, TEAMCREATE, dart_sharedmem_comm_list[0]);
		}
	}
	MPI_Group new_group[PROGRESS_NUM], progress_group;
	int progress, success, mark;
	int *progress_recv = (int*)malloc (sizeof(int)*size);

	int *progresshead_node, progress_count, *progress_node;
	progresshead_node = (int*)malloc (sizeof(int)*size);
	progress_node = (int*)malloc (sizeof(int)*size);

	for (iter = 0; iter < PROGRESS_NUM; iter++){
		success = 0;
		mark = -1;
		for (i = 0; i < sub_size; i++){
			for (j = 0; j <= abso_unitids[i]; j++)
			{
				if (dart_sharedmem_progress_table[0][j]!=-1){
					if (mark == -1) mark = j+iter;
					progress = j;
				}
			}

			if (progress == abso_unitids[i]){
				success = 1;
				break;
			}
			else mark = -1;
		}

		if (success)
			progress = mark;
		else
			progress = -1;
		MPI_Allgather (&progress, 1, MPI_INT, progress_recv, 1, MPI_INT, real_comm);
		for (i = 0; i < size; i++) progresshead_node[i]=0;
		for (i = 0; i < size; i++){
			int pp;
			int j;
			pp = progress_recv[i];
			if (pp != -1){
				for (j = 0; j <= i; j++){
					if (pp == progress_recv[j]) break;}
				progresshead_node[pp] = 1;
			}
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

	if (PROGRESS_NUM == 1){
		progress_group = new_group[0];}
	else {
		for (iter = 0; iter < PROGRESS_NUM;){
			if (iter == 0){
				MPI_Group_union (new_group[PROGRESS_UNIT+iter], new_group[PROGRESS_UNIT+iter+1], &progress_group);
				iter+=2;
			}else{
				MPI_Group_union (progress_group, new_group[PROGRESS_UNIT+iter], &progress_group);
				iter++;
			}
		}
	}
	MPI_Group sub_group;
	MPI_Group_union (progress_group, group->mpi_group, &sub_group);
//	int test_ranks[2], all_ranks[2];
//	for (i = 0; i < 2; i++)
//		test_ranks[i] = i;
//	MPI_Group_translate_ranks(sub_group, 2, test_ranks, group_all, all_ranks);
	real_subcomm = MPI_COMM_NULL;
	MPI_Comm_create (real_comm, sub_group, &real_subcomm);

	if (real_subcomm != MPI_COMM_NULL){
		MPI_Bcast (&progress_index, 1, MPI_INT32_T, 0, real_subcomm);
		dart_progress_index[index] = progress_index;}
	free (progress_node);
	free (progress_recv);
	free (progresshead_node);
	free (unitids);
	free (abso_unitids);
#endif
#endif



	if (subcomm != MPI_COMM_NULL) {
#ifdef SHAREDMEM_ENABLE
		int i;

		MPI_Comm sharedmem_comm;
		MPI_Group sharedmem_group;
#ifdef PROGRESS_ENABLE
		MPI_Win_create_dynamic (MPI_INFO_NULL, real_subcomm, &win);
	//	dart_win_lists[index] = win;
		dart_realteams[index] = real_subcomm;
		MPI_Comm_split_type (real_subcomm, MPI_COMM_TYPE_SHARED, 1, MPI_INFO_NULL, &sharedmem_comm);
#else
		MPI_Win_create_dynamic (MPI_INFO_NULL, subcomm, &win);
	//	dart_win_lists[index] = win;
		MPI_Comm_split_type (subcomm, MPI_COMM_TYPE_SHARED, 1, MPI_INFO_NULL, &sharedmem_comm);
#endif
		dart_sharedmem_comm_list[index] = sharedmem_comm;
		if (sharedmem_comm != MPI_COMM_NULL)
		{
			MPI_Comm_size (sharedmem_comm, &(dart_sharedmemnode_size[index]));
			MPI_Comm_group (sharedmem_comm, &sharedmem_group);
			int *dart_unit_mapping, *sharedmem_ranks;
#ifdef PROGRESS_ENABLE
			int user_size;
			MPI_Comm_size (user_comm_world, &user_size);
			dart_sharedmem_table[index] = (int*)malloc (sizeof(int) * user_size);
			dart_unit_mapping = (int*)malloc (sizeof(int)*(dart_sharedmemnode_size[index]-PROGRESS_NUM));
			sharedmem_ranks = (int*)malloc (sizeof(int) * (dart_sharedmemnode_size[index]-PROGRESS_NUM));
#else
			int size_all;
			MPI_Comm_size (MPI_COMM_WORLD, &size_all);
			dart_sharedmem_table[index] = (int*)malloc (sizeof(int) * size_all);
			dart_unit_mapping = (int*)malloc (sizeof(int) * dart_sharedmemnode_size[index]);
			sharedmem_ranks = (int*)malloc (sizeof(int) * dart_sharedmemnode_size[index]);
#endif
#ifdef PROGRESS_ENABLE
			for (i = 0; i < dart_sharedmemnode_size[index]-PROGRESS_NUM; i++){
				sharedmem_ranks[i] = i+PROGRESS_NUM;
			}
#else
			for (i = 0; i < dart_sharedmemnode_size[index]; i++){
				sharedmem_ranks[i] = i;
			}
#endif
#ifdef PROGRESS_ENABLE
			for (i = 0; i < user_size; i++)
#else
			for (i = 0; i < size_all; i++)
#endif
			{
				dart_sharedmem_table[index][i] = -1;
			}
#ifdef PROGRESS_ENABLE
			MPI_Group_translate_ranks (sharedmem_group, dart_sharedmemnode_size[index]-PROGRESS_NUM,
					sharedmem_ranks, user_group, dart_unit_mapping);
			for (i = 0; i < dart_sharedmemnode_size[index]-PROGRESS_NUM; i++){
				dart_sharedmem_table[index][dart_unit_mapping[i]] = i+PROGRESS_NUM;
			}
#else
			MPI_Group_translate_ranks (sharedmem_group, dart_sharedmemnode_size[index],
					sharedmem_ranks, group_all, dart_unit_mapping);
			for (i = 0; i < dart_sharedmemnode_size[index]; i++){
				dart_sharedmem_table[index][dart_unit_mapping[i]] = i;
			}
#endif
			free (sharedmem_ranks);
			free (dart_unit_mapping);
		}
		/*
		MPI_Comm_split_type(
      subcomm,
      MPI_COMM_TYPE_SHARED,
      1,
      MPI_INFO_NULL,
      &sharedmem_comm);
		dart_sharedmem_comm_list[index] = sharedmem_comm;
		if (sharedmem_comm != MPI_COMM_NULL) {
			MPI_Comm_size(
        sharedmem_comm,
        &(dart_sharedmemnode_size[index]));
		
	//		dart_unit_mapping[index] = (int*)malloc (
  //		  dart_sharedmem_size[index] * sizeof (int));

			MPI_Comm_group(sharedmem_comm, &sharedmem_group);
			MPI_Comm_group(MPI_COMM_WORLD, &group_all);

			int* dart_unit_mapping = (int *)malloc (
        dart_sharedmemnode_size[index] * sizeof (int));
			int* sharedmem_ranks = (int*)malloc (
        dart_sharedmemnode_size[index] * sizeof (int));
			
			dart_sharedmem_table[index] = (int*)malloc(size * sizeof(int));
		
			for (i = 0; i < dart_sharedmemnode_size[index]; i++) {
				sharedmem_ranks[i] = i;
			}
			
	//		MPI_Group_translate_ranks (sharedmem_group, dart_sharedmem_size[index], 
	//				sharedmem_ranks, group_all, dart_unit_mapping[index]);
			MPI_Group_translate_ranks(
        sharedmem_group,
        dart_sharedmemnode_size[index],
				sharedmem_ranks,
        group_all,
        dart_unit_mapping);

			for (i = 0; i < size; i++) {
				dart_sharedmem_table[index][i] = -1;
			}
			for (i = 0; i < dart_sharedmemnode_size[index]; i++) {
				dart_sharedmem_table[index][dart_unit_mapping[i]] = i;
			}
			free (sharedmem_ranks);
			free (dart_unit_mapping);
		}
*/
#else
		MPI_Win_create_dynamic (MPI_INFO_NULL, subcomm, &win);
//		dart_win_lists[index] = win;
#endif
		dart_win_lists[index] = win;
		MPI_Win_lock_all(0, win);
		DART_LOG_DEBUG ("%2d: TEAMCREATE	- create team %d out of parent team %d", 
           unit, *newteam, teamid);
	}
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
        }
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_team_destroy(
  dart_team_t teamid)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	MPI_Comm comm;
	MPI_Win win;
	uint16_t index;
	dart_unit_t id;

	int result = dart_adapt_teamlist_convert(teamid, &index);
	if (result == -1) {
		return DART_ERR_INVAL;
	}
        comm = dart_teams[index];

	dart_myid (&id);
	
//	free (dart_unit_mapping[index]);

//	MPI_Win_free (&(sharedmem_win_list[index]));
#ifdef SHAREDMEM_ENABLE
	free(dart_sharedmem_table[index]);
#ifdef PROGRESS_ENABLE
	MPI_Comm sharedmem_comm = dart_sharedmem_comm_list[index];
	dart_unit_t unitid;
	MPI_Comm_rank (sharedmem_comm, &unitid);
	if (unitid == PROGRESS_NUM){
		int iter;
		for (iter = 0; iter < PROGRESS_NUM; iter++){
			MPI_Send (&index, 1, MPI_UINT16_T, PROGRESS_UNIT+iter, TEAMDESTROY, dart_sharedmem_comm_list[0]);
		}
	}
#endif
#endif
	win = dart_win_lists[index];
	MPI_Win_unlock_all(win);
	MPI_Win_free(&win);
	dart_adapt_teamlist_recycle(index, result);

	/* -- Release the communicator associated with teamid -- */
	MPI_Comm_free (&comm);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	MPI_Comm_free (&dart_realteams[index]);
#endif
#endif

	DART_LOG_DEBUG("%2d: TEAMDESTROY	- destroy team %d", id, teamid);
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
        }
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_myid(dart_unit_t *unitid)
{
  if (dart_initialized()) {
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
    if (user_comm_world != MPI_COMM_NULL){
	    MPI_Comm_rank (user_comm_world, unitid);
    } else *unitid = -1;
#endif
#endif
#ifndef PROGRESS_ENABLE
    MPI_Comm_rank(MPI_COMM_WORLD, unitid);
#endif
  } else {
    *unitid = -1;
  }
	return DART_OK;
}

dart_ret_t dart_size(size_t *size)
{
	int s;
	*size = -1;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
		MPI_Comm_size (user_comm_world, &s);
		(*size) = s;}
#endif
#endif
#ifndef PROGRESS_ENABLE
	MPI_Comm_size (MPI_COMM_WORLD, &s);
	(*size) = s;
#endif
	return DART_OK;
}

dart_ret_t dart_team_myid(
  dart_team_t teamid,
  dart_unit_t *unitid)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	MPI_Comm comm;
	uint16_t index;
	int result = dart_adapt_teamlist_convert(teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];
	MPI_Comm_rank (comm, unitid); 
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

dart_ret_t dart_team_size(
  dart_team_t teamid,
  size_t *size)
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	MPI_Comm comm;
	uint16_t index;
	if (teamid == DART_TEAM_NULL) {
		return DART_ERR_INVAL;
	}
  int result = dart_adapt_teamlist_convert(teamid, &index);
	if (result == -1) {
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];

	int s;
	MPI_Comm_size (comm, &s);
	(*size) = s;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}

MPI_Comm dart_get_user_commworld ()
{
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
		return dart_teams[0];
	}
	return MPI_COMM_NULL;
#endif
#endif
#ifndef PROGRESS_ENABLE
	return MPI_COMM_WORLD;
#endif
}

dart_ret_t dart_team_unit_l2g(
  dart_team_t teamid,
  dart_unit_t localid,
  dart_unit_t *globalid)
{
#if 0
	dart_unit_t *unitids;
	int size; 
	int i = 0;
	dart_group_t group;
	dart_team_get_group (teamid, &group);
	MPI_Group_size (group.mpi_group, &size);
	if (localid >= size)
	{
		DART_LOG_ERROR ("Invalid localid input");
		return DART_ERR_INVAL;
	}
	unitids = (dart_unit_t*)malloc (sizeof(dart_unit_t) * size);
	dart_group_getmembers (&group, unitids);
	
	/* The unitids array is arranged in ascending order. */
	*globalid = unitids[localid];
//	printf ("globalid is %d\n", *globalid);
	return DART_OK;
#endif
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	int size;
	dart_group_t group;

	dart_team_get_group (teamid, &group);
	MPI_Group_size (group.mpi_group, &size);

	if (localid >= size) {
		DART_LOG_ERROR ("Invalid localid input: %d", localid);
		return DART_ERR_INVAL;
	}
	if (teamid == DART_TEAM_ALL) {
		*globalid = localid;	
	}
	else {
		MPI_Group group_all;
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
		MPI_Comm_group (user_comm_world, &group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
		MPI_Comm_group(MPI_COMM_WORLD, &group_all);
#endif
		MPI_Group_translate_ranks(
      group.mpi_group,
      1,
      &localid,
      group_all,
      globalid);
	}
#ifdef PROGRESS_ENABLE
	}
#endif
	return DART_OK;
}

dart_ret_t dart_team_unit_g2l(
  dart_team_t teamid,
  dart_unit_t globalid,
  dart_unit_t *localid)
{
#if 0
	dart_unit_t *unitids;
	int size;
	int i;
	dart_group_t group;
	dart_team_get_group (teamid, &group);
	MPI_Group_size (group.mpi_group, &size);
	unitids = (dart_unit_t *)malloc (sizeof (dart_unit_t) * size);

	dart_group_getmembers (&group, unitids);

	
	for (i = 0; (i < size) && (unitids[i] < globalid); i++);

	if ((i == size) || (unitids[i] > globalid))
	{
		*localid = -1;
		return DART_OK;
	}

	*localid = i;
	return DART_OK;
#endif
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	if (user_comm_world != MPI_COMM_NULL){
#endif
#endif
	if(teamid == DART_TEAM_ALL) {
		*localid = globalid;		
	}
	else {
		dart_group_t group;
		MPI_Group group_all;
		dart_team_get_group(teamid, &group);
#ifdef SHAHREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
		MPI_Comm_group (user_comm_world, &group_all);
#endif
#endif
#ifndef PROGRESS_ENABLE
		MPI_Comm_group(MPI_COMM_WORLD, &group_all);
#endif
		MPI_Group_translate_ranks(
      group_all,
      1,
      &globalid,
      group.mpi_group,
      localid);
	}
#ifdef SHAREDMEM_ENABLE
#ifdef PROGRESS_ENABLE
	}
#endif
#endif
	return DART_OK;
}
