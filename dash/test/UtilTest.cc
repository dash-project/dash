
#include "UtilTest.h"

#include <dash/util/CapturedMovable.h>

template <class T>
class ValueMock {
  bool moved       = false;
  bool copied      = false;

  T _value;
public:
  ValueMock() = delete;

  explicit ValueMock(const T & v) : _value(v) {
    DASH_LOG_TRACE("ValueMock", "ValueMock(T)");
  }
  ValueMock(const ValueMock & o) : _value(o._value) {
    copied = true;
    DASH_LOG_TRACE("ValueMock", "ValueMock(const self_t &)");
  }
  ValueMock(ValueMock && o) : _value(std::forward<T>(o._value)) {
    moved = true;
    DASH_LOG_TRACE("ValueMock", "ValueMock(self_t &&)");
  }
  ValueMock & operator=(const ValueMock & o) {
    _value = o._value;
    copied = true;
    DASH_LOG_TRACE("ValueMock", "operator=(const self_t &)");
  }
  ValueMock & operator=(ValueMock && o) {
    _value = o._value;
    moved = true;
    DASH_LOG_TRACE("ValueMock", "operator=(self_t &&)");
  }

  operator const T & () const { return _value; }
  operator T & () { return _value; }
};

template <class T>
class MovableMock {
  T _value;
public:
  bool dflt_ctor   = false;
  bool rval_ctor   = false;
  bool lval_ctor   = false;
  bool move_ctor   = false;
  bool moved       = false;
  bool copied      = false;
  bool cp_assigned = false;
  bool mv_assigned = false;

  MovableMock() : _value(0) {
    dflt_ctor = true;
    DASH_LOG_TRACE("MovableMock", "<", dash::internal::typestr(_value), ">",
                   "MovableMock()");
  }
// explicit MovableMock(const T & v) : _value(v) {
//   lval_ctor = true;
//   DASH_LOG_TRACE("MovableMock", "MovableMock(const T &)");
// }
  MovableMock(T && v) : _value(std::forward<T>(v)) {
    rval_ctor = true;
    DASH_LOG_TRACE("MovableMock", "<", dash::internal::typestr(_value), ">",
                   "MovableMock(T &&)");
  }
  MovableMock(const MovableMock & o) : _value(o._value) {
    copied = true;
    DASH_LOG_TRACE("MovableMock", "<", dash::internal::typestr(_value), ">",
                   "MovableMock(const other &)");
  }
  MovableMock(MovableMock && o) : _value(o._value) {
    move_ctor = true;
    moved     = true;
    DASH_LOG_TRACE("MovableMock", "<", dash::internal::typestr(_value), ">",
                   "MovableMock(other &&)");
  }
  MovableMock & operator=(const MovableMock & o) {
    cp_assigned = true;
    copied      = true;
    _value = o._value;
    DASH_LOG_TRACE("MovableMock", "<", dash::internal::typestr(_value), ">",
                   "operator =(const other &)");
    return *this;
  }
  MovableMock & operator=(MovableMock && o) {
    mv_assigned = true;
    moved       = true;
    _value = o._value;
    DASH_LOG_TRACE("MovableMock", "<", dash::internal::typestr(_value), ">",
                   "operator =(other &&)");
    return *this;
  }

  operator T () const { return _value; }
  operator T & () { return _value; }
};

template <class T>
class AcceptTest {
private:
  MovableMock<T> _captured;
public:
// explicit AcceptTest(const T & v) : _captured(v)
// { }
  AcceptTest(T && v) : _captured(std::forward<T>(v))
  { }
  T & value() { return static_cast<T &>(_captured); }
  const T & value() const { return static_cast<const T &>(_captured); }

  MovableMock<T> & captured() { return _captured; }
};

TEST_F(UtilTest, ReferenceCapture)
{
  DASH_TEST_LOCAL_ONLY();

  DASH_LOG_DEBUG("UtilTest.ReferenceCapture", "create with lvalue:");
  ValueMock<double> named(1.23);
  AcceptTest<ValueMock<double> &> acc_named(named);

  DASH_LOG_DEBUG("UtilTest.ReferenceCapture", "create with rvalue:");
  AcceptTest<ValueMock<double>> acc_moved(ValueMock<double>(1.23));

  EXPECT_EQ_U(1.23, acc_named.value());
  EXPECT_EQ_U(1.23, acc_moved.value());
}


