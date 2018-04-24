
#include <gtest/gtest.h>

#include <dash/Atomic.h>
#include <dash/Array.h>
#include <dash/Mutex.h>
#include <dash/Matrix.h>
#include <dash/Shared.h>

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Generate.h>

#include "../TestBase.h"
#include "AtomicTest.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <thread>
#include <chrono>


TEST_F(AtomicTest, FetchAndOp)
{
  typedef size_t value_t;

  value_t           val_init  = 100;
  dash::team_unit_t owner(dash::size() - 1);

  dash::Shared< dash::Atomic<value_t> > shared(owner);

  if (dash::myid() == 0) {
    shared.set(val_init);
  }
  // wait for initialization:
  shared.barrier();

  shared.get().fetch_add(2);
  // wait for completion of all atomic operations:
  shared.barrier();

  // incremented by 2 by every unit:
  value_t val_expect   = val_init + (dash::size() * 2);
  value_t s_val_actual = shared.get();
  EXPECT_EQ_U(val_expect, s_val_actual);

  dash::barrier();
}


TEST_F(AtomicTest, CompareExchange)
{
  typedef size_t value_t;

  value_t           val_init  = 10 * dash::size();
  dash::team_unit_t owner(dash::size() - 1);

  dash::Shared< dash::Atomic<value_t> > shared(owner);

  if (dash::myid() == 0) {
    shared.set(val_init);
  }
  // wait for initialization:
  shared.barrier();
  size_t i;
  for (i = 0; i < 2*dash::size(); ++i) {
    value_t expected = shared.get().get();
    value_t desired  = expected / 2;
    bool    result   = shared.get().compare_exchange(expected, desired);
    if (result) {
      break;
    }
  }

  // we should not need more tries than there are units
  ASSERT_LT(i, dash::size());

  dash::barrier();
}

template <typename T>
struct container {
  typedef struct container<T> self_t;

  self_t operator++(int) {
    self_t res = *this;
    res.val[0]++;
    res.val[1]++;
    return res;
  }

  T & operator[](int idx) {
    return val[idx];
  }

  const T & operator[](int idx) const {
    return val[idx];
  }

  constexpr bool operator==(const self_t & rhs) const {
    return val[0] == rhs.val[0] &&
           val[1] == rhs.val[1];
  }
  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  T val[2];
};

template <typename T>
std::ostream & operator<<(
  std::ostream & os,
  const container<T> & ct)
{
  std::ostringstream ss;
  ss << dash::typestr(ct) << "(" << ct[0] << "," << ct[1] << ")";
  return operator<<(os, ss.str());
}

template <>
std::ostream & operator<<(
  std::ostream                   & os,
  const container<unsigned char> & ct)
{
  std::ostringstream ss;
  ss << dash::typestr(ct) << "("
     << "0x" << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(ct[0]) << ","
     << "0x" << std::hex << std::setw(2) << std::setfill('0')
             << static_cast<int>(ct[1]) << ")";
  return operator<<(os, ss.str());
}

TEST_F(AtomicTest, PunnedType)
{
  typedef struct container<short> value_t;

  value_t           val_init = { -1, 10 };
  value_t           val_exch = {  1, 20 };
  dash::team_unit_t owner(dash::size() - 1);

  dash::Shared< dash::Atomic<value_t> > shared(owner);

  if (dash::myid() == 0) {
    // Set value via interface of dash::Shared, should delegate to
    // dash::GlobRef<Atomic<T>>.set():
    shared.set(val_init);
  }
  shared.barrier();

  if (dash::myid() == (dash::size() > 2 ? 1 : 0)) {
    // Set value via interface of dash::GlobRef<Atomic<T>>:
    shared.get().exchange(val_exch);
  }
  shared.barrier();

  EXPECT_EQ_U(val_exch, shared.get().load());

  // Test compare-exchange with old value = new value,
  // should always succeed:
  //
  // Note: Assuming CAS has no spurious failures
  //
  {
    // Test in several repetitions:
    for (short rep = 0; rep < 50; ++rep) {
      size_t i;
      for (i = 0; i < 2 * dash::size(); ++i) {
        value_t expected = shared.get().load();
        value_t desired  = expected;
        bool    result   = shared.get().compare_exchange(expected, desired);
        if (result) {
          EXPECT_EQ_U(expected, desired);
          break;
        }
      }
      shared.barrier();
      // Should not need more tries than there are units
      ASSERT_LE(i, dash::size());
    }
  }

  // Test compare-exchange with old value != new value,
  // should not fail more than (nunits - 1) times:
  //
  // Note: Assuming CAS has no spurious failures
  //
  {
    // Test in several repetitions:
    for (short rep = 0; rep < 50; ++rep) {
      value_t desired { rep, static_cast<short>(rep + dash::myid().id) };
      bool   success = false;
      size_t i       = 0;
      while (!success && ++i < dash::size()) {
        value_t expected = shared.get().load();
        success          = shared.get().compare_exchange(expected, desired);
      }
      shared.barrier();
      // Should not need more tries than there are units
      ASSERT_LE(i, dash::size());
    }
  }

  dash::barrier();
}

