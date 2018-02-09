
#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "GlobIterTest.h"

#include <type_traits>
#include <dash/Array.h>
#include <dash/Atomic.h>


TEST_F(GlobIterTest, IteratorTypes)
{
  {
    using array_t = dash::Array<int>;
    static_assert(std::is_trivially_copyable<array_t::iterator>::value,
        "Array::iterator not trivially copyable");
    static_assert(std::is_trivially_copyable<array_t::const_iterator>::value,
        "Array::const_iterator not trivially copyable");
  }
  {
    using array_t = dash::Array<dash::Atomic<int>>;
    static_assert(std::is_trivially_copyable<array_t::iterator>::value,
        "Array<Atomic<T>>::iterator not trivially copyable");
    static_assert(std::is_trivially_copyable<array_t::const_iterator>::value,
        "Array<Atomic<T>>::const_iterator not trivially copyable");
  }
}
