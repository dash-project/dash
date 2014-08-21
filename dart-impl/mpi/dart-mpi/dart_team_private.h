/** @file dart_adapt_team_private.h
 *  @date 25 Mar 2014
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
 *  	<li> Insert <-> team destroy: insert the destroyed team ID into the linked list of all the units belonging 
 *      to this destroyed team.<br>
 *
 *      	Other units do nothing on their own lists.<br>
 *
 *      	The ordering of the list should be kept the same after inserting.<br>
 *
 *      <li> Delete <-> team creation: deleting the created team ID from the linked list of all the units belonging 
 *      to this created team.<br>
 *
 *      	Other units do nothing on their own lists.
 *
 *      	It is tricky to find a minimum as well as common available team ID for all the units belonging to this team.
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
 *  <li> Structure: every unit maintain a counter named by 'next_availteamid', which indicates the next available teamid for itself.
 *
 *  <li> Operation on the counter:
 *	<ol>
 *  	<li> Team creation: 
 *
 *   		Calculating out the maximum among all the next_availteamids of those units belonging to the new created 
 *  subteam. And the maximum will be the new created subteam ID.<br>
 *
 *   		Modify next_availteamid for all the units belonging to the parent team. So nex_availteamid = maximum + 1.<br>
 *
 *  	<li> Team destroy:
 *  		 Do nothing on next_availteamid as this teamid would be discarded and not be reused.<br>
 *  	</ol>
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
#include "dart_deb_log.h"
#include "dart_mem.h"

extern dart_team_t next_availteamid;

/* @brief Translate the given teamid (indicated uniquely by the index) into its corresponding communicator. 
 * 
 * After locating the given teamid in the teamlist, 
 * we find that teamlist[i] equals to teamid, which meas teams[i] 
 * will be the corresponding communicator of teamid.
 */
extern MPI_Comm teams[MAX_TEAM_NUMBER];

/* @brief Store the sub-communicator with regard to certain node, where the units can 
 * communicate via shared memory.
 *
 * The units runing in certain node vary 
 * according to the specified team.
 * The values of numa_comm_list[i] are different for the units belonging to different nodes. 
 */
extern MPI_Comm sharedmem_comm_list[MAX_TEAM_NUMBER];

/* @brief Sets of units who are located in the same node for each unit in MAX_TEAM_NUMBER teams.
 *
 * Each element of this array will relate to certain team A.
 * Set of units belonging to the same node vary for different team.
 * Each unit stores all the IDs of those units (including itself) who are parts of team A as well as
 * located in the same node as it.
 */
//extern int* dart_unit_mapping[MAX_TEAM_NUMBER];

/* @brief This table is represented as a hash table
 * , which is used to determine the units who are located in the same node.
 *
 * Each element of this array will relate to certain team.
 */
extern int* dart_sharedmem_table[MAX_TEAM_NUMBER];

/* @brief Set of the size of node for each unit in MAX_TEAM_NUMBER teams.
 */
extern int dart_sharedmemnode_size[MAX_TEAM_NUMBER];

/* @brief Set of MPI dynamic window objects corresponding to MAX_TEAM_NUMBER teams. */
extern MPI_Win win_lists[MAX_TEAM_NUMBER];
/* @brief Initiate the teamlist.
 *
 * This call will be invoked within dart_adapt_init(), and each element in the returned list 
 * is thought to be an empty slot.
 */
int dart_adapt_teamlist_init ();

int dart_adapt_teamlist_destroy ();

/* @brief Allocate the first empty slot in the teamlist.
 *
 * This call will be invoked when a team with teamid is created, and only 
 * the units belonging to the given teamid can enter this call.
 *
 * @param[in]	teamid	The new created team ID.
 * @param[out]	index	The position of the given teamid in the teamlist.
 */
int dart_adapt_teamlist_alloc(dart_team_t teamid, int *index);

/* @brief Free the slot specified by index in the teamlist.
 *
 * This call will be invoked when a new team is destroyed. teamlist[index]
 * = -1, which means the index-th element goes back to an empty slot.
 */
int dart_adapt_teamlist_recycle(int index, int pos);

/* @brief Locate the given teamid in the teamlist.
 */
int dart_adapt_teamlist_convert (dart_team_t teamid, int* index);

#endif /*DART_ADAPT_TEAMNODE_H_INCLUDED*/

