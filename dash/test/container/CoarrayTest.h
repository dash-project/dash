#ifndef DASH__TEST__COARRAY_TEST_H_
#define DASH__TEST__COARRAY_TEST_H_

#include <gtest/gtest.h>

#include "../TestBase.h"

#include <dash/Coarray.h>
#include <dash/Comutex.h>
#include <dash/Coevent.h>

/**
 * Test fixture for class dash::co_array
 */
class CoarrayTest : public dash::test::TestBase {
public:
  /**
   * Check if not two units share the same core
   * within this team
   */
  bool core_mapping_is_unique(const dash::Team & );
};

#endif // DASH__TEST__COARRAY_TEST_H_