TEST_F(AtomicTest, PunnedTypeFetchOp)
{
  typedef struct container<unsigned char> value_t;

  value_t           val_init = { 0xa0, 0xa0 }; // 1010.0000 1010.0000
  value_t           val_op   = { 0xf0, 0x0f }; // 1111.0000 0000.1111
  value_t           val_xor  = { 0x50, 0xaf }; // 0101.0000 1010.1111
  value_t           val_zero = { 0x00, 0x00 };
  dash::team_unit_t owner(dash::size() - 1);

  dash::Shared< dash::Atomic<value_t> > shared(owner);

  if (dash::myid() == 0) {
    shared.set(val_init);
  }
  shared.barrier();

  DASH_LOG_DEBUG_VAR("AtomicTest.PunnedTypeFetchOp", shared.get().load());

  // Test in several repetitions:
  for (short rep = 0; rep < 50; ++rep) {
    value_t prev_val = shared.get().fetch_op(
                                      dash::bit_xor<value_t>(),
                                      val_op);
    DASH_LOG_DEBUG_VAR("AtomicTest.PunnedTypeFetchOp", val_op);
    DASH_LOG_DEBUG_VAR("AtomicTest.PunnedTypeFetchOp", prev_val);
    shared.barrier();

    EXPECT_TRUE_U(prev_val == val_zero ||
                  prev_val == val_xor  ||
                  prev_val == val_init);
  }
}

TEST_F(AtomicTest, ArrayElements)
{
  typedef int value_t;

  dash::Array<value_t> array(dash::size());
  value_t my_val = dash::myid() + 1;
  array.local[0] = my_val;
  DASH_LOG_TRACE("AtomicTest.ArrayElement", "barrier #0");
  array.barrier();

  value_t expect_init_acc = (dash::size() * (dash::size() + 1)) / 2;
  if (dash::myid() == 0) {
    // Create local copy for logging:
    value_t *            l_copy = new value_t[array.size()];
    std::vector<value_t> v_copy(array.size());
    dash::copy(array.begin(), array.end(), l_copy);
    std::copy(l_copy, l_copy + array.size(), v_copy.begin());
    DASH_LOG_DEBUG_VAR("AtomicTest.ArrayElement", v_copy);

    value_t actual_init_acc = std::accumulate(
                                l_copy, l_copy + array.size(), 0);
    EXPECT_EQ_U(expect_init_acc, actual_init_acc);
    delete[] l_copy;
  }
  DASH_LOG_TRACE("AtomicTest.ArrayElements", "barrier #1");
  array.barrier();

  dash::team_unit_t remote_prev(dash::myid() == 0
                            ? dash::size() - 1
                            : dash::myid() - 1);
  dash::team_unit_t remote_next(dash::myid() == dash::size() - 1
                            ? 0
                            : dash::myid() + 1);
  // Add own value to previous and next unit in the array's iteration order.
  // In effect, sum of all array values should have tripled.
  DASH_LOG_TRACE("AtomicTest.ArrayElements",
                 "prev: array @ unit(", remote_prev, ") +=", my_val);
  // in fact, this is a hack
  dash::GlobRef<dash::Atomic<value_t>>(
          array[remote_prev].dart_gptr()
        ).add(my_val);

  DASH_LOG_TRACE("AtomicTest.ArrayElements",
                 "next: array @ unit(", remote_next, ") +=", my_val);
  dash::GlobRef<dash::Atomic<value_t>>(
          array[remote_next].dart_gptr()
        ).fetch_add(my_val);

  DASH_LOG_TRACE("AtomicTest.ArrayElements", "barrier #2");
  array.barrier();

  value_t expect_local = my_val + remote_prev + 1 + remote_next + 1;
  value_t actual_local = array.local[0];
  EXPECT_EQ_U(expect_local, actual_local);

  if (dash::myid() == 0) {
    // Create local copy for logging:
    value_t *            l_copy = new value_t[array.size()];
    std::vector<value_t> v_copy(array.size());
    dash::copy(array.begin(), array.end(), l_copy);
    std::copy(l_copy, l_copy + array.size(), v_copy.begin());
    DASH_LOG_DEBUG_VAR("AtomicTest.ArrayElements", v_copy);

    value_t expect_res_acc = expect_init_acc * 3;
    value_t actual_res_acc = std::accumulate(
                               l_copy, l_copy + array.size(), 0);
    EXPECT_EQ_U(expect_res_acc, actual_res_acc);
    delete[] l_copy;
  }
}

