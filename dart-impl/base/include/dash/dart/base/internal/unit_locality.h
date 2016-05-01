/**
 * \file dash/dart/base/internal/unit_locality.h
 */
#ifndef DART__BASE__INTERNAL__UNIT_LOCALITY_H__
#define DART__BASE__INTERNAL__UNIT_LOCALITY_H__

#include <dash/dart/if/dart_types.h>


dart_ret_t dart__base__unit_locality__data(
  dart_unit_locality_t ** loc);

dart_ret_t dart__base__unit_locality__at(
  dart_unit_t             unit,
  dart_unit_locality_t ** loc);

dart_ret_t dart__base__unit_locality__init();

dart_ret_t dart__base__unit_locality__finalize();

#endif /* DART__BASE__INTERNAL__UNIT_LOCALITY_H__ */
