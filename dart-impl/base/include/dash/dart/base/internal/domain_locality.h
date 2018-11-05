/**
 * \file dash/dart/base/internal/domain_locality.h
 */
#ifndef DART__BASE__INTERNAL__DOMAIN_LOCALITY_H__
#define DART__BASE__INTERNAL__DOMAIN_LOCALITY_H__

#include <dash/dart/if/dart_types.h>

#include <dash/dart/base/internal/host_topology.h>

typedef int (*dart_domain_predicate_t)(dart_domain_locality_t * domain);


dart_ret_t dart__base__locality__domain__init(
  dart_domain_locality_t       * domain);

dart_ret_t dart__base__locality__domain__destruct(
  dart_domain_locality_t       * domain);

dart_ret_t dart__base__locality__domain__copy(
  const dart_domain_locality_t * domain_in,
  dart_domain_locality_t       * domain_out);

dart_ret_t dart__base__locality__domain__update_subdomains(
  dart_domain_locality_t       * domain);

dart_ret_t dart__base__locality__domain__child(
  const dart_domain_locality_t  * domain,
  const char                    * subdomain_tag,
  dart_domain_locality_t       ** subdomain_out);

dart_ret_t dart__base__locality__domain__parent(
  const dart_domain_locality_t  * domain,
  const char                   ** subdomain_tags,
  int                             num_subdomain_tags,
  dart_domain_locality_t       ** domain_out);

dart_ret_t dart__base__locality__domain__filter_subdomains(
  dart_domain_locality_t       * domain,
  const char                  ** subdomain_tags,
  int                            num_subdomain_tags,
  int                            remove_matches);

dart_ret_t dart__base__locality__domain__filter_subdomains_if(
  dart_domain_locality_t       * domain,
  dart_domain_predicate_t        pred);

dart_ret_t dart__base__locality__domain__add_subdomain(
  dart_domain_locality_t       * domain,
  dart_domain_locality_t       * subdomain,
  int                            subdomain_rel_id);

dart_ret_t dart__base__locality__domain__remove_subdomain(
  dart_domain_locality_t       * domain,
  int                            subdomain_rel_id);

dart_ret_t dart__base__locality__domain__move_subdomain(
  dart_domain_locality_t       * subdomain,
  dart_domain_locality_t       * new_parent_domain,
  int                            new_subdomain_rel_id);

dart_ret_t dart__base__locality__domain__create_subdomains(
  dart_domain_locality_t       * domain,
  dart_host_topology_t         * host_topology,
  dart_unit_mapping_t          * unit_mapping);


#endif /* DART__BASE__INTERNAL__DOMAIN_LOCALITY_H__ */
