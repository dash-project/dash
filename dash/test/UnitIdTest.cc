#include <libdash.h>
#include <array>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>  

#include "TestBase.h"
#include "UnitIdTest.h"

TEST_F(UnitIdTest, TypeCompatibility)
{
  DASH_TEST_LOCAL_ONLY();

  dash::team_unit_t   l_uid { 12 };
  dash::global_unit_t g_uid { 12 };

  // this must fail to compile
  //   l_uid = g_uid;
  
  dart_team_unit_t     l_dart_uid { 23 };
  dart_global_unit_t  g_dart_uid { 45 };

  dash::team_unit_t   l_dash_uid(l_dart_uid);
  dash::global_unit_t g_dash_uid(g_dart_uid);

  ASSERT_EQ(l_dart_uid.id, l_dash_uid.id);
  ASSERT_EQ(g_dart_uid.id, g_dash_uid.id);
}

