#ifndef DASH_DASH_INCLUDE_DASH_PAIR_H_
#define DASH_DASH_INCLUDE_DASH_PAIR_H_

#include <dash/Meta.h>

#include <type_traits>
#include <iostream>
#include <sstream>


namespace dash {

  /**
   * A trivially-copyable implementation of std::pair to be used
   * as element type of DASH containers.
   *
   * The implementation was mainly taken and adapted from std_pair.h.
   *
   * \todo Implementation of tuples are missing at the moment.
   */
  template<class T1, class T2>
  struct Pair
  {
    typedef T1 first_type;    /// @c first_type is the first bound type
    typedef T2 second_type;   /// @c second_type is the second bound type

    T1 first;                 /// @c first is a copy of the first object
    T2 second;                /// @c second is a copy of the second object

    /**
     * The default constructor.
     */
    constexpr Pair()
      : first(), second()
    { }

    /**
     * Two objects may be passed to a Pair constructor to be copied.
     */
    constexpr Pair(const T1& __a, const T2& __b)
      : first(__a), second(__b)
    { }

    /**
     * A Pair might be constructed from another pair iff first and second
     * are convertible.
     */
    template<class U1, class U2, class = typename
              std::enable_if<
                std::is_convertible<const U1&, T1>::value &&
                std::is_convertible<const U2&, T2>::value>::value>
    constexpr Pair(const Pair<U1, U2>& p)
      : first(p.first), second(p.second)
    { }

    constexpr Pair(const Pair&) = default;
    constexpr Pair(Pair&&) = default;

    template<class U1, class = typename
        std::enable_if<std::is_convertible<U1, T1>::value>::type>
    constexpr Pair(U1&& x, const T2& y)
      : first(std::forward<U1>(x)), second(y)
    { }

    template<class U2, class = typename
        std::enable_if<std::is_convertible<U2, T2>::value>::type>
    constexpr Pair(const T1& x, U2&& y)
      : first(x), second(std::forward<U2>(y))
    { }

    template<class U1, class U2, class = typename
        std::enable_if<std::is_convertible<U1, T1>::value &&
                       std::is_convertible<U2, T2>::value>::type>
    constexpr Pair(U1&& x, U2&& y)
      : first(std::forward<U1>(x)), second(std::forward<U2>(y))
    { }

    template<class U1, class U2, class = typename
        std::enable_if<std::is_convertible<U1, T1>::value &&
                       std::is_convertible<U2, T2>::value>::type>
    constexpr Pair(Pair<U1, U2>&& p)
      : first(std::forward<U1>(p.first)),
        second(std::forward<U2>(p.second))
    { }

    Pair&
    operator=(const Pair& p) = default;

    Pair&
    operator=(Pair&& p)
    noexcept(
        std::is_nothrow_move_assignable<T1>::value &&
        std::is_nothrow_move_assignable<T2>::value) = default;

    template<class U1, class U2>
    Pair&
    operator=(const Pair<U1, U2>& p)
    {
      first  = p.first;
      second = p.second;
      return *this;
    }

    template<class U1, class U2>
    Pair&
    operator=(Pair<U1, U2>&& p)
    {
      first  = std::forward<U1>(p.first);
      second = std::forward<U2>(p.second);
      return *this;
    }

    void
    swap(Pair& p)
    noexcept(noexcept(swap(first, p.first))
        && noexcept(swap(second, p.second)))
    {
      std::swap(first, p.first);
      std::swap(second, p.second);
    }
  };

  /**
   * Two pairs of the same type are equal iff their members are equal.
   */
  template<class T1, class T2>
  inline constexpr bool
  operator==(const Pair<T1, T2>& x, const Pair<T1, T2>& y)
  {
    return x.first == y.first && x.second == y.second;
  }

  /**
   * A pair is smaller than another pair if the first member is smaller
   * or the first is equal and the second is smaller.
   * See <http://gcc.gnu.org/onlinedocs/libstdc++/manual/utilities.html>
   */
  template<class T1, class T2>
  inline constexpr bool
  operator<(const Pair<T1, T2>& x, const Pair<T1, T2>& y)
  {
    return  x.first < y.first
      || (!(y.first < x.first) && !(x.second >= y.second));
  }

  /**
   * Inequality comparison operator implemented in terms of
   * equality operator.
   */
  template<class T1, class T2>
  inline constexpr bool
  operator!=(const Pair<T1, T2>& x, const Pair<T1, T2>& y)
  {
    return !(x == y);
  }

  /**
   * Greater-than operator implemented in terms of less-than operator.
   */
  template<class T1, class T2>
  inline constexpr bool
  operator>(const Pair<T1, T2>& x, const Pair<T1, T2>& y)
  {
    return y < x;
  }

  /**
   * Less-than-or-equal operator implemented in terms of less-than operator.
   */
  template<class T1, class T2>
  inline constexpr bool
  operator<=(const Pair<T1, T2>& x, const Pair<T1, T2>& y)
  {
    return !(y < x);
  }

  /**
   * Greater-than-or-equal operator implemented in terms of less-than operator.
   */
  template<class T1, class T2>
  inline constexpr bool
  operator>=(const Pair<T1, T2>& x, const Pair<T1, T2>& y)
  {
    return !(x < y);
  }

  /**
   * Wrapper for Pair::swap.
   */
  template<class T1, class T2>
  inline void
  swap(Pair<T1, T2>& x, Pair<T1, T2>& y)
  noexcept(noexcept(x.swap(y)))
  {
    x.swap(y);
  }

  namespace internal {
    /**
     * Duplicated here to not rely on an STL implementation
     */
    template <class T>
    struct strip
    {
      typedef T type;
    };
    template <class T>
    struct strip<std::reference_wrapper<T>>
    {
        typedef T& type;
    };
    template <class T>
    struct decay_and_strip
    {
        typedef typename strip<typename std::decay<T>::type>::type type;
    };
  } // namespace internal

  /**
   * Convennience wrapper to create a Pair object.
   */
  template<class T1, class T2>
  constexpr Pair<typename internal::decay_and_strip<T1>::type,
                 typename internal::decay_and_strip<T2>::type>
  make_pair(T1&& x, T2&& y)
  {
    typedef typename internal::decay_and_strip<T1>::type ds_type1;
    typedef typename internal::decay_and_strip<T2>::type ds_type2;
    typedef Pair<ds_type1, ds_type2>                     pair_type;
    return pair_type(std::forward<T1>(x), std::forward<T2>(y));
  }

  template<class T1, class T2>
  std::ostream & operator<<(
    std::ostream       & os,
    const Pair<T1, T2> & pair)
  {
    std::ostringstream ss;
    ss << dash::typestr(pair)
       << " { " << pair.first
       << " , " << pair.second
       << " } ";
    return operator<<(os, ss.str());
  }

} // namespace dash


#endif /* DASH_DASH_INCLUDE_DASH_PAIR_H_ */
