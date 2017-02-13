
#include "UniversalMemberTest.h"

#include <dash/util/UniversalMember.h>

#include <memory>

using namespace dash;


template <class T>
class MovableType {
  T _value;
public:
  MovableType() = delete;

  explicit MovableType(const T & v) : _value(v) {
    DASH_LOG_TRACE("MovableType", "MovableType(T)");
  }

  explicit MovableType(const MovableType & o)    = delete;
  MovableType & operator=(const MovableType & o) = delete;

  explicit MovableType(MovableType && o)
  : _value(std::forward<T>(o._value)) {
    DASH_LOG_TRACE("MovableType", "MovableType(self_t &&)");
  }
  MovableType & operator=(MovableType && o) {
    _value = std::move(o._value);
    DASH_LOG_TRACE("MovableType", "operator=(self_t &&)");
    return *this;
  }

  MovableType & operator=(const T & value) {
    _value = value;
    return *this;
  }

  operator       T & ()       { return _value; }
  operator const T & () const { return _value; }
};

template <class T>
class ImmovableType {
  T _value;
public:
  ImmovableType() = delete;

  explicit ImmovableType(const T & v) : _value(v) {
    DASH_LOG_TRACE("ImmovableType", "ImmovableType(T)");
  }

  explicit ImmovableType(const ImmovableType & o)    = delete;
  explicit ImmovableType(ImmovableType && o)         = delete;
  ImmovableType & operator=(const ImmovableType & o) = delete;
  ImmovableType & operator=(ImmovableType && o)      = delete;

  ImmovableType & operator=(const T & value) {
    _value = value;
    return *this;
  }

  operator       T & ()       { return _value; }
  operator const T & () const { return _value; }
};

template <
  class T,
  class ValueT = typename std::remove_const<
                   typename std::remove_reference<T>::type
                 >::type
>
UniversalMember<ValueT>
make_universal_member(T && val) {
  DASH_LOG_DEBUG("UniversalMemberTest", "make_universal_member(T &&)");
  return UniversalMember<ValueT>(std::forward<T>(val));
}



/* NOTE:
 *
 * There are just a few assertions needed as the prevention of temporary
 * copies is already checked at compile time via deleted copy- and move
 * in MovableType / ImmovableType.
 *
 */

TEST_F(UniversalMemberTest, TestHelpers)
{
  DASH_TEST_LOCAL_ONLY();

  MovableType<double>   movable_a(1.23);
  EXPECT_EQ_U(1.23, static_cast<double>(movable_a));

  MovableType<double>   movable_b(MovableType<double>(2.34));
  EXPECT_EQ_U(2.34, static_cast<double>(movable_b));

  ImmovableType<double> immovable(3.45);
  EXPECT_EQ_U(3.45, static_cast<double>(immovable));
}

TEST_F(UniversalMemberTest, InitFromLValAndRVal)
{
  typedef std::string value_t;

  DASH_TEST_LOCAL_ONLY();
  MovableType<value_t>   movable_a("movable_a");
  MovableType<value_t>   movable_b("movable_b");
  ImmovableType<value_t> immovable("immovable");

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from mov. lvalue:");
  UniversalMember<MovableType<value_t>> shared_movable(movable_a);
  EXPECT_EQ_U("movable_a", static_cast<value_t>(movable_a));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from mov. lvalue:");
  UniversalMember<ImmovableType<value_t>> shared_immovable(immovable);
  EXPECT_EQ_U("immovable", static_cast<value_t>(immovable));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- from rvalue:");
  UniversalMember<MovableType<value_t>> shared_moved(
                                      MovableType<value_t>("rvalue_a"));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make mov. lvalue:");
  auto make_movable = make_universal_member(movable_b);

  EXPECT_EQ_U("movable_b", static_cast<value_t>(movable_b));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- change ref'ed value:");
  make_movable = MovableType<value_t>("changed referenced value");

  EXPECT_EQ_U("changed referenced value", static_cast<value_t>(movable_b));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "-- make from rvalue:");
  auto make_moved = make_universal_member(MovableType<value_t>("rvalue_b"));

  DASH_LOG_DEBUG("UniversalMemberTest.OwnerCtor", "------------------");
}

template <class T>
class UniversalBase {
  dash::UniversalMember<T> _value;
public:
  constexpr explicit UniversalBase(T && value)
  : _value(std::forward<T>(value))
  { }

  constexpr explicit UniversalBase(const T & value)
  : _value(value)
  { }

        T & value()       { return _value; }
  const T & value() const { return _value; }
};

template <class T>
class UniversalOwner : public UniversalBase<T> {
  typedef UniversalOwner<T> self_t;
  typedef UniversalBase<T>  base_t;
public:
  constexpr explicit UniversalOwner(T && value)
  : base_t(std::forward<T>(value))
  { }

  constexpr explicit UniversalOwner(const T & value)
  : base_t(value)
  { }
};

template <
  class T,
  class ValueT = typename std::remove_const<
                   typename std::remove_reference<T>::type
                 >::type
>
UniversalOwner<ValueT>
make_universal_owner(T && val) {
  DASH_LOG_DEBUG("UniversalMemberTest", "make_universal_owner(T &&)");
  return UniversalOwner<ValueT>(std::forward<T>(val));
}

TEST_F(UniversalMemberTest, WrappedMember)
{
  typedef std::string value_t;

  ImmovableType<value_t> immovable("immovable");
  MovableType<value_t>   movable("movable");
  
  // Test passing to owner c'tor:
  {
    auto lref_owner = make_universal_owner(immovable);
    auto rval_owner = make_universal_owner(MovableType<value_t>("moved"));
    EXPECT_EQ_U("immovable", static_cast<value_t &>(lref_owner.value()));
    EXPECT_EQ_U("moved",     static_cast<value_t &>(rval_owner.value()));
  }

  UniversalOwner<ImmovableType<value_t>> lref_owner(immovable);
  UniversalOwner<MovableType<value_t>>   rval_owner(
                                           MovableType<value_t>("moved"));

  EXPECT_EQ_U("movable",   static_cast<value_t>(movable));
  EXPECT_EQ_U("immovable", static_cast<value_t>(immovable));
  EXPECT_EQ_U("immovable", static_cast<value_t &>(lref_owner.value()));
  EXPECT_EQ_U("moved",     static_cast<value_t &>(rval_owner.value()));

  movable            = "movable xx";
  lref_owner.value() = "immovable xx";
  rval_owner.value() = "moved xx";

  // Check if value of referenced variable has changed
  EXPECT_EQ_U("movable xx",   static_cast<value_t>(movable));
  EXPECT_EQ_U("immovable xx", static_cast<value_t>(immovable));
  EXPECT_EQ_U("immovable xx", static_cast<value_t>(lref_owner.value()));
  EXPECT_EQ_U("moved xx",     static_cast<value_t &>(rval_owner.value()));
}


