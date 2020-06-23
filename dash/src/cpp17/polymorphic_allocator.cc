/* polymorphic_allocator.cpp                  -*-C++-*-
 *
 *            Copyright 2012 Pablo Halpern.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#include <cpp17/polymorphic_allocator.h>

#ifdef __need_std_pmr_memory_resource
#undef __need_std_pmr_memory_resource

namespace std {

std::atomic<pmr::memory_resource *> pmr::memory_resource::s_default_resource(
    nullptr);

pmr::new_delete_resource *pmr::new_delete_resource_singleton()
{
  //This should be exception safe according to the standard. Otherwise we have
  //to use call_once which guarantees that.
  static new_delete_resource singleton;
  return &singleton;
}

}  // namespace cpp17

#endif

// end polymorphic_allocator.cpp
