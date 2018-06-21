#include "CoarrayTest.h"

#include <type_traits>

#include <dash/Types.h>
#include <dash/Atomic.h>
#include <dash/Mutex.h>
#include <dash/Algorithm.h>
#include <dash/util/TeamLocality.h>

// for std::lock_guard
#include <mutex>
#include <thread>
#include <random>

using namespace dash::coarray;

bool CoarrayTest::core_mapping_is_unique(const dash::Team & team){
  std::vector<int> unit_core_ids;

  for(dash::global_unit_t unit{0}; unit<team.size(); ++unit){
    const dash::util::UnitLocality uloc(unit);
    unit_core_ids.push_back(uloc.hwinfo().core_id);
  }

  std::sort(unit_core_ids.begin(), unit_core_ids.end());
  return (std::unique(unit_core_ids.begin(),
                      unit_core_ids.end()) == unit_core_ids.end());
}

TEST_F(CoarrayTest, TypesInterface)
{
  int n = 10;

  dash::Coarray<int>         i;
  dash::Coarray<int[10][20]> x;
  dash::Coarray<int[][20]>   y(n);

  // check scalar case
  {
    using value_type             = decltype(i)::value_type;
    using difference_type        = decltype(i)::difference_type;
    using index_type             = decltype(i)::index_type;
    using size_type              = decltype(i)::size_type;
    using iterator               = decltype(i)::iterator;
    using const_iterator         = decltype(i)::const_iterator;
    using reverse_iterator       = decltype(i)::reverse_iterator;
    using const_reverse_iterator = decltype(i)::const_reverse_iterator;
    using reference              = decltype(i)::reference;
    using const_reference        = decltype(i)::const_reference;
    using local_pointer          = decltype(i)::local_pointer;
    using const_local_pointer    = decltype(i)::const_local_pointer;
    using view_type              = decltype(i)::view_type<decltype(i)::ndim()>;
    using local_type             = decltype(i)::local_type;
    using pattern_type           = decltype(i)::pattern_type;

    static_assert(std::rank<value_type>::value == 0,
                  "base type must have rank 0");
  }
  // check fully specified array case
  {
    using value_type             = decltype(x)::value_type;
    using difference_type        = decltype(x)::difference_type;
    using index_type             = decltype(x)::index_type;
    using size_type              = decltype(x)::size_type;
    using iterator               = decltype(x)::iterator;
    using const_iterator         = decltype(x)::const_iterator;
    using reverse_iterator       = decltype(x)::reverse_iterator;
    using const_reverse_iterator = decltype(x)::const_reverse_iterator;
    using reference              = decltype(x)::reference;
    using const_reference        = decltype(x)::const_reference;
    using local_pointer          = decltype(x)::local_pointer;
    using const_local_pointer    = decltype(x)::const_local_pointer;
    using view_type              = decltype(x)::view_type<decltype(x)::ndim()>;
    using local_type             = decltype(x)::local_type;
    using pattern_type           = decltype(x)::pattern_type;

    static_assert(std::rank<value_type>::value == 0,
                  "base type must have rank 0");
  }

  // check partially specified array case
  {
    using value_type             = decltype(y)::value_type;
    using difference_type        = decltype(y)::difference_type;
    using index_type             = decltype(y)::index_type;
    using size_type              = decltype(y)::size_type;
    using iterator               = decltype(y)::iterator;
    using const_iterator         = decltype(y)::const_iterator;
    using reverse_iterator       = decltype(y)::reverse_iterator;
    using const_reverse_iterator = decltype(y)::const_reverse_iterator;
    using reference              = decltype(y)::reference;
    using const_reference        = decltype(y)::const_reference;
    using local_pointer          = decltype(y)::local_pointer;
    using const_local_pointer    = decltype(y)::const_local_pointer;
    using view_type              = decltype(y)::view_type<decltype(y)::ndim()>;
    using local_type             = decltype(y)::local_type;
    using pattern_type           = decltype(y)::pattern_type;

    static_assert(std::rank<value_type>::value == 0,
                  "base type must have rank 0");
  }
}

