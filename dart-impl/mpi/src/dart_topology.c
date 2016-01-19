/** @file topology.c
 *  @date 07 April 2015
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <mpi.h>
#include <string.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_topology.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/mpi/dart_team_private.h>

#ifdef _CRAYC
static const int NUM_NODES = 7798;
static const int NUM_TOPOLOGY_PARAMETERS = 3;

typedef int (*compfn)(const void*, const void*);

/* Static function to sort the nodes according to their position in machine hierarchy */ 
static int sort_nodes(dart_topology_t * node1, dart_topology_t * node2){

    /* Compare level 4 of nodes */
	if (node1->level[4] < node2->level[4])
        return -1;
    else if (node1->level[4] > node2->level[4])
        return 1;

    /* If level 4 of both nodes is same, then compare level 3 */
	if (node1->level[3] < node2->level[3])
        return -1;
    else if (node1->level[3] > node2->level[3])
        return 1;

    /* If level 3 of both nodes is same, then compare level 2 */
	if (node1->level[2] < node2->level[2])
        return -1;
    else if (node1->level[2] > node2->level[2])
        return 1;

    /* If level 2 of both nodes is same, then compare level 1 */
	if (node1->level[1] < node2->level[1])
        return -1;
    else if (node1->level[1] > node2->level[1])
        return 1;

    /* If level 1 of both nodes is same, then compare level 0 */
	if (node1->level[0] < node2->level[0])
        return -1;
    else if (node1->level[0] > node2->level[0])
        return 1;

    else
        return 0; /* definitely! */

}


/* The functions 'get_num_nodes_in_each_vertex' and 'count_num_vertices' can be utilized if in future we perform graph partitioning to perform the units / MPI processes mapping */
static void get_num_nodes_in_each_vertex(dart_topology_t * dart_topology, int num_units, int level_number, int * num_nodes_in_vertex, int max_value_of_vertex){
	int i, j;

	/* Set number of nodes in each vertex to zero */
	for(j = 0; j <= max_value_of_vertex; j++)
		num_nodes_in_vertex[j] = 0; // number of processes in vertex number 'j' at level 'level_number'

	/* Count number of tasks in each vertex */
	for(i = 0; i < num_units; i++){ //upper limit should be number of tasks rather than number of nodes as we need to go through all tasks
		for(j = 0; j <= max_value_of_vertex; j++){
			if(dart_topology[i].level[level_number] == j){
				num_nodes_in_vertex[j]++; // number of processes in vertex number 'j' at level 'level_number'
				continue;  	
			}
		}	
	}

//	/* To get number of nodes in each vertex, divide number of tasks in each vertex by number tasks per node */
//	for(j = 0; j <= max_value_of_vertex; j++)
//		num_nodes_in_vertex[j] /= tasksTopology.numTasksPerNode; // number of processes in vertex number 'j' at level 'level_number'
}

static int count_num_vertices(int * num_nodes_in_vertex, int max_value_of_vertex){
	int i, count_vertices = 0;

	for(i = 0; i <= max_value_of_vertex; i++){
		if(num_nodes_in_vertex[i] > 0)
			count_vertices++;
	}
	return count_vertices;	
}

dart_ret_t dart_top_get_node_id(int * node_id) {
	
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
	/* Get the name of the processor and then parse it to obtain NODE ID of unit / MPI process. */
    MPI_Get_processor_name(processor_name, &name_len);

    char *p = processor_name;
    while (*p) { // While there are more characters to process...
        if(isdigit(*p)) { // Upon finding a digit, ...
            *node_id = strtol(p, &p, 10); // Read a number, ...
            break;
        } 
        else{ 
            p++;
        }
    }

	return DART_OK;
}

dart_ret_t dart_top_alloc(dart_topology_t * dart_topology, int num_units) {
	
	return DART_OK;
}

