/**
 * \file dash/dart/base/locality.h
 */
#ifndef DART__BASE__LOCALITY_H__
#define DART__BASE__LOCALITY_H__

#include <dash/dart/if/dart_types.h>

typedef struct {
  char          host[DART_LOCALITY_HOST_MAX_SIZE];
  dart_unit_t * units;
  int           num_units;
} dart_node_units_t;


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

dart_ret_t dart__base__locality__global_domain_new(
  dart_domain_locality_t * global_domain);

dart_ret_t dart__base__locality__domain(
  const char              * domain_tag,
  dart_domain_locality_t ** locality);



dart_ret_t dart__base__locality__unit_locality_init(
  dart_unit_locality_t * loc);

dart_ret_t dart__base__locality__local_unit_new(
  dart_unit_locality_t * locality);

#endif /* DART__BASE__LOCALITY_H__ */
