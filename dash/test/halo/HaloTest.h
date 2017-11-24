#ifndef DASH__TEST__HALO_TEST_H_
#define DASH__TEST__HALO_TEST_H_

#include "../TestBase.h"

class HaloTest : public dash::test::TestBase {
protected:

  HaloTest() {
  }

  virtual ~HaloTest() {
  }

  static constexpr long ext_per_dim = 100;

};

#endif // DASH__TEST__HALO_TEST_H_
