/*
 * Independent parallel updates benchmark
 * for various containers
 */
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

typedef dash::util::Timer<
  dash::util::TimeMeasure::Clock
> Timer;

typedef dash::CSRPattern<
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
void init_values(ArrayType & a, unsigned);
template<typename Iter>
bool validate(Iter begin, Iter end, unsigned, unsigned);
bool validate(ArrayType & a, unsigned, unsigned);

double test_dash_pattern(ArrayType & a, unsigned, unsigned);
double test_dash_global_iter(ArrayType & a, unsigned, unsigned);
double test_dash_local_global_iter(ArrayType & a, unsigned, unsigned);
double test_dash_local_iter(ArrayType & a, unsigned, unsigned);
double test_dash_local_subscript(ArrayType & a, unsigned, unsigned);
double test_dash_local_pointer(ArrayType & a, unsigned, unsigned);
double test_stl_vector(unsigned, unsigned);
double test_stl_deque(unsigned, unsigned);
double test_raw_array(unsigned, unsigned);

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
  double num_updates = static_cast<double>(N * ELEM_PER_UNIT * REPEAT);
  // kilo-updates / usecs = giga-updates / sec
  return (num_updates / 1000.0f) / useconds;
}

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  if (dash::myid() == 0) {
    std::cout << "pattern type: " << PatternType::PatternName
              << std::endl;
  }

  std::deque<std::pair<int, int>> tests;

  tests.push_back({0          ,      0}); // this prints the header
  tests.push_back({4          , 100000});
  tests.push_back({16         ,  10000});
  tests.push_back({64         ,  10000});
  tests.push_back({256        ,  10000});
  tests.push_back({1024       ,   1000});
  tests.push_back({4096       ,   1000});
  tests.push_back({4 * 4096   ,    100});
  tests.push_back({16 * 4096  ,    100});
  tests.push_back({64 * 4096  ,     50});
  tests.push_back({128 * 4096 ,     20});

  for (auto test : tests) {
    perform_test(test.first, test.second);
  }

  dash::finalize();

  return 0;
}


void perform_test(
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  auto num_units = dash::size();
  if (ELEM_PER_UNIT == 0) {
    if (dash::myid() == 0) {
      cout << std::setw(10) << "elem/unit";
      cout << "," << std::setw(10) << "iterations";
      cout << "," << std::setw(11) << "pat";
      cout << "," << std::setw(11) << "g_it";
      cout << "," << std::setw(11) << "l_g_it";
      cout << "," << std::setw(11) << "l_it";
      cout << "," << std::setw(11) << "l[]";
      cout << "," << std::setw(11) << "l*";
      cout << "," << std::setw(11) << "stl vector";
      cout << "," << std::setw(11) << "stl deque";
      cout << "," << std::setw(11) << "raw array";
      cout << endl;
    }
    return;
  }

  std::vector<unsigned> local_sizes;
  for (size_t u = 0; u < num_units; ++u) {
    local_sizes.push_back(ELEM_PER_UNIT);
  }

  PatternType pat(local_sizes);
  ArrayType arr(
    pat
  );

  double t0 = test_dash_pattern(arr, ELEM_PER_UNIT, REPEAT);
  double t1 = test_dash_global_iter(arr, ELEM_PER_UNIT, REPEAT);
  double t2 = test_dash_local_global_iter(arr, ELEM_PER_UNIT, REPEAT);
  double t3 = test_dash_local_iter(arr, ELEM_PER_UNIT, REPEAT);
  double t4 = test_dash_local_subscript(arr, ELEM_PER_UNIT, REPEAT);
  double t5 = test_dash_local_pointer(arr, ELEM_PER_UNIT, REPEAT);
  double t6 = test_stl_vector(ELEM_PER_UNIT, REPEAT);
  double t7 = test_stl_deque(ELEM_PER_UNIT, REPEAT);
  double t8 = test_raw_array(ELEM_PER_UNIT, REPEAT);

  dash::barrier();

  if (dash::myid() == 0) {
    double gups0 = gups(num_units, t0, ELEM_PER_UNIT, REPEAT);
    double gups1 = gups(num_units, t1, ELEM_PER_UNIT, REPEAT);
    double gups2 = gups(num_units, t2, ELEM_PER_UNIT, REPEAT);
    double gups3 = gups(num_units, t3, ELEM_PER_UNIT, REPEAT);
    double gups4 = gups(num_units, t4, ELEM_PER_UNIT, REPEAT);
    double gups5 = gups(num_units, t5, ELEM_PER_UNIT, REPEAT);
    double gups6 = gups(num_units, t6, ELEM_PER_UNIT, REPEAT);
    double gups7 = gups(num_units, t7, ELEM_PER_UNIT, REPEAT);
    double gups8 = gups(num_units, t8, ELEM_PER_UNIT, REPEAT);

    cout << std::setw(10) << ELEM_PER_UNIT;
    cout << "," << std::setw(10) << REPEAT;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups0;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups1;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups2;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups3;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups4;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups5;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups6;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups7;
    cout << "," << std::setw(11) << std::fixed << std::setprecision(4) << gups8;
    cout << endl;
  }
}

void init_values(
  ArrayType & a,
  unsigned ELEM_PER_UNIT)
{
  if (dash::myid() == 0) {
    init_values(a.begin(), a.end(), ELEM_PER_UNIT);
  }
  dash::Team::All().barrier();
}

