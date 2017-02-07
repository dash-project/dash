
#include "UniversalMemberTest.h"

#include <dash/util/UniversalMember.h>

#include <memory>

using namespace dash;


template <class T>
class MovableMock {
  T _value;
public:
  bool moved       = false;
  bool copied      = false;

  MovableMock() = delete;

  explicit MovableMock(const T & v) : _value(v) {
    DASH_LOG_TRACE("MovableMock", "MovableMock(T)");
  }

  MovableMock(const MovableMock & o)             = delete;
  MovableMock & operator=(const MovableMock & o) = delete;

  MovableMock(MovableMock && o) : _value(std::forward<T>(o._value)) {
    moved = true;
    DASH_LOG_TRACE("MovableMock", "MovableMock(self_t &&)");
  }
  MovableMock & operator=(MovableMock && o) {
    _value = std::move(o._value);
    moved = true;
    DASH_LOG_TRACE("MovableMock", "operator=(self_t &&)");
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
    DASH_LOG_TRACE("MovableMock", "MovableMock(T)");
  }

  ImmovableMock(const ImmovableMock & o)             = delete;
  ImmovableMock & operator=(const ImmovableMock & o) = delete;
  ImmovableMock(ImmovableMock && o)                  = delete;
  ImmovableMock & operator=(ImmovableMock && o)      = delete;

  operator       T & ()       { return _value; }
  operator const T & () const { return _value; }
};

template <class T>
class UniversalTest {
  dash::UniversalMember<T> _value;
public:
  constexpr UniversalTest(T && value)
  : _value(std::forward<T>(value))
  { }

        T & value()       { return _value; }
  const T & value() const { return _value; }
};


template <class T>
UniversalMember<T>
make_shared_test(T & val) {
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "make_shared_test(T &):");
  return UniversalMember<T>(val);
}

template <class T>
UniversalMember<T>
make_shared_test(T && val) {
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "make_shared_test(T &&)");
  return UniversalMember<T>(std::move(val));
}


TEST_F(UniversalMemberTest, SelfTest)
{
  DASH_TEST_LOCAL_ONLY();

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- expl. constructor:");
  MovableMock<double> vmock_a(1.23);
  EXPECT_FALSE_U(vmock_a.copied);

  DASH_LOG_DEBUG("UniversalMemberTest.SelfTest", "-- using rvalue ref:");
  MovableMock<double> vmock_c(MovableMock<double>(2.34));
  EXPECT_FALSE_U(vmock_c.copied);
}

TEST_F(UniversalMemberTest, OwnerCtor)
{
  typedef std::string value_t;

  DASH_TEST_LOCAL_ONLY();
  MovableMock<value_t>   movable_a("movable_a");
  MovableMock<value_t>   movable_b("movable_b");
  ImmovableMock<value_t> immovable("immovable");

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from mov. lvalue:");
  UniversalMember<MovableMock<value_t>> shared_movable(movable_a);
  EXPECT_EQ_U("movable_a", static_cast<value_t>(movable_a));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from mov. lvalue:");
  UniversalMember<ImmovableMock<value_t>> shared_immovable(immovable);
  EXPECT_EQ_U("immovable", static_cast<value_t>(immovable));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from rvalue:");
  UniversalMember<MovableMock<value_t>> shared_moved(
                                      MovableMock<value_t>("rvalue_a"));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make mov. lvalue:");
  auto make_movable = make_shared_test(movable_b);

  EXPECT_EQ_U("movable_b", static_cast<value_t>(movable_b));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- change ref'ed value:");
  make_movable = MovableMock<value_t>("changed referenced value");

  EXPECT_EQ_U("changed referenced value", static_cast<value_t>(movable_b));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make from rvalue:");
  auto make_moved = make_shared_test(MovableMock<value_t>("rvalue_b"));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
}

