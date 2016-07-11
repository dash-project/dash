
/*
 * Include config and utmpx.h first to prevent previous include of utmpx.h
 * without _GNU_SOURCE in included headers:
 */
#include <dash/dart/base/config.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/netinfo.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#ifdef DART_ENABLE_NETLOC
//#  include <netloc.h>
//#  include <netloc/helper.h>
#endif



dart_ret_t dart_netinfo(
  dart_netinfo_t * netinfo)
{
  DART_LOG_DEBUG("dart_netinfo()");

  dart_netinfo_t net;

  net.global_node_id                = -1;
  net.value_net_level[0]            = -1;
  net.value_net_level[1]            = -1;
  net.value_net_level[2]            = -1;
  net.value_net_level[3]            = -1;
  net.value_net_level[5]            = -1;
  net.bw_at_level[0]                = -1;
  net.bw_at_level[1]                = -1;
  net.bw_at_level[2]                = -1;
  net.bw_at_level[3]                = -1;
  net.bw_at_level[5]                = -1;
  net.bw_bisection                  = -1;
  net.msg_transmit_time_at_level[0] = -1;
  net.msg_transmit_time_at_level[1] = -1;
  net.msg_transmit_time_at_level[2] = -1;
  net.msg_transmit_time_at_level[3] = -1;
  net.msg_transmit_time_at_level[5] = -1;


#ifdef DART_ENABLE_NETLOC

#endif /* DART_ENABLE_NETLOC */

#ifdef _CRAYC
  static const int NUM_NODES = 7798;
  static const int NUM_TOPOLOGY_PARAMETERS = 3;
  
  
  /** Get Node ID of the calling unit. Node ID is used as key 
  *   to search the topology file of machine to get the placement
  *   information of a node in network hierarchy */
  int global_node_id;
  char hostname[128];
  gethostname(hostname, sizeof(hostname));
  
  char *hname = hostname;
  while (*hname) { // While there are more characters to process...
    if(isdigit(*hname)) { // Upon finding a digit, ...
        global_node_id = strtol(hname, &hname, 10); // Read a number, ...
        break;
    } 
    else{ 
        hname++;
    }
  }
  
  net.global_node_id = global_node_id;
  
  /* ------------------------------------------------------------------------------------------- */
  /* ------------------ Read, Parse and Sort TOPOLOGY INFORMATION of machine ------------------- */
  /* ------------------------------------------------------------------------------------------- */

  int i, j;
  
  /** Allocate memory for storing the topology information and 
  *   then read the topology file of machine */
  char* topology[NUM_NODES][NUM_TOPOLOGY_PARAMETERS];
  for(int i = 0; i < NUM_NODES; i++) {
      for(int j = 0; j < NUM_TOPOLOGY_PARAMETERS; j++) 
          topology[i][j] = (char*) malloc(50 * sizeof(char)); 
  }
  FILE *file = fopen("topology.txt", "r");
  for(int i = 0; i < NUM_NODES; i++) {
      if(feof(file))
          break;
      fscanf(file, "%s %s %s", topology[i][0], topology[i][1], topology[i][2]);
  }
  fclose(file);
  
  
  /** There are total 5 off node levels in network hierarchy of 
  *   a Cray machine, which are:  
  *   0. Group number (First level after the root of machine hierarchy)
  *   1. Cabinet number within a group   
  *   2. Chassis number within a cabinet     
  *   3. Compute blade number within a chassis   
  *   4. Node number within a compute blade 
  *
  *   The concatenation of these 5 level values resolves to a 
  *   unique position of node in the machine hierarchy.
  */
  
  /** Parse topology information and save it to topology structure 
  *   of each node.
  */
  for(i = 0; i < NUM_NODES; i++){   
     if(net.global_node_id == atoi(topology[i][0])){
     char info[50];
     memcpy(info, topology[i][1], 50);   
     char* token[5];
     token[0] = strtok (info,"-");
     token[1] = strtok (NULL, "c");
     token[2] = strtok (NULL, "s");
     token[3] = strtok (NULL, "n");
     token[4] = strtok (NULL, "\0");   
     net.value_net_level[0] = atoi(token[0]+1); 
     net.value_net_level[1] = atoi(token[1]);
     net.value_net_level[2] = atoi(token[2]);
     net.value_net_level[3] = atoi(token[3]);
     net.value_net_level[4] = atoi(token[4]);   
     break;
  	}
  } 
  
#endif /* _CRAYC */

  DART_LOG_TRACE("dart_netinfo: finished: "
                 "global node id:%d "
                 "value_net_level[%d]:%d "
                 "value_net_level[%d]:%d "
                 "value_net_level[%d]:%d "
                 "value_net_level[%d]:%d "
                 "value_net_level[%d]:%d ",
                 net.global_node_id,
                 0, net.value_net_level[0],
                 1, net.value_net_level[1],
                 2, net.value_net_level[2],
                 3, net.value_net_level[3],
                 4, net.value_net_level[4],
                 );

  *netinfo = net;

  DART_LOG_DEBUG("dart_netinfo >");
  return DART_OK;
}