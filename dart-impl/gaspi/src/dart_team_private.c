#include <stdio.h>
#include <assert.h>

#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/base/logging.h>

gaspi_group_t gaspi_group_id_top;

dart_team_t dart_next_availteamid;

dart_team_struct_t dart_teams[DART_MAX_TEAM_NUMBER];



struct dart_free_teamlist_entry
{
    uint16_t index;
    struct dart_free_teamlist_entry* next;
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

/**
 * create list for all dart teams
 *
 * dart_free_teamlist_header points to the head of the list
 * each entry has the default index for the dart_teams array
 *
 *  e.g. DART_MAX_TEAM_NUMBER == 2
 *
 * dart_teams[2];
 *
 * dart_free_teamlist_header
 *         |
 *         |
 *  -------------
 * | index = 0 |     -------------
 * | next----------->| index = 1 |
 * |           |     | next-------->NULL
 * -------------     |           |
 *                   -------------
 */
int dart_adapt_teamlist_init ()
{
    dart_free_teamlist_ptr pre = NULL;
    dart_free_teamlist_ptr newAllocateEntry;
    for(int i = 0; i < DART_MAX_TEAM_NUMBER; i++)
    {
        newAllocateEntry = (dart_free_teamlist_ptr) malloc(sizeof (dart_free_entry));
        assert(newAllocateEntry);

        newAllocateEntry->index = i;
        if (pre != NULL)
        {
            pre->next = newAllocateEntry;
        }
        else
        {
            dart_free_teamlist_header = newAllocateEntry;
        }
        pre = newAllocateEntry;
    }
    newAllocateEntry->next = NULL;

    dart_allocated_teamlist_size = 0;
    return 0;
}

int dart_adapt_teamlist_destroy ()
{
    dart_free_teamlist_ptr pre, p = dart_free_teamlist_header;
    while (p)
    {
        pre = p;
        p = p->next;
        free (pre);
    }
    dart_allocated_teamlist_size = 0;
    return 0;
}

int dart_adapt_teamlist_alloc (dart_team_t teamid, uint16_t* index)
{
    dart_free_teamlist_ptr p;
    if (dart_free_teamlist_header != NULL)
    {
        *index = dart_free_teamlist_header->index;
        p = dart_free_teamlist_header;

        dart_free_teamlist_header = dart_free_teamlist_header->next;
        free(p);
        /* The allocated teamlist array should be arranged in an increasing order based on
         * the member of allocated_teamid.
         *
         * Notes: the newly created teamid will always be increased because the teamid
         * is not reused after certain team is destroyed.
         */
        dart_allocated_teamlist_array[dart_allocated_teamlist_size].index = *index;
        dart_allocated_teamlist_array[dart_allocated_teamlist_size].allocated_teamid = teamid;
        dart_allocated_teamlist_size++;

        /* If allocated successfully, the position of the new element in the allcoated array
         * is returned.
         */
        return (dart_allocated_teamlist_size - 1);

    }
    else
    {
        fprintf(stderr,"Out of bound: exceed the MAX_TEAM_NUMBER limit");
        return -1;
    }
}

int dart_adapt_teamlist_recycle (uint16_t index, int pos)
{
    dart_free_teamlist_ptr newAllocateEntry = (dart_free_teamlist_ptr) malloc (sizeof (dart_free_entry));
    assert(newAllocateEntry);

    newAllocateEntry->index   = index;
    newAllocateEntry->next    = dart_free_teamlist_header;
    dart_free_teamlist_header = newAllocateEntry;
    /*
     * The allocated teamlist array should be keep as an ordered array
     * after deleting the given element.
     */
    for (int i = pos; i < dart_allocated_teamlist_size - 1; i ++)
    {
        dart_allocated_teamlist_array[i].allocated_teamid = dart_allocated_teamlist_array[i + 1].allocated_teamid;
        dart_allocated_teamlist_array[i].index = dart_allocated_teamlist_array[i + 1].index;
    }
    dart_allocated_teamlist_size --;
    return 0;
}

int dart_adapt_teamlist_convert (dart_team_t teamid, uint16_t* index)
{
    if (teamid == DART_TEAM_ALL)
    {
        DART_LOG_TRACE("dart_adapt_teamlist_convert: teamid == DART_TEAM_ALL -> index == 0");
        *index = 0;
        return 0;
    }
    /* Locate the teamid in the allocated teamlist array by using the binary-search approach. */
    DART_LOG_DEBUG("dart_adapt_teamlist_convert: Using binary search tree to locate teamid: %d", teamid);
    int imin, imax;
    imin = 0;
    imax = dart_allocated_teamlist_size;
    while (imin < imax)
    {
        int imid = (imin + imax) >> 1;
        if (dart_allocated_teamlist_array[imid].allocated_teamid < teamid)
        {
            imin = imid + 1;
        }
        else
        {
            imax = imid;
        }
    }
    if ((imax == imin) && (dart_allocated_teamlist_array[imin].allocated_teamid == teamid))
    {
        *index = dart_allocated_teamlist_array[imin].index;
        /* If search successfully, the position of the teamid in array is returned. */
        DART_LOG_DEBUG("dart_adapt_teamlist_convert: Found index: %d for teamid: %d", (*index), teamid);
        return imin;
    }
    else
    {
        DART_LOG_ERROR("dart_adapt_teamlist_convert: Invalid teamid input: %d. Total teams: %d. Index %d. Team with this index: %d\n", teamid, dart_allocated_teamlist_size, imin, dart_allocated_teamlist_array[imin].allocated_teamid);
        return -1;
    }
}
