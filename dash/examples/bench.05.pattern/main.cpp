/* 
 * Sequential GUPS benchmark for various containers
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include "../bench.h"
#include <libdash.h>

#include <array>
#include <vector>
#include <deque>
#include <iostream>
#include <iomanip>
#include <unistd.h>

using namespace std;

#ifndef TYPE
#define TYPE int
#endif 

typedef dash::TilePattern<
  1,
  dash::ROW_MAJOR,
  int
> PatternType;
typedef dash::Array<
  TYPE,
  int,
  PatternType
> ArrayType;

template<typename Iter>
void init_values(Iter begin, Iter end, unsigned);

double test_pattern_gups(ArrayType & a, unsigned, unsigned);

void perform_test(unsigned ELEM_PER_UNIT, unsigned REPEAT);

double gups(
  /// Number of units
  unsigned N,
  /// Duration in microseconds
  double   useconds,
  /// Elements per unit
  unsigned ELEM_PER_UNIT,
  /// Number of iterations
  unsigned REPEAT) {
  double num_updates = static_cast<double>(N * ELEM_PER_UNIT * REPEAT);
  // kilo-updates / usecs = giga-updates / sec
  return (num_updates / 1000.0f) / useconds;
}

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);

  dash::util::Timer::Calibrate(
    dash::util::TimeMeasure::Clock, 0);

  if (dash::myid() == 0) {
    std::cout << "pattern type: " << PatternType::PatternName
              << std::endl;
  }

  std::deque<std::pair<int, int>> tests;

  tests.push_back({0          , 0}); // this prints the header
  tests.push_back({4          , 10000000});
  tests.push_back({16         , 1000000});
  tests.push_back({64         , 1000000});
  tests.push_back({256        , 100000});
  tests.push_back({1024       , 100000});
  tests.push_back({4096       , 10000});
  tests.push_back({4*4096     , 1000});
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
           << "elem/unit"
           << ", "
           << std::setw(10)
           << "iterations"
           << ", "
           << std::setw(11)
           << "reg.bal"
           << endl;
    }
    return;
  }
  
  ArrayType arr_reg_bal(
    // Total number of elements
    ELEM_PER_UNIT * num_units,
    // 1-dimensional distribution
    dash::DistributionSpec<1>(
      dash::TILE(ELEM_PER_UNIT))
  );
  
  double t_bal_reg = test_pattern_gups(arr_reg_bal, ELEM_PER_UNIT, REPEAT);

  dash::barrier();
  
  if (dash::myid() == 0) {
    double gups_reg_bal = gups(num_units, t_bal_reg, ELEM_PER_UNIT, REPEAT);

    cout << std::setw(10)
         << ELEM_PER_UNIT
         << ", "
         << std::setw(10) 
         << REPEAT
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << gups_reg_bal
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

double test_pattern_gups(
  ArrayType & a,
  unsigned ELEM_PER_UNIT, 
  unsigned REPEAT)
{
  typedef typename ArrayType::index_type index_t;
  typename ArrayType::local_type loc = a.local;
  const PatternType & pattern = a.pattern();

  init_values(a.lbegin(), a.lend(), ELEM_PER_UNIT);

  auto ts_start = dash::util::Timer::Now();
  for (auto i = 0; i < REPEAT; ++i) {
    for (auto g_idx = 0; g_idx < a.size(); ++g_idx) {
      auto g_coords  = std::array<index_t, 1> { g_idx };
      auto local_pos = pattern.local_index(g_coords);
      auto unit_id   = local_pos.unit;
      auto l_index   = local_pos.index;
      if (unit_id == dash::myid()) {
        ++loc[l_index];
      }
    }
  }
  return dash::util::Timer::ElapsedSince(ts_start);
}


