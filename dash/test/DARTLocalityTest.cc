#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "DARTLocalityTest.h"

TEST_F(DARTLocalityTest, UnitLocality)
{
  DASH_LOG_TRACE("DARTLocalityTest.DomainLocality",
                 "get local unit locality descriptor");
  dart_unit_locality_t * uloc;
  ASSERT_EQ_U(DART_OK, dart_unit_locality(_dash_id, &uloc));
  DASH_LOG_TRACE("DARTLocalityTest.DomainLocality",
                 "pointer to local unit locality descriptor:", uloc);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", *uloc);

  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->unit);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->domain_tag);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->host);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->numa_id);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->core_id);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->num_cores);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->min_cpu_mhz);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->max_cpu_mhz);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", uloc->num_threads);

  EXPECT_EQ_U(uloc->unit, _dash_id);
  // Units in Team::All() are members of the global domain:
  EXPECT_EQ_U(std::string(uloc->domain_tag), ".");

  // Units may group multiple cores:
  EXPECT_GE_U(uloc->core_id,     0);
  EXPECT_GT_U(uloc->num_cores,   0);
  EXPECT_GT_U(uloc->num_threads, 0);

  // Get domain locality from unit locality descriptor:
  DASH_LOG_TRACE("DARTLocalityTest.UnitLocality",
                 "get local unit's domain descriptor");
  dart_domain_locality_t * dloc;
  ASSERT_EQ_U(DART_OK, dart_domain_locality(uloc->domain_tag, &dloc));
  DASH_LOG_TRACE("DARTLocalityTest.UnitLocality",
                 "pointer to local unit's domain descriptor:", dloc);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.UnitLocality", *dloc);
  // Global domain has locality level 0 (DART_LOCALITY_SCOPE_GLOBAL):
  EXPECT_EQ_U(dloc->level, 0);
  EXPECT_EQ_U(dloc->level, DART_LOCALITY_SCOPE_GLOBAL);
}

TEST_F(DARTLocalityTest, DomainLocality)
{
  DASH_LOG_TRACE("DARTLocalityTest.DomainLocality",
                 "get global domain descriptor");
  dart_domain_locality_t * dloc;
  ASSERT_EQ_U(DART_OK, dart_domain_locality(".", &dloc));
  DASH_LOG_TRACE("DARTLocalityTest.DomainLocality",
                 "pointer to global domain descriptor: ", dloc);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", *dloc);

  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->domain_tag);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->level);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->host);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->num_domains);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->num_nodes);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->num_sockets);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->num_numa);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->num_cores);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->min_cpu_mhz);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->max_cpu_mhz);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->min_threads);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->max_threads);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->cache_sizes[0]);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->cache_sizes[1]);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality", dloc->cache_sizes[2]);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality",
                     dloc->cache_line_sizes[0]);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality",
                     dloc->cache_line_sizes[1]);
  DASH_LOG_TRACE_VAR("DARTLocalityTest.DomainLocality",
                     dloc->cache_line_sizes[2]);

  EXPECT_GT_U(dloc->num_sockets, 0);
  EXPECT_GT_U(dloc->num_numa,    0);
  EXPECT_GT_U(dloc->num_cores,   0);
  EXPECT_GT_U(dloc->min_threads, 0);
  EXPECT_GT_U(dloc->max_threads, 0);

  // Units in Team::All() are members of the global domain:
  EXPECT_EQ_U(std::string(dloc->domain_tag), ".");
  // Global domain has locality level 0 (DART_LOCALITY_SCOPE_GLOBAL):
  EXPECT_EQ_U(dloc->level, 0);
  EXPECT_EQ_U(dloc->level, DART_LOCALITY_SCOPE_GLOBAL);
}
