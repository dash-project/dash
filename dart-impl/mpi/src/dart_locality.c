/**
 * \file dart_locality.c
 *
 */
#include <dash/dart/base/config.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/internal/unit_locality.h>
#include <dash/dart/base/internal/compiler_tweaks.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <mpi.h>

#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>

/* ==================================================================== *
 * Domain Locality                                                      *
 * ==================================================================== */

dart_ret_t dart_team_locality_init(
  dart_team_t                     team)
{
  return dart__base__locality__create(team);
}

dart_ret_t dart_team_locality_finalize(
  dart_team_t                     team)
{
  return dart__base__locality__delete(team);
}

dart_ret_t dart_domain_team_locality(
  dart_team_t                     team,
  const char                    * domain_tag,
  dart_domain_locality_t       ** team_domain_out)
{
  DART_LOG_DEBUG("dart_domain_team_locality() team(%d) domain(%s)",
                 team, domain_tag);
  dart_ret_t ret;

  *team_domain_out = NULL;

  dart_domain_locality_t * team_domain = NULL;
  ret = dart__base__locality__team_domain(team, &team_domain);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_domain_team_locality: "
                   "dart__base__locality__team_domain failed (%d)", ret);
    return ret;
  }
  DART_ASSERT(team_domain != NULL);

  *team_domain_out = team_domain;

  if (strcmp(domain_tag, team_domain->domain_tag) != 0) {
    dart_domain_locality_t * team_subdomain;
    ret = dart__base__locality__domain(
            team_domain, domain_tag, &team_subdomain);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart_domain_team_locality: "
                     "dart__base__locality__domain failed "
                     "for domain tag '%s' -> (%d)", domain_tag, ret);
      *team_domain_out = NULL;
      return ret;
    }
    *team_domain_out = team_subdomain;
  }

  DART_ASSERT(*team_domain_out != NULL);

  DART_LOG_DEBUG("dart_domain_team_locality > team(%d) domain(%s) -> %p",
                 team, domain_tag, (void *)(*team_domain_out));
  return DART_OK;
}

dart_ret_t dart_domain_create(
  dart_domain_locality_t       ** domain_out)
{
  return dart__base__locality__create_domain(domain_out);
}

dart_ret_t dart_domain_clone(
  const dart_domain_locality_t  * domain_in,
  dart_domain_locality_t       ** domain_out)
{
  return dart__base__locality__clone_domain(domain_in, domain_out);
}

dart_ret_t dart_domain_destroy(
  dart_domain_locality_t        * domain)
{
  return dart__base__locality__destruct_domain(domain);
}

dart_ret_t dart_domain_assign(
  dart_domain_locality_t        * domain_lhs,
  const dart_domain_locality_t  * domain_rhs)
{
  return dart__base__locality__assign_domain(domain_lhs, domain_rhs);
}

dart_ret_t dart_domain_find(
  const dart_domain_locality_t  * domain_in,
  const char                    * domain_tag,
  dart_domain_locality_t       ** subdomain_out)
{
  DART_LOG_DEBUG("dart_domain_find() domain_in(%p) domain_tag(%s)",
                 domain_in, domain_tag);
  dart_ret_t ret = dart__base__locality__domain(
                     domain_in, domain_tag, subdomain_out);
  DART_LOG_DEBUG("dart_domain_find > %d", ret);
  return ret;
}

dart_ret_t dart_domain_select(
  dart_domain_locality_t        * domain_in,
  int                             num_subdomain_tags,
  const char                   ** subdomain_tags)
{
  return dart__base__locality__select_subdomains(
           domain_in, subdomain_tags, num_subdomain_tags);
}

dart_ret_t dart_domain_exclude(
  dart_domain_locality_t        * domain_in,
  int                             num_subdomain_tags,
  const char                   ** subdomain_tags)
{
  return dart__base__locality__exclude_subdomains(
           domain_in, subdomain_tags, num_subdomain_tags);
}

dart_ret_t dart_domain_add_subdomain(
  dart_domain_locality_t        * domain,
  dart_domain_locality_t        * subdomain,
  int                             subdomain_rel_id)
{
  return dart__base__locality__add_subdomain(
           domain, subdomain, subdomain_rel_id);
}

dart_ret_t dart_domain_remove_subdomain(
  dart_domain_locality_t        * domain,
  int                             subdomain_rel_id)
{
  return dart__base__locality__remove_subdomain(
           domain, subdomain_rel_id);
}

dart_ret_t dart_domain_move_subdomain(
  dart_domain_locality_t        * domain,
  dart_domain_locality_t        * new_parent_domain,
  int                             new_domain_rel_id)
{
  return dart__base__locality__move_subdomain(
           domain, new_parent_domain, new_domain_rel_id);
}

