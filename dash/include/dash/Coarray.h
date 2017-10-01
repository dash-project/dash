#ifndef COARRAY_H_INCLUDED
#define COARRAY_H_INCLUDED

#include <type_traits>

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/iterator/GlobIter.h>

#include <dash/pattern/BlockPattern.h>
#include <dash/Dimensional.h>
#include <dash/TeamSpec.h>
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

namespace coarray {

// forward decls
template<typename Container>
inline void sync_images(const Container & image_ids);

namespace detail {

template <
  class ArrayT,
  class ValueT,
  std::size_t Rank,
  std::size_t... Is >
constexpr std::array<ValueT, Rank>
__get_type_extents_as_array_impl(
    dash::ce::index_sequence<Is...>) {
  return { ( static_cast<ValueT>(std::extent<ArrayT,Is>::value) )... };
}

template <
  class ArrayT,
  class ValueT,
  std::size_t Rank >
constexpr std::array<ValueT, Rank>
__get_type_extents_as_array() {
  return __get_type_extents_as_array_impl<ArrayT,ValueT,Rank>(
      dash::ce::make_index_sequence<Rank>());
}

template<
  typename element_type,
  typename pattern_type,
  int rank> 
struct __get_local_type {
  using type = LocalMatrixRef<element_type,
                              rank+1,
                              rank-1,
                              pattern_type>;
};

template<
  typename element_type,
  typename pattern_type>
struct __get_local_type<element_type, pattern_type, 1> {
  using type = element_type &;
};

template<typename element_type>
struct __get_ref_type {
  using type = GlobAsyncRef<element_type>;
};

/**
 * atomics cannot be accessed asynchronously
 */
template<typename element_type>
struct __get_ref_type<dash::Atomic<element_type>> {
  using type = GlobRef<dash::Atomic<element_type>>;
};

template<typename element_type>
struct __get_const_ref_type {
  using type = GlobAsyncRef<const element_type>;
};

/**
 * atomics cannot be accessed asynchronously
 */
template<typename element_type>
struct __get_const_ref_type<dash::Atomic<element_type>> {
  using type = GlobRef<dash::Atomic<const element_type>>;
};

} // namespace detail

/**
 * helper to create a coarray pattern for coarrays where the local size of
 * each unit is equal (symmetric allocation).
 */
template<
  typename T,
  typename IndexType = dash::default_index_t>
struct make_coarray_symmetric_pattern {
  using type = BlockPattern<
                std::rank<
                  typename dash::remove_atomic<T>::type>::value+1,
                ROW_MAJOR,
                IndexType>;
};

} // namespace coarray

/**
 * A fortran style co_array.
 * 
 * \ingroup DashCoarrayConcept
 * 
 */
template<
  typename T,
  typename IndexType = dash::default_index_t,
  class PatternType  = typename coarray::make_coarray_symmetric_pattern<T,IndexType>::type >
class Coarray {
private:

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

  using self_t          = Coarray<T, IndexType, PatternType>;
  
  using _underl_type    = typename dash::remove_atomic<T>::type;
  using _native_type    = typename std::remove_all_extents<_underl_type>::type;
  
  using _rank           = std::integral_constant<int,
                            std::rank<_underl_type>::value>;
  
  using _size_type      = typename std::make_unsigned<IndexType>::type;
  using _sspec_type     = SizeSpec<_rank::value+1, _size_type>;
  using _pattern_type   = PatternType;
  
  /**
   * _element_type has no extent and is wrapped with \cdash::Atomic, if coarray
   * was declared with \cdash::Atomic<T>
   */
  using _element_type   = typename __insert_atomic_if_necessary<T, _native_type>::type;
  
  using _storage_type   = Matrix<_element_type, _rank::value+1, IndexType, _pattern_type>;
  
  template<int _subrank = _rank::value>
  using _view_type      = typename _storage_type::template view_type<_subrank>;
  using _local_type     = typename coarray::detail::__get_local_type<
                                      _element_type,
                                      _pattern_type,
                                      _rank::value>::type; 

