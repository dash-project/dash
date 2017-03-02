#ifndef COARRAY_H_INCLUDED
#define COARRAY_H_INCLUDED

#include <type_traits>

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/iterator/GlobIter.h>

#include <dash/pattern/BlockPattern.h>
#include <dash/util/ArrayExpr.h>

#include <dash/Array.h>
#include <dash/Matrix.h>

#include <dash/Dimensional.h>

/**
 * \defgroup  DashCoArrayConcept  co_array Concept
 *
 * \ingroup DashContainerConcept
 * \{
 * \par Description
 *
 * A fortran style co_array.
 *
 * DASH co_arrays support delayed allocation (\c dash::co_array::allocate),
 * so global memory of an array instance can be allocated any time after
 * declaring a \c dash::co_array variable.
 *
 * \par Types
 *
 * \TODO: Types
 *
 */

namespace dash {

/**
 * fortran style co_array
 */
template<
  typename T,
  typename IndexType = dash::default_index_t>
class Co_array {
private:
  
  template<typename __T, typename __S, int __rank>
  struct __get_type_extens_as_array {
    using array_t = std::array<__S,__rank>;
    static constexpr array_t value = dash::ce::append(
          __get_type_extens_as_array<__T, __S, __rank-1>::value,
          static_cast<__S>(std::extent<__T, __rank-1>::value));
  };

  template<typename __T, typename __S>
  struct __get_type_extens_as_array<__T, __S, 0> {
    using array_t = std::array<__S,0>;
    static constexpr array_t value = std::array<__S, 0>();
  };

private:
  static constexpr int _rank = std::rank<T>::value;
  
  using _index_type     = typename std::make_unsigned<IndexType>::type;
  using _sspec_type     = SizeSpec<_rank+1, _index_type>;
  using _pattern_type   = BlockPattern<_rank+1, ROW_MAJOR, _index_type>;
  using _element_type   = typename std::remove_all_extents<T>::type;
  using _storage_type   = Matrix<_element_type, _rank+1, _index_type, _pattern_type>;
  template<int _subrank = _rank>
  using _view_type      = MatrixRef<_element_type, _rank+1, _subrank, _pattern_type>;
  using _local_type     = LocalMatrixRef<_element_type, _rank+1, _rank-1, _pattern_type>;

public:
  // Types
  using value_type             = _element_type;
  using difference_type        = IndexType;
  using index_type             = IndexType;
  using size_type              = _index_type;
  using iterator               = GlobIter<_element_type, _pattern_type>; 
  using const_iterator         = GlobIter<const _element_type, _pattern_type>; 
  using reverse_iterator       = GlobIter<_element_type, _pattern_type>; 
  using const_reverse_iterator = GlobIter<const _element_type, _pattern_type>; 
  using reference              = GlobRef<_element_type>;
  using const_reference        = GlobRef<_element_type>;
  using local_pointer          = _element_type *;
  using const_local_pointer    = const _element_type *;
  template<int subrank>
  using view_type              = _view_type<subrank>;
  using local_type             = _local_type;
  using pattern_type           = _pattern_type;
  
private:
  constexpr _sspec_type _make_size_spec() const noexcept {
    return _sspec_type(dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              __get_type_extens_as_array<T, size_type, _rank>::value));
  }

  constexpr _sspec_type _make_size_spec(const size_type first_dim) const noexcept {
    static_assert(std::get<0>(__get_type_extens_as_array<T, size_type, _rank>::value) == 0,
                  "Array type is fully specified");
    
    return _sspec_type(dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              dash::ce::replace_nth<0>(
                first_dim,
                __get_type_extens_as_array<T, size_type, _rank>::value)));
  }
  
  constexpr std::array<index_type, _rank> & _offsets_unit(const team_unit_t & unit){
    return _storage.pattern().global(unit, std::array<index_type,_rank> {});
  }
  
  constexpr std::array<size_type, _rank> & _extents_unit(const team_unit_t & unit){
    return _storage.pattern().local_extents(unit);
  }
  
