/** @file dart_adapt_teamnode.h
 *  @date 22 Nov 2013
 *  @brief Function prototypes for operations on team hierarchical tree.
 *
 *  Questions on the teamID numbering rules:
 *
 *  - The team ID of newteam will be unique with respect to the parent team.
 *    Whereby this rule, whether the following tree instance is correct or not?
 *    (Probably right)
 *
 *0                       DART_TEAM_ALL = 0 (0,1,2,3,4,5)       
 *                        *                          *
 *1            teamID = 1 (0,1,2,3)                 teamID = 2 (3,4,5)
 *              *             *                    *             *
 *2    teamID = 0 (1,2)    teamID = 2 (0,3)  teamID = 0 (3,4)   teamID = 1 (4,5)              
 */

#ifndef DART_ADAPT_TEAMNODE_H_INCLUDED
#define DART_ADAPT_TEAMNODE_H_INCLUDED

#define MAX_TEAM 256
#include "dart_deb_log.h"

/** @brief The teamnode type in the dart team hierarchical tree.
 *
 *  Every team created during the dart programme would corresponds to a 
 *  node in the team hierarchical tree.
 *
 *  Therefore, the team node should cover the informations representing 
 *  a unique team.
 *
 *  Here, the team hierarchical structure is depicted with the way of
 *  a binary tree.
 */
struct dart_teamnode_struct
{
	struct dart_teamnode_struct* child; /* The first child team. */
	int32_t team_id; /* The identifier for this team. */
	int32_t next_team_id[MAX_TEAM]; /*The next available child-team id for this team. */
	MPI_Comm mpi_comm; /* The corresponding communicator for this team. */
	struct dart_teamnode_struct* sibling; /* Set of child-teams. */
	struct dart_teamnode_struct* parent; /* Parent team. */
};

typedef struct dart_teamnode_struct* dart_teamnode_t;


/** @brief Initialize the team tree.
 *  
 *  This function will be called within the dart_adapt_init body.
 */
dart_ret_t dart_adapt_teamnode_create ();

/** @brief Finalize the team tree.
 *  
 *  This function will be called within the dart_adapt_exit body.
 *
 *  @return		Dart status.
 */ 
dart_ret_t dart_adapt_teamnode_destroy ();

/** @brief Insert newly generated team into the team tree.
 */
dart_ret_t dart_adapt_teamnode_add (dart_team_t teamid, MPI_Comm comm, dart_team_t *newteam);

/** @brief Remove the specified teamid from the team tree.
 */
dart_ret_t dart_adapt_teamnode_remove (dart_team_t teamid);

/** @brief Query the corresponding node in the team tree based upon the 
 *  specified teamid.
 */
dart_ret_t dart_adapt_teamnode_query (dart_team_t teamid, dart_teamnode_t* p);

#endif /*DART_ADAPT_TEAMNODE_H_INCLUDED*/
