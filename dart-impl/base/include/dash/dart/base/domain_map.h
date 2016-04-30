/**
 * \file dart/base/domain_map.h
 *
 */
#ifndef DART__BASE__DOMAIN_MAP_H__
#define DART__BASE__DOMAIN_MAP_H__

#include <dash/dart/if/dart_types.h>


struct dart__base__domain_tree_node_s
  {
    /** Number of subordinate nodes. */
    int num_child_nodes;
    /** Array of subordinate nodes. */
    struct dart__base__domain_tree_node_s * child_nodes;

    /** The node's index among its siblings. */
    int relative_id;

    /** Level in the domain tree. */
    int level;

    /** Mapped element. Locality descriptor of referenced domain. */
    dart_domain_locality_t * domain;
  };
struct dart__base__domain_tree_node_s;
typedef struct dart__base__domain_tree_node_s
  dart__base__domain_tree_node_t;


dart_ret_t dart__base__domain_map__init();

dart_ret_t dart__base__domain_map__finalize();

dart_ret_t dart__base__domain_map__add_subdomains(
  const char             * domain_tag,
  dart_domain_locality_t * child_domains,
  int                      num_child_domains);

dart_ret_t dart__base__domain_map__find(
  const char              * domain_tag,
  dart_domain_locality_t ** locality);

#endif /* DART__BASE__DOMAIN_MAP_H__ */
