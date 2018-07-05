#ifndef DASH__FUTURE_H__INCLUDED
#define DASH__FUTURE_H__INCLUDED

#include <cstddef>
#include <functional>
#include <sstream>
#include <iostream>

#include <dash/Exception.h>
#include <dash/internal/Logging.h>


namespace dash {

template<typename ResultT>
class Future
{
private:
  typedef Future<ResultT>                self_t;
  typedef std::function<ResultT (void)>  get_func_t;
  typedef std::function<bool (ResultT*)> test_func_t;
  typedef std::function<void (void)>     destroy_func_t;

private:
  get_func_t     _get_func;
  test_func_t    _test_func;
  destroy_func_t _destroy_func;
  ResultT        _value;
  bool           _ready = false;

public:
  // For ostream output
  template<typename ResultT_>
  friend std::ostream & operator<<(
      std::ostream & os,
      const Future<ResultT_> & future);

public:

  Future()
  { }

  Future(ResultT & result)
  : _value(result),
    _ready(true)
  { }

  Future(const get_func_t & func)
  : _get_func(func)
  { }

  Future(
    const get_func_t     & get_func,
    const test_func_t    & test_func)
  : _get_func(get_func),
    _test_func(test_func)
  { }

  Future(
    const get_func_t     & get_func,
    const test_func_t    & test_func,
    const destroy_func_t & destroy_func)
  : _get_func(get_func),
    _test_func(test_func),
    _destroy_func(destroy_func)
  { }

  Future(const self_t& other) = delete;
  Future(self_t&& other)      = default;

  ~Future() {
    if (_destroy_func) {
      _destroy_func();
    }
  }

  /// copy-assignment is not permitted
  Future<ResultT> & operator=(const self_t& other) = delete;
  Future<ResultT> & operator=(self_t&& other)      = default;

  void wait()
  {
    DASH_LOG_TRACE_VAR("Future.wait()", _ready);
    if (_ready) {
      return;
    }
    if (!_get_func) {
      DASH_LOG_ERROR("Future.wait()", "No function");
      DASH_THROW(
        dash::exception::RuntimeError,
        "Future not initialized with function");
    }
    _value = _get_func();
    _ready = true;
    DASH_LOG_TRACE_VAR("Future.wait >", _ready);
  }

  bool test()
  {
    if (!_ready && _test_func) {
      _ready = _test_func(&_value);
    }
    return _ready;
  }

  ResultT & get()
  {
    DASH_LOG_TRACE_VAR("Future.get()", _ready);
    wait();
    DASH_LOG_TRACE_VAR("Future.get >", _value);
    return _value;
  }

  bool valid()
  {
    return (_ready || _get_func != nullptr);
  }

}; // class Future

template<typename ResultT>
std::ostream & operator<<(
  std::ostream & os,
  const Future<ResultT> & future)
{
  std::ostringstream ss;
  ss << "dash::Future<" << typeid(ResultT).name() << ">(";
  if (future._ready) {
    ss << future._value;
  } else {
    ss << "not ready";
  }
  ss << ")";
  return operator<<(os, ss.str());
}

}  // namespace dash

#endif // DASH__FUTURE_H__INCLUDED
