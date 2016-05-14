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
  const char              * domain_tag,
  dart_domain_locality_t ** domain_out);

/**
 * Split locality domain hierarchy at given domain tag into \c num_parts
 * groups at specified scope.
 *
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_split(
  dart_domain_locality_t  * domain_in,
  dart_locality_scope_t     scope,
  int                       num_parts,
  dart_domain_locality_t  * split_domain_out);

/**
 * Domain tags of all domains with the specified locality scope.
 *
 * \ingroup DartLocality
 */
dart_ret_t dart_scope_domains(
  dart_domain_locality_t  * domain_in,
  dart_locality_scope_t     scope,
  int                     * num_domains_out,
  char                  *** domain_tags_out);

dart_ret_t dart_group_domains(
  dart_domain_locality_t  * domain_in,
  int                       num_groups,
  const int               * group_sizes,
  const char            *** group_domain_tags);

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