TEST_F(AtomicTest, AlgorithmVariant){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  array_t array(dash::size());

  dash::fill(array.begin(), array.end(), 0);
  dash::barrier();

  for(int i=0; i<dash::size(); ++i){
    dash::atomic::add(array[i], i+1);
  }

  dash::barrier();
  for(int i=0; i<dash::size(); ++i){
    value_t elem_arr_local = dash::atomic::load(array[i]);
    ASSERT_EQ_U(elem_arr_local, static_cast<value_t>((dash::size()*(i+1))));
  }

  dash::barrier();
  for(int i=0; i<dash::size(); ++i){
    dash::atomic::sub(array[i], 1);
  }
  dash::barrier();
  for(int i=0; i<dash::size(); ++i){
    value_t elem_arr_local = dash::atomic::load(array[i]);
    ASSERT_EQ_U(elem_arr_local, static_cast<value_t>((dash::size()*(i))));
  }

  dash::barrier();
  for(int i=0; i<dash::size(); ++i){
    dash::atomic::multiply(array[i], -1);
  }
  dash::barrier();
  for(int i=0; i<dash::size(); ++i){
    value_t elem_arr_local = dash::atomic::load(array[i]);
    ASSERT_EQ_U(
      elem_arr_local,
      static_cast<value_t>((dash::size()*i*(std::pow(-1, dash::size()))))
    );
  }
}

TEST_F(AtomicTest, AtomicInContainer){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;
  using matrix_t = dash::Matrix<atom_t,1>;

  array_t array(dash::size());
  matrix_t matrix(dash::size());

  // supported as Atomic<value_t>(value_t T) is available
  dash::fill(array.begin(), array.end(), 0);
  dash::fill(matrix.begin(), matrix.end(), 0);
  dash::barrier();

  for(int i=0; i<dash::size(); ++i){
    array[i].add(i+1);
    matrix[i].add(i+1);
  }

  dash::barrier();

  LOG_MESSAGE("Trivial Type: is_atomic_type %d",
      dash::is_atomic<value_t>::value);
  LOG_MESSAGE("Atomic Type:  is_atomic_type %d",
      dash::is_atomic<atom_t>::value);

  for(int i=0; i<dash::size(); ++i){
    value_t elem_arr_local = dash::atomic::load(array[i]);
    ASSERT_EQ_U(elem_arr_local, static_cast<value_t>((dash::size()*(i+1))));
  }
  for(int i=0; i<dash::size(); ++i){
    value_t elem_arr_local = dash::atomic::load(matrix[i]);
    ASSERT_EQ_U(elem_arr_local, static_cast<value_t>((dash::size()*(i+1))));
  }
}

