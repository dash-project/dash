#include "TestBase.h"
#include "CoArrayTest.h"

#include <type_traits>


TEST_F(CoArrayTest, TypesInterface)
{
  dash::Co_array<int>         i;
  //dash::Co_array<int[10][20]> x;
  //dash::Co_array<int[][20]>   y(n);
  
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
    //using view_type              = decltype(i)::view_type;
    //using local_type             = decltype(i)::local_type;
    using pattern_type           = decltype(i)::pattern_type;
    
    static_assert(std::rank<value_type>::value == 0,
                  "base type must have rank 0");
  }
  // check fully specified array case
#if 0  
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
    //using view_type              = decltype(x)::view_type;
    //using local_type             = decltype(x)::local_type;
    using pattern_type           = decltype(x)::pattern_type;
    
    static_assert(std::rank<value_type>::value == 0,
                  "base type must have rank 0");
  }
#endif
}
TEST_F(CoArrayTest, ContainerInterface)
{
  // TODO
}
