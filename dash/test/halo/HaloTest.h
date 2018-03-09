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
  static constexpr long boundary_width = 5;
  static constexpr long ext_diff = ext_per_dim - boundary_width;

  template<typename MatrixT>
  void init_matrix3D(MatrixT& matrix);
};

#endif // DASH__TEST__HALO_TEST_H_
