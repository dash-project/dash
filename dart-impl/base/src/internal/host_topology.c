/**
 * \file dash/dart/base/internal/host_topology.c
 *
 */
#include <string.h>
#include <limits.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/base/internal/host_topology.h>
#include <dash/dart/base/internal/unit_locality.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

/* ======================================================================== *
 * Private Functions                                                        *
 * ======================================================================== */

static int cmpstr_(const void * p1, const void * p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

/* ======================================================================== *
 * Internal Functions                                                       *
 * ======================================================================== */

dart_ret_t dart__base__host_topology__create(
  char                 * hostnames[],
  dart_team_t            team,
  dart_host_topology_t * topo)
{
  const int max_host_len = DART_LOCALITY_HOST_MAX_SIZE;
  size_t    num_units;
  DART_ASSERT_RETURNS(dart_team_size(team, &num_units), DART_OK);

  /* Sort host names to find duplicates in one pass: */
  qsort(hostnames, num_units, sizeof(char*), cmpstr_);
  /* Find unique host names in array 'hosts': */
  size_t last_host_idx  = 0;
  /* Maximum number of units mapped to a single host: */
  int    max_host_units = 0;
  /* Number of units mapped to current host: */
  int    num_host_units = 0;
  DART_LOG_TRACE("dart__base__host_topology__init: filtering host names");
  for (size_t u = 0; u < num_units; ++u) {
    ++num_host_units;
    if (u == last_host_idx) { continue; }
    /* copies next differing host name to the left, like:
     *
     *     [ a a a a b b b c c c ]  last_host_index++ = 1
     *         .---------'
     *         v
     * ->  [ a b a a b b b c c c ]  last_host_index++ = 2
     *           .-----------'
     *           v
     * ->  [ a b c a b b b c c c ]  last_host_index++ = 3
     * ...
     */
    if (strcmp(hostnames[u], hostnames[last_host_idx]) != 0) {
      ++last_host_idx;
      strncpy(hostnames[last_host_idx], hostnames[u], max_host_len);
      if (num_host_units > max_host_units) {
        max_host_units = num_host_units;
      }
      num_host_units = 0;
    }
  }
  /* All entries after index last_host_ids are duplicates now: */
  int    num_hosts = last_host_idx + 1;
  DART_LOG_TRACE("dart__base__host_topology__init: number of hosts: %d",
                 num_hosts);
  DART_LOG_TRACE("dart__base__host_topology__init: maximum number of units "
                 "mapped to a hosts: %d", max_host_units);

  /* Map units to hosts: */
  topo->node_units = malloc(num_hosts * sizeof(dart_node_units_t));
  for (int h = 0; h < num_hosts; ++h) {
    dart_node_units_t * node_units = &topo->node_units[h];
    /* allocate array with capacity of maximum units on a single host: */
    node_units->units = malloc(sizeof(dart_unit_t) * max_host_units);
    strncpy(node_units->host, hostnames[h], max_host_len);
    node_units->num_units = 0;
    DART_LOG_TRACE("dart__base__host_topology__init: mapping units to %s",
                   hostnames[h]);
    for (int    u = 0; u < num_units; ++u) {
      dart_unit_locality_t * ul;
      DART_ASSERT_RETURNS(dart__base__unit_locality__at(u, &ul), DART_OK);
      if (strncmp(ul->host, hostnames[h], max_host_len) == 0) {
        node_units->units[node_units->num_units] = ul->unit;
        node_units->num_units++;
      }
    }
    /* shrink unit array to required capacity: */
#if 0
    if (node_units->num_units < max_host_units) {
      DART_LOG_TRACE("dart__base__host_topology__init: shrinking node unit "
                     "array from %d to %d elements",
                     max_host_units, node_units->num_units);
      node_units->units = realloc(node_units->units, node_units->num_units);
      DART_ASSERT(node_units->units != NULL);
    }
#endif
  }
  topo->num_hosts  = num_hosts;
#if 0
  topo->host_names = (char **)(realloc(hostnames, num_hosts));
#else
  topo->host_names = hostnames;
#endif

  DART_ASSERT(topo->host_names != NULL);

  /* Classify hostnames into categories 'node' and 'module'.
   * Typically, modules have the hostname of their nodes as prefix in their
   * hostname, e.g.:
   *
   *   computer-node-124           <-- node, heterogenous
   *   |- compute_node-host-sys    <-- module, homogenous
   *   |- compute-node-124-mic0    <-- module, homogenous
   *   '- compute-node-124-mic1    <-- module, homogenous
   *
   * Find shortest strings in array of distinct host names:
   */
  int hostname_min_len = INT_MAX;
  int hostname_max_len = 0;
  for (int n = 0; n < num_hosts; ++n) {
    topo->node_units[n].level     = 0;
    topo->node_units[n].parent[0] = '\0';
    int hostname_len = strlen(topo->host_names[n]);
    if (hostname_len < hostname_min_len) {
      hostname_min_len = hostname_len;
    }
    if (hostname_len > hostname_max_len) {
      hostname_max_len = hostname_len;
    }
  }
  topo->num_host_levels = 0;
  topo->num_nodes       = num_hosts;
  if (hostname_min_len != hostname_max_len) {
    topo->num_nodes = 0;
    /* Match short hostnames as prefix of every other hostname: */
    for (int top = 0; top < num_hosts; ++top) {
      if (strlen(topo->host_names[top]) ==
          (int)hostname_min_len) {
        ++topo->num_nodes;
        /* Host name is candidate, test for all other hostnames: */
        char * short_name = topo->host_names[top];
        for (int sub = 0; sub < num_hosts; ++sub) {
          char * other_name = topo->host_names[sub];
          /* Other hostname is longer and has short host name in prefix: */
          if (strlen(other_name) > (int)hostname_min_len &&
              strncmp(short_name, other_name, hostname_min_len) == 0) {
            /* Increment topology level of other host: */
            int node_level = topo->node_units[top].level + 1;
            if (node_level > topo->num_host_levels) {
              topo->num_host_levels = node_level;
            }
            topo->node_units[sub].level = node_level;
            /* Set short hostname as parent: */
            strncpy(topo->node_units[sub].parent, short_name,
                    DART_LOCALITY_HOST_MAX_SIZE);
          }
        }
      }
    }
  }
  return DART_OK;
}

dart_ret_t dart__base__host_topology__delete(
  dart_host_topology_t * topo)
{
  DART_LOG_DEBUG("dart__base__host_topology__delete()");
  if (topo->node_units != NULL) {
    free(topo->node_units);
    topo->node_units = NULL;
  }
  if (topo->host_names != NULL) {
    free(topo->host_names);
    topo->host_names = NULL;
  }
  DART_LOG_DEBUG("dart__base__host_topology__delete >");
  return DART_OK;
}


dart_ret_t dart__base__host_topology__num_nodes(
  dart_host_topology_t  * topo,
  int                   * num_nodes)
{
  *num_nodes = topo->num_nodes;
  return DART_OK;
}

dart_ret_t dart__base__host_topology__node(
  dart_host_topology_t  * topo,
  int                     node_index,
  const char           ** node_hostname)
{
  int n_index = 0;
  for (int h = 0; h < topo->num_hosts; ++h) {
    if (topo->node_units[h].level == 0) {
      if (n_index == node_index) {
        *node_hostname = topo->host_names[h];
        return DART_OK;
      }
      n_index++;
    }
  }
  return DART_ERR_NOTFOUND;
}

dart_ret_t dart__base__host_topology__num_node_modules(
  dart_host_topology_t  * topo,
  const char            * node_hostname,
  int                   * num_modules)
{
  *num_modules = 0;
  for (int h = 0; h < topo->num_hosts; ++h) {
    /* also includes node itself */
    char * m_hostname = topo->node_units[h].host;
    if (strncmp(node_hostname, m_hostname, strlen(node_hostname)) == 0) {
      *num_modules += 1;
    }
  }
  return DART_OK;
}

dart_ret_t dart__base__host_topology__node_module(
  dart_host_topology_t  * topo,
  const char            * node_hostname,
  int                     module_index,
  const char           ** module_hostname)
{
  int m_index = 0;
  for (int h = 0; h < topo->num_hosts; ++h) {
    char * m_hostname = topo->host_names[h];
    /* also includes node itself */
    if (strncmp(node_hostname, m_hostname, strlen(node_hostname)) == 0) {
      if (m_index == module_index) {
        *module_hostname = m_hostname;
        return DART_OK;
      }
      m_index++;
    }
  }
  return DART_ERR_NOTFOUND;
}

dart_ret_t dart__base__host_topology__node_units(
  dart_host_topology_t  * topo,
  const char            * hostname,
  dart_unit_t          ** units,
  int                   * num_units)
{
  DART_LOG_TRACE("dart__base__host_topolgoy__node_units() host: %s",
                 hostname);
  *num_units     = 0;
  *units         = NULL;
  int host_found = 0;
  /*
   * Also includes units in sub-modules, e.g. a query for host name
   * "some-node" would also include units from "sub-node-*":
   */

  /* First pass: Find total number of units: */
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_node_units_t * node_units = &topo->node_units[h];
    if (strncmp(node_units->host, hostname, strlen(hostname))
        == 0) {
      *num_units += node_units->num_units;
      host_found  = 1;
    }
  }
  if (!host_found) {
    DART_LOG_ERROR("dart__base__host_topology__node_units ! "
                   "no entry for host '%s')", hostname);
    return DART_ERR_NOTFOUND;
  }
  /* Second pass: Copy unit ids: */
  dart_unit_t * node_unit_ids = malloc(*num_units * sizeof(dart_unit_t));
  int           node_unit_idx = 0;
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_node_units_t * node_units = &topo->node_units[h];
    if (strncmp(node_units->host, hostname, strlen(hostname))
        == 0) {
      for (int nu = 0; nu < node_units->num_units; ++nu) {
        node_unit_ids[node_unit_idx + nu] = node_units->units[nu];
      }
      node_unit_idx += node_units->num_units;
    }
  }
  *units = node_unit_ids;
  DART_LOG_TRACE("dart__base__host_topology__node_units > num_units: %d",
                 *num_units);
  return DART_OK;
}

dart_ret_t dart__base__host_topology__module_units(
  dart_host_topology_t  * topo,
  const char            * hostname,
  dart_unit_t          ** units,
  int                   * num_units)
{
  DART_LOG_TRACE("dart__base__host_topolgoy__module_units() host: %s",
                 hostname);
  *num_units     = 0;
  *units         = NULL;
  int host_found = 0;
  /*
   * Does not include units in sub-modules, e.g. a query for host name
   * "some-node" would not include units from "sub-node-*":
   */
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_node_units_t * node_units = &topo->node_units[h];
    if (strncmp(node_units->host, hostname, DART_LOCALITY_HOST_MAX_SIZE)
        == 0) {
      *num_units += node_units->num_units;
      *units      = node_units->units;
      host_found  = 1;
    }
  }
  if (!host_found) {
    DART_LOG_ERROR("dart__base__host_topology__module_units ! "
                   "no entry for host '%s')", hostname);
    return DART_ERR_NOTFOUND;
  }
  DART_LOG_TRACE("dart__base__host_topology__module_units > num_units: %d",
                 *num_units);
  return DART_OK;
}

