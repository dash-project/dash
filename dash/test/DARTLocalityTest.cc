#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "DARTLocalityTest.h"

TEST_F(DARTLocalityTest, UnitLocality)
{
  DASH_LOG_TRACE("DARTLocalityTest.Domains",
                 "get local unit locality descriptor");
  dart_unit_locality_t * ul;
  ASSERT_EQ_U(DART_OK, dart_unit_locality(DART_TEAM_ALL, _dash_id, &ul));
  DASH_LOG_TRACE("DARTLocalityTest.Domains",
                 "pointer to local unit locality descriptor:", ul);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", *ul);

  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->unit);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->domain_tag);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.host);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.numa_id);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.cpu_id);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.num_cores);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.min_cpu_mhz);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.max_cpu_mhz);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.min_threads);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", ul->hwinfo.max_threads);

  EXPECT_EQ_U(_dash_id, ul->unit);

  // Units may group multiple cores:
  EXPECT_GE_U(ul->hwinfo.cpu_id,      -1); // -1 if unknown, >= 0 if set
  EXPECT_GE_U(ul->hwinfo.num_cores,   -1); // -1 if unknown, >  0 if set
  EXPECT_GE_U(ul->hwinfo.min_threads, -1); // -1 if unknown, >  0 if set
  EXPECT_GE_U(ul->hwinfo.max_threads, -1); // -1 if unknown, >  0 if set

  EXPECT_NE_U(ul->hwinfo.num_cores,    0); // must be either -1 or > 0
  EXPECT_NE_U(ul->hwinfo.min_threads,  0); // must be either -1 or > 0
  EXPECT_NE_U(ul->hwinfo.max_threads,  0); // must be either -1 or > 0

  // Get domain locality from unit locality descriptor:
  DASH_LOG_TRACE("DARTLocalityTest.UnitLocality",
                 "get local unit's domain descriptor");
  dart_domain_locality_t * dl;
  ASSERT_EQ_U(
    DART_OK,
    dart_domain_team_locality(DART_TEAM_ALL, ul->domain_tag, &dl));
  DASH_LOG_TRACE("DARTLocalityTest.UnitLocality",
                 "pointer to local unit's domain descriptor:", dl);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", *dl);

  EXPECT_GT_U(dl->level, 2);
  EXPECT_EQ_U(dl->scope, DART_LOCALITY_SCOPE_CORE);
}

TEST_F(DARTLocalityTest, Domains)
{
  DASH_LOG_TRACE("DARTLocalityTest.Domains",
                 "get global domain descriptor");
  dart_domain_locality_t * dl;
  ASSERT_EQ_U(DART_OK, dart_domain_team_locality(DART_TEAM_ALL, ".", &dl));
  DASH_LOG_TRACE("DARTLocalityTest.Domains",
                 "pointer to global domain descriptor: ", dl);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.Domains", *dl);

  DASH_LOG_TRACE_VAR("DARTLocalityTest.Domains", dl->domain_tag);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.Domains", dl->level);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.Domains", dl->num_domains);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.Domains", dl->num_nodes);

  // Global domain has locality level 0 (DART_LOCALITY_SCOPE_GLOBAL):
  EXPECT_EQ_U(dl->level, 0);
  EXPECT_EQ_U(dl->level, DART_LOCALITY_SCOPE_GLOBAL);
}
