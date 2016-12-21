/**
 * \file dash/dart/base/locality.h
 */
#ifndef DART__BASE__LOCALITY_H__
#define DART__BASE__LOCALITY_H__

#include <dash/dart/if/dart_types.h>

#include <dash/dart/base/internal/domain_locality.h>


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

static inline
dart_ret_t dart__base__locality__create_domain(
  dart_domain_locality_t          ** domain_out)
{
  *domain_out = malloc(sizeof(dart_domain_locality_t));
  return dart__base__locality__domain__init(*domain_out);
}

static inline
dart_ret_t dart__base__locality__clone_domain(
  const dart_domain_locality_t     * domain_in,
  dart_domain_locality_t          ** domain_out)
{
  dart_ret_t ret = dart__base__locality__create_domain(domain_out);
  if (DART_OK != ret) { return ret; }
  return dart__base__locality__domain__copy(domain_in, *domain_out);
}

static inline
dart_ret_t dart__base__locality__assign_domain(
  dart_domain_locality_t           * domain_lhs,
  const dart_domain_locality_t     * domain_rhs)
{
  dart_ret_t ret = dart__base__locality__domain__destruct(domain_lhs);
  if (DART_OK != ret) { return ret; }
  return dart__base__locality__domain__copy(domain_rhs, domain_lhs);
}

static inline
dart_ret_t dart__base__locality__destruct_domain(
  dart_domain_locality_t           * domain)
{
  dart_ret_t ret = dart__base__locality__domain__destruct(domain);
  if (ret != DART_OK) { return ret; }
  free(domain);
  return DART_OK;
}

static inline
dart_ret_t dart__base__locality__domain_select_subdomains(
  dart_domain_locality_t           * domain,
  const char                      ** subdomain_tags,
  int                                num_subdomain_tags)
{
  static const int remove_matches = 0;
  return dart__base__locality__domain__filter_subdomains(
           domain, subdomain_tags, num_subdomain_tags, remove_matches);
}

static inline
dart_ret_t dart__base__locality__domain_exclude_subdomains(
  dart_domain_locality_t           * domain,
  const char                      ** subdomain_tags,
  int                                num_subdomain_tags)
{
  static const int remove_matches = 1;
  return dart__base__locality__domain__filter_subdomains(
           domain, subdomain_tags, num_subdomain_tags, remove_matches);
}

dart_ret_t dart__base__locality__team_domain(
  dart_team_t                        team,
  dart_domain_locality_t          ** domain_out);

dart_ret_t dart__base__locality__domain(
  const dart_domain_locality_t     * domain_in,
  const char                       * domain_tag,
  dart_domain_locality_t          ** domain_out);

dart_ret_t dart__base__locality__domain_split_tags(
  const dart_domain_locality_t     * domain_in,
  dart_locality_scope_t              scope,
  int                                num_parts,
  int                             ** group_sizes_out,
  char                          **** group_domain_tags_out);

dart_ret_t dart__base__locality__domain_group(
  dart_domain_locality_t           * domain,
  int                                group_size,
  const char                      ** group_subdomain_tags,
  char                             * group_domain_tag_out);

dart_ret_t dart__base__locality__scope_domains(
  const dart_domain_locality_t     * domain_in,
  dart_locality_scope_t              scope,
  int                              * num_domains_out,
  dart_domain_locality_t         *** domains_out);

dart_ret_t dart__base__locality__scope_domain_tags(
  const dart_domain_locality_t     * domain_in,
  dart_locality_scope_t              scope,
  int                              * num_domains_out,
  char                           *** domain_tags_out);

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

dart_ret_t dart__base__locality__unit(
  dart_team_t                        team,
  dart_team_unit_t                   unit,
  dart_unit_locality_t            ** locality);

#endif /* DART__BASE__LOCALITY_H__ */
