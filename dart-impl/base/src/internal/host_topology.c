/**
 * \file dash/dart/base/internal/host_topology.c
 *
 */

/*
 * Include config and first to prevent previous include without _GNU_SOURCE
 * in included headers:
 */
#include <dash/dart/base/config.h>
#ifdef DART__PLATFORM__LINUX
#  define _GNU_SOURCE
#  include <unistd.h>
#endif

#include <string.h>
#include <limits.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_communication.h>

#include <dash/dart/base/internal/host_topology.h>
#include <dash/dart/base/internal/unit_locality.h>
#include <dash/dart/base/internal/hwloc.h>

#include <dash/dart/base/string.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

#ifdef DART_ENABLE_HWLOC
#  include <hwloc.h>
#  include <hwloc/helper.h>
#endif

/* ===================================================================== *
 * Private Functions                                                     *
 * ===================================================================== */

static int cmpstr_(const void * p1, const void * p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

dart_ret_t dart__base__host_topology__module_locations(
  dart_module_location_t ** module_locations,
  int                     * num_modules)
{
  *module_locations = NULL;
  *num_modules      = 0;

#if defined(DART_ENABLE_HWLOC) && defined(DART_ENABLE_HWLOC_PCI)
  DART_LOG_TRACE("dart__base__host_topology__module_locations: using hwloc");

  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology,
#if HWLOC_API_VERSION < 0x00020000
                             HWLOC_TOPOLOGY_FLAG_IO_DEVICES
                           | HWLOC_TOPOLOGY_FLAG_IO_BRIDGES
  /*                       | HWLOC_TOPOLOGY_FLAG_WHOLE_IO  */
#else
                             HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM
#endif
                          );
  hwloc_topology_load(topology);
  DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                 "hwloc: indexing PCI devices");
  /* Alternative: HWLOC_TYPE_DEPTH_PCI_DEVICE */
  int n_pcidev = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);

  DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                 "hwloc: %d PCI devices found", n_pcidev);
  for (int pcidev_idx = 0; pcidev_idx < n_pcidev; pcidev_idx++) {
    hwloc_obj_t coproc_obj =
      hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, pcidev_idx);
    if (NULL != coproc_obj) {
      DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                     "hwloc: PCI device: (name:%s arity:%d)",
                     coproc_obj->name, coproc_obj->arity);
      if (NULL != coproc_obj->name &&
          NULL != strstr(coproc_obj->name, "Xeon Phi")) {
        DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                       "hwloc: Xeon Phi device");
        if (coproc_obj->arity > 0) {
          for (int pd_i = 0; pd_i < (int)coproc_obj->arity; pd_i++) {
            hwloc_obj_t coproc_child_obj = coproc_obj->children[pd_i];
            DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                           "hwloc: Xeon Phi child node: (name:%s arity:%d)",
                           coproc_child_obj->name, coproc_child_obj->arity);
            (*num_modules)++;
            *module_locations = realloc(*module_locations,
                                       *num_modules *
                                         sizeof(dart_module_location_t));
            dart_module_location_t * module_loc =
              module_locations[(*num_modules)-1];

            char * hostname     = module_loc->host;
            char * mic_hostname = module_loc->module;
            char * mic_dev_name = coproc_child_obj->name;

            gethostname(hostname, DART_LOCALITY_HOST_MAX_SIZE);

            int n_chars_written = snprintf(
                                    mic_hostname, DART_LOCALITY_HOST_MAX_SIZE,
                                    "%s-%s", hostname, mic_dev_name);
            if (n_chars_written < 0 ||
                n_chars_written >= DART_LOCALITY_HOST_MAX_SIZE) {
              DART_LOG_ERROR("dart__base__host_topology__module_locations: "
                             "MIC host name '%s-%s could not be assigned",
                             hostname, mic_dev_name);
            }

            DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                           "hwloc: Xeon Phi module hostname: %s "
                           "node hostname: %s",
                           module_loc->module, module_loc->host);

            /* Get host of MIC device: */
            hwloc_obj_t mic_host_obj =
              hwloc_get_non_io_ancestor_obj(topology, coproc_obj);
            if (mic_host_obj != NULL) {

              module_loc->pos.scope =
                dart__base__hwloc__obj_type_to_dart_scope(mic_host_obj->type);
              module_loc->pos.index = mic_host_obj->logical_index;
              DART_LOG_TRACE("dart__base__host_topology__module_locations: "
                             "hwloc: Xeon Phi scope pos: "
                             "(type:%d -> scope:%d idx:%d)",
                             mic_host_obj->type,
                             module_loc->pos.scope,
                             module_loc->pos.index);
            }
          }
        }
      }
    }
  }
  hwloc_topology_destroy(topology);
  DART_LOG_TRACE("dart__base__host_topology__module_locations > "
                 "num_modules:%d", *num_modules);