dart_ret_t dart_top_inquire(dart_topology_t * dart_topology, int num_units) {

/* ------------------------------------------------------------------------------------------- */
/* ------------------ Read, Parse and Sort TOPOLOGY INFORMATION of machine ------------------- */
/* ------------------------------------------------------------------------------------------- */

	int i, j;

	/* 1. Get Node ID of the calling unit. Node ID is used as key to search the topology file of machine to get the placement information of a node in network hierarchy */
    int node_id;
	dart_top_get_node_id(&node_id);

	/* 2. Perform collective Allgather operation to obtain NODE IDs of all the units / MPI processes */
	int * node_ids_buffer;
    node_ids_buffer = (int *) malloc(num_units * sizeof(int));
	MPI_Allgather(&node_id, 1, MPI_INT, node_ids_buffer, 1, MPI_INT, MPI_COMM_WORLD);

	/* 3. Allocate memory for storing the topology information and then read the topology file of machine */
    char* topology[NUM_NODES][NUM_TOPOLOGY_PARAMETERS];
    for(int i = 0; i < NUM_NODES; i++) {
        for(int j = 0; j < NUM_TOPOLOGY_PARAMETERS; j++) 
            topology[i][j] = (char*) malloc(50 * sizeof(char)); 
    } 

    FILE *file = fopen("HazelHenTopologyNew.txt", "r");
    for(int i = 0; i < NUM_NODES; i++) {
        if(feof(file))
            break;
        fscanf(file, "%s %s %s", topology[i][0], topology[i][1], topology[i][2]);
    }
    fclose(file);

	/* 4. Parse topology information and save it to topology structure of each node */
	/* NOTE: As Cray scheduler can assign processes/units to nodes in any order (e.g. process 0 on NID 7811, process 1 on NID), we need to go for suboptimal method for parsing topology information of machine */
//	dart_topology	= (dart_topology_t *) malloc(num_units * sizeof(dart_topology_t));
	for(i = 0; i < num_units; i++){
		for(j = 0; j < NUM_NODES; j++){

			if(node_ids_buffer[i] == atoi(topology[j][0])){
				dart_topology[i].node_id = atoi(topology[j][0]);
	
				char info[50];
				memcpy(info, topology[j][1], 50);

				char* token[5];
				token[0] = strtok (info,"-");
				token[1] = strtok (NULL, "c");
				token[2] = strtok (NULL, "s");
				token[3] = strtok (NULL, "n");
				token[4] = strtok (NULL, "\0");
	
				dart_topology[i].level[4] = atoi(token[0]+1); 
				dart_topology[i].level[3] = atoi(token[1]);
				dart_topology[i].level[2] = atoi(token[2]);
				dart_topology[i].level[1] = atoi(token[3]);
				dart_topology[i].level[0] = atoi(token[4]);

				dart_topology[i].num_levels = 5;

				break;
			}
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
    return DART_OK;
}


dart_ret_t dart_top_get_num_levels(dart_topology_t * dart_topology, int* num_levels) {

	*num_levels = dart_topology->num_levels;
	return DART_OK;
}

dart_ret_t dart_top_get_level_value(dart_topology_t * dart_topology, int level, int* level_value) {

	*level_value = dart_topology->level[level];
	return DART_OK;
}

dart_ret_t dart_top_set_nearest(dart_topology_t *  dart_topology, int num_units, int num_units_per_node, dart_team_t * reordered_team){
	int unit_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &unit_id);        

	qsort(dart_topology, num_units, sizeof(dart_topology_t), (compfn)sort_nodes);

    int num_units_in_dim[3] 	        = {0, 0, 0}; /* Number of units in each dimension */
	int num_units_per_node_in_dim[3]	= {0, 0, 0}; /* Number of units in each dimension of a single node */	

   /*  Perform balanced distribution of total number of units among coordinate directions */
    MPI_Dims_create(num_units, 3, num_units_in_dim);

	/* On node tasks distibution for performing multicore group tasks mapping */
    MPI_Dims_create(num_units_per_node, 3, num_units_per_node_in_dim);

	const int num_levels				=	5;
	int num_vertices[5]					=	{3, 15, 2, 3, 11}; // extracted from the topology file of HazelHen -- Max vertex value at each level
//	int num_nodes_at_network_level[4]	=	{4, 64, 768, 9216};
	int num_nodes_third_level[3]		=	{2, 2};
	int num_nodes_second_level[3]		=	{4, 2, 2};
	int num_nodes_first_level[3]		=	{2, 2};

	/* Given the nodes are sorted according to their multiple hierarchical levels, we can now assign the new task IDs to each task moving down the hierarchy and maintaing spatial locality */
	int x, y, z, a, b, i, j, k, unit_number = 0;
	for(x = 0; x < num_nodes_second_level[0]; x++) {
		for(y = 0; y < num_nodes_second_level[1]; y++) {
			for(z = 0; z < num_nodes_second_level[2]; z++) {
				for(a = 0; a < num_nodes_first_level[0]; a++) {
					for(b = 0; b < num_nodes_first_level[1]; b++) {
						for(i = 0; i < num_units_per_node_in_dim[0]; i++) {
							for(j = 0; j < num_units_per_node_in_dim[1]; j++) {
								for(k = 0; k < num_units_per_node_in_dim[2]; k++) {
									dart_topology[unit_number].offset_at_level[1]	= x * num_units_per_node_in_dim[0] * num_units_in_dim[1] * num_units_in_dim[2] + y * num_nodes_first_level[0] * num_units_per_node_in_dim[1] * num_units_in_dim[2] + z * num_nodes_first_level[1]  * num_units_per_node_in_dim[2];
									dart_topology[unit_number].offset_at_level[0]		= a * num_units_per_node_in_dim[1] * num_units_in_dim[2] + b * num_units_per_node_in_dim[2]; 
									dart_topology[unit_number].reordered_unit_id			= dart_topology[unit_number].offset_at_level[1] + dart_topology[unit_number].offset_at_level[0] + i * num_units_in_dim[1] * num_units_in_dim[2] + j * num_units_in_dim[2] + k;	
									dart_topology[unit_number].unit_id = unit_number;
									unit_number++;
								}		
							}
						}
					}
				}
			}
		}
	}

	
    int proxy_rank = dart_topology[unit_id].reordered_unit_id;
	MPI_Comm reordered_comm;
    MPI_Comm_split(MPI_COMM_WORLD, 0, proxy_rank, &reordered_comm);

	
	*reordered_team = DART_TEAM_NULL;
	uint16_t index;
	dart_team_t max_teamid = -1;

	/* Get the maximum next_availteamid among all the units belonging to
   * the parent team specified by 'teamid'. */
	MPI_Allreduce(&dart_next_availteamid, &max_teamid, 1, MPI_INT32_T, MPI_MAX, MPI_COMM_WORLD);
	dart_next_availteamid = max_teamid + 1;

	if (reordered_comm != MPI_COMM_NULL) {
		int result = dart_adapt_teamlist_alloc(max_teamid, &index);
		if (result == -1) {
			return DART_ERR_OTHER;
		}
		/* max_teamid is thought to be the new created team ID. */
		*reordered_team = max_teamid;
		dart_teams[index] = reordered_comm;
	}
	
    return DART_OK;
}

#else
dart_ret_t dart_top_alloc(dart_topology_t * dart_topology, int num_units){
    return DART_ERR_NOTFOUND;
}
dart_ret_t dart_top_inquire(dart_topology_t * dart_topology, int num_units){
    return DART_ERR_NOTFOUND;
}
dart_ret_t dart_top_get_num_levels(dart_topology_t * dart_topology, int* num_levels){
    return DART_ERR_NOTFOUND;
}
dart_ret_t dart_top_get_level_value(dart_topology_t * dart_topology, int level, int* level_value){
    return DART_ERR_NOTFOUND;
}
dart_ret_t dart_top_set_nearest(dart_topology_t *  dart_topology, int num_units, int num_units_per_node, MPI_Comm * reordered_comm){
    return DART_ERR_NOTFOUND;
}
dart_ret_t dart_top_get_node_id(int * node_id){
    return DART_ERR_NOTFOUND;
}
#endif
