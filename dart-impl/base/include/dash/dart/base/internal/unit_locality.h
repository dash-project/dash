/**
 * \file dash/dart/base/internal/unit_locality.h
 */
#ifndef DART__BASE__INTERNAL__UNIT_LOCALITY_H__
#define DART__BASE__INTERNAL__UNIT_LOCALITY_H__

#include <dash/dart/if/dart_types.h>

typedef struct
{
  dart_unit_locality_t  * unit_localities;
  size_t                  num_units;
  dart_team_t             team;
} dart_unit_mapping_t;

dart_ret_t dart__base__unit_locality__create(
  dart_team_t             team,
  dart_unit_mapping_t  ** unit_mapping);

dart_ret_t dart__base__unit_locality__destruct(
  dart_unit_mapping_t   * unit_mapping);

dart_ret_t dart__base__unit_locality__at(
  dart_unit_mapping_t   * unit_mapping,
  dart_team_unit_t        unit,
  dart_unit_locality_t ** loc);

#endif /* DART__BASE__INTERNAL__UNIT_LOCALITY_H__ */
