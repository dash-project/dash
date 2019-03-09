#ifndef DASH__ITERATOR__INTERNAL__GLOBREF_BASE_H__INCLUDED
#define DASH__ITERATOR__INTERNAL__GLOBREF_BASE_H__INCLUDED

#include <cstdint>
#include <memory>
#include <type_traits>

#include <dash/TypeTraits.h>
#include <dash/dart/if/dart_globmem.h>

namespace dash {

/// Forward declarations
template <typename T, class MemSpaceT>
class GlobPtr;

namespace detail {

template <typename ReferenceT, typename TargetT>
struct add_const_from_type {
  using type = TargetT;
};

template <typename ReferenceT, typename TargetT>
struct add_const_from_type<const ReferenceT, TargetT> {
  using type = typename std::add_const<TargetT>::type;
};

template <class...>
struct null_v : std::integral_constant<int, 0> {
};

// clang-format off

template<class LHS, class RHS>
using is_implicitly_convertible = std::is_convertible<
        typename std::add_lvalue_reference<LHS>::type,
        typename std::add_lvalue_reference<RHS>::type>;

#if 0
template <class LHS, class RHS>
using is_explicitly_convertible = std::integral_constant<bool,
      // 1) not implicitly convertible
      !is_implicitly_convertible<LHS, RHS>::value &&
      // 2.1) It is constructible  or...
      (std::is_constructible<
        typename std::add_lvalue_reference<RHS>::type,
        typename std::add_lvalue_reference<LHS>::type>::value ||
      // 2.2) if RHS is a base of RHS and both classes are non-polymorphic
      (std::conditional<std::is_const<LHS>::value,
        std::is_const<RHS>,
        std::true_type>::type::value

       && std::is_base_of<LHS, RHS>::value
        && !std::is_polymorphic<LHS>::value
        && !std::is_polymorphic<RHS>::value))>;
#else
template <typename From, typename To>
struct is_explicitly_convertible {
  template <typename T>
  static void f(T);

  template <typename F, typename T>
  static constexpr auto test(int)
      -> decltype(f(static_cast<T>(std::declval<F>())), true)
  {
    return true;
  }

  template <typename F, typename T>
  static constexpr auto test(...) -> bool
  {
    return false;
  }

  static bool const value = test<From, To>(0);
};
#endif


template <class LHS, class RHS>
using enable_implicit_copy_ctor = null_v<
    typename std::enable_if<
      is_implicitly_convertible<LHS, RHS>::value,
      LHS>::type>;

template <class LHS, class RHS>
using enable_explicit_copy_ctor = null_v<
    typename std::enable_if<
      !is_implicitly_convertible<LHS, RHS>::value &&
      is_explicitly_convertible<
        typename std::add_lvalue_reference<LHS>::type,
        typename std::add_lvalue_reference<RHS>::type>::value,
      LHS>::type>;

// clang-format on

template <typename T1, typename T2>
inline std::size_t constexpr offset_of(T1 T2::*member) noexcept
{
  constexpr T2 dummy{};
  return std::size_t{std::uintptr_t(std::addressof(dummy.*member))} -
         std::size_t{std::uintptr_t(std::addressof(dummy))};
}

template <class T>
class GlobRefBase {
 public:
  using value_type          = T;
  using const_value_type    = typename std::add_const<T>::type;
  using nonconst_value_type = typename std::remove_const<T>::type;

 protected:
  /**
   * PRIVATE: Constructor, creates an GlobRefBase object referencing an
   * element in global memory.
   */
  explicit constexpr GlobRefBase(dart_gptr_t const& dart_gptr)
    : m_dart_pointer(dart_gptr)
  {
  }
  explicit constexpr GlobRefBase(dart_gptr_t&& dart_gptr)
    : m_dart_pointer(std::move(dart_gptr))
  {
  }

 public:
  constexpr GlobRefBase() = delete;

  GlobRefBase(GlobRefBase const&)     = default;
  GlobRefBase(GlobRefBase&&) noexcept = default;

  /**
   * Constructor: creates an GlobRefBase object referencing an element in
   * global memory.
   */
  template <class MemSpaceT>
  explicit constexpr GlobRefBase(
      /// Pointer to referenced object in global memory
      const GlobPtr<value_type, MemSpaceT>& gptr)
    : GlobRefBase(gptr.dart_gptr())
  {
  }

  /**
   * Constructor: creates an GlobRefBase object referencing an element in
   * global memory.
   */
  template <class MemSpaceT>
  explicit constexpr GlobRefBase(
      /// Pointer to referenced object in global memory
      GlobPtr<value_type, MemSpaceT>&& gptr)
    : GlobRefBase(std::move(gptr.dart_gptr()))
  {
  }

  // clang-format off
  /**
   * Copy constructor, implicit if at least one of the following conditions is
   * satisfied:
   *    1) value_type and _From are exactly the same types (including const and
   *    volatile qualifiers
   *    2) value_type and _From are the same types after removing const and
   *    volatile qualifiers and value_type itself is const.
   */
  // clang-format on
  template <
      typename _From,
      long = detail::enable_implicit_copy_ctor<_From, value_type>::value>
  constexpr GlobRefBase(const GlobRefBase<_From>& gref) noexcept
    : GlobRefBase(gref.dart_gptr())
  {
  }

  // clang-format off
  /**
   * Copy constructor, implicit if at least one of the following conditions is
   * satisfied:
   *    1) value_type and _From are exactly the same types (including const and
   *    volatile qualifiers
   *    2) value_type and _From are the same types after removing const and
   *    volatile qualifiers and value_type itself is const.
   */
  // clang-format on
  template <
      typename _From,
      long = detail::enable_implicit_copy_ctor<_From, value_type>::value>
  constexpr GlobRefBase(GlobRefBase<_From>&& gref) noexcept
    : GlobRefBase(std::move(gref.dart_gptr()))
  {
  }

  // clang-format off
  /**
   * Copy constructor, explicit if the following conditions are satisfied.
   *    1) value_type and _From are the same types after excluding const and
   *    volatile qualifiers
   *    2) value_type is const and _From is non-const
   */
  // clang-format on
  template <
      typename _From,
      int = detail::enable_explicit_copy_ctor<_From, value_type>::value>
  explicit constexpr GlobRefBase(const GlobRefBase<_From>& gref) noexcept
    : GlobRefBase(gref.dart_gptr())
  {
  }

  // clang-format off
  /**
   * Copy constructor, explicit if the following conditions are satisfied.
   *    1) value_type and _From are the same types after excluding const and
   *    volatile qualifiers
   *    2) value_type is const and _From is non-const
   */
  // clang-format on
  template <
      typename _From,
      int = detail::enable_explicit_copy_ctor<_From, value_type>::value>
  explicit constexpr GlobRefBase(GlobRefBase<_From>&& gref) noexcept
    : GlobRefBase(std::move(gref.dart_gptr()))
  {
  }

  constexpr dart_gptr_t const& dart_gptr() const& noexcept
  {
    return this->m_dart_pointer;
  }

  constexpr dart_gptr_t const&& dart_gptr() const&& noexcept
  {
    return std::move(this->m_dart_pointer);
  }

  constexpr dart_gptr_t& dart_gptr() & noexcept
  {
    return this->m_dart_pointer;
  }

  constexpr dart_gptr_t&& dart_gptr() && noexcept
  {
    return std::move(this->m_dart_pointer);
  }

 private:
  dart_gptr_t m_dart_pointer{DART_GPTR_NULL};
};

}  // namespace detail
}  // namespace dash
#endif
