/**
 * \file dash/dart/base/locality.h
 */
#ifndef DART__BASE__LOCALITY_H__
#define DART__BASE__LOCALITY_H__

#include <dash/dart/if/dart_types.h>


dart_ret_t dart__base__locality__init();

dart_ret_t dart__base__locality__finalize();



dart_ret_t dart__base__locality__domain_init(
  dart_domain_locality_t * loc);

dart_ret_t dart__base__locality__set_subdomains(
  const char             * domain_tag,
  dart_domain_locality_t * subdomains,
  int                      num_subdomains);

dart_ret_t dart__base__locality__domain_delete(
  dart_domain_locality_t * loc);

dart_ret_t dart__base__locality__domain(
  const char              * domain_tag,
  dart_domain_locality_t ** locality);



dart_ret_t dart__base__locality__unit_locality_init(
  dart_unit_locality_t * loc);

dart_ret_t dart__base__locality__local_unit_new(
  dart_unit_locality_t * locality);

dart_ret_t dart__base__locality__node_units(
  const char   * hostname,
  dart_unit_t ** units,
  size_t       * num_units);

#endif /* DART__BASE__LOCALITY_H__ */