#endif /* ifdef DART_ENABLE_HWLOC */
  return DART_OK;
}

dart_ret_t dart__base__host_topology__update_module_locations(
  dart_unit_mapping_t  * unit_mapping,
  dart_host_topology_t * topo)
{
  int num_hosts    = topo->num_hosts;
  dart_team_t team = unit_mapping->team;

  /*
   * Initiate all-to-all exchange of module locations like Xeon Phi
   * hostnames and their assiocated NUMA domain in their parent node.
   *
   * Select one leader unit per node for communication:
   */

  dart_unit_locality_t * my_uloc;

  /*
   * unit ID of leader unit (relative to the team specified in unit_mapping)
   * of the active unit's local compute node.
   * Example:
   *
   *  node 0:                     node 1:
   *    unit ids: { 0, 1, 2 }       unit ids: { 3, 4, 5 }
   *
   * leader unit at units 0,1,2: 0
   * leader unit at units 3,4,5: 3
   */
  dart_team_unit_t       local_leader_unit_id = DART_UNDEFINED_TEAM_UNIT_ID;
  dart_team_unit_t       my_id;
  dart_group_t           leader_group;
  dart_group_t           local_group;
  dart_team_t            leader_team; /* team of all node leaders   */

  DART_ASSERT_RETURNS(
    dart_team_myid(team, &my_id),
    DART_OK);
  DART_ASSERT_RETURNS(
    dart__base__unit_locality__at(
      unit_mapping, my_id, &my_uloc),
    DART_OK);
  DART_ASSERT_RETURNS(
    dart_group_create(&leader_group),
    DART_OK);
  DART_ASSERT_RETURNS(
    dart_group_create(&local_group),
    DART_OK);

  dart_unit_locality_t * unit_locality;
  dart__base__unit_locality__at(
    unit_mapping, my_id, &unit_locality);

  char * local_hostname = unit_locality->hwinfo.host;
  DART_LOG_TRACE("dart__base__host_topology__init: "
                 "local_hostname:%s", local_hostname);


  /* Compose leader group and local group: */
  for (int h = 0; h < num_hosts; h++) {
    /* Get unit ids at local unit's host */
    dart_host_units_t  * host_units  = &topo->host_units[h];
    dart_host_domain_t * host_domain = &topo->host_domains[h];
    /* Select first unit id at local host as leader: */
    dart_global_unit_t leader_unit_id = host_units->units[0];
    dart_group_addmember(leader_group, leader_unit_id);
    DART_LOG_TRACE("dart__base__host_topology__init: "
                   "num. units on host %s: %d",
                   topo->host_names[h], host_units->num_units);
    DART_LOG_TRACE("dart__base__host_topology__init: "
                   "leader unit on host %s: %d",
                   topo->host_names[h], leader_unit_id);
    DART_ASSERT(
      strncmp(topo->host_names[h], host_domain->host,
              DART_LOCALITY_HOST_MAX_SIZE) == 0);
    if (strncmp(host_domain->host, local_hostname,
                DART_LOCALITY_HOST_MAX_SIZE) == 0) {
      /* set local leader: */
      DART_ASSERT_RETURNS(
        dart_team_unit_g2l(team, leader_unit_id,
                           &local_leader_unit_id),
        DART_OK);
      /* collect units in local group: */
      for (int u_idx = 0; u_idx < host_units->num_units; u_idx++) {
        DART_LOG_TRACE("dart__base__host_topology__init: "
                       "add unit %d to local group",
                       host_units->units[u_idx]);
        dart_group_addmember(local_group, host_units->units[u_idx]);
      }
    }
  }

  DART_LOG_TRACE("dart__base__host_topology__init: "
                 "myid:%d (in team %d) local_leader_unit_id:%d",
                 my_id, team, local_leader_unit_id);

  size_t num_leaders;
  DART_ASSERT_RETURNS(
    dart_group_size(leader_group, &num_leaders),
    DART_OK);
  DART_LOG_TRACE("dart__base__host_topology__init: num_leaders:%d",
                 num_leaders);

  if (num_leaders > 1) {
    DART_LOG_TRACE("dart__base__host_topology__init: create leader team");
    DART_ASSERT_RETURNS(
      dart_team_create(team, leader_group, &leader_team),
      DART_OK);
    DART_LOG_TRACE("dart__base__host_topology__init: leader team: %d",
                   leader_team);
  } else {
    leader_team = team;
  }

  DART_ASSERT_RETURNS(
    dart_group_destroy(&leader_group),
    DART_OK);

  if (my_id.id == local_leader_unit_id.id) {
    dart_module_location_t * module_locations = NULL;
    dart_team_unit_t my_leader_id;
    DART_ASSERT_RETURNS(
      dart_team_myid(leader_team, &my_leader_id),
      DART_OK);
    DART_LOG_TRACE("dart__base__host_topology__init: "
                   "num_leaders:%d my_leader_id:%d (in team %d)",
                   num_leaders, my_leader_id, leader_team);

    /* local module locations to send: */
    int max_node_modules  = 2;
    int num_local_modules = 0;
    dart_module_location_t * local_module_locations;
    DART_ASSERT_RETURNS(
      dart__base__host_topology__module_locations(
        &local_module_locations, &num_local_modules),
      DART_OK);
    /* Number of bytes to receive from each leader unit in allgatherv: */
    size_t * recvcounts      = calloc(num_leaders, sizeof(size_t));
    /* Displacement at which to place data received from each leader:  */
    size_t * displs          = calloc(num_leaders, sizeof(size_t));
    recvcounts[my_leader_id.id] = num_local_modules *
                                 sizeof(dart_module_location_t);
    /*
     * Allgather of leaders is only required if there is more than
     * one leader unit.
     */
    if (num_leaders > 1) {
      /* all module locations to receive: */
      module_locations = malloc(sizeof(dart_module_location_t) *
                                max_node_modules * num_leaders);
      DART_ASSERT_RETURNS(
        dart_allgather(
          NULL,
          recvcounts,
          1,
          DART_TYPE_SIZET,
          leader_team),
        DART_OK);

      displs[0] = 0;
      for (size_t lu = 1; lu < num_leaders; lu++) {
        DART_LOG_TRACE("dart__base__host_topology__init: "
                       "allgather: leader unit %sz sent %sz",
                       lu, recvcounts[lu]);
        displs[lu] = displs[lu-1] + recvcounts[lu];
      }

      DART_ASSERT_RETURNS(
        dart_allgatherv(
          local_module_locations,
          recvcounts[my_leader_id.id],
          DART_TYPE_BYTE,
          module_locations,
          recvcounts,
          displs,
          leader_team),
        DART_OK);

      DART_LOG_DEBUG("dart__base__host_topology__init: "
                     "free(local_module_locations)");
      free(local_module_locations);
    } else {
      module_locations = local_module_locations;
    }

    topo->num_nodes       = topo->num_hosts;
    topo->num_host_levels = 0;
    for (size_t lu = 0; lu < num_leaders; lu++) {
      /* Number of modules received from leader unit lu: */
      size_t lu_num_modules = recvcounts[lu] /
                              sizeof(dart_module_location_t);
      for (size_t m = 0; m < lu_num_modules; m++) {
        int m_displ = displs[lu] / sizeof(dart_module_location_t);
        dart_module_location_t * module_loc =
          &module_locations[m_displ + m];
#ifdef DART_ENABLE_LOGGING
        dart_team_unit_t   luid = {lu};
        dart_global_unit_t gu;
        DART_ASSERT_RETURNS(
          dart_team_unit_l2g(leader_team, luid, &gu),
          DART_OK);
        DART_LOG_TRACE("dart__base__host_topology__init: "
                       "leader unit id: %d (global unit id: %d) "
                       "module_location { "
                       "host:%s module:%s scope:%d rel.idx:%d } "
                       "num_hosts:%d",
                       luid.id, gu.id, module_loc->host, module_loc->module,
                       module_loc->pos.scope, module_loc->pos.index,
                       num_hosts);
#endif // DART_ENABLE_LOGGING
        for (int h = 0; h < num_hosts; ++h) {
          dart_host_domain_t * host_domain = &topo->host_domains[h];
          if (strncmp(host_domain->host, module_loc->module,
                      DART_LOCALITY_HOST_MAX_SIZE)
              == 0) {
            DART_LOG_TRACE("dart__base__host_topology__init: "
                           "setting parent of %s to %s",
                           host_domain->host, module_loc->host);
            /* Classify host as module: */
            strncpy(host_domain->parent, module_loc->host,
                    DART_LOCALITY_HOST_MAX_SIZE);
            host_domain->scope_pos = module_loc->pos;
            host_domain->level = 1;
            if (topo->num_host_levels < host_domain->level) {
              topo->num_host_levels = host_domain->level;
            }
            break;
          }
        }
      }
    }
    if (num_leaders > 1) {
      dart_barrier(leader_team);
    }

    DART_LOG_DEBUG("dart__base__host_topology__init: "
                   "free(module_locations)");
    free(module_locations);

    DART_LOG_DEBUG("dart__base__host_topology__init: "
                   "free(displs)");
    free(displs);

    DART_LOG_DEBUG("dart__base__host_topology__init: "
                   "free(recvcounts)");
    free(recvcounts);

    if (num_leaders > 1) {
      DART_LOG_TRACE("dart__base__host_topology__init: finalize leader team");
      DART_ASSERT_RETURNS(
        dart_team_destroy(&leader_team),
        DART_OK);
    }
  }
  dart_barrier(team);

  /*
   * Broadcast updated host topology data from leader to all units at
   * local node:
   */
  if (DART_UNDEFINED_UNIT_ID != local_leader_unit_id.id) {
    dart_team_t      local_team; 
    dart_team_unit_t host_topo_bcast_root = local_leader_unit_id;
    dart_team_t      host_topo_bcast_team = team;
    if (num_hosts > 1) {
      DART_LOG_TRACE("dart__base__host_topology__init: create local team");
      DART_ASSERT_RETURNS(
        dart_team_create(team, local_group, &local_team),
        DART_OK);
      /* Leader unit ID local team is always 0: */
      host_topo_bcast_team    = local_team;
      host_topo_bcast_root.id = 0;
    }

    DART_LOG_TRACE("dart__base__host_topology__init: "
                   "broadcasting module locations from leader unit %d "
                   "to units in team %d",
                   local_leader_unit_id, host_topo_bcast_team);

    DART_ASSERT_RETURNS(
      dart_bcast(
        topo->host_domains,
        sizeof(dart_host_domain_t) * num_hosts,
        DART_TYPE_BYTE,
        host_topo_bcast_root,
        host_topo_bcast_team),
      DART_OK);

    if (num_hosts > 1) {
      DART_LOG_TRACE("dart__base__host_topology__init: finalize local team");
      DART_ASSERT_RETURNS(
        dart_team_destroy(&local_team),
        DART_OK);
    }

    DART_LOG_TRACE("dart__base__host_topology__init: updated host topology:");
    topo->num_nodes = num_hosts;
    for (int h = 0; h < num_hosts; ++h) {
      /* Get unit ids at local unit's host */
      dart_host_domain_t * hdom = &topo->host_domains[h];
      if (hdom->level > 0) {
        topo->num_nodes--;
      }
      DART_LOG_TRACE("dart__base__host_topology__init: "
                     "host[%d]: (host:%s parent:%s level:%d, scope_pos:"
                     "(scope:%d rel.idx:%d))",
                     h, hdom->host, hdom->parent, hdom->level,
                     hdom->scope_pos.scope, hdom->scope_pos.index);
    }
  }


  DART_ASSERT_RETURNS(
    dart_group_destroy(&local_group),
    DART_OK);

#if 1
  /* Classify hostnames into categories 'node' and 'module'.
   * Typically, modules have the hostname of their nodes as prefix in their
   * hostname, e.g.:
   *
   *   computer-node-124           <-- node, heterogenous
   *   |- compute_node-124-sys     <-- module, homogenous
   *   |- compute-node-124-mic0    <-- module, homogenous
   *   '- compute-node-124-mic1    <-- module, homogenous
   *
   * Find shortest strings in array of distinct host names:
   */
  int hostname_min_len = INT_MAX;
  int hostname_max_len = 0;
  for (int n = 0; n < num_hosts; ++n) {
    topo->host_domains[n].level     = 0;
    topo->host_domains[n].parent[0] = '\0';
    int hostname_len = strlen(topo->host_names[n]);
    if (hostname_len < hostname_min_len) {
      hostname_min_len = hostname_len;
    }
    if (hostname_len > hostname_max_len) {
      hostname_max_len = hostname_len;
    }
  }
  DART_LOG_TRACE("dart__base__host_topology__init: "
                 "host name length min: %d, max: %d",
                 hostname_min_len, hostname_max_len);

  topo->num_host_levels = 0;
  topo->num_nodes       = num_hosts;
  if (hostname_min_len != hostname_max_len) {
    topo->num_nodes = 0;
    int num_modules = 0;
    /* Match short hostnames as prefix of every other hostname: */
    for (int top = 0; top < num_hosts; ++top) {
      if (strlen(topo->host_names[top]) == (size_t)hostname_min_len) {
        ++topo->num_nodes;
        /* Host name is node, find its modules in all other hostnames: */
        char * short_name = topo->host_names[top];
        DART_LOG_TRACE("dart__base__host_topology__init: node: %s",
                       short_name);
        for (int sub = 0; sub < num_hosts; ++sub) {
          char * other_name = topo->host_names[sub];
          /* Other hostname is longer and has short host name in prefix: */
          if (strlen(other_name) > (size_t)hostname_min_len &&
              strncmp(short_name, other_name, hostname_min_len) == 0) {
            DART_LOG_TRACE("dart__base__host_topology__init: "
                           "module: %s, parent node: %s",
                           other_name, short_name);
            num_modules++;
            /* Increment topology level of other host: */
            int node_level = topo->host_domains[top].level + 1;
            if (node_level > topo->num_host_levels) {
              topo->num_host_levels = node_level;
            }
            topo->host_domains[sub].level = node_level;
            /* Set short hostname as parent: */
            strncpy(topo->host_domains[sub].parent, short_name,
                    DART_LOCALITY_HOST_MAX_SIZE);
          }
        }
      }
    }
    if (num_hosts > topo->num_nodes + num_modules) {
      /* some hosts are modules of node that is not in host names: */
      topo->num_nodes += num_hosts - (topo->num_nodes + num_modules);
    }
    DART_LOG_TRACE("dart__base__host_topology__init: "
                   "hosts: %d nodes: %d modules: %d",
                   topo->num_hosts, topo->num_nodes, num_modules);
  }
#endif
  DART_LOG_TRACE("dart__base__host_topology__init >");
  return DART_OK;
}

