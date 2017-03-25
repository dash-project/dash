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

#include <dash/atomic/Type_traits.h>

/**
 * \defgroup DashCoarrayConcept  Coarray Concept
 *
 * \ingroup DashContainerConcept
 * \{
 * \par Description
 *
 * A fortran style coarray.
 *
 * DASH Coarrays support delayed allocation (\c dash::Coarray::allocate),
 * so global memory of an array instance can be allocated any time after
 * declaring a \c dash::Coarray variable.
 *
 * \par Types
 * 
 * \sa DashCoarrayLib
 * \sa dash::Comutex
 * 
 * \}
 */

namespace dash {

// forward decls
namespace coarray {

template<typename Container>
inline void sync_images(const Container & image_ids);

}

/**
 * A fortran style co_array.
 * 
 * \ingroup DashCoarrayConcept
 * 
 */
template<
  typename T,
  typename IndexType = dash::default_index_t>
class Coarray {
private:
  
  template<typename __T, typename __S, int __rank>
  struct __get_type_extents_as_array {
    using array_t = std::array<__S,__rank>;
    static constexpr array_t value = dash::ce::append(
          __get_type_extents_as_array<__T, __S, __rank-1>::value,
          static_cast<__S>(std::extent<__T, __rank-1>::value));
  };

  template<typename __T, typename __S>
  struct __get_type_extents_as_array<__T, __S, 0> {
    using array_t = std::array<__S,0>;
    static constexpr array_t value = {};
  };
  
  /**
   * Trait to transform \cdash::Atomic<T[]...> to 
   * \cdash::Atomic<T>
   * __T  denotes the full type including \cdash::Atomic<>
   * __UT denotes the underlying type without extens
   */
  template<typename __T, typename __UT>
  struct __insert_atomic_if_necessary {
    typedef __UT type;
  };
  template<typename __T, typename __UT>
  struct __insert_atomic_if_necessary<dash::Atomic<__T>, __UT> {
    typedef dash::Atomic<__UT> type;
  };
  
private:
  
  using _underl_type    = typename dash::remove_atomic<T>::type;
  using _native_type    = typename std::remove_all_extents<_underl_type>::type;
  
  using _rank           = std::integral_constant<int,
                            std::rank<_underl_type>::value>;
  
  using _index_type     = typename std::make_unsigned<IndexType>::type;
  using _sspec_type     = SizeSpec<_rank::value+1, _index_type>;
  using _pattern_type   = BlockPattern<_rank::value+1, ROW_MAJOR, _index_type>;
  
  /**
   * _element_type has no extent and is wrapped with \cdash::Atomic, if coarray
   * was declared with \cdash::Atomic<T>
   */
  using _element_type   = typename __insert_atomic_if_necessary<T, _native_type>::type;
  
  using _storage_type   = Matrix<_element_type, _rank::value+1, _index_type, _pattern_type>;
  
  template<int _subrank = _rank::value>
  using _view_type      = typename _storage_type::template view_type<_subrank>;
  using _local_type     = LocalMatrixRef<_element_type,
                                         _rank::value+1,
                                         _rank::value-1,
                                         _pattern_type>;
  using _offset_type    = std::array<_index_type, _rank::value>;

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
              __get_type_extents_as_array<_underl_type, size_type, _rank::value>::value));
  }

  constexpr _sspec_type _make_size_spec(const size_type first_dim) const noexcept {
    static_assert(std::get<0>(__get_type_extents_as_array<_underl_type, size_type, _rank::value>::value) == 0,
                  "Array type is fully specified");
    
    return _sspec_type(dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              dash::ce::replace_nth<0>(
                first_dim,
                __get_type_extents_as_array<_underl_type, size_type, _rank::value>::value)));
  }
  
  constexpr _offset_type & _offsets_unit(const team_unit_t & unit) const noexcept {
    return _storage.pattern().global(unit, std::array<index_type,_rank::value> {});
  }
  
  constexpr _offset_type & _extents_unit(const team_unit_t & unit) const noexcept {
    return _storage.pattern().local_extents(unit);
  }
  
