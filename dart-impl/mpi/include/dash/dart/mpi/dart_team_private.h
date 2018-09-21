/** @file dart_team_private.h
 *  @date 25 Aug 2014
 *  @brief Function prototypes for operations on available teamID linked list.
 *
 *  Question on the teamID numbering rules:
 *
 *  - The team ID of newteam will be unique with respect to the parent team.
 *    Whereby this rule, whether the following tree instance is correct or not?
 *    (Probably right)
 *    <pre>
 *
 *0                       DART_TEAM_ALL = 0 (0,1,2,3,4,5)
 *                        *                          *
 *1            teamID = 1 (0,1,2,3)                 teamID = 2 (3,4,5)
 *              *             *                    *             *
 *2    teamID = 0 (1,2)    teamID = 2 (0,3)  teamID = 0 (3,4)   teamID = 1 (4,5)
 *   </pre>
 *  Answer:
 *
 *  - It is wrong, furthermore, the teamID numbering rule should get improved.
 *
 *  - From the above tree, we can see unit 3 is in two different team with the same id 2
 *  in the mean time, which should be avoided in our new teamID numbering rule.
 *
 *  Two new proposed teamID numbering rules:
 *
 *  1. The first teamID numbering rule proposal:
 *  When a unit exists in several different teams, it requires those teams' ID should also be distinct
 *  with each other.
 *
 *  Algorithm complying with the above new rule:
 *  <ul>
 *  <li> Structure: Every unit maintain a linked list consisting of all the available teamid for itself.
 *  This list should be arranged in an increasing order based upon the ID.
 *
 *  <li> Operation on the list:
 *      <ol>
 *    <li> Insert <-> team destroy: insert the destroyed team ID into the linked list of all the units belonging
 *      to this destroyed team.<br>
 *
 *        Other units do nothing on their own lists.<br>
 *
 *        The ordering of the list should be kept the same after inserting.<br>
 *
 *      <li> Delete <-> team creation: deleting the created team ID from the linked list of all the units belonging
 *      to this created team.<br>
 *
 *        Other units do nothing on their own lists.
 *
 *        It is tricky to find a minimum as well as common available team ID for all the units belonging to this team.
 *      </ol>
 *  <li> The destroyed team ID can be recycled and reused again.
 *  </ul>
 *
 *  Legend: (show how the linked lists work here)
 *  <pre>
 *  + Insert  - Delete
 *  (0, 1, 2, 3) _ 0
 *  0: 1 --- 2 --- 3 --- 4
 *  1: 1 --- 2 --- 3 --- 4
 *  2: 1 --- 2 --- 3 --- 4
 *  3: 1 --- 2 --- 3 --- 4
 *
 *
 *  +(0,1,2) _ 1(sub_team ID) -> 0 (parent team ID)
 *  0: 2 --- 3 --- 4
 *  1: 2 --- 3 --- 4
 *  2: 2 --- 3 --- 4
 *  3: 1 --- 2 --- 3 --- 4
 *
 *  +(1,3) _ 2 -> 0
 *  0: 2 --- 3 --- 4
 *  1: 3 --- 4
 *  2: 2 --- 3 --- 4
 *  3: 1 --- 3 --- 4
 *
 *  +(0,2) _ 2 -> 1
 *  0: 3 --- 4
 *  1: 3 --- 4
 *  2: 3 --- 4
 *  3: 1 --- 3 --- 4
 *
 *  -(0,1,2)
 *  0: 1 --- 3 --- 4
 *  1: 1 --- 3 --- 4
 *  2: 1 --- 3 --- 4
 *  3: 1 --- 3 --- 4
 *  </pre>
 *  2. The second team numbering rule proposal: (should meet the below three requirements)\n
 *  All the sub-teamids should be unique with respect to their parent teamid.\n
 *  Its teamid won't be reused again after a team is destroyed.\n
 *  When a unit belongs to several different teams, those teams' ID shouldn't be identical.\n
 *
 *  Algorithm complying with the above rule:
 *  <ul>
 *  <li> Structure: every unit maintain a counter named by 'next_availteamid',
 *       which indicates the next available teamid for itself.
 *
 *  <li> Operation on the counter:
 *  <ol>
 *    <li>
 *       Team creation:
 *
 *       Calculating out the maximum among all the next_availteamids of those
 *       units belonging to the new created
 *       subteam. And the maximum will be the new created subteam ID.<br>
 *
 *       Modify next_availteamid for all the units belonging to the parent
 *       team. So nex_availteamid = maximum + 1.<br>
 *    </li>
 *    <li>
 *       Team destroy:
 *       Do nothing on next_availteamid as this teamid would be discarded and
 *       not be reused.<br>
 *    </li>
 *  </ol>
 *  </ul>
 *  Legend:
 *  <pre>
 *  ALL - 0       0-(0,1,2,3)-1       1-(1,2,3)-2      0-(4,5)-3
 *  0 1           2   X               3                4
 *  1 1           2   X               3   X            4
 *  2 1    ---->  2   X        ---->  3   X      ----> 4
 *  3 1           2   X               3   X            4
 *  4 1           2                   2       -        4   X
 *  5 1           2                   2        - or    4   X
 *                                              ---->  3
 *                                                     3
 *                                                     3
 *                                                     3
 *                                                     3   X
 *                                                     3   X
 *  </pre>
 *
 *  Conclusion: The second new team numbering rule proposal is adopted on current stage.
 */