/* ===================================================================== *
 * Internal Functions                                                    *
 * ===================================================================== */

dart_ret_t dart__base__host_topology__create(
  dart_unit_mapping_t   * unit_mapping,
  dart_host_topology_t ** host_topology)
{
  *host_topology   = NULL;
  dart_team_t team = unit_mapping->team;
  size_t num_units;

  DART_LOG_TRACE("dart__base__host_topology__create: team:%d", team);

  DART_ASSERT_RETURNS(dart_team_size(team, &num_units), DART_OK);
  DART_ASSERT_MSG(num_units == unit_mapping->num_units,
                  "Number of units in mapping differs from team size");

  /* Copy host names of all units into array:
   */
  const int max_host_len = DART_LOCALITY_HOST_MAX_SIZE;
  DART_LOG_TRACE("dart__base__locality__create: copying host names");
  char ** hostnames = malloc(sizeof(char *) * num_units);
  for (size_t u = 0; u < num_units; ++u) {
    hostnames[u] = malloc(sizeof(char) * max_host_len);
    dart_unit_locality_t * ul;
    dart_team_unit_t luid = {u};
    DART_ASSERT_RETURNS(
      dart__base__unit_locality__at(unit_mapping, luid, &ul),
      DART_OK);
    strncpy(hostnames[u], ul->hwinfo.host, max_host_len);
  }

  dart_host_topology_t * topo = malloc(sizeof(dart_host_topology_t));

  /*
   * Find unique host names in array 'hostnames' and count units per
   * host in same pass:
   */
  DART_LOG_TRACE("dart__base__host_topology__init: "
                 "filtering host names of %ld units", num_units);
  qsort(hostnames, num_units, sizeof(char*), cmpstr_);
  size_t last_host_idx  = 0;
  /* Maximum number of units mapped to a single host: */
  int    max_host_units = 0;
  /* Number of units mapped to current host: */
  int    num_host_units = 0;
  for (size_t u = 0; u < num_units; ++u) {
    ++num_host_units;
    if (u == last_host_idx) { continue; }
    /* copies next differing host name to the left, like:
     *
     *     [ a a a a b b b c c c ]  last_host_index++ = 1
     *         .-----'
     *         v
     * ->  [ a b a a b b b c c c ]  last_host_index++ = 2
     *           .---------'
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
  if (max_host_units == 0) {
    /* All units mapped to same host: */
    max_host_units = num_host_units;
  }
  /* All entries after index last_host_ids are duplicates now: */
  int num_hosts = last_host_idx + 1;
  DART_LOG_TRACE("dart__base__host_topology__init: number of hosts: %d",
                 num_hosts);
  DART_LOG_TRACE("dart__base__host_topology__init: max. number of units "
                 "mapped to a hosts: %d", max_host_units);

  /* Map units to hosts: */
  topo->host_domains = malloc(num_hosts * sizeof(dart_host_domain_t));
  topo->host_units   = malloc(num_hosts * sizeof(dart_host_units_t));

  for (int h = 0; h < num_hosts; ++h) {
    dart_host_domain_t * host_domain = &topo->host_domains[h];
    dart_host_units_t  * host_units  = &topo->host_units[h];
    /* Histogram of NUMA ids: */
    int numa_id_hist[DART_LOCALITY_MAX_NUMA_ID] = { 0 };
    /* Allocate array with capacity of maximum units on a single host: */
    host_units->units      = malloc(sizeof(dart_unit_t) * max_host_units);
    host_units->num_units  = 0;
    host_domain->host[0]   = '\0';
    host_domain->parent[0] = '\0';
    host_domain->num_numa  = 0;
    host_domain->level     = 0;
    host_domain->scope_pos.scope = DART_LOCALITY_SCOPE_NODE;
    host_domain->scope_pos.index = 0;

    memset(host_domain->numa_ids, 0,
           sizeof(int) * DART_LOCALITY_MAX_NUMA_ID);
    strncpy(host_domain->host, hostnames[h], max_host_len);

    DART_LOG_TRACE("dart__base__host_topology__init: mapping units to %s",
                   hostnames[h]);
    /* Iterate over all units: */
    for (size_t u = 0; u < num_units; ++u) {
      dart_unit_locality_t * ul;
      dart_team_unit_t luid = {u};
      DART_ASSERT_RETURNS(
        dart__base__unit_locality__at(unit_mapping, luid, &ul),
        DART_OK);
      if (strncmp(ul->hwinfo.host, hostnames[h], max_host_len) == 0) {
        /* Unit is local to host at index h: */
        dart_global_unit_t guid;
        DART_ASSERT_RETURNS(
          dart_team_unit_l2g(team, ul->unit, &guid),
          DART_OK);
        host_units->units[host_units->num_units] = guid;
        host_units->num_units++;

        int unit_numa_id = ul->hwinfo.numa_id;

        DART_LOG_TRACE("dart__base__host_topology__init: "
                       "mapping unit %ld to host '%s', NUMA id: %d",
                       u, hostnames[h], unit_numa_id);
        if (unit_numa_id >= 0) {
          if (numa_id_hist[unit_numa_id] == 0) {
            host_domain->numa_ids[host_domain->num_numa] = unit_numa_id;
            host_domain->num_numa++;
          }
          numa_id_hist[unit_numa_id]++;
        }
      }
    }
    DART_LOG_TRACE("dart__base__host_topology__init: "
                   "found %d NUMA domains on host %s",
                   host_domain->num_numa, hostnames[h]);
    for (int n = 0; n < host_domain->num_numa; ++n) {
      DART_LOG_TRACE("dart__base__host_topology__init: numa_id[%d]:%d",
                     n, host_domain->numa_ids[n]);
    }

    /* Shrink unit array to required capacity: */
    if (host_units->num_units < max_host_units) {
      DART_LOG_TRACE("dart__base__host_topology__init: shrinking node unit "
                     "array from %d to %d elements",
                     max_host_units, host_units->num_units);
      host_units->units = realloc(host_units->units,
                                  host_units->num_units *
                                    sizeof(dart_unit_t));
      DART_ASSERT(host_units->units != NULL);
    }
  }

  topo->num_host_levels = 0;
  topo->num_nodes       = num_hosts;
  topo->num_hosts       = num_hosts;
  topo->num_units       = num_units;
  topo->host_names      = hostnames;

  DART_ASSERT_RETURNS(
    dart__base__host_topology__update_module_locations(
      unit_mapping, topo),
    DART_OK);

#if 0
  topo->num_hosts  = num_hosts;
//topo->host_names = (char **)(realloc(hostnames, num_hosts * sizeof(char*)));
  topo->host_names = hostnames;
  DART_ASSERT(topo->host_names != NULL);
#endif

  *host_topology = topo;
  return DART_OK;
}

