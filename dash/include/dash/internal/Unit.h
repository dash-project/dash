#ifndef DASH__UNIT_H_
#define DASH__UNIT_H_

/**
 * Implementation of typed unit IDs.
 */


#include <dash/dart/if/dart_types.h>

namespace dash {

enum unit_scope {
  local_unit,
  global_unit
};

/**
 * Typed encapsulation of a \ref dart_unit_t used to differentiate between IDs
 * in different unit scopes, i.e., global and team-local unit IDs.
 *
 * \see global_unit_t
 * \see team_unit_t
 */
template <unit_scope IdScope, typename DARTType>
struct unit_id : public DARTType {

  typedef unit_id<IdScope, DARTType> self_t;

public:

  template<unit_scope U, typename D>
  friend std::ostream & operator<<(std::ostream& os, unit_id<U, D> id);

  template <unit_scope U, typename D>
  friend constexpr unit_id<U, D> operator+(unit_id<U, D> lhs, unit_id<U, D> rhs);

  template <unit_scope U, typename D>
  friend constexpr unit_id<U, D> operator-(unit_id<U, D> lhs, unit_id<U, D> rhs);

  template <unit_scope U, typename D>
  friend constexpr bool operator==(unit_id<U, D> lhs, unit_id<U, D> rhs);

  template <unit_scope U, typename D>
  friend constexpr bool operator<(unit_id<U, D> lhs, unit_id<U, D> rhs);

  template <unit_scope U, typename D>
  friend constexpr bool operator>(unit_id<U, D> lhs, unit_id<U, D> rhs);


  /** default initialization to zero */
  constexpr explicit unit_id() noexcept
  : DARTType(DART_UNDEFINED_UNIT_ID) { }

  /**
   * Initialization using a \c dart_unit_t
   * 
   * Note that declaration explicit to prevent implicit conversion from
   * existing \c dart_unit_t variables.
   * However, it prevents the following from working:
   *
   * \code
   * dash::global_unit_t unit = 0;
   * \endcode
   *
   * Instead, use:
   *
   * \code
   * dash::global_unit_t unit {0};
   * \endcode
   */
  constexpr explicit unit_id(dart_unit_t id) noexcept
  : DARTType(id) { }


  /**
   * Initialization using a \c unit_id
   *
   * Use it as:
   * \code
   * dash::global_unit_t unit = dash::myid();
   * \endcode
   */
  constexpr unit_id(const self_t & uid) = default;

  /**
   * Initialization from corresponding DART unit type in same scope.
   *
   * Allows to do:
   *
   * \code
   * dart_global_unit_t  dart_guid { 34 }
   * dash::global_unit_t g_unit(dart_guid);
   * \endcode
   *
   */
  constexpr unit_id(DARTType dart_scope_uid) noexcept
  : DARTType(dart_scope_uid) { }

  /**
   * Explicit Initialization from another \c unit_id type
   *
   * Allows to do:
   *
   * \code
   * dash::global_unit_t g_unit{0};
   * dash::team_unit_t   l_unit(g_unit);
   * \endcode
   *
   * This explicit conversion is useful for example when
   * working on the global team \c dash::Team::All().
   *
   */
  template<unit_scope Scope, typename DT>
  constexpr explicit unit_id<IdScope, DARTType>(
    const unit_id<Scope, DT> & uid) noexcept
  : DARTType(uid.id) { }

  /**
   * Type-safe assignment operator that let's you do the following:
   *
   * \code
   * dash::global_unit_t g_unit{0};
   * dash::global_unit_t g_unit2{2};
   *
   * g_unit = 3;
   * g_unit2 = g_unit;
   * \endcode
   *
   * \todo Not constexpr since C++11 does not allow for multi-statement
   *       constexpr functions (C++14 does).
   *
   * \todo
   * fuchsto@devreal: Why not returning self_t & as required for
   *                  operator= semantics?
   *                  Otherwise, fundamental semantics would be broken.
   *                  Consider:
   *                  \code
   *                    dart_unit_t   uid_a { 4 };
   *                    dart_unit_t   uid_b { 5 };
   *                    global_unit_t guid;
   *                    (guid = uid_a) = uid_b;
   *                    -> guid: { id: 4 }
   *                  \endcode
   */
  const unit_id<IdScope, DARTType> operator=(
    unit_id<IdScope, DARTType> uid) noexcept {
    this->id = uid.id;
    return *this;
  }
  
