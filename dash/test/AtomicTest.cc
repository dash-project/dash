
#include <gtest/gtest.h>

#include <dash/Atomic.h>
#include <dash/Array.h>
#include <dash/Matrix.h>
#include <dash/Shared.h>

#include <dash/algorithm/Copy.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Generate.h>

#include "TestBase.h"
#include "AtomicTest.h"

#include <vector>
#include <algorithm>
#include <numeric>


TEST_F(AtomicTest, FetchAndOp)
{
  typedef size_t value_t;

  value_t     val_init  = 100;
  dash::team_unit_t owner(dash::size() - 1);
  dash::Shared<value_t> shared(owner);

  if (dash::myid() == 0) {
    shared.set(val_init);
  }
  // wait for initialization:
  dash::barrier();

  dash::AtomicAddress<value_t> atomic(shared);

  atomic.fetch_and_add(2);
  // wait for completion of all atomic operations:
  dash::barrier();

  // incremented by 2 by every unit:
  value_t val_expect   = val_init + (dash::size() * 2);
  value_t a_val_actual = atomic.get();
  value_t s_val_actual = shared.get();
  EXPECT_EQ_U(val_expect, a_val_actual);
  EXPECT_EQ_U(val_expect, s_val_actual);

  dash::barrier();
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
  dash::AtomicAddress<value_t>(array[remote_prev]).add(my_val);

  DASH_LOG_TRACE("AtomicTest.ArrayElements",
                 "next: array @ unit(", remote_next, ") +=", my_val);
  dash::AtomicAddress<value_t>(array[remote_next]).fetch_and_add(my_val);

  DASH_LOG_TRACE("AtomicTest.ArrayElements", "barrier #2");
  array.barrier();

  value_t expect_local = my_val + remote_prev + 1 + remote_next + 1;
  value_t actual_local = array.local[0];
  EXPECT_EQ(expect_local, actual_local);

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

TEST_F(AtomicTest, ContainerOfAtomics){
  using value_t    = int;
  using atomic_t   = dash::AtomicAddress<value_t>;
  using base_arr_t = dash::Array<value_t>;
  using atom_arr_t = dash::Array<atomic_t>;

  base_arr_t  base_array(dash::size());
  atom_arr_t  atom_array(dash::size());
  
  int pos = dash::myid().id;
  std::function<atomic_t()> gen = [&](){
    return atomic_t(base_array.begin() + pos);
  };
  dash::fill(base_array.begin(), base_array.end(), 0);
  dash::generate(atom_array.begin(), atom_array.end(), gen);

  dash::barrier();

  // each unit increments all values by index+1
  for(int i=0; i<dash::size(); ++i){
    atomic_t elem_arr = atom_array[i];
    elem_arr.add(i+1);
  }
  
  dash::barrier();

  for(int i=0; i<dash::size(); ++i){
    atomic_t elem_arr      = atom_array[i];
    value_t elem_arr_local = elem_arr.get();
    ASSERT_EQ_U(elem_arr_local, static_cast<value_t>((dash::size()*(i+1))));
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
}

TEST_F(AtomicTest, AtomicInContainer){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  array_t array(dash::size());

  // supported as Atomic<value_t>(value_t T) is available
  dash::fill(array.begin(), array.end(), 0);
  dash::barrier();

  for(int i=0; i<dash::size(); ++i){
    array[i].add(i+1);
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
}

TEST_F(AtomicTest, AtomicInterface){
  using value_t = int;
  using atom_t  = dash::Atomic<value_t>;
  using array_t = dash::Array<atom_t>;

  array_t array(10);

  dash::fill(array.begin(), array.end(), 0);
  dash::barrier();

  ++(array[0]);
  array[1]++;
  --(array[2]);
  array[3]--;
  
  dash::barrier();
  ASSERT_EQ_U(array[0].load(), dash::size());
  ASSERT_EQ_U(array[1].load(), dash::size());
  ASSERT_EQ_U(array[2].load(), -dash::size());
  ASSERT_EQ_U(array[3].load(), -dash::size());
  
  dash::barrier();
  
  if(dash::myid() == 0){
    auto oldval = array[3].exchange(1);
    ASSERT_EQ_U(oldval, -dash::size());
  }
  dash::barrier();
  ASSERT_EQ_U(array[3].load(), 1);
}
