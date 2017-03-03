#include "TestBase.h"
#include "CoArrayTest.h"

#include <type_traits>

#include <dash/Types.h>

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
  using namespace dash::co_array;
  
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