#ifndef DASH__FUTURE_H__INCLUDED
#define DASH__FUTURE_H__INCLUDED

#include <functional>

namespace dash {

template<typename ResultT>
class Future
{
private:
  typedef Future<ResultT>     self_t;

private:
  std::function<ReultT, void> _func;
  ResultT                     _value;
  bool                        _ready = false;

protected:
  Future();

public:

  Future(const std::function<ResultT, void> & func)
  : _ready(false),
    _func(func)
  {
  }

  void wait()
  {
    _value = _func();
    _ready = true;
    return _value;
  }

  bool test() const
  {
    return _ready;
  }

  ResultT & get()
  {
    return _value;
  }

}; // class Future


}  // namespace dash

#endif // DASH__FUTURE_H__INCLUDED
