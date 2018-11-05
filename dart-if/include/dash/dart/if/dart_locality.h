/**
 * \file dart_locality.h
 * \defgroup  DartLocality  Locality- and topolgy discovery
 * \ingroup   DartInterface
 *
 * A set of routines to query and remodel the locality domain hierarchy and the logical arrangement of teams.
 *
 */

#ifndef DART__LOCALITY_H_
#define DART__LOCALITY_H_

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>


/**
 * \defgroup  DartLocality  Locality- and topolgy discovery
 * \ingroup   DartInterface
 */
#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/**
 * Initialize information of the specified team.
 *
 * \threadsafe_none
 * \ingroup DartLocality
 */
dart_ret_t dart_team_locality_init(
  dart_team_t                     team)                 DART_NOTHROW;

/**
 * Initialize information of the specified team.
 *
 * \threadsafe_none
 * \ingroup DartLocality
 */
dart_ret_t dart_team_locality_finalize(
  dart_team_t                     team)                 DART_NOTHROW;

/**
 * Locality information of the team domain with the specified id tag.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_team_locality(
  dart_team_t                     team,
  const char                    * domain_tag,
  dart_domain_locality_t       ** team_domain_out)      DART_NOTHROW;

/**
 * Default constructor.
 * Create an empty locality domain object.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_create(
  dart_domain_locality_t       ** domain_out)           DART_NOTHROW;

/**
 * Copy-constructor.
 * Create a new locality domain object as a deep copy of a specified
 * locality domain.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_clone(
  const dart_domain_locality_t  * domain_in,
  dart_domain_locality_t       ** domain_out)           DART_NOTHROW;

/**
 * Destructor.
 * Delete a locality domain object.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_destroy(
  dart_domain_locality_t        * domain)               DART_NOTHROW;

/**
 * Assignment operator.
 * Overwrites domain object \c domain_lhs with a deep copy of domain object
 * \c domain_rhs.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_assign(
  dart_domain_locality_t        * domain_lhs,
  const dart_domain_locality_t  * domain_rhs)           DART_NOTHROW;

/**
 * Locality information of the subdomain with the specified id tag.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_find(
  const dart_domain_locality_t  * domain_in,
  const char                    * domain_tag,
  dart_domain_locality_t       ** subdomain_out)        DART_NOTHROW;

/**
 * Remove domains in locality domain hierarchy that do not match the
 * specified domain tags and are not a parent of a matched domain.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_select(
  dart_domain_locality_t        * domain_in,
  int                             num_subdomain_tags,
  const char                   ** subdomain_tags)       DART_NOTHROW;

/**
 * Remove domains in locality domain hierarchy matching the specified domain
 * tags.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_exclude(
  dart_domain_locality_t        * domain_in,
  int                             num_subdomain_tags,
  const char                   ** subdomain_tags)       DART_NOTHROW;

/**
 * Insert locality domain into subdomains of a domain at the specified
 * relative index.
 *
 * Tags of inserted subdomains are updated according to the parent domain.
 * Units mapped to inserted subdomains are added to ancestor domains
 * recursively. Units mapped to inserted subdomains must not be mapped
 * to the target domain hierarchy already.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_add_subdomain(
  dart_domain_locality_t        * domain,
  dart_domain_locality_t        * subdomain,
  int                             subdomain_rel_id)     DART_NOTHROW;

/**
 * Move locality domain in the locality hierarchy.
 *
 * The specified domain is added to the children of another domain in the
 * same locality hierarchy at the specified relative index.
 *
 * Tags of inserted subdomains are updated according to the parent domain.
 * Units mapped to inserted subdomains are added to ancestor domains
 * recursively. Units mapped to inserted subdomains must not be mapped
 * to the target domain hierarchy already.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_move_subdomain(
  dart_domain_locality_t        * domain,
  dart_domain_locality_t        * new_parent_domain,
  int                             new_domain_rel_id)    DART_NOTHROW;

/**
 * Split locality domain hierarchy at given domain tag into \c num_parts
 * groups at specified scope.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_split_scope(
  const dart_domain_locality_t  * domain_in,
  dart_locality_scope_t           scope,
  int                             num_parts,
  dart_domain_locality_t        * split_domain_out)     DART_NOTHROW;

/**
 * Domain tags of domains at the specified locality scope.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_scope_tags(
  const dart_domain_locality_t  * domain_in,
  dart_locality_scope_t           scope,
  int                           * num_domains_out,
  char                        *** domain_tags_out)      DART_NOTHROW;

/**
 * Locality domains at the specified locality scope.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_scope_domains(
  const dart_domain_locality_t  * domain_in,
  dart_locality_scope_t           scope,
  int                           * num_domains_out,
  dart_domain_locality_t      *** domains_out)          DART_NOTHROW;

/**
 * Adds entries to locality hierarchy to group locality domains.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_domain_group(
  dart_domain_locality_t        * domain_in,
  int                             num_group_subdomains,
  const char                   ** group_subdomain_tags,
  char                          * group_domain_tag_out) DART_NOTHROW;

/**
 * Locality information of the unit with the specified team-relative id.
 *
 * \threadsafe
 * \ingroup DartLocality
 */
dart_ret_t dart_unit_locality(
  dart_team_t                     team,
  dart_team_unit_t                unit,
  dart_unit_locality_t         ** loc)                  DART_NOTHROW;

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__LOCALITY_H_ */
