#ifndef DASH__TEST__FOR_EACH_TEST_H_
#define DASH__TEST__FOR_EACH_TEST_H_

#include "../TestBase.h"

#include <dash/Array.h>

#include <vector>


/**
 * Test fixture for algorithm dash::for_each.
 */
class ForEachTest : public dash::test::TestBase {
protected:
  typedef double                  Element_t;
  typedef dash::Array<Element_t>  Array_t;
  typedef typename Array_t::pattern_type::index_type
    index_t;

  /// Using a prime to cause inconvenient strides
  size_t _num_elem      = 251;
  /// Stores indices passed to count_invoke
  std::vector<index_t> _invoked_indices;

  ForEachTest() {
  }

  virtual ~ForEachTest() {
  }

public:
  void count_invoke(index_t index) {
    _invoked_indices.push_back(index);
  }
};


struct SimpleExecutor {
  bool operator==(const SimpleExecutor&) const noexcept {
    return true;
  }

  bool operator!=(const SimpleExecutor&) const noexcept {
    return false;
  }

  // Context could return something useful when the Executor is more
  // complicated.
  const SimpleExecutor &context() {
    return *this;
  }

  template <class Property>
    SimpleExecutor require(const Property &) const noexcept {
    // This executor satisfies all problems
    return *this;
  }

  template <class Function>
  void execute(Function &f) const noexcept {
    std::forward<Function>(f)();
  }

  template <class Function, class Shape, class SharedFactory>
  void bulk_execute(Function f, Shape shape, SharedFactory sf) const noexcept {
    auto shared_state(sf());
    auto local_range = std::get<1>(shape);
    auto local_index = std::get<2>(shape);

    auto nelems = local_index.end - local_index.begin;
    for(std::size_t i = 0; i < nelems; ++i) {
      f(local_range.begin, i, shared_state);
    }
  }
};

struct SimplePolicy {
  SimpleExecutor executor() {
    return SimpleExecutor();
  }
};


#endif // DASH__TEST__FOR_EACH_TEST_H_