  using _offset_type    = std::array<IndexType, _rank::value+1>;

public:
  // Types
  using value_type             = _element_type;
  using difference_type        = IndexType;
  using index_type             = IndexType;
  using size_type              = _size_type;
  using iterator               = GlobIter<_element_type, _pattern_type>; 
  using const_iterator         = GlobIter<const _element_type, _pattern_type>; 
  using reverse_iterator       = GlobIter<_element_type, _pattern_type>; 
  using const_reverse_iterator = GlobIter<const _element_type, _pattern_type>; 
  using reference              = typename coarray::detail::__get_ref_type<_element_type>::type;
  using const_reference        = typename coarray::detail::__get_const_ref_type<_element_type>::type;
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
              coarray::detail::__get_type_extents_as_array<_underl_type,
                size_type, _rank::value>()));
  }

  constexpr _sspec_type _make_size_spec(const size_type & first_dim) const noexcept {
    static_assert(
        std::get<0>(coarray::detail::__get_type_extents_as_array<_underl_type,
          size_type, _rank::value>()) == 0,
      "Array type is fully specified");
    
    return _sspec_type(dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              dash::ce::replace_nth<0>(
                first_dim,
                coarray::detail::__get_type_extents_as_array<_underl_type,
                size_type, _rank::value>())));
  }
  
  constexpr _offset_type _offsets_unit(const team_unit_t & unit) const noexcept {
    return _storage.pattern().global(unit, std::array<index_type, _rank::value+1> {});
  }
  
  constexpr _offset_type _extents_unit(const team_unit_t & unit) const noexcept {
    return _storage.pattern().local_extents(unit);
  }
  
public:
  
  static constexpr dim_t ndim() noexcept {
    return static_cast<dim_t>(_rank::value + 1);
  }
  
  /**
   * Constructor for scalar types and fully specified array types:
   * \code
   *   dash::Coarray<int>         i;
   *   dash::Coarray<int[10][20]> x;
   * \endcode
   */
  explicit Coarray(Team & team = Team::All()) {
    if(dash::is_initialized() &&
        (std::is_array<_underl_type>::value == false 
         || std::extent<_underl_type, 0>::value != 0))
    {
      _storage.allocate(_pattern_type(_make_size_spec(),
                                      DistributionSpec<_rank::value+1>(),
                                      TeamSpec<_rank::value+1,
                                               IndexType>(team),
                                      team));
    }
  }

  /**
   * Constructor for array types with one unspecified dimension:
   * \code
   *   dash::Coarray<int[10][]> y;
   * \endcode
   */
  template<
  int __rank = _rank::value,
  typename = typename std::enable_if<(__rank != 0)>::type>
  explicit Coarray(const size_type & first_dim, Team & team = Team::All()) {
    if(dash::is_initialized()){
      _storage.allocate(_pattern_type(_make_size_spec(first_dim),
                                      DistributionSpec<_rank::value+1>(),
                                      TeamSpec<_rank::value+1>(team),
                                      team));
    }
  }
  
#if 0
  // disabled as currently no at least 2-dimensional pattern supports this
  /**
   * Constructor for array types, where local size is not equal among
   * all units. Requires a pattern that supports an asymmetric layout.
   * Possibly generated using
   * 
   * \todo enforce using pattern properties
   * 
   * \cdash::coarray::make_coarray_asymmetric_pattern
   * \param extents vector of all extents
   * \param team
   */
  explicit Coarray(std::vector<size_type> extents,
                   Team & team = Team::All()){
    if(dash::is_initialized()){
      extents.insert(extents.begin(), static_cast<size_type>(dash::size()));
      // asymmetric pattern
      const _pattern_type a_pattern(extents,
                                    DistributionSpec<_rank::value+1>(),
                                    TeamSpec<_rank::value+1>(team),
                                    team);
      _storage.allocate(a_pattern);
    }
  }
