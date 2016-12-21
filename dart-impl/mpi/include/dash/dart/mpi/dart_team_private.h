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

extern dart_team_t dart_next_availteamid;

extern MPI_Comm dart_comm_world;
#define DART_COMM_WORLD dart_comm_world


typedef struct dart_team_data {
  /**
   * @brief The communicator corresponding to this team.
   */
  MPI_Comm comm;

  /**
   * @brief MPI dynamic window object corresponding this team.
   */
  MPI_Win window;

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

} dart_team_data_t;

extern dart_team_data_t dart_team_data[DART_MAX_TEAM_NUMBER];



#if 0

/* @brief Translate the given teamid (indicated uniquely by the index) into its corresponding communicator.
 *
 * After locating the given teamid in the teamlist,
 * we find that teamlist[i] equals to teamid, which means teams[i]
 * will be the corresponding communicator of teamid.
 */
extern MPI_Comm dart_teams[DART_MAX_TEAM_NUMBER];

/* @brief Store the sub-communicator with regard to certain node, where the units can
 * communicate via shared memory.
 *
 * The units running in certain node vary
 * according to the specified team.
 * The values of dart_sharedmem_comm_list[i] are different for the units belonging to different nodes.
 */
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
extern MPI_Comm dart_sharedmem_comm_list[DART_MAX_TEAM_NUMBER];

/* @brief Sets of units who are located in the same node for each unit in MAX_TEAM_NUMBER teams.
 *
 * Each element of this array will relate to certain team A.
 * Set of units belonging to the same node vary for different team.
 * Each unit stores all the IDs of those units (including itself) who are parts of team A and
 * located in the same node as it is.
 */
//extern int* dart_unit_mapping[MAX_TEAM_NUMBER];

/* @brief This table is represented as a hash table
 * , which is used to determine the units who are located in the same node.
 *
 * Each element of this array will relate to certain team.
 */
extern int* dart_sharedmem_table[DART_MAX_TEAM_NUMBER];

/* @brief Set of the size of node for each unit in MAX_TEAM_NUMBER teams.
 */
extern int dart_sharedmemnode_size[DART_MAX_TEAM_NUMBER];
#endif

/* @brief Set of MPI dynamic window objects corresponding to MAX_TEAM_NUMBER teams. */
extern MPI_Win dart_win_lists[DART_MAX_TEAM_NUMBER];

#endif // 0

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

extern char* *dart_sharedmem_local_baseptr_set;
#endif
/* @brief Initiate the free-team-list and allocated-team-list.
 *
 * This call will be invoked within dart_init(), and the free teamlist consist of
 * 256 nodes with index ranging from 0 to 255. The allocated teamlist array is set to
 * be empty.
 */
int dart_adapt_teamlist_init ();

/* @brief Destroy the free-team-list and allocated-team-list.
 *
 * This call will be invoked within dart_eixt(), and the free teamlist is freed,
 * the allocated teamlist array is reset back to be empty.
 */
int dart_adapt_teamlist_destroy ();

/* @brief Allocate the first available index from the free-team-list.
 *
 * This call will be invoked when a team with teamid is created, and only
 * the units belonging to the given teamid can enter this call.
 *
 * @param[in]  teamid  The newly created team ID.
 * @param[out] index   The unique ID related to the newly created team.
 */
int dart_adapt_teamlist_alloc(dart_team_t teamid, uint16_t *index);

/* @brief Insert the freed index into the free-team-list, and delete the element with given index
 * from the allocated-team-list-array.
 *
 * This call will be invoked when a new team is destroyed.
 */
int dart_adapt_teamlist_recycle(uint16_t index, int pos);

/* @brief Locate the given teamid in the alloated-team-list-array.
 */
int dart_adapt_teamlist_convert (dart_team_t teamid, uint16_t* index);

#endif /*DART_ADAPT_TEAMNODE_H_INCLUDED*/