#ifndef DART_ADAPT_TEAM_PRIVATE_H_INCLUDED
#define DART_ADAPT_TEAM_PRIVATE_H_INCLUDED

#include <mpi.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_segment.h>
#include <dash/dart/base/macro.h>

extern dart_team_t dart_next_availteamid DART_INTERNAL;

extern MPI_Comm dart_comm_world DART_INTERNAL;
#define DART_COMM_WORLD dart_comm_world

#define DART_MAX_TEAM_NUMBER (256)

typedef struct dart_team_data {

  struct dart_team_data *next;

  /**
   * @brief The communicator corresponding to this team.
   */
  MPI_Comm comm;

  /**
   * @brief MPI dynamic window object corresponding this team.
   */
  MPI_Win window;

  dart_segmentdata_t segdata;

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  /**
   * @brief Store the sub-communicator with regard to certain node, where the units can
   * communicate via shared memory.
   */
  MPI_Comm sharedmem_comm;

  /**
   * @brief Hash table to determine the units who are located in the same node.
   */
  dart_team_unit_t *sharedmem_tab;

  /**
   *  @brief Size of the node's communicator.
   */
  int sharedmem_nodesize;

#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

  dart_unit_t unitid;

  int         size;

  dart_team_t teamid;

  struct dart_lock_struct *allocated_locks;

} dart_team_data_t;

/* @brief Initiate the free-team-list and allocated-team-list.
 *
 * This call will be invoked within dart_init(), and the free teamlist consist of
 * 256 nodes with index ranging from 0 to 255. The allocated teamlist array is set to
 * be empty.
 */
dart_ret_t dart_adapt_teamlist_init() DART_INTERNAL;

/* @brief Destroy the free-team-list and allocated-team-list.
 *
 * This call will be invoked within dart_eixt(), and the free teamlist is freed,
 * the allocated teamlist array is reset back to be empty.
 */
dart_ret_t dart_adapt_teamlist_destroy() DART_INTERNAL;

/* @brief Allocate the first available index from the free-team-list.
 *
 * This call will be invoked when a team with teamid is created, and only
 * the units belonging to the given teamid can enter this call.
 *
 * @param[in]  teamid  The newly created team ID.
 * @param[out] index   The unique ID related to the newly created team.
 */
dart_ret_t dart_adapt_teamlist_alloc(dart_team_t teamid) DART_INTERNAL;

/**
 * Deallocate the teamlist entry.
 */
dart_ret_t
dart_adapt_teamlist_dealloc(dart_team_t teamid) DART_INTERNAL;

/**
 * Retrieve the \c dart_team_data for \c teamid.
 */
dart_team_data_t *
dart_adapt_teamlist_get(dart_team_t teamid) DART_INTERNAL;

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
/*
 * Allocate shared memory communicator for the given \c team_data.
 * Shared between \c dart_initialize and \c dart_team_create.
 */
dart_ret_t dart_allocate_shared_comm(
  dart_team_data_t *team_data) DART_INTERNAL;
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

#endif /*DART_ADAPT_TEAMNODE_H_INCLUDED*/

