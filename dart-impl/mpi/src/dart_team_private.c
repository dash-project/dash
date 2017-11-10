/** @file dart_team_private.c
 *  @date 25 Aug 2014
 *  @brief Implementations for the operations on teamlist.
 */
#include <stdio.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/mpi/dart_team_private.h>

#define DART_TEAM_HASH_SIZE (256)

dart_team_t dart_next_availteamid = (DART_TEAM_ALL + 1);

MPI_Comm dart_comm_world;

static dart_team_data_t *dart_team_data[DART_TEAM_HASH_SIZE];

static int
dart_adapt_teamlist_hash(dart_team_t teamid)
{
  return (teamid % DART_TEAM_HASH_SIZE);
}

#if 0

struct dart_free_teamlist_entry {
  struct dart_free_teamlist_entry * next;
  uint16_t index;
};
typedef struct dart_free_teamlist_entry dart_free_entry;
typedef dart_free_entry* dart_free_teamlist_ptr;

static dart_free_teamlist_ptr dart_free_teamlist_header;

/* Structure of the allocated teamlist entry */
struct dart_allocated_teamlist_entry
{
  uint16_t index;
  dart_team_t allocated_teamid;
};
typedef struct dart_allocated_teamlist_entry dart_allocated_entry;

/* This array is used to store all the correspondences between indice and teams */
static dart_allocated_entry dart_allocated_teamlist_array[DART_TEAM_HASH_SIZE];

/* Indicate the length of the allocated teamlist */
static int dart_allocated_teamlist_size;

int dart_adapt_teamlist_init ()
{
	int i;
	dart_free_teamlist_ptr pre = NULL;
	dart_free_teamlist_ptr newAllocateEntry;
	for (i = 0; i < DART_TEAM_HASH_SIZE; i++) {
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
#endif


dart_ret_t
dart_adapt_teamlist_init()
{
  memset(dart_team_data, 0, sizeof(dart_team_data_t*) * DART_TEAM_HASH_SIZE);

  return DART_OK;
}

dart_team_data_t *
dart_adapt_teamlist_get(dart_team_t teamid)
{
  int slot = dart_adapt_teamlist_hash(teamid);
  dart_team_data_t *res = dart_team_data[slot];
  while (res != NULL && res->teamid != teamid) {
    res = res->next;
  }

  return res;
}

dart_ret_t
dart_adapt_teamlist_dealloc(dart_team_t teamid)
{
  int slot = dart_adapt_teamlist_hash(teamid);
  dart_team_data_t *res = dart_team_data[slot];
  dart_team_data_t **prev = NULL;

  while (res != NULL && res->teamid == teamid) {
    prev = &res;
    res = res->next;
  }

  // not found!
  if (res == NULL) {
    return DART_ERR_INVAL;
  }

  if (prev == NULL) {
    dart_team_data[slot] = res->next;
  } else {
    (*prev)->next = res->next;
  }

  res->next = NULL;
  free(res);
  return DART_OK;
}

dart_ret_t
dart_adapt_teamlist_alloc(dart_team_t teamid)
{
  int slot = dart_adapt_teamlist_hash(teamid);
  dart_team_data_t *res = calloc(1, sizeof(dart_team_data_t));
  res->teamid = teamid;
  res->unitid = DART_UNDEFINED_UNIT_ID;
  res->next = dart_team_data[slot];
  dart_team_data[slot] = res;
  dart_segment_init(&(res->segdata), teamid);
  return DART_OK;
}


dart_ret_t dart_adapt_teamlist_destroy()
{
  for (int i = 0; i < DART_TEAM_HASH_SIZE; i++) {
    dart_team_data_t *elem = dart_team_data[i];
    while (elem != NULL) {
      dart_team_data_t *tmp = elem;
      elem = tmp->next;
      tmp->next = NULL;
      free(tmp);
    }
    dart_team_data[i] = NULL;
  }
  return DART_OK;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
dart_ret_t dart_allocate_shared_comm(dart_team_data_t *team_data)
{
  int size;

  MPI_Comm_size(team_data->comm, &size);

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

    MPI_Comm_group(sharedmem_comm, &sharedmem_group);
    MPI_Comm_group(team_data->comm, &group_all);

    int * dart_unit_mapping  = malloc(
        team_data->sharedmem_nodesize * sizeof(int));
    int * sharedmem_ranks    = malloc(
        team_data->sharedmem_nodesize * sizeof(int));
    team_data->sharedmem_tab = malloc(size * sizeof(dart_team_unit_t));

    for (int i = 0; i < team_data->sharedmem_nodesize; i++) {
      sharedmem_ranks[i] = i;
    }

    MPI_Group_translate_ranks(
      sharedmem_group,
      team_data->sharedmem_nodesize,
      sharedmem_ranks,
      group_all,
      dart_unit_mapping);

    for (int n = 0; n < size; n++) {
      team_data->sharedmem_tab[n] = DART_UNDEFINED_TEAM_UNIT_ID;
    }
    for (int i = 0; i < team_data->sharedmem_nodesize; i++) {
      team_data->sharedmem_tab[dart_unit_mapping[i]] = DART_TEAM_UNIT_ID(i);
    }
    free(sharedmem_ranks);
    free(dart_unit_mapping);
  }

  return DART_OK;
}
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