TEST_F(CoarrayTest, ContainerInterface)
{
  dash::Coarray<int>         i;
  dash::Coarray<int[10][20]> x;

  int value = 10;

  // access syntax
  // custom proxy reference necessary
  i(0) = value; // global access
  i    = value; // local access
  x(0)[1][2] = value; // global access
  // access using team_unit_t
  x(static_cast<dash::team_unit_t>(0))[1][2] = value; // global access
  x[2][3]    = value; // local access

  // conversion test
  int b = i;
#if 1
  // inc / dec test
  int c = i++;
  int d = i--;
  int e = ++i;
  int f = --i;

  // expression test
  int g = ((b + i) * i) / i;
  int h = i + b;

  // Coarray to Coarray
  if(num_images() >= 2){
    x(0)[3][4] = x(1)[1][2];
  }
#endif

  dash::Coarray<int[10]>   swap_a;
  dash::Coarray<int[10]>   swap_b;
  swap_a[0] = 0;
  swap_b[0] = 1;

  swap_a.flush_local();
  swap_b.flush_local();

  dash::barrier();
  std::swap(swap_a, swap_b);
  dash::barrier();

  int value_a = swap_a[0];
  int value_b = swap_b[0];

  ASSERT_EQ_U(value_a, 1);
  ASSERT_EQ_U(value_b, 0);
}

TEST_F(CoarrayTest, ElementAccess)
{
  constexpr int size = 10;
  dash::Coarray<int> x;
  x = 100*this_image();
  x.sync_all();
  // every unit reads data of right neighbour
  auto nextunit = (this_image() + 1) % num_images();
  int value = x(nextunit);
  ASSERT_EQ_U(value, 100*nextunit);
}

TEST_F(CoarrayTest, ArrayElementAccess)
{
  constexpr int size = 10;
  dash::Coarray<int[size]> x;
  for(int i=0; i<size; ++i){
    x[i] = i + 100*this_image();
  }
  x.sync_all();
  // every unit reads data of right neighbour
  for(int i=0; i<size; ++i){
    auto nextunit = (this_image() + 1) % num_images();
    int value = x(nextunit)[i];
    ASSERT_EQ_U(value, i + 100*nextunit);
  }
}

TEST_F(CoarrayTest, Collectives)
{
  dash::Coarray<int>         i;
  dash::Coarray<int[10][20]> x;

  if(this_image() == 0){
    i = 10;
  }
  cobroadcast(i, dash::team_unit_t{0});
  ASSERT_EQ_U(static_cast<int>(i), 10);

  std::fill(x.lbegin(), x.lend(), 2);
  x.barrier();
  coreduce(x, dash::plus<int>());
  x.barrier();
  ASSERT_EQ_U(static_cast<int>(x[5][0]), 2 * dash::size());
}

