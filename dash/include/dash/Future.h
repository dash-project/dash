#ifndef DASH__FUTURE_H__INCLUDED
#define DASH__FUTURE_H__INCLUDED

#include <functional>

namespace dash {

template<typename ResultT>
class Future
{
private:
  typedef Future<ResultT>               self_t;
  typedef std::function<ResultT (void)> func_t;

private:
  func_t    _func;
  ResultT   _value;
  bool      _ready     = false;
  bool      _has_value = false;
  bool      _has_func  = false;

public:
  Future()
  : _ready(false),
    _has_value(false),
    _has_func(false)
  { }

  Future(const func_t & func)
  : _func(func),
    _ready(false),
    _has_value(false),
    _has_func(true)
  { }

  Future(
    const func_t    func,
    const ResultT & value)
  : _func(func),
    _value(value),
    _ready(false),
    _has_value(true),
    _has_func(true)
  { }

  Future(
    const ResultT & value)
  : _value(value),
    _ready(true),
    _has_value(false),
    _has_func(true)
  { }

  Future(
    const self_t & other)
  : _func(other._func),
    _value(other._value),
    _ready(other._ready),
    _has_value(other._has_value),
    _has_func(other._has_func)
  { }

  Future<ResultT> & operator=(const self_t & other)
  {
    if (this != &other) {
      _func      = other._func;
      _value     = other._value;
      _ready     = other._ready;
      _has_value = other._has_value;
      _has_func  = other._has_func;
    }
    return *this;
  }

  void wait()
  {
    DASH_LOG_TRACE_VAR("Future.wait()", _ready);
    if (_ready) {
      return;
    }
    if (!_has_func) {
      DASH_LOG_ERROR("Future.wait()", "No function");
      DASH_THROW(
        dash::exception::RuntimeError,
        "Future not initialized with function");
    }
    ResultT result = _func();
    if (!_has_value) {
      _value = result;
    }
    _ready = true;
    DASH_LOG_TRACE_VAR("Future.wait >", _ready);
  }

  bool test() const
  {
    return _ready;
  }

  ResultT & get()
  {
    DASH_LOG_TRACE_VAR("Future.get()", _ready);
    wait();
    DASH_LOG_TRACE_VAR("Future.get >", _value);
    return _value;
  }

}; // class Future


}  // namespace dash

#endif // DASH__FUTURE_H__INCLUDED
