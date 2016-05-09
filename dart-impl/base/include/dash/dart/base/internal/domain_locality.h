/**
 * \file dash/dart/base/internal/domain_locality.h
 */
#ifndef DART__BASE__INTERNAL__DOMAIN_LOCALITY_H__
#define DART__BASE__INTERNAL__DOMAIN_LOCALITY_H__

#include <dash/dart/if/dart_types.h>


dart_ret_t dart__base__locality__domain_locality_init(
  dart_domain_locality_t * loc);

dart_ret_t dart__base__locality__domain_delete(
  dart_domain_locality_t * domain);

dart_ret_t dart__base__locality__create_subdomains(
  dart_domain_locality_t * domain,
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping);


#endif /* DART__BASE__INTERNAL__DOMAIN_LOCALITY_H__ */
