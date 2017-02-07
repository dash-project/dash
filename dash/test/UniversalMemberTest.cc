
#include "UniversalMemberTest.h"

#include <dash/util/UniversalMember.h>

#include <memory>


template <class T>
class ValueMock {
  T _value;
public:
  bool moved       = false;
  bool copied      = false;

  ValueMock() = delete;

  ~ValueMock() {
//  DASH_LOG_TRACE("ValueMock", "~ValueMock (", this, ")", _value);
  }

  explicit ValueMock(const T & v) : _value(v) {
    DASH_LOG_TRACE("ValueMock", "ValueMock(T)");
  }

  ValueMock(const ValueMock & o)             = delete;
  ValueMock & operator=(const ValueMock & o) = delete;

  ValueMock(ValueMock && o) : _value(std::forward<T>(o._value)) {
    moved = true;
    DASH_LOG_TRACE("ValueMock", "ValueMock(self_t &&)");
  }
  ValueMock & operator=(ValueMock && o) {
    _value = std::move(o._value);
    moved = true;
    DASH_LOG_TRACE("ValueMock", "operator=(self_t &&)");
    return *this;
  }

  operator       T & ()       { return _value; }
  operator const T & () const { return _value; }
};

template <class T>
class ImmovableMock {
  T _value;
public:
  bool moved       = false;
  bool copied      = false;

  ImmovableMock() = delete;

  explicit ImmovableMock(const T & v) : _value(v) {
    DASH_LOG_TRACE("ValueMock", "ValueMock(T)");
  }

  ImmovableMock(const ImmovableMock & o)             = delete;
  ImmovableMock & operator=(const ImmovableMock & o) = delete;
  ImmovableMock(ImmovableMock && o)                  = delete;
  ImmovableMock & operator=(ImmovableMock && o)      = delete;

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
        T & value()       { return _captured; }
  const T & value() const { return _captured; }

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

template <class T>
class UniversalTest {
  typedef typename std::remove_reference<T>::type ValueT;

  dash::UniversalMember<T> _value;
public:
  constexpr UniversalTest(T && value)
  : _value(std::forward<T>(value))
  { }

        T & value()       { return *_value.get(); }
  const T & value() const { return *_value.get(); }
};

template <class T>
class SharedPtrTest {
  typedef typename std::remove_reference<T>::type ValueT;

  std::shared_ptr<T> _value;
public:
  SharedPtrTest(SharedPtrTest<T> && other)      = default;
  SharedPtrTest(const SharedPtrTest<T> & other) = default;
  explicit SharedPtrTest(T && value)
  : _value(std::make_shared<T>(std::move(value))) {
    DASH_LOG_DEBUG("UniversalMemberTest.SharedPtrTest", "(T &&)");
  }
  explicit SharedPtrTest(T & value)
  : _value(&value, [](T *) { /* no deleter */ }) {
    DASH_LOG_DEBUG("UniversalMemberTest.SharedPtrTest", "(T &)");
  }

        T & value()       { return *_value.get(); }
  const T & value() const { return *_value.get(); }
};


template <class T>
SharedPtrTest<T>
make_shared_test(T & val) {
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "make_shared_test(T &):");
  return SharedPtrTest<T>(val);
}

template <
  class    T,
  typename ValueT =
             typename std::remove_const<
               typename std::remove_reference<T>::type
             >::type
>
SharedPtrTest<T>
make_shared_test(T && val) {
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "make_shared_test(T &&)");
  return SharedPtrTest<T>(std::move(val));
}


TEST_F(UniversalMemberTest, SelfTest)
{
  DASH_TEST_LOCAL_ONLY();

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- expl. constructor:");
  ValueMock<double> vmock_a(1.23);
  EXPECT_FALSE_U(vmock_a.copied);

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- using rvalue ref:");
  ValueMock<double> vmock_c(ValueMock<double>(2.34));
  EXPECT_FALSE_U(vmock_c.copied);
}

TEST_F(UniversalMemberTest, OwnerCtor)
{
  typedef std::string value_t;

  DASH_TEST_LOCAL_ONLY();
  ValueMock<value_t>     movable_a("movable_a");
  ValueMock<value_t>     movable_b("movable_b");
  ImmovableMock<value_t> immovable("immovable");

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from mov. lvalue:");
  SharedPtrTest<ValueMock<value_t>> shared_movable(movable_a);
  EXPECT_EQ_U("movable_a", static_cast<value_t>(movable_a));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from mov. lvalue:");
  SharedPtrTest<ImmovableMock<value_t>> shared_immovable(immovable);
  EXPECT_EQ_U("immovable", static_cast<value_t>(immovable));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from rvalue:");
  SharedPtrTest<ValueMock<value_t>> shared_moved(
                                      ValueMock<value_t>("rvalue_a"));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make mov. lvalue:");
  auto make_movable    = make_shared_test(movable_b);

  EXPECT_EQ_U("movable_b", static_cast<value_t>(movable_b));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- change ref'ed value:");
  make_movable.value() = ValueMock<value_t>("changed referenced value");

  EXPECT_EQ_U("changed referenced value", static_cast<value_t>(movable_b));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make from rvalue:");
  auto make_moved     = make_shared_test(ValueMock<value_t>("rvalue_b"));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");

#if 0

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from lvalue ref:");
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
#endif
}