dart_ret_t dart__base__host_topology__destruct(
  dart_host_topology_t * topo)
{
  DART_LOG_DEBUG("dart__base__host_topology__destruct()");
  if (NULL != topo->host_domains) {
    DART_LOG_DEBUG("dart__base__host_topology__init: "
                   "free(topo->host_domains)");
    free(topo->host_domains);
    topo->host_domains = NULL;
  }
  if (NULL != topo->host_names) {
    for (size_t h = 0; h < topo->num_units; ++h) {
      if (NULL != topo->host_names[h]) {
        DART_LOG_DEBUG("dart__base__host_topology__init: "
                       "free(topo->host_names[%d])", h);
        free(topo->host_names[h]);
        topo->host_names[h] = NULL;
      }
    }
    DART_LOG_DEBUG("dart__base__host_topology__init: "
                   "free(topo->host_names)");
    free(topo->host_names);
    topo->host_names = NULL;
  }
  if (NULL != topo->host_units) {
    if (NULL != topo->host_units->units) {
      DART_LOG_DEBUG("dart__base__host_topology__init: "
                     "free(topo->host_units->units)");
      free(topo->host_units->units);
      topo->host_units->units = NULL;
    }
    DART_LOG_DEBUG("dart__base__host_topology__init: "
                   "free(topo->host_units)");
    free(topo->host_units);
    topo->host_units = NULL;
  }
  DART_LOG_DEBUG("dart__base__host_topology__destruct >");
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
  *node_hostname = NULL;
  int n_index    = 0;
  for (int h = 0; h < topo->num_hosts; ++h) {
    if (topo->host_domains[h].level == 0) {
      if (n_index == node_index) {
        *node_hostname = topo->host_names[h];
        return DART_OK;
      }
      n_index++;
    }
  }
  DART_LOG_ERROR("dart__base__host_topology__node: "
                 "failed to load node at index:%d, num.hosts:%d num.nodes:%d",
                 node_index, topo->num_hosts, topo->num_nodes);
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
    char * m_hostname = topo->host_domains[h].host;
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
  dart_global_unit_t   ** units,
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
    dart_host_domain_t * host_domain = &topo->host_domains[h];
    dart_host_units_t  * host_units  = &topo->host_units[h];
    if (strncmp(host_domain->host, hostname, strlen(hostname))
        == 0) {
      *num_units += host_units->num_units;
      host_found  = 1;
    }
  }
  if (!host_found) {
    DART_LOG_ERROR("dart__base__host_topology__node_units ! "
                   "no entry for host '%s')", hostname);
    return DART_ERR_NOTFOUND;
  }
  /* Second pass: Copy unit ids: */
  dart_global_unit_t * node_unit_ids = malloc(*num_units *
                                              sizeof(dart_global_unit_t));
  int node_unit_idx = 0;
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_host_domain_t * host_domain = &topo->host_domains[h];
    dart_host_units_t  * host_units  = &topo->host_units[h];
    if (strncmp(host_domain->host, hostname, strlen(hostname))
        == 0) {
      for (int nu = 0; nu < host_units->num_units; ++nu) {
        node_unit_ids[node_unit_idx + nu] = host_units->units[nu];
      }
      node_unit_idx += host_units->num_units;
    }
  }
  *units = node_unit_ids;

  DART_LOG_TRACE_ARRAY(
    "dart__base__host_topology__node_units >", "%d", *units, *num_units);
  return DART_OK;
}

