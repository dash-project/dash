/** @file mpi_adapt_team_private.h
 *  @date 22 Nov 2013
 *  @brief Function prototypes for operations on covertform.
 *
 *  A team can't determined by the teamID uniquely. Therefore
 *  here induce a convertform where each team is delegated to 
 *  a unique_id.
 *
 *  The unique_id is globally unique, and every running process
 *  should maintain such a synchronous convertform consistently.
 */

#ifndef DART_ADAPT_MPI_TEAM_PRIVATE_H_INCLUDED
#define DART_ADAPT_MPI_TEAM_PRIVATE_H_INCLUDED


/* Convertform type. */
struct team2unique
{
	dart_team_t team;
	int flag; /* Indicates if current unique_id has been occupied or not, 1:occupied, 0:empty. */
};
typedef struct team2unique uniqueitem_t;


/* -- The operation on convert form -- */

/** @brief Initialize a dart convert form.
 *
 *  called within the dart_adapt_init body.
 */
dart_ret_t dart_adapt_convertform_create ();

/** @brief Obtain the unique_id for the specified teamid.
 */
dart_ret_t dart_adapt_team_uniqueid (dart_team_t teamid, int32_t* unique_id);

/** @brief Insert (teamid,1) into convertform.
 */
dart_ret_t dart_adapt_convertform_add (dart_team_t teamid);

/** @brief Remove (teamid,1) from convertform.
 *         Reset flag.
 */
dart_ret_t dart_adapt_convertform_remove (dart_team_t teamid);

#endif /* DART_ADAPT_TEAM_GROUP_H_INCLUDED*/
