#include <libdash.h>

#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::setprecision;

// ==========================================================================
// Type definitions
// ==========================================================================

typedef double
  ElementType;
typedef dash::default_index_t
  IndexType;

typedef dash::LoadBalancePattern<1>
  PatternType;

typedef dash::Array<ElementType, IndexType, PatternType>
  ArrayType;


int main(int argc, char **argv)
{
  static const int NELEM = 100;

  dash::init(&argc, &argv);

  dash::util::TeamLocality tloc(dash::Team::All());
  PatternType pattern(
    dash::SizeSpec<1>(NELEM),
    tloc);

  ArrayType arr_a(pattern);
  ArrayType arr_b(pattern);
  ArrayType arr_c(pattern);

  for (size_t li = 0; li < arr_a.lsize(); li++) {
    arr_a.local[li] = 1 + ((42 * (li + 1)) % 1024);
    arr_b.local[li] = 1 + ((42 * (li + 1)) % 1024);
  }

  dash::barrier();

  auto min_git  = dash::transform(arr_a.begin(), arr_a.end(),
                                  arr_b.begin(),
                                  arr_c.begin(),
                                  dash::plus<ElementType>());

  DASH_LOG_DEBUG("perform_test", "Waiting for completion of all units");
  dash::barrier();

  dash::finalize();

  return 0;
}

