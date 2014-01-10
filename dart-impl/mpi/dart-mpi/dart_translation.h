#ifndef DART_TRANSLATION_H_INCLUDED
#define DART_TRANSLATION_H_INCLUDED
#include<stdio.h>
#include<mpi.h>
#define MAX_NUMBER 256
#include "./dart_types.h"



typedef struct
{
	MPI_Win win;
//	MPI_Group group;
}GMRh;

typedef struct
{
	int offset;
	GMRh handle;
}info_t;

struct node
{
	info_t trans;
	struct node* next;
};
typedef struct node node_info;
typedef node_info* node_t;

//extern info_t trantable_localalloc[MAX_NUMBER];
//extern node_t trantable_globalalloc[MAX_NUMBER];

extern dart_ret_t dart_trantable_create (int uniqueid);
extern dart_ret_t dart_trantable_add (int uniqueid, info_t item);
extern dart_ret_t dart_trantable_remove (int uniqueid, int offset);
extern MPI_Comm dart_trantable_query (int uniqueid, int offset, int *begin);
#endif /*DART_TRANSLATION_H_INCLUDED*/