#endif
  
  template<
  int __rank = _rank::value,
  typename = typename std::enable_if<(__rank == 0)>::type>
  explicit Coarray(const value_type & value, Team & team = Team::All()){
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH has to be initialized");
    _storage.allocate(_pattern_type(_make_size_spec(),
                                    DistributionSpec<_rank::value+1>(),
                                    TeamSpec<_rank::value+1>(team),
                                    team));
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

  constexpr const_iterator cbegin() const noexcept {
    return _storage.begin();
  }
  
  iterator end() noexcept {
    return _storage.end();
  }
  
  constexpr const_iterator end() const noexcept {
    return _storage.end();
  }

  constexpr const_iterator cend() const noexcept {
    return _storage.cend();
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

  /**
   * For fully specified array types, \c dash::Coarray<T[N...]> is a fixed-size
   * container. Hence the value equals the value returned by \c max_size .
   * For containers with one open dimension, the size is determined by dash.
   * If dash is not initialized, it always returns zero.
   * \TODO Currently _storage.size() is returned.
   */
  constexpr size_type max_size() const noexcept {
    return _storage.size();
  }

  constexpr bool empty() const noexcept {
    return _storage.size() == 0;
  }

  void swap(self_t && other){
    std::swap(this->_storage, other._storage);
    dash::barrier();
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
   * allocate an array which was initialized before dash has been initialized
   */
  inline void allocate(const size_type & n){
    if(n > 0){
      _storage.allocate(_pattern_type(_make_size_spec(n)));
    }
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
    _storage.flush_all();
    _storage.barrier();
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
    // TODO: workaround to get async access
    return static_cast<reference>(_storage.at(local_unit));
  }

  /**
   * optimized bracket operator for accessing local elements
   * of 1-D Coarray (const version)
   */
  template<int __rank = _rank::value>
  typename std::enable_if<(__rank == 1),
             const local_type>::type
  inline operator[](const index_type & idx) const 
  {
    return const_cast<const local_type>(
        *(_storage.lbegin()+idx));
  }

  /**
   * optimized bracket operator for accessing local elements
   * of 1-D Coarray
   */
  template<int __rank = _rank::value>
  typename std::enable_if<(__rank == 1), local_type>::type
  inline operator[](const index_type & idx) {
    return *(_storage.lbegin()+idx);
  }
  /**
   * Provides access to local array part
   * \code
   *   dash::Coarray<int[10][20]> x;
   *   x[2][3] = 42;
   * \endcode
   */
  template<int __rank = _rank::value>
  typename std::enable_if<(__rank > 1), local_type>::type
  inline operator[](const index_type & idx) {
    return _storage.local[0][idx];
  }
  
  /**
   * allows fortran like local assignment of scalars
   * \code
   *   dash::Coarray<int> i;
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
   *   dash::Coarray<int> i;
   *   i = 42;
   *   int b = i;
   * \endcode
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline operator value_type() const {
    return *(_storage.lbegin());
  }
  
  /**
   * convert scalar Coarray to a global reference.
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  explicit inline operator reference() {
    return static_cast<reference>(
            *(_storage.begin()+static_cast<index_type>(dash::myid())));
  }
  
  /**
   * Get a reference to a member of a certain type at the
   * specified offset
   */
  template<
    typename MEMTYPE,
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline MEMTYPE & member(size_t offs) {
    char * s_begin = reinterpret_cast<char *>(_storage.lbegin());
    s_begin += offs;
    return *(reinterpret_cast<MEMTYPE*>(s_begin));
  }

  /**
   * Get the member via pointer to member
   * \code
   * struct value_t {double a; int b;};
   * dash::Coarray<value_t> x;
   * int b = x.member(&value_t::b);
   * \endcode
   */
  template<
    class MEMTYPE,
    class P=T,
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline MEMTYPE & member(const MEMTYPE P::*mem) {
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<MEMTYPE>(offs);
  }

  /**
   * allows fortran like local access of scalars
   * \code
   *   dash::Coarray<int> i;
   *   i = 42;
   *   i += 21;
   * \endcode
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type & operator +=(const value_type & value) {
    return *(_storage.lbegin()) += value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type & operator -=(const value_type & value) {
    return *(_storage.lbegin()) -= value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type & operator *=(const value_type & value) {
    return *(_storage.lbegin()) *= value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type & operator /=(const value_type & value) {
    return *(_storage.lbegin()) /= value;
  }
  
  /**
   * allows fortran like local access of scalars
   * \code
   *   dash::Coarray<int> i;
   *   i = 42;
   *   int b = i + 21;
   * \endcode
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator +(const value_type & value) const {
    return *(_storage.lbegin()) + value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator -(const value_type & value) const {
    return *(_storage.lbegin()) - value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator *(const value_type & value) const {
    return *(_storage.lbegin()) * value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator /(const value_type & value) const {
    return *(_storage.lbegin()) / value;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator ++() {
    return ++(*(_storage.lbegin()));
  }
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator ++(int) {
    return (*(_storage.lbegin()))++;
  }
  
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator --() {
    return --(*(_storage.lbegin()));
  }
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __rank = _rank::value,
    typename = typename std::enable_if<(__rank == 0)>::type>
  inline value_type operator --(int) {
    return (*(_storage.lbegin()))--;
  }
  
private:
  /// storage backend
  _storage_type _storage;
};
} // namespace dash

/* ======================================================================== */
/* Ugly global overloads necessary to mimic fortran co_array interface      */
/* All types are supported to which dash::Coarray can be converted         */
/* ======================================================================== */
template<
  typename Lhs,
  typename T>
inline Lhs operator+(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs + static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
inline Lhs operator-(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs - static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
inline Lhs operator*(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs * static_cast<Lhs>(rhs);
}

template<
  typename Lhs,
  typename T>
inline Lhs operator/(const Lhs & lhs, const dash::Coarray<T> & rhs) {
  return lhs / static_cast<Lhs>(rhs);
}

#include <dash/coarray/Utils.h>
#include <dash/Coevent.h>

#endif /* COARRAY_H_INCLUDED */
