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
  typedef std::function<bool (void)>     test_func_t;
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
  : _ready(false)
  { }

  Future(ResultT & result)
  : _value(result),
    _ready(true)
  { }

  Future(const get_func_t & func)
  : _get_func(func),
    _ready(false)
  { }


  Future(
    const get_func_t     & get_func,
    const test_func_t    & test_func,
    const destroy_func_t & destroy_func)
  : _get_func(get_func),
    _test_func(test_func),
    _destroy_func(destroy_func),
    _ready(false)
  { }

  Future(
    const self_t & other)
  : _get_func(other._get_func),
    _test_func(other._test_func),
    _destroy_func(other._destroy_func),
    _value(other._value),
    _ready(other._ready)
  { }

  ~Future() {
    if (_destroy_func) {
      _destroy_func();
    }
  }

  Future<ResultT> & operator=(const self_t & other)
  {
    if (this != &other) {
      _get_func     = other._get_func;
      _test_func    = other._test_func;
      _destroy_func = other._destroy_func;
      _value        = other._value;
      _ready        = other._ready;
    }
    return *this;
  }

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

  bool test() const
  {
    if (!_ready && _test_func) {
      // do not set _ready here because we might have to call _get_func() above
      return _test_func();
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
