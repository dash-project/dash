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

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <mpi.h>

#include <unistd.h>
#include <stdio.h>
#include <sched.h>

/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

dart_ret_t dart_domain_locality(
  dart_team_t               team,
  const char              * domain_tag,
  dart_domain_locality_t ** locality)
{
  DART_LOG_DEBUG("dart_domain_locality() team(%d) domain(%s) -> %p",
                 team, domain_tag, *locality);
  dart_ret_t ret;

  dart_domain_locality_t * dloc;
  ret = dart__base__locality__domain(team, domain_tag, &dloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_domain_locality: dart__base__locality__domain "
                   "failed (%d)", ret);
    return ret;
  }
  *locality = dloc;

  DART_LOG_DEBUG("dart_domain_locality > team(%d) domain(%s) -> %p",
                 team, domain_tag, *locality);
  return DART_OK;
}

dart_ret_t dart_domain_split(
  dart_domain_locality_t  * domain_in,
  dart_locality_scope_t     scope,
  int                       num_parts,
  dart_domain_locality_t  * split_domain_loc_out)
{
  DART_LOG_DEBUG("dart_domain_split() team(%d) domain(%s) "
                 "into %d parts at scope %d",
                 domain_in->team, domain_in->domain_tag, num_parts, scope);
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

#ifdef DART_ENABLE_LOGGING
  for (int p = 0; p < num_parts; p++) {
    DART_LOG_TRACE("dart_domain_split: groups[%d] size: %d",
                   p, group_sizes[p]);
    for (int g = 0; g < group_sizes[p]; g++) {
      DART_LOG_TRACE("dart_domain_split:            tag: %s",
                     group_domain_tags[p][g]);
    }
  }
#endif

  /* Use grouping of domain tags to create new locality domain
   * hierarchy:
   */

#if 0
  dart_domain_locality_t * input_domain_cpy =
    malloc(sizeof(dart_domain_locality_t));

  /* Deep copy of domain: */
  DART_LOG_TRACE("dart_domain_split: copying input domain");
  DART_ASSERT_RETURNS(
    dart__base__locality__copy_domain(
      domain_in,
      input_domain_cpy),
    DART_OK);
  /* Create groups in copy of domain: */
  DART_LOG_TRACE("dart_domain_split: grouping domain)");
  DART_ASSERT_RETURNS(
    dart__base__locality__domain_group(
      input_domain_cpy,
      num_parts,
      group_sizes,
      group_domain_tags),
    DART_OK);
#endif
  for (int p = 0; p < num_parts; p++) {
    DART_LOG_DEBUG("dart_domain_split: domain split %d / %d",
                   p + 1, num_parts);
    /* Deep copy of grouped domain so we do not have to recalculate
     * groups for every split group : */
    DART_LOG_TRACE("dart_domain_split: copying input domain");
    DART_ASSERT_RETURNS(
      dart__base__locality__copy_domain(
        domain_in,
        split_domain_loc_out + p),
      DART_OK);
  }
  for (int p = 0; p < num_parts; p++) {
    /* Drop domains that are not in split group: */
    DART_LOG_TRACE("dart_domain_split: selecting subdomains");
    DART_ASSERT_RETURNS(
      dart__base__locality__select_subdomains(
        split_domain_loc_out + p,
        group_domain_tags[p],
        group_sizes[p]),
      DART_OK);
  }

  DART_LOG_DEBUG("dart_domain_split >");
  return DART_OK;
}

dart_ret_t dart_scope_domains(
  dart_domain_locality_t  * domain_in,
  dart_locality_scope_t     scope,
  int                     * num_domains_out,
  char                  *** domain_tags_out)
{
  *num_domains_out = 0;
  *domain_tags_out = NULL;

  return dart__base__locality__scope_domains(
           domain_in, scope,
           num_domains_out,
           domain_tags_out);
}

/* ====================================================================== *
 * Unit Locality                                                          *
 * ====================================================================== */

dart_ret_t dart_unit_locality(
  dart_team_t             team,
  dart_unit_t             unit,
  dart_unit_locality_t ** locality)
{
  DART_LOG_DEBUG("dart_unit_locality() team(%d) unit(%d)", team, unit);
  *locality = NULL;

  dart_unit_locality_t * uloc;
  dart_ret_t ret = dart__base__locality__unit(team, unit, &uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_unit_locality: "
                   "dart__base__unit_locality__get(unit:%d) failed (%d)",
                   unit, ret);
    return ret;
  }
  *locality = uloc;

  DART_LOG_DEBUG("dart_unit_locality > team(%d) unit(%d) -> %p",
                 team, unit, *locality);
  return DART_OK;
}