TEST_F(CoarrayTest, Synchronization)
{
  std::chrono::time_point<std::chrono::system_clock> start, end;

  if(num_images() < 3){
    SKIP_TEST_MSG("This test requires at least 3 units");
  }
  dash::barrier();

  start = std::chrono::system_clock::now();

  if(this_image() != 2){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  sync_images(std::array<int,2>{0,1});
  end = std::chrono::system_clock::now();
  sync_all();
  int elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                      (end-start).count();

  // sleeps for pretty printing only
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  LOG_MESSAGE("Unit %d finished after %d ms",
              static_cast<int>(this_image()), elapsed_ms);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if(this_image() != 2){
    ASSERT_GE_U(elapsed_ms, 490);
  } else {
    ASSERT_LE_U(elapsed_ms, 200);
  }
}

TEST_F(CoarrayTest, Iterators)
{
  dash::Coarray<int>         i;
  dash::Coarray<int[10][20]> x;

  EXPECT_EQ_U(std::distance(i.begin(), i.end()), dash::size());

  // -------------------------------------------------------------------------
  // Bug in DASH Matrix
  EXPECT_EQ_U(std::distance(x(0).begin(), x(0).end()), 10*20);

  dash::NArray<int, 3> matrix(dash::size(), 10, 20);
  EXPECT_EQ_U(std::distance(matrix[0].begin(), matrix[0].end()), 10*20);

  int visited = 0;
  auto curpos = matrix[0].begin();
  while(curpos != matrix[0].end()){
    ++curpos;
    ++visited;
  }
  // -------------------------------------------------------------------------
}

TEST_F(CoarrayTest, CoFutures)
{
  dash::Coarray<int> x;
  int i = static_cast<int>(this_image());
  x = i;
  x.barrier();

  // at this point, there is no possibility to get an async MatrixRef
  //auto a = x(i).async;
}

TEST_F(CoarrayTest, MemoryModel)
{
  int i = static_cast<int>(this_image());
  {
    // scalar case
    using Coarray_t = dash::Coarray<dash::Atomic<int>>;
    using Reference = Coarray_t::reference;

    Coarray_t x;
    x(i) = i;
    x.barrier();
    x(i) += 1;
    // add to local part
    x += 10;
    x -= 5;
    int result = x(i).load();
    EXPECT_EQ_U(result, i+6);

    EXPECT_EQ_U(++x, i+7);
    EXPECT_EQ_U(--x, i+6);
    
    // check type conversion to base type
    result = static_cast<int>(x);
    EXPECT_EQ_U(result, i+6);
  }

  dash::barrier();

  {
    // array case
    using coarr_atom_t = dash::Coarray<dash::Atomic<int>[10][20]>;
    coarr_atom_t y;
    y(i)[0][0] = i;
    y(i)[0][0] += 1;
    int result = y(i)[0][0].load();
    EXPECT_EQ_U(result, i+1);
  }
}

TEST_F(CoarrayTest, Mutex){
  dash::Mutex mx;

  dash::Coarray<int> arr;

  arr = 0;
  arr.sync_all();

  mx.lock();
  int tmp = arr(0);
  // use explicit gref object. The following is not valid,
  // as different GlobRefs are returned
  // arr(0) = x; arr(0).flush();
  auto gref = arr(0);
  gref = tmp + 1;
  //gref.flush();
  LOG_MESSAGE("Before %d, after %d", tmp, static_cast<int>(arr(0)));
  mx.unlock();

  arr.sync_all();

  if(this_image() == 0){
    int result = arr;
    EXPECT_EQ_U(result, static_cast<int>(dash::size()));
  }

  arr.sync_all();
  // this even works with std::lock_guard
  {
    std::lock_guard<dash::Mutex> lg(mx);
    LOG_MESSAGE("Lock aquired at unit %d",
                static_cast<int>(this_image()));
    int tmp = arr(0);
    auto gref = arr(0);
    gref = tmp + 1;
    //gref.flush();
    LOG_MESSAGE("Lock released at unit %d",
                static_cast<int>(this_image()));
  }

  arr.sync_all();

  if(this_image() == 0){
    int result = arr;
    EXPECT_EQ_U(result, static_cast<int>(dash::size())*2);
  }
}

TEST_F(CoarrayTest, Comutex){
  // Check runtime conditions
  // This might deadlock if multiple units are pinned to the same CPU
  if(!core_mapping_is_unique(dash::Team::All())){
    SKIP_TEST_MSG("Multiple units are mapped to the same core => possible deadlock");
  }

  // Test Setup
  const int repetitions = 10;

  dash::Comutex comx;
  dash::Coarray<int> arr;

  std::random_device rd;
#ifndef DEBUG
  unsigned int seed = rd();
#else
  // avoid non-deterministic code coverage changes
  unsigned int seed = 42;
#endif
  std::default_random_engine dre(seed);
  std::uniform_int_distribution<int> uniform_dist(0, dash::size()-1);

  arr = 0;
  arr.sync_all();

  // only for logging
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // each unit adds 1 to a random unit exactly n times
  for(int i=0; i<repetitions; ++i){
    int rand_unit = uniform_dist(dre);
    LOG_MESSAGE("Update unit %d", rand_unit);
    {
      std::lock_guard<dash::Mutex> lg(comx(rand_unit));
      int tmp = arr(rand_unit);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      arr(rand_unit) = tmp + 1;
      //arr(rand_unit).flush();
    }
  }
  arr.sync_all();
  // only for logging
  std::this_thread::sleep_for(std::chrono::microseconds(100));

  // sum should be dash::size() * repetitions
  auto sum = dash::accumulate(arr.begin(), arr.end(), 0, dash::plus<int>());
  if(this_image() == 0){
    ASSERT_EQ_U(sum, dash::size() * repetitions);
  }
}

// declare befor dash is initialized
dash::Coarray<int> delay_alloc_arr;

TEST_F(CoarrayTest, DelayedAllocation)
{
  int i = static_cast<int>(this_image());
  EXPECT_EQ_U(delay_alloc_arr.size(), 0);
  dash::barrier();

  delay_alloc_arr.allocate();
  delay_alloc_arr(i) = i;
  delay_alloc_arr.barrier();

  int result = delay_alloc_arr(i);
  EXPECT_EQ_U(result, i);

  dash::barrier();

  delay_alloc_arr.deallocate();
  EXPECT_EQ_U(delay_alloc_arr.size(), 0);
}

TEST_F(CoarrayTest, StructType)
{
  struct value_t {double a; int b;};
  dash::Coarray<value_t> x;
  double a_exp = static_cast<double>(this_image()) + 0.1;
  int    b_exp = static_cast<int>(this_image());

  x.member(&value_t::a) = a_exp;
  x.member(&value_t::b) = b_exp;
  x.sync_all();
  double a_got_loc = x.member(&value_t::a);
  int    b_got_loc = x.member(&value_t::b);
  ASSERT_EQ_U(a_got_loc, a_exp);
  ASSERT_EQ_U(b_got_loc, b_exp);

  value_t val_loc = x;
  ASSERT_EQ_U(val_loc.a, a_exp);
  ASSERT_EQ_U(val_loc.b, b_exp);

  int next_image = (static_cast<int>(this_image()) + 1) % num_images();
  double a_got_rem = x(next_image).member(&value_t::a);
  int    b_got_rem = x(next_image).member(&value_t::b);

  value_t val_rem = x(next_image);

  if(this_image() != (num_images()-1)){
    ASSERT_EQ_U(a_got_rem, (a_exp + 1));
    ASSERT_EQ_U(b_got_rem, (b_exp + 1));
    ASSERT_EQ_U(val_rem.a, (a_exp + 1));
    ASSERT_EQ_U(val_rem.b, (b_exp + 1));

  } else {
    ASSERT_EQ_U(a_got_rem, 0.1);
    ASSERT_EQ_U(b_got_rem, 0);
    ASSERT_EQ_U(val_rem.a, 0.1);
    ASSERT_EQ_U(val_rem.b, 0);
  }
  x.sync_all();
}

TEST_F(CoarrayTest, CoEvent)
{
  dash::Coevent events;

  if(num_images() < 2){
    SKIP_TEST_MSG("This test requires at least 2 units");
  }
  if(!core_mapping_is_unique(dash::Team::All())){
    SKIP_TEST_MSG("Multiple units are mapped to the same core => possible deadlock");
  }

  if(this_image() == 0){
    events(1).post();
    LOG_MESSAGE("event posted to unit 1");
  }
  // TODO this barrier should not be necessary, but without
  // the gptr is not updated
  dash::barrier();

  if(this_image() == 1){
    LOG_MESSAGE("waiting for incoming event");
    events.wait();
    LOG_MESSAGE("event recieved");
  }
  dash::barrier();

  if(num_images() < 3){
    return;
  }

  events(0).post();
  // same here
  dash::barrier();
  // wait for all events, similar to barrier
  if(this_image() == 0){
    LOG_MESSAGE("waiting for incoming event");
    events.wait(num_images());
    LOG_MESSAGE("event recieved");
  }

  dash::barrier();
  if(this_image() != 0){
    events(0).post();
  }
  dash::barrier();
#if 0
  // should work, but does not in mpich
  int recv_events = events(0).test();
  LOG_MESSAGE("received %d events on unit 0", recv_events);
  ASSERT_GT_U(recv_events, 0);
#endif
  if(this_image() == 0){
    ASSERT_GT_U(events.test(), 0);
    events.wait(num_images()-1);
  }
}

TEST_F(CoarrayTest, CoEventIter)
{
  if(num_images() < 3){
    SKIP_TEST_MSG("This test requires at least 3 units");
  }
  // Check runtime conditions
  // This might deadlock if multiple units are pinned to the same CPU
  if(!core_mapping_is_unique(dash::Team::All())){
    SKIP_TEST_MSG("Multiple units are mapped to the same core => possible deadlock");
  }

  dash::Coevent events;

  auto snd = events.begin()+1;
  (*snd).post();

  if(num_images() == 3){
    ASSERT_EQ_U(events.begin()+3, events.end());
  }
  dash::barrier();
  if(this_image() == 1){
    events.wait(num_images());
  }
}