  /**
   * Mixed-type assignment is explicitely prohibited to prevent
   * accidental mistakes.
   *
   * \todo
   * fuchsto@devreal: Why not returning self_t & as required for
   *                  operator= semantics?
   *                  Otherwise, fundamental semantics would be broken.
   *                  Consider:
   *                  \code
   *                    dart_unit_t   uid_a { 4 };
   *                    dart_unit_t   uid_b { 5 };
   *                    global_unit_t guid;
   *                    (guid = uid_a) = uid_b;
   *                    -> guid: { id: 4 }
   *                  \endcode
   */
  template<unit_scope Scope, typename D>
  const unit_id<IdScope, D> operator=(const unit_id<Scope, D> & id) = delete;

  /**
   * Assignment from a \c dart_unit_t.
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit;
   * g_unit = 2;
   * \endcode
   *
   * \todo
   * fuchsto@devreal: Why not returning self_t & as required for
   *                  operator= semantics?
   *                  Otherwise, fundamental semantics would be broken.
   *                  Consider:
   *                  \code
   *                    dart_unit_t   uid_a { 4 };
   *                    dart_unit_t   uid_b { 5 };
   *                    global_unit_t guid;
   *                    (guid = uid_a) = uid_b;
   *                    -> guid: { id: 4 }
   *                  \endcode
   */
  const self_t operator=(dart_unit_t id) noexcept {
    this->id = id;
    return *this;
  }


  /**
   * In-place addition operator
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit;
   * g_unit += 2;
   * \endcode
   */
  template<
    typename T,
    typename std::enable_if<
               std::is_integral<T>::value, int >::type = 0>
  const unit_id operator+=(T id) noexcept {
    this->id += id;
    return *this;
  }
  
  /**
   * In-place difference operator
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit{3};
   * g_unit -= 2;
   * \endcode
   */
  template<
    typename T,
    typename std::enable_if<
               std::is_integral<T>::value, int >::type = 0>
  const unit_id operator-=(T id) noexcept {
    this->id -= id;
    return *this;
  }

  /**
   * In-place multiplication operator
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit{1};
   * g_unit *= 2;
   * \endcode
   */
  template<
    typename T,
    typename std::enable_if<
               std::is_integral<T>::value, int >::type = 0>
  const unit_id operator*=(T id) noexcept {
    this->id *= id;
    return *this;
  }

  /**
   * In-place division operator
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit = dash::myid();
   * g_unit /= 2;
   * \endcode
   */
  template<
    typename T,
    typename std::enable_if<
               std::is_integral<T>::value, int >::type = 0>
  const unit_id operator/=(T id) noexcept {
    this->id /= id;
    return *this;
  }

  /**
   * In-place modulo operator
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit = dash::myid();
   * g_unit %= 2;
   * \endcode
   */
  template<
    typename T,
    typename std::enable_if<
               std::is_integral<T>::value, int >::type = 0>
  const unit_id operator%=(T id) noexcept {
    this->id %= id;
    return *this;
  }

  /**
   * Prefix increment operator
   *
   * Use this as:
   * \code
   * for (dash::global_unit_t unit{0}; unit < dash::size(); ++unit)
   * {
   *   // work with unit
   * }
   * \endcode
   */
  const unit_id operator++() noexcept {
    this->id++;
    return *this;
  }

  /**
   * Prefix decrement operator
   *
   * Use this as:
   * \code
   * for (dash::global_unit_t unit{dash::size()}; unit > 0; --unit)
   * {
   *   // work with unit
   * }
   * \endcode
   */
  const unit_id operator--() noexcept {
    this->id--;
    return *this;
  }

  /**
   * Postfix increment operator
   *
   * Use this as:
   * \code
   * for (dash::global_unit_t unit{0}; unit > dash::size(); unit++)
   * {
   *   // work with unit
   * }
   * \endcode
   */
  const unit_id operator++(int) noexcept {
    unit_id<IdScope, DARTType> tmp(*this);
    this->id++;
    return tmp;
  }

  /**
   * Postfix decrement operator
   *
   * Use this as:
   * \code
   * for (dash::global_unit_t unit{dash::size()}; unit > 0; unit--)
   * {
   *   // work with unit
   * }
   * \endcode
   */
  const unit_id operator--(int) noexcept {
    unit_id<IdScope, DARTType> tmp(*this);
    this->id--;
    return tmp;
  }