public:
  
  static constexpr dim_t ndim() noexcept {
    return static_cast<dim_t>(_rank::value);
  }
  
  /**
   * Constructor for scalar types and fully specified array types:
   * \code
   *   dash::Co_array<int>         i;
   *   dash::Co_array<int[10][20]> x;
   * \endcode
   */
  Coarray() {
    if(dash::is_initialized()){
      _storage.allocate(_pattern_type(_make_size_spec()));
    }
  }
    
  /**
   * Constructor for array types with one unspecified dimension:
   * \code
   *   dash::Co_array<int[10][]> y;
   * \endcode
   */
  explicit Coarray(const size_type & first_dim) {
    if(dash::is_initialized()){
      _storage.allocate(_pattern_type(_make_size_spec(first_dim)));
    }
  }
  
  template<
  int __rank = _rank::value,
  typename = typename std::enable_if<(__rank == 0)>::type>
  explicit Coarray(const value_type & value){
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH has to be initialized");
    _storage.allocate(_pattern_type(_make_size_spec()));
    *(_storage.lbegin()) = value;
    sync_all();
  }
  
  /* ======================================================================== */
  /*                         DASH Container Concept                           */
  /* ======================================================================== */
  
  constexpr const pattern_type & pattern() const noexcept {
    return _storage.pattern();
  }
  
  iterator begin() noexcept {
    return _storage.begin();
  }
  
  constexpr const_iterator begin() const noexcept {
    return _storage.begin();
  }
  
  iterator end() noexcept {
    return _storage.end();
  }
  
  constexpr const_iterator end() const noexcept {
    return _storage.end();
  }
  
  local_pointer lbegin() noexcept {
    return _storage.lbegin();
  }
  
  constexpr const_local_pointer lbegin() const noexcept {
    return _storage.lbegin();
  }
  
  local_pointer lend() noexcept {
    return _storage.lend();
  }
  
  constexpr const_local_pointer lend() const noexcept {
    return _storage.lend();
  }
  
  constexpr size_type size() const noexcept {
    return _storage.size();
  }
  
  constexpr size_type local_size() const noexcept {
    return _storage.local_size();
  }
  
  constexpr bool is_local(index_type gi) const noexcept {
    return _storage.is_local(gi);
  }
  
  /**
   * allocate an array which was initialized before dash has been initialized
   */
  inline void allocate(){
    _storage.allocate(_pattern_type(_make_size_spec()));
  }
  
  /**
   * free the memory allocated by this coarray. After deallocation, the coarray
   * cannot be used anymore.
   */
  inline void deallocate(){
    _storage.deallocate();
  }
  
  inline Team & team() {
    return _storage.team();
  }
  
  inline void barrier() {
    _storage.barrier();
  }
  
  /**
   * Blocks until all team members of this container have reached the statement
   * and flushes the memory.
   */
  inline void sync_all() {
    _storage.barrier();
    _storage.flush_all();
  }
  
  /**
   * Blocks until all selected team members of this container have reached
   * the statement and flushes the memory.
   */
  template<typename Container>
  inline void sync_images(const Container & image_ids){
    dash::coarray::sync_images(image_ids);
    _storage.flush_all();
  }
  
  inline void flush(){
    _storage.flush();
  }
  
  inline void flush_all(){
    _storage.flush_all();
  }
  
  inline void flush_local(){
    _storage.flush_local();
  }
  
  inline void flush_local_all(){
    _storage.flush_local_all();
  }

  /* ======================================================================== */
  /*                      Operators for element access                        */
  /* ======================================================================== */
  /**
   * Operator to select remote unit
   */
  template<int __rank = _rank::value>
  inline view_type<__rank> operator()(const team_unit_t & unit) {
    return this->operator ()(static_cast<index_type>(unit));
  }
  /**
   * Operator to select remote unit for array types
   */
  template<int __rank = _rank::value>
  typename std::enable_if<(__rank != 0), view_type<__rank>>::type
  inline operator()(const index_type & local_unit) {
    return _storage[local_unit];
  }
  
  /**
   * Operator to select remote unit for scalar types
   */
  template<int __rank = _rank::value>
  typename std::enable_if<(__rank == 0), reference>::type
  inline operator()(const index_type & local_unit) {
    return _storage.at(local_unit);
  }

  /**
   * Provides access to local array part
   * \code
   *   dash::Co_array<int[10][20]> x;
   *   x[2][3] = 42;
   * \endcode
   */
  template<int __rank = _rank::value>
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
  template<int __rank = _rank::value>
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
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  operator value_type() const {
    return *(_storage.lbegin());
  }
  
  /**
   * support 
   * @return 
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  explicit operator reference() {
    return *(_storage.begin()+static_cast<index_type>(dash::myid()));
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
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator +=(const value_type & value) {
    return *(_storage.lbegin()) += value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator -=(const value_type & value) {
    return *(_storage.lbegin()) -= value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type & operator *=(const value_type & value) {
    return *(_storage.lbegin()) *= value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
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
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator +(const value_type & value) const {
    return *(_storage.lbegin()) + value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator -(const value_type & value) const {
    return *(_storage.lbegin()) - value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator *(const value_type & value) const {
    return *(_storage.lbegin()) * value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator /(const value_type & value) const {
    return *(_storage.lbegin()) / value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator ++() {
    return ++(*(_storage.lbegin()));
  }
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator ++(int) {
    return (*(_storage.lbegin()))++;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  value_type operator --() {
    return --(*(_storage.lbegin()));
  }
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
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
Lhs operator+(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs + static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
Lhs operator-(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs - static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
Lhs operator*(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs * static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
Lhs operator/(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs / static_cast<Lhs>(rhs);
}

#include <dash/coarray/Utils.h>

#endif /* COARRAY_H_INCLUDED */
