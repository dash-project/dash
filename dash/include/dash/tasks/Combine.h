#ifndef DASH__TASKS__COMBINE_H__
#define DASH__TASKS__COMBINE_H__

#include <dash/tasks/Tasks.h>
#include <dash/dart/if/dart_tasking.h>

#include <vector>
#include <algorithm>

namespace dash{
namespace tasks{

template<typename ValueType>
class Combinator {

private:
  std::vector<ValueType> _tls;

public:

  Combinator(const ValueType& init = ValueType())
  : _tls(dash::tasks::numthreads(), init)
  { }

  ValueType& local(){
    return _tls[dash::tasks::threadnum()];
  }

  const ValueType& local() const{
    return _tls[dash::tasks::threadnum()];
  }

  template<typename BinaryOp>
  ValueType reduce(BinaryOp op) {
    return std::accumulate(std::next(_tls.begin()),
                             _tls.end(), *_tls.begin(), op);
  }

  void clear(const ValueType& init = ValueType()){
    std::fill(_tls.begin(), _tls.end(), init);
  }

};

} // namespace tasks
} // namespace dash

#endif // DASH__TASKS__COMBINE_H__
