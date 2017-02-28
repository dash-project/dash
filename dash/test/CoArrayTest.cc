#include "TestBase.h"
#include "CoArrayTest.h"


TEST_F(CoArrayTest, TypesInterface)
{
  dash::Co_array<int>         i;
  //dash::Co_array<int[10][20]> x;
  //dash::Co_array<int[][20]>   y(n);
  
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
}
TEST_F(CoArrayTest, ContainerInterface)
{
  // TODO
}
