/**
 * \file dash/dart/base/internal/host_topology.h
 */
#ifndef DART__BASE__INTERNAL__HOST_TOPOLOGY_H__
#define DART__BASE__INTERNAL__HOST_TOPOLOGY_H__

#include <dash/dart/if/dart_types.h>


typedef struct {
  char          host[DART_LOCALITY_HOST_MAX_SIZE];
  char          parent[DART_LOCALITY_HOST_MAX_SIZE];
  dart_unit_t * units;
  size_t        num_units;
  int           level;
} dart_node_units_t;

typedef struct {
  size_t              num_nodes;
  size_t              num_modules;
  size_t              num_hosts;
  size_t              num_host_levels;
  char **             host_names;
  dart_node_units_t * node_units;
} dart_host_topology_t;




dart_ret_t dart__base__host_topology__node_units(
  dart_host_topology_t  * topo,
  const char            * hostname,
  dart_unit_t          ** units,
  size_t                * num_units);

/**
 * Resolve the host topology from the unit's host names in a specified
 * team.
 * Expects host names in array ordered by unit rank such that the jth entry
 * in the array contains the host name of unit j.
 */
dart_ret_t dart__base__host_topology__create(
  char                  * unit_hostnames[],
  dart_team_t             team,
  dart_host_topology_t  * topo);

dart_ret_t dart__base__host_topology__delete(
  dart_host_topology_t * topo);


#endif /* DART__BASE__INTERNAL__HOST_TOPOLOGY_H__ */
