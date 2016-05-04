/**
 * \file dash/dart/if/dart_locality.h
 *
 * Functions for querying topology and node-level locality information of
 * teams and units.
 *
 */
#ifndef DART__LOCALITY_H_
#define DART__LOCALITY_H_

#include <dash/dart/if/dart_types.h>


/**
 * \defgroup  DartLocality  Locality information routines in DART
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * Locality information of the domain with the specified id tag.
 *
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_locality(
  dart_team_t               team,
  const char              * domain,
  dart_domain_locality_t ** loc);

/**
 * Locality information of the unit with the specified global id.
 *
 * \ingroup DartLocality
 */
dart_ret_t dart_unit_locality(
  dart_team_t               team,
  dart_unit_t               unit,
  dart_unit_locality_t   ** loc);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__LOCALITY_H_ */
