#ifndef DART_TEAMNODE_H_INCLUDED
#define DART_TEAMNODE_H_INCLUDED

#define MAX_TEAM 128
struct dart_team_node
{
	struct dart_team_node* child;
	int32_t team_id;
	int32_t next_team_id[MAX_TEAM];
	MPI_Comm mpi_comm;
	struct dart_team_node* sibling;
	struct dart_team_node* parent;
};

typedef struct dart_team_node dart_teamnode;
typedef dart_teamnode* dart_teamnode_t;

extern dart_teamnode_t dart_header;

extern dart_teamnode_t dart_teamnode_create ();
extern dart_teamnode_t dart_teamnode_destroy ();
extern void dart_teamnode_add (dart_team_t team, MPI_Comm comm, dart_team_t *newteam);
extern void dart_teamnode_remove (dart_team_t team);
extern dart_teamnode_t dart_teamnode_query (dart_team_t team);
#endif /*DART_TEAMNODE_H_INCLUDED*/