public:
  /**
   * Constructor for scalar types and fully specified array types:
   * \code
   *   dash::Co_array<int>         i;
   *   dash::Co_array<int[10][20]> x;
   * \endcode
   */
  constexpr Co_array():
    _storage(_pattern_type(_make_size_spec())) { }
    
  explicit constexpr Co_array(const size_type & first_dim):
    _storage(_pattern_type(_make_size_spec(first_dim))) {  }

  /* ======================================================================== */
  /*                      Operators for element access                        */
  /* ======================================================================== */
  /**
   * Operator to select remote unit
   */
  template<int __rank = _rank>
  inline view_type<__rank> operator()(const team_unit_t & unit) {
    return this->operator ()(static_cast<index_type>(unit));
  }
  /**
   * Operator to select remote unit
   */
  template<int __rank = _rank>
  inline view_type<__rank> operator()(const index_type & local_unit) {
    return _storage[local_unit];
  }

  /**
   * Provides access to local array part
   * \code
   *   dash::Co_array<int[10][20]> x;
   *   x[2][3] = 42;
   * \endcode
   */
  template<int __rank = _rank>
  typename std::enable_if<(__rank != 0), local_type>::type
  operator[](const index_type & idx) {
    return _storage.local[0][0];
  }
  
  /**
   * allows fortran like local assignment of scalars
   * \code
   *   dash::Co_array<int> i;
   *   i = 42;
   * \endcode
   */
  template<int __rank = _rank>
  typename std::enable_if<(__rank == 0), value_type>::type
  inline operator=(const value_type & value){
    return *(_storage.lbegin()) = value;
  }
  
  /**
   * allows fortran like local access of scalars
   * \code
   *   dash::Co_array<int> i;
   *   i = 42;
   *   int b = i;
   * \endcode
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  operator value_type() const {
    return *(_storage.lbegin());
  }
  
  /**
   * allows fortran like local access of scalars
   * \code
   *   dash::Co_array<int> i;
   *   i = 42;
   *   i += 21;
   * \endcode
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator +=(const value_type & value) {
    return *(_storage.lbegin()) += value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator -=(const value_type & value) {
    return *(_storage.lbegin()) -= value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator *=(const value_type & value) {
    return *(_storage.lbegin()) *= value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator /=(const value_type & value) {
    return *(_storage.lbegin()) /= value;
  }
  
  /**
   * allows fortran like local access of scalars
   * \code
   *   dash::Co_array<int> i;
   *   i = 42;
   *   int b = i + 21;
   * \endcode
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator +(const value_type & value) const {
    return *(_storage.lbegin()) + value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator -(const value_type & value) const {
    return *(_storage.lbegin()) - value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator *(const value_type & value) const {
    return *(_storage.lbegin()) * value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator /(const value_type & value) const {
    return *(_storage.lbegin()) / value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator ++() {
    return ++(*(_storage.lbegin()));
  }
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator ++(int) {
    return (*(_storage.lbegin()))++;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator --() {
    return --(*(_storage.lbegin()));
  }
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator --(int) {
    return (*(_storage.lbegin()))--;
  }
  
private:
  /// storage backend
  _storage_type _storage;
};
} // namespace dash

/* ======================================================================== */
/* Ugly global overloads necessary to mimic fortran co_array interface      */
/* All types are supported to which dash::Co_array can be converted         */
/* ======================================================================== */
template<
  typename Lhs,
  typename T>
Lhs operator+(const Lhs & lhs, const dash::Co_array<T> & rhs) {
  return lhs + static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
Lhs operator-(const Lhs & lhs, const dash::Co_array<T> & rhs) {
  return lhs - static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
Lhs operator*(const Lhs & lhs, const dash::Co_array<T> & rhs) {
  return lhs * static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
Lhs operator/(const Lhs & lhs, const dash::Co_array<T> & rhs) {
  return lhs / static_cast<Lhs>(rhs);
}

#endif /* COARRAY_H_INCLUDED */
