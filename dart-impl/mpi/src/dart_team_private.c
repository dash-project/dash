/** @file dart_team_private.c
 *  @date 25 Aug 2014
 *  @brief Implementations for the operations on teamlist.
 */
#include <stdio.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/mpi/dart_team_private.h>


dart_team_t dart_next_availteamid;

MPI_Comm dart_comm_world;

#if 0
MPI_Comm dart_teams[DART_MAX_TEAM_NUMBER];

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
MPI_Comm dart_sharedmem_comm_list[DART_MAX_TEAM_NUMBER];
#endif

MPI_Win dart_win_lists[DART_MAX_TEAM_NUMBER];

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
int* dart_sharedmem_table[DART_MAX_TEAM_NUMBER];
int dart_sharedmemnode_size[DART_MAX_TEAM_NUMBER];
#endif

#endif // 0

dart_team_data_t dart_team_data[DART_MAX_TEAM_NUMBER];

struct dart_free_teamlist_entry {
  struct dart_free_teamlist_entry * next;
  uint16_t index;
};
typedef struct dart_free_teamlist_entry dart_free_entry;
typedef dart_free_entry* dart_free_teamlist_ptr;

dart_free_teamlist_ptr dart_free_teamlist_header;

/* Structure of the allocated teamlist entry */
struct dart_allocated_teamlist_entry
{
	uint16_t index;
	dart_team_t allocated_teamid;
};
typedef struct dart_allocated_teamlist_entry dart_allocated_entry;

/* This array is used to store all the correspondences between indice and teams */
dart_allocated_entry dart_allocated_teamlist_array[DART_MAX_TEAM_NUMBER];


/* Indicate the length of the allocated teamlist */
int dart_allocated_teamlist_size;

int dart_adapt_teamlist_init ()
{
	int i;
	dart_free_teamlist_ptr pre = NULL;
	dart_free_teamlist_ptr newAllocateEntry;
	for (i = 0; i < DART_MAX_TEAM_NUMBER; i++) {
		newAllocateEntry =
      (dart_free_teamlist_ptr)malloc(sizeof (dart_free_entry));
		newAllocateEntry -> index = i;
		if (pre != NULL) {
			pre -> next = newAllocateEntry;
		} else {
			dart_free_teamlist_header = newAllocateEntry;
		}
		pre = newAllocateEntry;
	}
	newAllocateEntry -> next = NULL;

	dart_allocated_teamlist_size = 0;
	return 0;
}

int dart_adapt_teamlist_destroy ()
{
	dart_free_teamlist_ptr pre, p = dart_free_teamlist_header;
	while (p) {
		pre = p;
		p = p -> next;
		free (pre);
	}
	dart_allocated_teamlist_size = 0;
	return 0;
}

int dart_adapt_teamlist_alloc (dart_team_t teamid, uint16_t* index)
{
	dart_free_teamlist_ptr p;
	if (dart_free_teamlist_header != NULL) {
		*index = dart_free_teamlist_header -> index;
		p = dart_free_teamlist_header;

		dart_free_teamlist_header = dart_free_teamlist_header -> next;
		free (p);
#if 0
		for (j = dart_allocated_teamlist_size - 1; (j>=0)&&(dart_allocated_teamlist_array[j].allocated_teamid > teamid); j--)
		{
			dart_allocated_teamlist_array[j + 1].index = dart_allocated_teamlist_array[j].index;
			dart_allocated_teamlist_array[j + 1].allocated_teamid = dart_allocated_teamlist_array[j].allocated_teamid;
		}
		dart_allocated_teamlist_array[j+1].index = *index;
		dart_allocated_teamlist_array[j+1].allocated_teamid = teamid;
#endif

		/* The allocated teamlist array should be arranged in an increasing order based on
		 * the member of allocated_teamid.
		 *
		 * Notes: the newly created teamid will always be increased because the teamid
		 * is not reused after certain team is destroyed.
		 */
		dart_allocated_teamlist_array[dart_allocated_teamlist_size].index = *index;
		dart_allocated_teamlist_array[dart_allocated_teamlist_size].allocated_teamid = teamid;
		dart_allocated_teamlist_size ++;

		/* If allocated successfully, the position of the new element in the allcoated array
		 * is returned.
		 */
		return (dart_allocated_teamlist_size - 1);

	} else {
		DART_LOG_ERROR ("Out of bound: exceed the MAX_TEAM_NUMBER limit");
		return -1;
	}
}

