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
 * \see local_unit_t
 */
template <unit_scope IdScope>
struct unit_id {
public:

  template<unit_scope U>
  friend std::ostream & operator<<(std::ostream& os, unit_id<U> id);

  template <unit_scope U>
  friend constexpr unit_id<U> operator+(unit_id<U> lhs, unit_id<U> rhs);

  template <unit_scope U>
  friend constexpr unit_id<U> operator-(unit_id<U> lhs, unit_id<U> rhs);

  template <unit_scope U>
  friend constexpr bool operator==(unit_id<U> lhs, unit_id<U> rhs);

  template <unit_scope U>
  friend constexpr bool operator<(unit_id<U> lhs, unit_id<U> rhs);

  template <unit_scope U>
  friend constexpr bool operator>(unit_id<U> lhs, unit_id<U> rhs);


  /** default initialization to zero */
  constexpr explicit unit_id() noexcept : id(0) { }

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
  : id(id) { }


  /**
   * Initialization using a \c unit_id
   *
   * Use it as:
   * \code
   * dash::global_unit_t unit = dash::myid();
   * \endcode
   */
  constexpr unit_id(const unit_id<IdScope>& uid)
  : id(uid.id) { }

  /**
   * Explicit Initialization from another \c unit_id type
   *
   * Allows to do:
   *
   * \code
   * dash::global_unit_t g_unit{0};
   * dash::local_unit_t  l_unit(g_unit);
   * \endcode
   *
   * This explicit conversion is useful for example when
   * working on the global team \c dash::Team::All().
   *
   */
  template<unit_scope Scope>
  constexpr explicit unit_id<IdScope>(const unit_id<Scope> & uid) noexcept
  : id(uid) { }

  /**
   * \todo Not constexpr since C++11 does not allow for multi-statement
   *       constexpr functions (C++14 does).
   */

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
   */
  const unit_id<IdScope> operator=(unit_id<IdScope> uid) noexcept {
    this->id = uid.id;
    return *this;
  }
  
  /**
   * Mixed-type assignment is explicitely prohibited to prevent
   * accidental mistakes.
   */
  template<unit_scope Scope>
  const unit_id<IdScope> operator=(const unit_id<Scope> & id) = delete;

  /**
   * Assignment from a \c dart_unit_t.
   *
   * Use this as:
   * \code
   * dash::global_unit_t g_unit;
   * g_unit = 2;
   * \endcode
   */
  const unit_id operator=(dart_unit_t id) noexcept {
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
    unit_id<IdScope> tmp(*this);
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
    unit_id<IdScope> tmp(*this);
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
   */
  constexpr operator dart_unit_t() const noexcept { return id; }

  /**
   * Address-of operator that returns the address of the unit ID
   *
   * Use this as:
   * \code
   * dash::global_unit_t unit{1};
   * dart_myid(&unit);
   * \endcode
   *
   */
  dart_unit_t* operator&() {
    return &this->id;
  }

  /**
   * Address-of operator that returns the address of the unit ID
   *
   * Use this as:
   * \code
   * dash::global_unit_t unit{1};
   * dart_myid(&unit);
   * \endcode
   *
   */
  dart_unit_t const* operator&() const {
    return &this->id;
  }

private:
  dart_unit_t id;
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
template <unit_scope IdScope>
constexpr unit_id<IdScope> operator+(unit_id<IdScope> lhs, unit_id<IdScope> rhs) {
  return unit_id<IdScope>(lhs.id + rhs.id);
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
template <unit_scope IdScope>
constexpr unit_id<IdScope> operator-(unit_id<IdScope> lhs, unit_id<IdScope> rhs) {
  return unit_id<IdScope>(lhs.id - rhs.id);
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
template <unit_scope IdScope>
constexpr bool operator==(unit_id<IdScope> lhs, unit_id<IdScope> rhs) {
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
template <unit_scope IdScope>
constexpr bool operator!=(unit_id<IdScope> lhs, unit_id<IdScope> rhs) {
  return !(lhs == rhs);
}


/**
 * Prevent mistakes from using mixed-type smaller-than operator.
 */
template <unit_scope LHSScope, unit_scope RHSScope>
constexpr bool operator<(unit_id<LHSScope> lhs, unit_id<RHSScope> rhs) = delete;

/**
 * Prevent mistakes from using mixed-type larger-than operator.
 */
template <unit_scope LHSScope, unit_scope RHSScope>
constexpr bool operator>(unit_id<LHSScope> lhs, unit_id<RHSScope> rhs) = delete;

/**
 * Prevent mistakes from using mixed-type equality operator.
 */
template <unit_scope LHSScope, unit_scope RHSScope>
constexpr bool operator==(unit_id<LHSScope> lhs, unit_id<RHSScope> rhs) = delete;

/**
 * Less-than operator for same-type unit_id objects.
 */
template <unit_scope IdScope>
constexpr bool operator<(unit_id<IdScope> lhs, unit_id<IdScope> rhs) {
  return lhs.id < rhs.id;
}

/**
 * Larger-than operator for same-type unit_id objects.
 */
template <unit_scope IdScope>
constexpr bool operator>(unit_id<IdScope> lhs, unit_id<IdScope> rhs) {
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
template<unit_scope IdScope>
std::ostream & operator<<(std::ostream& os, unit_id<IdScope> id)
{
  return os<<id.id;
}

} // namespace dash

#endif