dart_ret_t dart__base__host_topology__host_domain(
  dart_host_topology_t      * topo,
  const char                * hostname,
  const dart_global_unit_t ** units,
  int                       * num_units,
  const int                ** numa_ids,
  int                       * num_numa_domains)
{
  DART_LOG_TRACE("dart__base__host_topolgoy__module_units() host: %s",
                 hostname);
  *num_units        = 0;
  *num_numa_domains = 0;
  *units            = NULL;
  int host_found    = 0;
  /*
   * Does not include units in sub-modules, e.g. a query for host name
   * "some-node" would not include units from "sub-node-*":
   */
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_host_domain_t * host_domain = &topo->host_domains[h];
    dart_host_units_t  * host_units  = &topo->host_units[h];
    if (strncmp(host_domain->host, hostname, DART_LOCALITY_HOST_MAX_SIZE)
        == 0) {
      *num_numa_domains  = host_domain->num_numa;
      *numa_ids          = host_domain->numa_ids;
      *num_units         = host_units->num_units;
      *units             = host_units->units;
      host_found         = 1;
      break;
    }
  }
  if (!host_found) {
    DART_LOG_ERROR("dart__base__host_topology__module_units ! "
                   "no entry for host '%s')", hostname);
    return DART_ERR_NOTFOUND;
  }

  DART_LOG_TRACE_ARRAY(
    "dart__base__host_topology__module_units >", "%d", *units, *num_units);
  return DART_OK;
}

