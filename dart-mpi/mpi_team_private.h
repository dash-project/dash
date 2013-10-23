#ifndef DART_MPI_TEAM_PRIVATE_H_INCLUDED
#define DART_MPI_TEAM_PRIVATE_H_INCLUDED


// the unique is globally unique, every running processes should mantain such a synchronous convert form 
struct team2unique
{
	dart_team_t team;
	int flag;
};
typedef struct team2unique uniqueitem;
extern uniqueitem convertform[MAX_TEAM_NUMBER];

extern dart_ret_t dart_team_uniqueid (dart_team_t team, int32_t* unique_id);
extern dart_ret_t dart_convertform_add (dart_team_t team);
extern dart_ret_t dart_convertform_remove (dart_team_t team);

#endif /* DART_TEAM_GROUP_H_INCLUDED*/
