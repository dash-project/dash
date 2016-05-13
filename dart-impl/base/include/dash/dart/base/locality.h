/**
 * \file dash/dart/base/locality.h
 */
#ifndef DART__BASE__LOCALITY_H__
#define DART__BASE__LOCALITY_H__

#include <dash/dart/if/dart_types.h>

typedef int (*dart_domain_predicate_t_)(dart_domain_locality_t * domain);

/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__init();

dart_ret_t dart__base__locality__finalize();

/* ======================================================================== *
 * Create / Delete                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__create(
  dart_team_t team);

dart_ret_t dart__base__locality__delete(
  dart_team_t team);

/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__domain(
  dart_team_t                team,
  const char               * domain_tag,
  dart_domain_locality_t  ** domain_out);

dart_ret_t dart__base__locality__domain_split_tags(
  dart_domain_locality_t   * domain_in,
  dart_locality_scope_t      scope,
  int                        num_parts,
  int                     ** group_sizes_out,
  char                  **** group_domain_tags_out);

dart_ret_t dart__base__locality__domain_group(
  dart_domain_locality_t   * domain,
  int                        num_groups,
  int                      * group_sizes,
  char                   *** group_domain_tags);

dart_ret_t dart__base__locality__scope_domains(
  dart_domain_locality_t   * domain_in,
  dart_locality_scope_t      scope,
  int                      * num_domains_out,
  char                   *** domain_tags_out);

dart_ret_t dart__base__locality__copy_domain(
  dart_domain_locality_t   * domain_in,
  dart_domain_locality_t   * domain_out);

dart_ret_t dart__base__locality__child_domain(
  dart_domain_locality_t   * domain,
  char                     * subdomain_tag,
  dart_domain_locality_t  ** subdomain_out);

dart_ret_t dart__base__locality__parent_domain(
  dart_domain_locality_t   * domain,
  char                     * subdomain_tags[],
  int                        num_subdomain_tags,
  dart_domain_locality_t  ** domain_out);

dart_ret_t dart__base__locality__select_subdomains_if(
  dart_domain_locality_t   * domain,
  dart_domain_predicate_t_   pred);

dart_ret_t dart__base__locality__select_subdomains(
  dart_domain_locality_t   * domain,
  char                     * subdomain_tags[],
  int                        num_subdomain_tags);

dart_ret_t dart__base__locality__remove_subdomains(
  dart_domain_locality_t   * domain,
  char                     * subdomain_tags[],
  int                        num_subdomain_tags);

dart_ret_t dart__base__locality__group_subdomains(
  dart_domain_locality_t   * domain,
  char                     * subdomain_tags[],
  int                        num_subdomain_tags);

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

dart_ret_t dart__base__locality__unit(
  dart_team_t                team,
  dart_unit_t                unit,
  dart_unit_locality_t    ** locality);

#endif /* DART__BASE__LOCALITY_H__ */