int dart_adapt_teamlist_recycle (uint16_t index, int pos)
{
	int i;
	dart_free_teamlist_ptr newAllocateEntry =
    (dart_free_teamlist_ptr)malloc (sizeof (dart_free_entry));
	newAllocateEntry -> index = index;
	newAllocateEntry -> next = dart_free_teamlist_header;
	dart_free_teamlist_header = newAllocateEntry;
	/* The allocated teamlist array should be keep as an ordered array
	 * after deleting the given element.
	 */
	for (i = pos; i < dart_allocated_teamlist_size - 1; i ++) {
		dart_allocated_teamlist_array[i].allocated_teamid =
      dart_allocated_teamlist_array[i + 1].allocated_teamid;
		dart_allocated_teamlist_array[i].index =
      dart_allocated_teamlist_array[i + 1].index;
	}
	dart_allocated_teamlist_size --;
	return 0;
}

int dart_adapt_teamlist_convert(
  dart_team_t   teamid,
  uint16_t    * index)
{
	if (teamid == DART_TEAM_ALL) {
		*index = 0;
		return 0;
	}
	/* Locate the team id in the allocated teamlist array by using the
   * binary-search approach.
   */
	int imin, imax;
	imin = 0;
	imax = dart_allocated_teamlist_size - 1;
	while (imin < imax)	{
		int imid = (imin + imax) >> 1;
		if (dart_allocated_teamlist_array[imid].allocated_teamid < teamid) {
			imin = imid + 1;
		}	else {
			imax = imid;
		}
	}
	if ((imax == imin) &&
      (dart_allocated_teamlist_array[imin].allocated_teamid == teamid)) {
		*index = dart_allocated_teamlist_array[imin].index;
		/* If search successfully, the position of the teamid in array is
     * returned.
     */
		return imin;
	} else {
		DART_LOG_ERROR("Invalid teamid input: %d", teamid);
		return -1;
	}
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
dart_ret_t dart_allocate_shared_comm(dart_team_data_t *team_data)
{
  int    i;
  size_t n;
  size_t      size;

  dart_size(&size);

  MPI_Comm sharedmem_comm;
  MPI_Group sharedmem_group, group_all;
  MPI_Comm_split_type(
    team_data->comm,
    MPI_COMM_TYPE_SHARED,
    1,
    MPI_INFO_NULL,
    &sharedmem_comm);
  team_data->sharedmem_comm = sharedmem_comm;

  if (sharedmem_comm != MPI_COMM_NULL) {
    MPI_Comm_size(
      sharedmem_comm,
        &(team_data->sharedmem_nodesize));

  // dart_unit_mapping[index] = (int*)malloc (
  // dart_sharedmem_size[index] * sizeof (int));

    MPI_Comm_group(sharedmem_comm, &sharedmem_group);
    MPI_Comm_group(DART_COMM_WORLD, &group_all);

    int * dart_unit_mapping = malloc(
        team_data->sharedmem_nodesize * sizeof(int));
    int * sharedmem_ranks = malloc(
        team_data->sharedmem_nodesize * sizeof(int));
    team_data->sharedmem_tab = malloc(size * sizeof(int));

    for (i = 0; i < team_data->sharedmem_nodesize; i++) {
      sharedmem_ranks[i] = i;
    }

  // MPI_Group_translate_ranks (sharedmem_group, dart_sharedmem_size[index],
  //     sharedmem_ranks, group_all, dart_unit_mapping[index]);
    MPI_Group_translate_ranks(
      sharedmem_group,
      team_data->sharedmem_nodesize,
      sharedmem_ranks,
      group_all,
      dart_unit_mapping);

    for (n = 0; n < size; n++) {
      team_data->sharedmem_tab[n] = DART_UNDEFINED_TEAM_UNIT_ID;
    }
    for (i = 0; i < team_data->sharedmem_nodesize; i++) {
      team_data->sharedmem_tab[dart_unit_mapping[i]] = DART_TEAM_UNIT_ID(i);
    }
    free(sharedmem_ranks);
    free(dart_unit_mapping);
  }

  return DART_OK;
}
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
