#ifndef DASH__TEST__GLOB_STENCIL_ITER_TEST_H_
#define DASH__TEST__GLOB_STENCIL_ITER_TEST_H_

#include <gtest/gtest.h>
#include <libdash.h>
#include <dash/experimental/Halo.h>
#include <dash/experimental/iterator/GlobStencilIter.h>
/**
 * Test fixture for class dash::GlobStencilIter.
 */
class GlobStencilIterTest : public ::testing::Test {
protected:

  GlobStencilIterTest() {
  }

  virtual ~GlobStencilIterTest() {
  }

  virtual void SetUp() {
    dash::init(&TESTENV.argc, &TESTENV.argv);
  }

  virtual void TearDown() {
    dash::finalize();
  }
};

#endif // DASH__TEST__CARTESIAN_TEST_H_
