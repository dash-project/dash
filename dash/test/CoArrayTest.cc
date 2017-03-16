#include "TestBase.h"
#include "CoArrayTest.h"

#include <type_traits>

#include <dash/Types.h>
#include <dash/Atomic.h>
#include <dash/Mutex.h>

// for std::lock_guard
#include <mutex>

using namespace dash::co_array;

TEST_F(CoArrayTest, TypesInterface)
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

TEST_F(CoArrayTest, ContainerInterface)
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
  if(dash::co_array::num_images() >= 2){
    x(0)[3][4] = x(1)[1][2];
  }
#endif
}

TEST_F(CoArrayTest, Collectives)
{
  dash::Coarray<int>         i;
  dash::Coarray<int[10][20]> x;
  
  if(this_image() == 0){
    i = 10;
  }
  cobroadcast(i, dash::team_unit_t{0});
  ASSERT_EQ_U(static_cast<int>(i), 10);
  
  std::fill(x.begin(), x.end(), 2);
  x.barrier();
  coreduce(x, dash::plus<int>());
  x.barrier();
  ASSERT_EQ_U(static_cast<int>(x[5][0]), 2 * dash::size());
}

TEST_F(CoArrayTest, Iterators)
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

TEST_F(CoArrayTest, CoFutures)
{
  dash::Coarray<int> x;
  int i = static_cast<int>(this_image());
  x = i;
  x.barrier();
  
  // at this point, there is no possibility to get an async MatrixRef
  //auto a = x(i).async;
}

TEST_F(CoArrayTest, MemoryModel)
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
    int result = x(i).load();
    EXPECT_EQ_U(result, i+1);
    // at this point, there is no possibility to get an async MatrixRef
    //auto a = x(i).async;
  }
  
  dash::barrier();

  // blocked by issue 322
  {
    // array case
    using coarr_atom_t = dash::Coarray<dash::Atomic<int[10][20]>>;
    coarr_atom_t y;
    y(i)[0][0] = i;
    y(i)[0][0] += 1;
    int result = y(i)[0][0].load();
    EXPECT_EQ_U(result, i+1);
  }
}

TEST_F(CoArrayTest, Mutex){
  dash::Mutex mx;
  
  dash::Coarray<int> arr;
  
  arr = 0;
  
  mx.lock();
  int tmp = arr(0);
  arr(0) = tmp + 1;
  LOG_MESSAGE("Before %d, after %d", tmp, static_cast<int>(arr(0)));
  // I guess here a flush is required, blocked by issue 322
  mx.unlock();
  
  dash::barrier();
  
  if(this_image() == 0){
    int result = arr;
    EXPECT_EQ_U(result, static_cast<int>(dash::size()));
  }
  
  dash::barrier();
  // this even works with std::lock_guard
  {
    std::lock_guard<dash::Mutex> lg(mx);
    int tmp = arr(0);
    arr(0) = tmp + 1;
  }
  
  dash::barrier();
  
  if(this_image() == 0){
    int result = arr;
    EXPECT_EQ_U(result, static_cast<int>(dash::size())*2);
  }
}

// declare befor dash is initialized
dash::Coarray<int> delay_alloc_arr;

TEST_F(CoArrayTest, DelayedAllocation)
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