dart_ret_t dart_domain_split_scope(
  const dart_domain_locality_t  * domain_in,
  dart_locality_scope_t           scope,
  int                             num_parts,
  dart_domain_locality_t        * domains_out)
{
  DART_LOG_DEBUG("dart_domain_split_scope() team(%d) domain(%s) "
                 "into %d parts at scope %d",
                 domain_in->team, domain_in->domain_tag, num_parts,
                 scope);

  int    * group_sizes       = NULL;
  char *** group_domain_tags = NULL;

  /* Get domain tags for a split, grouped by locality scope.
   * For 4 domains in the specified scope, a split into 2 parts results
   * in a grouping of domain tags like:
   *
   *   group_domain_tags = {
   *     { split_domain_0, split_domain_1 },
   *     { split_domain_2, split_domain_3 }
   *   }
   */
  DART_ASSERT_RETURNS(
    dart__base__locality__domain_split_tags(
      domain_in, scope, num_parts, &group_sizes, &group_domain_tags),
    DART_OK);

  /* Use grouping of domain tags to create new locality domain
   * hierarchy:
   */
  for (int p = 0; p < num_parts; p++) {
    DART_LOG_DEBUG("dart_domain_split_scope: split %d / %d",
                   p + 1, num_parts);

#ifdef DART_ENABLE_LOGGING
    DART_LOG_TRACE("dart_domain_split_scope: groups[%d] size: %d",
                   p, group_sizes[p]);
    for (int g = 0; g < group_sizes[p]; g++) {
      DART_LOG_TRACE("dart_domain_split:            |- tags[%d]: %s",
                     g, group_domain_tags[p][g]);
    }
#endif

    /* Deep copy of grouped domain so we do not have to recalculate
     * groups for every split group : */
    DART_LOG_TRACE("dart_domain_split_scope: copying input domain");
    DART_ASSERT_RETURNS(
      dart__base__locality__domain__init(
        domains_out + p),
      DART_OK);
    DART_ASSERT_RETURNS(
      dart__base__locality__assign_domain(
        domains_out + p,
        domain_in),
      DART_OK);

    /* Drop domains that are not in split group: */
    DART_LOG_TRACE("dart_domain_split_scope: selecting subdomains");
PRAGMA__PUSH
PRAGMA__IGNORE
    DART_ASSERT_RETURNS(
      dart__base__locality__select_subdomains(
        domains_out + p,
        (const char **)(group_domain_tags[p]),
        group_sizes[p]),
      DART_OK);
PRAGMA__POP
  }

  DART_LOG_DEBUG("dart_domain_split_scope >");
  return DART_OK;
}

dart_ret_t dart_domain_scope_tags(
  const dart_domain_locality_t  * domain_in,
  dart_locality_scope_t           scope,
  int                           * num_domains_out,
  char                        *** domain_tags_out)
{
  *num_domains_out = 0;
  *domain_tags_out = NULL;

  return dart__base__locality__scope_domain_tags(
           domain_in,
           scope,
           num_domains_out,
           domain_tags_out);
}

dart_ret_t dart_domain_scope_domains(
  const dart_domain_locality_t  * domain_in,
  dart_locality_scope_t           scope,
  int                           * num_domains_out,
  dart_domain_locality_t      *** domains_out)
{
  *num_domains_out = 0;
  *domains_out     = NULL;

  return dart__base__locality__scope_domains(
           domain_in,
           scope,
           num_domains_out,
           domains_out);
}

dart_ret_t dart_domain_group(
  dart_domain_locality_t        * domain_in,
  int                             num_group_subdomains,
  const char                   ** group_subdomain_tags,
  char                          * group_domain_tag_out)
{
  return dart__base__locality__domain_group(
           domain_in,
           num_group_subdomains,
           group_subdomain_tags,
           group_domain_tag_out);
}

/* ==================================================================== *
 * Unit Locality                                                        *
 * ==================================================================== */

dart_ret_t dart_unit_locality(
  dart_team_t                     team,
  dart_team_unit_t                unit,
  dart_unit_locality_t         ** locality)
{
  DART_LOG_DEBUG("dart_unit_locality() team(%d) unit(%d)", team, unit.id);

  dart_ret_t ret = dart__base__locality__unit(team, unit, locality);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_unit_locality: "
                   "dart__base__unit_locality__get(unit:%d) failed (%d)",
                   unit.id, ret);
    *locality = NULL;
    return ret;
  }

  DART_LOG_DEBUG("dart_unit_locality > team(%d) unit(%d) -> %p",
                 team, unit.id, (void*)(*locality));
  return DART_OK;
}

