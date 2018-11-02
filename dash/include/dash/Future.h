#ifndef DASH__FUTURE_H__INCLUDED
#define DASH__FUTURE_H__INCLUDED

#include <cstddef>
#include <functional>
#include <sstream>
#include <iostream>

#include <dash/Exception.h>
#include <dash/internal/Logging.h>


namespace dash {

/**
 * Implementation of a future used to wait for an operation to complete
 * and access the value returned by that operation.
 */
template<typename ResultT>
class Future
{
public:
  typedef Future<ResultT>                self_t;

  /**
   * Callback function returning the result value.
   */
  typedef std::function<ResultT (void)>  get_func_t;

  /**
   * Callback function to test for the availability of the result value.
   */
  typedef std::function<bool (ResultT*)> test_func_t;

  /**
   * Callback function called upon destruction.
   */
  typedef std::function<void (void)>     destroy_func_t;

private:
  /// Function returning the value
  get_func_t     _get_func;
  /// Function used to test for the availability of a value
  test_func_t    _test_func;
  /// Function called upon destruction of the future
  destroy_func_t _destroy_func;
  /// The value to be returned byt the future
  ResultT        _value;
  /// Whether or not the value is available
  bool           _ready = false;

public:
  // For ostream output
  template<typename ResultT_>
  friend std::ostream & operator<<(
      std::ostream & os,
      const Future<ResultT_> & future);

public:

  /**
   * Default constructor, creates a future that invalid.
   *
   * \see dash::Future::valid
   */
  Future() noexcept
  { }

  /**
   * Create a future from an already available value.
   */
  Future(const ResultT & result)
  : _value(result),
    _ready(true)
  { }

  /**
   * Create a future using a function that returns the value.
   *
   * \param get_func Function returning the result value.
   */
  Future(const get_func_t & get_func)
  : _get_func(get_func)
  { }

  /**
   * Create a future using a function that returns the value and a function
   * to test whether the value returned by \c get_func is ready.
   *
   * \param get_func  Function returning the result value.
   * \param test_func Function returning \c true and assigning the result value
   *                  to the pointer passed to it if the value is available.
   */
  Future(
    const get_func_t     & get_func,
    const test_func_t    & test_func)
  : _get_func(get_func),
    _test_func(test_func)
  { }

  /**
   * Create a future using a function that returns the value and a function
   * to test whether the value returned by \c get_func is ready, plus a function
   * to be called upon destruction of the future.
   *
   * \param get_func  Function returning the result value.
   * \param test_func Function returning \c true and assigning the result value
   *                  to the pointer passed to it if the value is available.
   * \param destroy_func Function called upon destruction of the future.
   */
  Future(
    const get_func_t     & get_func,
    const test_func_t    & test_func,
    const destroy_func_t & destroy_func)
  : _get_func(get_func),
    _test_func(test_func),
    _destroy_func(destroy_func)
  { }

  /**
   * Copy construction is not permited.
   */
  Future(const self_t& other) = delete;

  /**
   * Default move constructor.
   */
  Future(self_t&& other)      = default;

  /**
   * Destructor. Calls the \c destroy_func passed to the constructor,
   * if available.
   */
  ~Future() {
    if (_destroy_func) {
      _destroy_func();
    }
  }

  /**
   * Copy assignment is not permited.
   */
  self_t & operator=(const self_t& other) = delete;

  /**
   * Move assignment is defaulted.
   */
  self_t & operator=(self_t&& other)      = default;


  /**
   * Wait for the value to become available. It is safe to call \ref get
   * after this call returned.
   */
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

  /**
   * Test whether the value is available. It is safe to call \ref get
   * after this call returned \c true. This function will block if no
   * test-function has been provided.
   *
   * \return \c true if the value is available, \c false otherwise.
   */
  bool test()
  {
    if (!_ready && _test_func) {
      _ready = _test_func(&_value);
    }
    return _ready;
  }

  /**
   * Return the value after making sure that the value is available.
   */
  ResultT get()
  {
    DASH_LOG_TRACE_VAR("Future.get()", _ready);
    wait();
    DASH_LOG_TRACE_VAR("Future.get >", _value);
    return _value;
  }


  /**
   * Check whether the future is valid, i.e., whether either a value or a
   * function to access the valid has been provided.
   */
  bool valid() noexcept
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