TEST_F(AtomicTest, AtomicInterface){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;
  size_t num_elem = std::max(static_cast<size_t>(10), dash::size());

  array_t array(num_elem);

  dash::fill(array.begin(), array.end(), 0);
  dash::barrier();

  ++(array[0]);
  array[1]++;
  --(array[2]);
  array[3]--;

  dash::barrier();
  ASSERT_EQ_U(array[0].load(), dash::size());
  ASSERT_EQ_U(array[3].load(), -dash::size());

  dash::barrier();

  if(dash::myid() == 0){
    auto oldval = array[3].exchange(1);
    ASSERT_EQ_U(oldval, -dash::size());
  }
  dash::barrier();
  ASSERT_EQ_U(array[3].load(), 1);
  dash::barrier();

  value_t myid     = static_cast<value_t>(dash::myid().id);
  value_t id_right = (myid + 1) % dash::size();

  array[myid].store(myid);
  array.barrier();
  ASSERT_EQ_U(id_right, array[id_right].load());
  array.barrier();
  array[myid].op(dash::plus<value_t>(), 2);
  array.barrier();
  ASSERT_EQ_U(id_right+2, array[id_right].fetch_op(dash::plus<value_t>(), 1));
  array.barrier();
  array[myid].exchange(-myid);
  array[1].exchange(-1);
  array.barrier();
  ASSERT_EQ_U(-myid, array[myid].load());
  array.barrier();
  array[1].multiply(-1);
  array.barrier();
  ASSERT_EQ_U(std::pow(-1, dash::size()+1), array[1].get());
  bool ret = array[myid].compare_exchange(0,10);
  if(myid == 0){
    ASSERT_EQ_U(true, ret);
    ASSERT_EQ_U(10, array[myid].load());
  } else {
    ASSERT_EQ_U(false, ret);
  }
  array.barrier();
  array[0] += 1;
  array[0] -= 1;
  array.barrier();
  ASSERT_EQ_U(10, array[0].get());
}

TEST_F(AtomicTest, MutexInterface){
  dash::Mutex mx;

  dash::Shared<int> shared(dash::team_unit_t{0});

  if(dash::myid() == 0){
    shared.set(0);
  }

  mx.lock();
  int tmp = shared.get();
  std::this_thread::sleep_for(std::chrono::microseconds(100));
  shared.set(tmp + 1);
  LOG_MESSAGE("Before %d, after %d", tmp, static_cast<int>(shared.get()));
  // I guess here a flush is required, blocked by issue 322
  mx.unlock();

  dash::barrier();

  while(!mx.try_lock()){  }
  // lock aquired
  tmp = shared.get();
  std::this_thread::sleep_for(std::chrono::microseconds(100));
  shared.set(tmp + 1);
  mx.unlock();

  dash::barrier();

  if(dash::myid() == 0){
    int result = shared.get();
    EXPECT_EQ_U(result, static_cast<int>(dash::size()*2));
  }

  dash::barrier();

  // this even works with std::lock_guard
  {
    std::lock_guard<dash::Mutex> lg(mx);
    int tmp = shared.get();
    shared.set(tmp + 1);
  }

  dash::barrier();

  if(dash::myid() == 0){
    int result = shared.get();
    EXPECT_EQ_U(result, static_cast<int>(dash::size())*3);
  }
}


TEST_F(AtomicTest, AtomicSignal){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }

  array_t array(dash::size());
  dash::fill(array.begin(), array.end(), 0);
  array.barrier();

  if (dash::myid() != 0) {
    // send the signal
    array[0].add(1);
  } else {

    // wait for a signal to arrive
    int  count;
    auto agref = array[0];
    do {
      count = agref.get();
    } while (count == 0);
    ASSERT_GT_U(count, 0);
  }
}


