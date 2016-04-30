/**
 * \file dash/dart/base/locality.h
 */
#ifndef DART__BASE__LOCALITY_H__
#define DART__BASE__LOCALITY_H__

#include <dash/dart/if/dart_types.h>


void dart__base__locality__domain_locality_init(
  dart_domain_locality_t * loc);

void dart__base__locality__unit_locality_init(
  dart_unit_locality_t * loc);


dart_ret_t dart__base__locality__global_domain(
  dart_domain_locality_t ** global_domain);

dart_ret_t dart__base__locality__local_unit(
  dart_unit_locality_t ** locality);

#endif /* DART__BASE__LOCALITY_H__ */
