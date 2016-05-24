/**
 * \file dash/dart/base/internal/host_topology.h
 */
#ifndef DART__BASE__INTERNAL__HOST_TOPOLOGY_H__
#define DART__BASE__INTERNAL__HOST_TOPOLOGY_H__

#include <dash/dart/if/dart_types.h>
#include <dash/dart/base/internal/unit_locality.h>


typedef struct
{
  char                host[DART_LOCALITY_HOST_MAX_SIZE];
  char                parent[DART_LOCALITY_HOST_MAX_SIZE];
  dart_unit_t       * units;
  int                 num_units;
  int                 num_numa;
  int                 level;
}
dart_node_units_t;

typedef struct
{
  int                 num_nodes;
  int                 num_hosts;
  int                 num_host_levels;
  char **             host_names;
  dart_node_units_t * node_units;
}
dart_host_topology_t;


/**
 * Resolve the host topology from the unit's host names in a specified
 * team.
 * Expects host names in array ordered by unit rank such that the jth entry
 * in the array contains the host name of unit j.
 */
dart_ret_t dart__base__host_topology__create(
    char                  * unit_hostnames[],
    dart_team_t             team,
    dart_unit_mapping_t   * unit_mapping,
    dart_host_topology_t  * topo);

dart_ret_t dart__base__host_topology__destruct(
  dart_host_topology_t  * topo);


dart_ret_t dart__base__host_topology__num_nodes(
    dart_host_topology_t  * topo,
    int                   * num_nodes);

dart_ret_t dart__base__host_topology__node(
    dart_host_topology_t  * topo,
    int                     node_index,
    const char           ** node_hostname);

dart_ret_t dart__base__host_topology__num_node_modules(
    dart_host_topology_t  * topo,
    const char            * node_hostname,
    int                   * num_modules);

dart_ret_t dart__base__host_topology__node_module(
    dart_host_topology_t  * topo,
    const char            * node_hostname,
    int                     module_index,
    const char           ** module_hostname);

dart_ret_t dart__base__host_topology__node_units(
    dart_host_topology_t  * topo,
    const char            * node_hostname,
    dart_unit_t          ** units,
    int                   * num_units);

dart_ret_t dart__base__host_topology__module_units(
  dart_host_topology_t  * topo,
  const char            * module_hostname,
  dart_unit_t          ** units,
  int                   * num_units,
  int                   * num_numa_domains);


#endif /* DART__BASE__INTERNAL__HOST_TOPOLOGY_H__ */
