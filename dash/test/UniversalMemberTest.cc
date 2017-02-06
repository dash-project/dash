
#include "UniversalMemberTest.h"

#include <dash/util/UniversalMember.h>

template <class T>
class ValueMock {
  T _value;
public:
  bool moved       = false;
  bool copied      = false;

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
    _value = std::move(o._value);
    moved = true;
    DASH_LOG_TRACE("ValueMock", "operator=(self_t &&)");
  }

  operator       T & ()       { return _value; }
  operator const T & () const { return _value; }
};

template <class T>
class AcceptTest {
private:
  dash::UniversalMember<T> _captured;
public:
  AcceptTest(T && v) : _captured(std::forward<T>(v))
  { }
        T & value()       { return static_cast<T &>(_captured); }
  const T & value() const { return static_cast<const T &>(_captured); }

  const dash::UniversalMember<T> & captured() const { return _captured; }
};

template <class T>
AcceptTest<T &>
make_accept_test(T & val) {
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "make_accept_test(T &):");
  return AcceptTest<T &>(val);
}

template <class T>
AcceptTest<T>
make_accept_test(T && val) {
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "make_accept_test(T &&)");
  return AcceptTest<T>(std::move(val));
}


TEST_F(UniversalMemberTest, SelfTest)
{
  DASH_TEST_LOCAL_ONLY();

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- expl. constructor:");
  ValueMock<double> vmock_a(1.23);
  EXPECT_FALSE_U(vmock_a.copied);

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- using named ref:");
  ValueMock<double> vmock_b(vmock_a);
  EXPECT_TRUE_U(vmock_b.copied);

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- using rvalue ref:");
  ValueMock<double> vmock_c(ValueMock<double>(2.34));
  EXPECT_FALSE_U(vmock_c.copied);
}

TEST_F(UniversalMemberTest, OwnerCtor)
{
  DASH_TEST_LOCAL_ONLY();

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from lvalue ref:");
  ValueMock<double> named(1.23);
  AcceptTest<ValueMock<double> &> acc_named(named);

  EXPECT_FALSE_U(acc_named.value().moved);
  EXPECT_FALSE_U(acc_named.value().copied);

  // Must produce compiler error:
  //   AcceptTest<ValueMock<double>> acc_named_value(named);
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from rvalue:");
  AcceptTest<ValueMock<double>>   acc_moved(ValueMock<double>(1.23));

  EXPECT_TRUE_U(acc_moved.value().moved);
  EXPECT_FALSE_U(acc_moved.value().copied);

  EXPECT_EQ_U(1.23, acc_named.value());
  EXPECT_EQ_U(1.23, acc_moved.value());


  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make from lvalue ref:");
  auto make_named = make_accept_test(named);

  EXPECT_FALSE_U(make_named.value().moved);
  EXPECT_FALSE_U(make_named.value().copied);

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make from rvalue:");
  auto make_moved = make_accept_test(ValueMock<double>(1.23));

  EXPECT_TRUE_U(make_moved.value().moved);
  EXPECT_FALSE_U(make_moved.value().copied);

  EXPECT_EQ_U(1.23, make_named.value());
  EXPECT_EQ_U(1.23, make_moved.value());

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- change ref'd value:");
  make_named.value() = ValueMock<double>(2.34);

  EXPECT_FALSE_U(make_named.value().copied);
  EXPECT_TRUE_U(make_named.value().moved);

  EXPECT_EQ_U(2.34, make_named.value());
  EXPECT_EQ_U(2.34, acc_named.value());
}