  /**
   * Cast to \c dart_unit_t
   *
   * Use this as:
   * \code
   * dart_gptr_t gptr;
   * dash::global_unit_t unit{1};
   * dart_gptr_setunit(&gptr, unit);
   * \endcode
   *
   * This can also be used for comparison with integral type constants:
   * \code
   * dash::global_unit_t unit = dash::myid();
   * if (unit == 0)
   * {
   *   // do something on root unit
   * }
   * \endcode
   *
   * \todo
   * fuchsto@devreal: Sure that this shouldn't be marked explicit?
   *                  In effect, this is an implicit conversion to int.
   *                  Could be harmless, however, as unit_id(int) is marked
   *                  explicit.
   */
  constexpr operator dart_unit_t() const noexcept { return this->id; }

};

/**
 * Addition operator for two \ref unit_id objects.
 *
 * Use this as:
 * \code
 * dash::global_unit_t g_unit_x, g_unit_1{1}, g_unit_2{2};
 * g_unit_x = g_unit_1 + g_unit_2;
 * \endcode
 */
template <unit_scope IdScope, typename DARTType>
constexpr unit_id<IdScope, DARTType> operator+(
    unit_id<IdScope, DARTType> lhs,
    unit_id<IdScope, DARTType> rhs) {
  return unit_id<IdScope, DARTType>(lhs.id + rhs.id);
}

/**
 * Difference operator for two \ref unit_id objects.
 *
 * Use this as:
 * \code
 * dash::global_unit_t g_unit_x, g_unit_1{1}, g_unit_2{2};
 * g_unit_x = g_unit_1 - g_unit_2;
 * \endcode
 */
template <unit_scope IdScope, typename DARTType>
constexpr unit_id<IdScope, DARTType> operator-(
    unit_id<IdScope, DARTType> lhs,
    unit_id<IdScope, DARTType> rhs) {
  return unit_id<IdScope, DARTType>(lhs.id - rhs.id);
}

/**
 * Equality operator for two \ref unit_id objects.
 *
 * Use this as:
 * \code
 * dash::global_unit_t g_unit{0};
 * if (g_unit == dash::myid())
 * {
 *   // do something on root unit
 * }
 * \endcode
 */
template <unit_scope IdScope, typename DARTType>
constexpr bool operator==(
    unit_id<IdScope, DARTType> lhs,
    unit_id<IdScope, DARTType> rhs) {
  return lhs.id == rhs.id;
}


/**
 * Inequality operator for two \ref unit_id objects.
 *
 * Use it as:
 * \code
 * dash::global_unit_t g_unit{0};
 * if (g_unit != dash::myid())
 * {
 *   // do something on non-root unit
 * }
 * \endcode
 */
template <unit_scope IdScope, typename DARTType>
constexpr bool operator!=(
    unit_id<IdScope, DARTType> lhs,
    unit_id<IdScope, DARTType> rhs) {
  return !(lhs == rhs);
}


/**
 * Prevent mistakes from using mixed-type smaller-than operator.
 */
template <
  unit_scope LHSScope,
  typename   LHSDARTType,
  unit_scope RHSScope,
  typename   RHSDARTType>
constexpr bool operator<(
    unit_id<LHSScope, LHSDARTType> lhs,
    unit_id<RHSScope, RHSDARTType> rhs) = delete;

/**
 * Prevent mistakes from using mixed-type larger-than operator.
 */
template <
  unit_scope LHSScope,
  typename   LHSDARTType,
  unit_scope RHSScope,
  typename   RHSDARTType>
constexpr bool operator>(
    unit_id<LHSScope, LHSDARTType> lhs,
    unit_id<RHSScope, RHSDARTType> rhs) = delete;

/**
 * Prevent mistakes from using mixed-type equality operator.
 */
template <
  unit_scope LHSScope,
  typename LHSDARTType,
  unit_scope RHSScope,
  typename RHSDARTType>
constexpr bool operator==(
    unit_id<LHSScope, LHSDARTType> lhs,
    unit_id<RHSScope, RHSDARTType> rhs) = delete;

/**
 * Less-than operator for same-type unit_id objects.
 */
template <
  unit_scope IdScope,
  typename DARTType>
constexpr bool operator<(
    unit_id<IdScope, DARTType> lhs,
    unit_id<IdScope, DARTType> rhs) {
  return lhs.id < rhs.id;
}

/**
 * Larger-than operator for same-type unit_id objects.
 */
template <unit_scope IdScope, typename DARTType>
constexpr bool operator>(
    unit_id<IdScope, DARTType> lhs,
    unit_id<IdScope, DARTType> rhs) {
  return lhs.id > rhs.id;
}

/**
 * Stream operator used to print unit_id.
 *
 * Use it as:
 * \code
 * dash::global_unit_t unit = dash::myid();
 * std::cout << "My ID: " << unit << std::endl;
 * \endcode
 */
template<unit_scope IdScope, typename DARTType>
std::ostream & operator<<(
    std::ostream &             os,
    unit_id<IdScope, DARTType> id)
{
  return os << id.id;
}

} // namespace dash

#endif