TEST_F(AtomicTest, ElementCompare){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  array_t array(dash::size());

  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }

  // asynchronous atomic set
  if (dash::myid() == 0) {
    for (size_t i = 0; i < dash::size(); ++i) {
      array.async[i].set(i);
    }
  }
  array.barrier();

  ASSERT_EQ_U(array[dash::myid()].get(), dash::myid());

  int val = -1;
  // kick off an asynchronous transfer
  array.async[dash::myid()].get(&val);

  // wait for the above asynchronous transfer to complete
  array.flush();
  ASSERT_EQ_U(dash::myid(), val);

  dash::barrier();

  // test fetch_op

  if (dash::myid() == 0) {
    std::vector<value_t> vec(dash::size());
    for (size_t i = 0; i < dash::size(); ++i) {
      array.async[i].exchange(0, &vec[i]);
    }
    array.flush();
    for (size_t i = 0; i < dash::size(); ++i) {
      ASSERT_EQ_U(i, vec[i]);
    }
  }

  dash::barrier();

  // atomic increment on unit zero
  array.async[0].add(1);

  // flush on AtomicAsyncRef
  array.async[0].flush();
  dash::barrier();
  if (dash::myid() == 0) ASSERT_EQ_U(array.async[0].get(), dash::size());

  dash::fill(array.begin(), array.end(), 0);

  array.async[dash::myid()].fetch_add(dash::myid(), &val);
  array.barrier();
  ASSERT_EQ_U(0, val);

  if (dash::myid() == 1) {
    std::vector<value_t> vec(dash::size());
    for (size_t i = 0; i < dash::size(); ++i) {
      array.async[i].compare_exchange(i, 2*i, &vec[i]);
    }
    array.flush();
    for (size_t i = 0; i < dash::size(); ++i) {
      ASSERT_EQ_U(i, vec[i]);
      ASSERT_EQ_U(2*i, array[i].get());
    }
  }

  dash::barrier();
  dash::fill(array.begin(), array.end(), 1);
  dash::barrier();

  if (dash::myid() == 0) {
    for (size_t i = 0; i < dash::size(); ++i) {
      array.async[i].multiply(2);
    }
  }
  array.barrier();
  ASSERT_EQ_U(2, array[dash::myid()].get());
  array.barrier();

}

TEST_F(AtomicTest, ConstTest) {

  dash::Array<dash::Atomic<int>> array(dash::size());
  const dash::Array<dash::Atomic<int>>& carr = array;
  array[dash::myid()].set(0);
  dash::barrier();

  // conversion non-const -> const
  dash::GlobRef<dash::Atomic<const int>> gref1 = array[0];
  // assignment const -> const
  dash::GlobRef<dash::Atomic<const int>> gref2 = carr[0];
  // explicit conversion const->non-const
  dash::GlobRef<dash::Atomic<int>> gref3 =
                        static_cast<dash::GlobRef<dash::Atomic<int>>>(carr[0]);

  // should fail!
  //gref1.add(1);

  // works
  ASSERT_EQ_U(0, gref1.get());

  /**
   * GlobAsyncRef
   */

  // conversion non-const -> const
  dash::GlobAsyncRef<dash::Atomic<const int>> agref1 = array.async[0];
  // assignment const -> const
  dash::GlobAsyncRef<dash::Atomic<const int>> agref2 = carr.async[0];
  // explicit conversion const->non-const
  dash::GlobAsyncRef<dash::Atomic<int>> agref3 =
                        static_cast<dash::GlobAsyncRef<dash::Atomic<int>>>(carr.async[0]);

  // should fail!
  //agref1.add(1);

  // works
  ASSERT_EQ_U(0, agref1.get());
}

TEST_F(AtomicTest, AsyncAtomic){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  array_t array(dash::size());

  dash::fill(array.begin(), array.end(), 0);
  dash::barrier();

  ASSERT_EQ_U(0, array[dash::myid()]);
  ASSERT_EQ_U(0UL, array[dash::myid()]);
  ASSERT_EQ_U(array[dash::myid()], 0);
  ASSERT_EQ_U(array[dash::myid()], 0UL);
  // forbidden
  //ASSERT_EQ_U(array[0], array[dash::myid()]);

  // OK
  ASSERT_EQ_U(array[0].get(), array[dash::myid()]);
}

TEST_F(AtomicTest, LongDouble){
  using value_t = long double;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  array_t array(dash::size());
  array[dash::myid()] = 0.0;
  dash::barrier();

  array[0].fetch_add(1.0);
  dash::barrier();
  if (dash::myid() == 0) {
    ASSERT_EQ_U(static_cast<size_t>(array[0]), dash::size());
  }

  dash::barrier();
  array[0].exchange(dash::myid()*1.0);
  dash::barrier();

  if (dash::myid() == 0) {
    ASSERT_LT_U(static_cast<size_t>(array[0]), dash::size());
    ASSERT_GE_U(static_cast<size_t>(array[0]), 0);
    array[0].set(0.0);
  }

  dash::barrier();
  array[0].add(1.0);
  dash::barrier();
  if (dash::myid() == 0) {
    ASSERT_EQ_U(static_cast<size_t>(array[0]), dash::size());
  }

  // atomic compare_exchange not allowed for floats
  // array[0].compare_exchange(dash::size()*1.0, dash::myid()*1.0);

}
