#include <libdash.h>
#include <array>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <fstream>  

#include "TestBase.h"
#include "UnitIdTest.h"

TEST_F(UnitIdTest, TypeCompatibility) {
  dash::local_unit_t  l_uid { 12 };
  dash::global_unit_t g_uid { 12 };

  l_uid = g_uid;
}
