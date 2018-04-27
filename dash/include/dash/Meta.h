#ifndef DASH__META_H__INCLUDED
#define DASH__META_H__INCLUDED

#include <dash/Types.h>
#include <dash/meta/TypeInfo.h>
#include <dash/util/ArrayExpr.h>

#include <type_traits>


#ifndef DOXYGEN

#define DASH__META__DEFINE_TRAIT__HAS_TYPE(DepType) \
  template<typename T> \
  struct has_type_##DepType { \
  private: \
    typedef typename std::decay<T>::type DT; \
    typedef char                      yes; \
    typedef struct { char array[2]; } no; \
    template<typename C> static yes test(typename C:: DepType *); \
    template<typename C> static no  test(...); \
  public: \
    static constexpr bool value = sizeof(test<DT>(0)) == sizeof(yes); \
  };

#endif // DOXYGEN

namespace dash {


template<class...> struct conjunction : std::true_type { };

template<class Cond0> struct conjunction<Cond0> : Cond0 { };

template<class Cond0, class... CondN>
struct conjunction<Cond0, CondN...> 
: std::conditional<
    bool(Cond0::value),
    conjunction<CondN...>,
    Cond0 >
{ };



/**
 * Definition of type trait \c dash::has_type_iterator<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c iterator.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(iterator)
/**
 * Definition of type trait \c dash::has_type_const_iterator<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c const_iterator.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(const_iterator)
/**
 * Definition of type trait \c dash::has_type_reference<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c reference.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(reference)
/**
 * Definition of type trait \c dash::has_type_const_reference<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c const_reference.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(const_reference)
/**
 * Definition of type trait \c dash::has_type_value_type<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c value_type.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(value_type)

/**
 * Definition of type trait \c dash::detail::has_type_pattern_type<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c pattern_type.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(pattern_type);

/**
 * Definition of type trait \c dash::detail::has_type_const_type<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c const_type.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(const_type);

/**
 * Definition of type trait \c dash::detail::has_type_const_type<T>
 * with static member \c value indicating whether type \c T provides
 * dependent type \c nonconst_type.
 */
DASH__META__DEFINE_TRAIT__HAS_TYPE(nonconst_type);

} // namespace dash

#include <type_traits>
#include <functional>

#ifndef DOXYGEN

namespace dash {

// ==========================================================================
// dash::const_value_cast<T &>::type -> const T &
// dash::const_value_cast<T *>::type -> const T *
// dash::const_value_cast<T  >::type -> const T
// --------------------------------------------------------------------------

template <typename T>
struct const_value_cast;

template <typename T>
struct const_value_cast<T *> {
  typedef const T * type;
};

template <typename T>
struct const_value_cast<T &> {
  typedef const T & type;
};

template <typename T>
struct const_value_cast {
  typedef typename
    std::conditional<
      dash::has_type_const_type<T>::value,
      typename T::const_type,
      const T
    >::type
  type;
};


// ==========================================================================
// dash::nonconst_value_cast<const T &>::type -> T &
// dash::nonconst_value_cast<const T *>::type -> T *
// dash::nonconst_value_cast<const T  >::type -> T
// --------------------------------------------------------------------------

template <typename T>
struct nonconst_value_cast;

template <typename T>
struct nonconst_value_cast<const T *> {
  typedef T * type;
};

template <typename T>
struct nonconst_value_cast<const T &> {
  typedef T & type;
};

template <typename T>
struct nonconst_value_cast {
  typedef typename
    std::conditional<
      dash::has_type_nonconst_type<T>::value,
      typename T::nonconst_type,
      typename std::remove_const<T>::type
    >::type
  type;
};


// ==========================================================================
// dash::array_value_cast<T>(std::array<U, N>)::type -> std::array<T, N>
// --------------------------------------------------------------------------

namespace detail {

template<typename T, typename U, std::size_t I, std::size_t... Is>
constexpr auto array_value_cast_indexed(
  const std::array<U, I> &a, dash::ce::index_sequence<Is...>)
  -> std::array<T, I> {
  return {{ static_cast<T>(std::get<Is>(a))... }};
}

} // namespace detail

template<typename T, typename U, std::size_t N>
constexpr auto array_value_cast(
  const std::array<U, N> & a) -> std::array<T, N> {
  // tag dispatch to helper with array indices
  return detail::array_value_cast_indexed<T>(
           a, dash::ce::make_index_sequence<N>());
}


/*
 * For reference, see
 *
 *   "Working Draft, C++ Extension for Ranges"
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/n4569.pdf
 *
 */

template <class I>
using reference_t = typename std::add_lvalue_reference<I>::type;

template <class I>
using rvalue_reference_t =
  decltype(std::move(std::declval<dash::reference_t<I>>()));

// void f(ValueType && val) { 
//   std::string i(move(val)); 
//   // ...
// } 
//
// ValueType val;
// std::bind(f, dash::make_adv(std::move(val)))();

template<typename T> struct adv { 
  T _value; 
  explicit adv(T && value) : _value(std::forward<T>(value)) {} 
  template<typename ...U> T && operator()(U &&...) { 
    return std::forward<T>(_value); 
  } 
}; 

template<typename T> adv<T> make_adv(T && value) { 
  return adv<T> { std::forward<T>(value) }; 
}

} // namespace dash

namespace std {
  // Must be defined in global std namespace.
  template<typename T> 
  struct is_bind_expression< dash::adv<T> > : std::true_type {}; 
} 

#endif // DOXYGEN

#endif // DASH__META_H__INCLUDED
