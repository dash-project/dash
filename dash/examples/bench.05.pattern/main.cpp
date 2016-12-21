#include <libdash.h>

#include "../bench.h"
#include "MockPattern.h"

#include <array>
#include <vector>
#include <deque>
#include <iostream>
#include <iomanip>
#include <unistd.h>

using std::cout;
using std::endl;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

#ifndef TYPE
#define TYPE int
#endif

typedef dash::MockPattern<
  1,
  dash::ROW_MAJOR,
  int
> MockPattern_t;
typedef dash::CSRPattern<
  1,
  dash::ROW_MAJOR,
  int
> IrregPattern_t;
typedef dash::TilePattern<
  1,
  dash::ROW_MAJOR,
  int
> TilePattern_t;

typedef dash::Array<
  TYPE,
  int,
  MockPattern_t
> ArrayMockDist_t;
typedef dash::Array<
  TYPE,
  int,
  IrregPattern_t
> ArrayIrregDist_t;
typedef dash::Array<
  TYPE,
  int,
  TilePattern_t
> ArrayTiledDist_t;

template<typename Iter>
void init_values(Iter begin, Iter end, unsigned);

template<class ArrayType>
double test_pattern_gups(ArrayType & a, unsigned, unsigned);

template<class ArrayType>
double test_raw_gups(ArrayType & a, unsigned, unsigned);

void perform_test(unsigned ELEM_PER_UNIT, unsigned REPEAT);

double gups(
  /// Number of units
  unsigned N,
  /// Duration in microseconds
  double   useconds,
  /// Elements per unit
  unsigned ELEM_PER_UNIT,
  /// Number of iterations
  unsigned REPEAT)
{
  double num_derefs_per_unit =
    static_cast<double>(N * ELEM_PER_UNIT * REPEAT);
  // kilo-updates / usecs = giga-updates / sec
  return N * (num_derefs_per_unit / 1000.0f) / useconds;
}

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  std::deque<std::pair<int, int>> tests;

  tests.push_back({0          , 0}); // this prints the header
  tests.push_back({4          , 1000000});
  tests.push_back({16         , 100000});
  tests.push_back({64         , 100000});
  tests.push_back({256        , 10000});
  tests.push_back({1024       , 10000});
  tests.push_back({4096       , 1000});
  tests.push_back({4*4096     , 500});
  tests.push_back({16*4096    , 100});
  tests.push_back({64*4096    , 50});

  for (auto test: tests) {
    perform_test(test.first, test.second);
  }

  dash::finalize();

  return 0;
}

void perform_test(
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT) {
  auto num_units = dash::size();
  if (ELEM_PER_UNIT == 0) {
    if (dash::myid() == 0) {
      cout << std::setw(10)
           << "units"
           << ", "
           << std::setw(10)
           << "elem/unit"
           << ", "
           << std::setw(10)
           << "iterations"
           << ", "
           << std::setw(11)
           << "mock"
           << ", "
           << std::setw(11)
           << "irreg"
           << ", "
           << std::setw(11)
           << "tiled"
           << ", "
           << std::setw(11)
           << "raw"
           << endl;
    }
    return;
  }

  std::vector<unsigned> local_sizes;
  for (size_t u = 0; u < num_units; ++u) {
    local_sizes.push_back(ELEM_PER_UNIT);
  }

  MockPattern_t mock_pat(
    // Local sizes
    local_sizes
  );
  ArrayMockDist_t arr_mock_dist(
    mock_pat
  );
  IrregPattern_t irreg_pat(
    // Local sizes
    local_sizes
  );
  ArrayIrregDist_t arr_irreg_dist(
    irreg_pat
  );
  ArrayTiledDist_t arr_tiled_dist(
    // Total number of elements
    ELEM_PER_UNIT * num_units,
    // 1-dimensional distribution
    dash::DistributionSpec<1>(
      dash::TILE(ELEM_PER_UNIT))
  );

  double t_mock  = test_pattern_gups(arr_mock_dist,  ELEM_PER_UNIT, REPEAT);
  double t_irreg = test_pattern_gups(arr_irreg_dist, ELEM_PER_UNIT, REPEAT);
  double t_tiled = test_pattern_gups(arr_tiled_dist, ELEM_PER_UNIT, REPEAT);
  double t_raw   = test_raw_gups(    arr_tiled_dist, ELEM_PER_UNIT, REPEAT);

  dash::barrier();

  if (dash::myid() == 0) {
    double gups_mock  = gups(num_units, t_mock,  ELEM_PER_UNIT, REPEAT);
    double gups_irreg = gups(num_units, t_irreg, ELEM_PER_UNIT, REPEAT);
    double gups_tiled = gups(num_units, t_tiled, ELEM_PER_UNIT, REPEAT);
    double gups_raw   = gups(num_units, t_raw,   ELEM_PER_UNIT, REPEAT);

    cout << std::setw(10)
         << num_units
         << ", "
         << std::setw(10)
         << ELEM_PER_UNIT
         << ", "
         << std::setw(10)
         << REPEAT
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << gups_mock
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << gups_irreg
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << gups_tiled
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << gups_raw
         << endl;
  }
}

template<typename Iter>
void init_values(
  Iter begin,
  Iter end,
  unsigned ELEM_PER_UNIT) {
  auto i = 0;
  for(auto it = begin; it != end; ++it, ++i) {
    *it = i;
  }
}

template <class ArrayType>
double test_pattern_gups(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  typedef typename ArrayType::pattern_type pattern_t;
  typedef typename ArrayType::index_type index_t;
  typedef typename ArrayType::local_type local_t;
  local_t loc               = a.local;
  const pattern_t & pattern = a.pattern();

  init_values(a.lbegin(), a.lend(), ELEM_PER_UNIT);

  auto a_size   = a.size();
  auto ts_start = Timer::Now();
  auto myid     = pattern.team().myid();
  for (auto i = 0; i < REPEAT; ++i) {
    for (auto g_idx = 0; g_idx < a_size; ++g_idx) {
      auto local_pos = pattern.local(g_idx);
      auto unit_id   = local_pos.unit;
      auto l_index   = local_pos.index;
      if (unit_id == myid) {
        ++loc[l_index];
      }
    }
  }
  return Timer::ElapsedSince(ts_start);
}

template <class ArrayType>
double test_raw_gups(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  typedef typename ArrayType::pattern_type pattern_t;
  typedef typename ArrayType::index_type index_t;
  typedef typename ArrayType::local_type local_t;
  local_t loc               = a.local;
  const pattern_t & pattern = a.pattern();
  auto lsize                = ELEM_PER_UNIT;
  auto lbegin_global        = pattern.global(0);
  auto lend_global          = pattern.global(lsize-1);

  init_values(a.lbegin(), a.lend(), ELEM_PER_UNIT);

  auto a_size   = a.size();
  auto ts_start = Timer::Now();
  auto myid     = dash::myid();
  for (auto i = 0; i < REPEAT; ++i) {
    int l_idx = 0;
    for (auto g_idx = 0; g_idx < a_size; ++g_idx) {
      if (lbegin_global <= g_idx && g_idx <= lend_global) {
        ++loc[l_idx];
        ++l_idx;
      }
    }
  }
  return Timer::ElapsedSince(ts_start);
}