template<typename Iter>
void init_values(
  Iter begin,
  Iter end,
  unsigned ELEM_PER_UNIT)
{
  auto i = 0;
  for (auto it = begin; it != end; ++it, ++i) {
    *it = i;
  }
}

template<typename Iter>
bool validate(
  Iter begin,
  Iter end,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  typedef typename ArrayType::value_type value_t;
  bool valid = true;
  auto i     = 0;
  for (auto it = begin; it != end; ++it, ++i) {
    value_t expected = i + REPEAT;
    value_t value    = *it;
    if (value != expected) {
      valid = false;
      cerr << "Validation failed: "
           << "array[" << i << "] == "
           << value << " != "
           << expected
           << " -- elements/unit: " << ELEM_PER_UNIT
           << endl;
      break;
    }
  }
  return valid;
}

bool validate(
  ArrayType & arr,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  arr.barrier();
  if (dash::myid() == 0) {
    return validate(
             arr.begin(), arr.end(),
             ELEM_PER_UNIT, REPEAT);
  }
  return true;
}

double test_dash_pattern(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  typedef typename ArrayType::index_type index_t;
  typedef typename ArrayType::size_type  extent_t;
  init_values(a, ELEM_PER_UNIT);
  const PatternType & pattern = a.pattern();

  typename ArrayType::local_type loc = a.local;
  Timer timer;
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (extent_t g_idx = 0; g_idx < a.size(); ++g_idx) {
      auto local_pos = pattern.local(g_idx);
      auto unit_id   = local_pos.unit;
      auto l_index   = local_pos.index;
      if (unit_id == pattern.team().myid()) {
        ++loc[l_index];
      }
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    a, ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_dash_global_iter(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  typedef typename ArrayType::value_type   value_t;
  typedef typename ArrayType::pattern_type pattern_t;
  init_values(a, ELEM_PER_UNIT);

  Timer timer;
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (auto it = a.begin(); it != a.end(); ++it) {
      value_t * local_ptr = it.local();
      if (local_ptr != nullptr) {
        ++(*local_ptr);
      }
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    a, ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_dash_local_global_iter(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  typedef typename ArrayType::value_type   value_t;
  typedef typename ArrayType::pattern_type pattern_t;
  init_values(a, ELEM_PER_UNIT);

  // Global offset of first local element:
  auto l_begin_gidx = a.pattern().lbegin();

  dash::GlobIter<value_t, pattern_t> l_git  = a.begin() + l_begin_gidx;
  const dash::GlobIter<value_t, pattern_t> l_gend = l_git + ELEM_PER_UNIT;

  // Iterate over local elements but use global iterator to dereference
  // elements.
  Timer timer;
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (auto it = l_git; it != l_gend; ++it) {
      value_t * local_ptr = it.local();
      if (local_ptr != nullptr) {
        ++(*local_ptr);
      }
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    a, ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_dash_local_iter(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  init_values(a, ELEM_PER_UNIT);

  Timer timer;
  auto lend = a.lend();
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (auto it = a.lbegin(); it != lend; ++it) {
      ++(*it);
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    a, ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_dash_local_subscript(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  init_values(a, ELEM_PER_UNIT);

  Timer timer;
  typename ArrayType::local_type loc = a.local;
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (unsigned j = 0; j < ELEM_PER_UNIT; ++j) {
      ++loc[j];
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    a, ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_dash_local_pointer(
  ArrayType & a,
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  init_values(a, ELEM_PER_UNIT);

  Timer timer;
  auto lbegin = a.lbegin();
  auto lend   = a.lend();

  for (unsigned i = 0; i < REPEAT; ++i) {
    for (auto j = lbegin; j != lend; ++j) {
      ++(*j);
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    a, ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_stl_vector(
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  std::vector<TYPE> arr(ELEM_PER_UNIT);
  init_values(arr.begin(), arr.end(), ELEM_PER_UNIT);

  Timer timer;
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (unsigned j = 0; j < ELEM_PER_UNIT; ++j) {
      arr[j]++;
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    arr.begin(), arr.end(),
    ELEM_PER_UNIT, REPEAT );
  return time_elapsed;
}

double test_stl_deque(
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  std::deque<TYPE> arr(ELEM_PER_UNIT);
  init_values(arr.begin(), arr.end(), ELEM_PER_UNIT);

  Timer timer;
  for (unsigned i = 0; i < REPEAT; ++i) {
    for (unsigned j = 0; j < ELEM_PER_UNIT; ++j) {
      arr[j]++;
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    arr.begin(), arr.end(),
    ELEM_PER_UNIT, REPEAT);
  return time_elapsed;
}

double test_raw_array(
  unsigned ELEM_PER_UNIT,
  unsigned REPEAT)
{
  TYPE * arr = new TYPE[ELEM_PER_UNIT];
  init_values(
    arr, arr + ELEM_PER_UNIT,
    ELEM_PER_UNIT);

  Timer timer;
  for (unsigned i = 0; i < REPEAT; i++) {
    for (unsigned j = 0; j < ELEM_PER_UNIT; j++) {
      arr[j]++;
    }
  }
  auto time_elapsed = timer.Elapsed();

  validate(
    arr, arr + ELEM_PER_UNIT,
    ELEM_PER_UNIT, REPEAT);

  delete[] arr;
  return time_elapsed;
}

