
#ifndef DART_TOPOLOGY_H_INCLUDED
#define DART_TOPOLOGY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#define DART_INTERFACE_ON

#ifdef _CRAYC
typedef struct {
	int unit_id;
    int reordered_unit_id;	/* New unit id respecting the application's communication pattern -- nearest neighbor for now */
	int num_units;
	int node_id;			/* Unique node id for each node on Cray's HazelHen machine */
	int num_levels;			/* Number of network hierarchical levels */
	int num_cores;			/* Number of cores per node  */
	int num_sockets;		/* Number of on-node sockets */
	
	/*
	There are total 4 off node network hierarchy levels on Cray machine 'HazelHen', which are: 
		Rank 0 (4 nodes)   or Intra-Aries network 
    	Rank 1 (64 nodes)  or Intra-Backplane network for Intra-Chassis communication 
    	Rank 2 (768 nodes) or Inter-Backplane network for Inter-Chassis as well as Inter-Cabinet communication 
    	Rank 3 (more than 768 nodes) or Inter-Group network 

	Therefore we need to calculate offset of each level to calculate reordered unit ids */
    int offset_at_level[4];       

    /* 
    There are total 5 off node levels in network hierarchy of Cray machine 'HazelHen' 
		4. Group number -- First level after the root of machine hierarchy on Hazel Hen    
		3. Cabinet number within a group   
		2. Chassis number within a cabinet     
		1. Compute blade number within a chassis   
		0. Node number within a compute blade 
    */
    int level[5];
} dart_topology_t;
#else
typedef struct {
	int node_id;		/* Unique node id */
	int num_levels;		/* Number of network hierarchical levels */
	int num_cores;		/* Number of cores per node  */
	int num_sockets;	/* Number of on-node sockets */
} dart_topology_t;
#endif

/* 
	------------------------------------------------------------------------------------
	Routines to inquire Hardware Toplogy
	NOTE: All of the DART topology routines should have function name prefix 'dart_top_'.
	-------------------------------------------------------------------------------------
*/

/* Allocate the memory dynamically for storing the topology of the allocated nodes on machine */
dart_ret_t dart_top_alloc(dart_topology_t * dart_topology, int num_units); 

/*	Inquire the topology information of calling unit. 
	This routine returns the node ID and position of the node in hardware topology (i.e. value of each hardware
	topology level for 'calling' unit).
	Nodes with same values of levels upto a particular level in hardware topology can form a team in order to 
	communicate effectively within the team and with other teams.
 */
dart_ret_t dart_top_inquire(dart_topology_t * dart_topology, int num_units); //collective routine -- needs to be called by all units

/* Get the total number of levels in hardware topology */
dart_ret_t dart_top_get_num_levels(dart_topology_t * dart_topology, int* num_levels);


/* Get the value of particular level of hardware topology for 'calling' DART unit */
dart_ret_t dart_top_get_level_value(dart_topology_t * dart_topology, int level, int* level_value);

/* Perform units / MPI processe mapping for nearest neighbor communication using the topology information of machine -- currently uses approach like Hilbert's space filling curve */
dart_ret_t dart_top_set_nearest(dart_topology_t *  dart_topology, int num_units, int num_units_per_node, dart_team_t * reordered_team);

/* Get node id of the calling unit. Node id is used as key to search the machine hierarchy file for obtaining the placement info of the node */
dart_ret_t dart_top_get_node_id(int * node_id);
#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TOPOLOGY_H_INCLUDED */
