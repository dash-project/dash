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
 * \sa dash::Coevent
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
type_extents_as_array_impl(
    dash::ce::index_sequence<Is...>) {
  return { ( static_cast<ValueT>(std::extent<ArrayT,Is>::value) )... };
}

template <
  class ArrayT,
  class ValueT,
  std::size_t Rank >
constexpr std::array<ValueT, Rank>
type_extents_as_array() {
  return type_extents_as_array_impl<ArrayT,ValueT,Rank>(
      dash::ce::make_index_sequence<Rank>());
}

template <
  typename T >
constexpr bool type_is_complete() {
  return (std::get<0>(coarray::detail::type_extents_as_array<T,
          int, std::rank<T>::value>()) > 0);
}

template<
  typename element_type,
  typename pattern_type,
  int rank>
struct local_ref_type {
  using type = LocalMatrixRef<element_type,
                              rank+1,
                              rank-1,
                              pattern_type>;
};

template<
  typename element_type,
  typename pattern_type>
struct local_ref_type<dash::Atomic<element_type>, pattern_type, 1> {
  using type = GlobRef<element_type>;
};

template<
  typename element_type,
  typename pattern_type>
struct local_ref_type<element_type, pattern_type, 1> {
  using type = element_type &;
};

template<typename element_type>
struct ref_type {
// GAR needs conversion operator
//  using type = GlobAsyncRef<element_type>;
  using type = GlobRef<element_type>;
};

/**
 * atomics cannot be accessed asynchronously
 */
template<typename element_type>
struct ref_type<dash::Atomic<element_type>> {
  using type = GlobRef<dash::Atomic<element_type>>;
};

template<typename element_type>
struct const_ref_type {
  using type = GlobAsyncRef<const element_type>;
};

/**
 * atomics cannot be accessed asynchronously
 */
template<typename element_type>
struct const_ref_type<dash::Atomic<element_type>> {
  using type = GlobRef<dash::Atomic<const element_type>>;
};

} // namespace detail

/**
 * helper to create a coarray pattern for coarrays where the local size of
 * each unit is equal (symmetric allocation).
 */
template<
  typename T,
  typename IndexType = dash::default_index_t,
  MemArrange Arrangement = ROW_MAJOR>
struct make_coarray_symmetric_pattern {
  using type = BlockPattern<
                std::rank<
                  typename dash::remove_atomic<T>::type>::value+1,
                Arrangement,
                IndexType>;
};

} // namespace coarray

/**
 * A fortran style coarray.
 *
 * Interface of the Coarray for scalar and array types showing local
 * and global accesses:
 * \code
 * dash::Coarray<int> i;          // scalar Coarray
 * dash::Coarray<int[10][20]> x;  // 2D-Coarray
 * dash::Coarray<int[][20]> y(n); // one open dim,
 *                                // set at runtime in ctor
 *
 * // access syntax
 * i(unit) = value; // global access
 * i       = value; // local access
 *
 * x(unit)[idx1][idx2] = value; // global access
 * x[idx1][idx2]       = value; // local access
 * \endcode
 *
 * \ingroup DashCoarrayConcept
 */
template<
  typename T,
  typename IndexType = dash::default_index_t,
  MemArrange Arrangement = ROW_MAJOR >
class Coarray {
private:

  using self_t          = Coarray<T, IndexType, Arrangement>;

  /**
   * _element_type has no extent and is wrapped with \c dash::Atomic, if coarray
   * was declared with \c dash::Atomic<T>
   */
  using _element_type   = typename std::remove_all_extents<T>::type;
  using _base_type      = typename dash::remove_atomic<_element_type>::type;

  using _valuetype_rank = std::integral_constant<int,
                            std::rank<T>::value>;
  using _rank           = std::integral_constant<int,
                            _valuetype_rank::value+1>;

  using _size_type      = typename std::make_unsigned<IndexType>::type;
  using _sspec_type     = SizeSpec<_rank::value, _size_type>;
  using _pattern_type   = typename coarray::make_coarray_symmetric_pattern<T,IndexType, Arrangement>::type;

  using _storage_type   = Matrix<_element_type, _rank::value, IndexType, _pattern_type>;

  template<int _subrank = _valuetype_rank::value>
  using _view_type      = typename _storage_type::template view_type<_subrank>;
  using _local_ref_type = typename coarray::detail::local_ref_type<
                                      _element_type,
                                      _pattern_type,
                                      _valuetype_rank::value>::type;

  using _offset_type    = std::array<IndexType, _rank::value>;

public:
  // Types
  using value_type             = _element_type;
  /// Same as value_type but without atomic wrapper
  using value_base_type        = _base_type;
  using difference_type        = IndexType;
  using index_type             = IndexType;
  using size_type              = _size_type;
  using iterator               = GlobIter<_element_type, _pattern_type>;
  using const_iterator         = GlobIter<const _element_type, _pattern_type>;
  using reverse_iterator       = GlobIter<_element_type, _pattern_type>;
  using const_reverse_iterator = GlobIter<const _element_type, _pattern_type>;
  using reference              = typename coarray::detail::ref_type<_element_type>::type;
  using const_reference        = typename coarray::detail::const_ref_type<_element_type>::type;
  using local_pointer          = _element_type *;
  using const_local_pointer    = const _element_type *;
  template<int subrank>
  using view_type              = _view_type<subrank>;
  using local_type             = _local_ref_type;
  using pattern_type           = _pattern_type;

private:
  constexpr _sspec_type _make_size_spec() const noexcept {
    return _sspec_type(dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              coarray::detail::type_extents_as_array<T,
                size_type, _valuetype_rank::value>()));
  }

  constexpr _sspec_type _make_size_spec(const size_type & first_dim) const noexcept {
    static_assert(
        !coarray::detail::type_is_complete<T>(),
      "Array type may not be fully specified");

    return _sspec_type(dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              dash::ce::replace_nth<0>(
                first_dim,
                coarray::detail::type_extents_as_array<T,
                size_type, _valuetype_rank::value>())));
  }

  constexpr _offset_type _offsets_unit(const team_unit_t & unit) const noexcept {
    return _storage.pattern().global(unit, std::array<index_type, _rank::value> {});
  }

  constexpr _offset_type _extents_unit(const team_unit_t & unit) const noexcept {
    return _storage.pattern().local_extents(unit);
  }

  inline index_type _myid() const {
    return static_cast<index_type>(_storage.team().myid());
  }

public:

  static constexpr dim_t ndim() noexcept {
    return static_cast<dim_t>(_rank::value);
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
        (std::is_array<_base_type>::value == false
         || std::extent<_base_type, 0>::value != 0))
    {
      allocate(team);
    }
  }

  /**
   * Constructor for array types with one unspecified dimension:
   * \code
   *   dash::Coarray<int[10][]> y;
   * \endcode
   */
  template<
  int __valuetype_rank = _valuetype_rank::value,
  typename = typename std::enable_if<(__valuetype_rank != 0)>::type>
  explicit Coarray(const size_type & first_dim, Team & team = Team::All()) {
    if(dash::is_initialized()){
      allocate(first_dim, team);
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
   * \c dash::coarray::make_coarray_asymmetric_pattern
   * \param extents vector of all extents
   * \param team
   */
  explicit Coarray(std::vector<size_type> extents,
                   Team & team = Team::All()){
    if(dash::is_initialized()){
      extents.insert(extents.begin(), static_cast<size_type>(dash::size()));
      // asymmetric pattern
      const _pattern_type a_pattern(extents,
                                    DistributionSpec<_rank::value>(),
                                    TeamSpec<_rank::value>(team),
                                    team);
      _storage.allocate(a_pattern);
    }
  }
#endif

  template<
  int __valuetype_rank = _valuetype_rank::value,
  typename = typename std::enable_if<(__valuetype_rank == 0)>::type>
  explicit Coarray(const value_type & value, Team & team = Team::All()){
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH has to be initialized");
    allocate(team);
    *(_storage.lbegin()) = value;
    _storage.barrier();
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
  inline void allocate(Team & team = dash::Team::All()){
    if(_storage.size() == 0){
      _storage.allocate(_pattern_type(_make_size_spec(),
                                      DistributionSpec<_rank::value>(),
                                      TeamSpec<_rank::value,
                                               IndexType>(team),
                                      team));
    }
  }

  /**
   * allocate an array which was initialized before dash has been initialized
   */
  inline void allocate(
      const size_type & n,
      Team & team = dash::Team::All())
  {
    if((_storage.size() == 0) && (n > 0)){
      _storage.allocate(_pattern_type(_make_size_spec(n),
                                      DistributionSpec<_rank::value>(),
                                      TeamSpec<_rank::value,
                                               IndexType>(team),
                                      team));
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
    _storage.barrier();
  }

  /**
   * Blocks until all selected team members of this container have reached
   * the statement and flushes the memory.
   */
  template<typename Container>
  inline void sync_images(const Container & image_ids){
    _storage.flush();
    dash::coarray::sync_images(image_ids);
  }

  inline void flush(){
    _storage.flush();
  }

  inline void flush_local(){
    _storage.flush_local();
  }

  /* ======================================================================== */
  /*                      Operators for element access                        */
  /* ======================================================================== */
  /**
   * Operator to select remote unit
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  inline view_type<__valuetype_rank> operator()(const team_unit_t & unit) {
    return this->operator ()(static_cast<index_type>(unit));
  }
  /**
   * Operator to select remote unit for array types
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(__valuetype_rank != 0), view_type<__valuetype_rank>>::type
  inline operator()(const index_type & unit) {
    return _storage[unit];
  }

  /**
   * Operator to select remote unit for scalar types
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(__valuetype_rank == 0), reference>::type
  inline operator()(const index_type & unit) {
    return _storage.at(unit);
  }

  /**
   * optimized bracket operator for accessing local elements
   * of 1-D Coarray (const version)
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(
            __valuetype_rank == 1 &&
            !is_atomic<value_type>::value),
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
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(
            __valuetype_rank == 1 &&
            !is_atomic<value_type>::value), local_type>::type
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
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(
      __valuetype_rank > 1 &&
      !is_atomic<value_type>::value), local_type>::type
  inline operator[](const index_type & idx) {
    // The Coarray internally is based on dash::Narray, with extents:
    // global: team.size() x T[0] x ... x T[n]
    // local: 1 x T[0] x ... x T[n]
    // The first (0) dimenson of the Narray is used for the Coindex.
    // Example: Coarray<T[10][5]> ~ NArray<T, 3,...>(team.size(), 10, 5)
    //
    // The pattern of the NArray is created in a way, that the offset in the
    // first (0) dimension selects the unit the data is local to. While the
    // dimensionality of the local and global parts are always equal, the rank
    // of the local part is reduced by one (the co-dimension).
    //
    // Hence: deref. first dimension and return a view on the remaining ones.
    // This is logically equivalent to _storage[myid][idx] but uses (faster)
    // local view types
    return _storage.local[0][idx];
  }

  /**
   * operator for local atomic accesses
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(
            __valuetype_rank > 0 &&
            is_atomic<value_type>::value), local_type>::type
  inline operator[](const index_type & idx) {
    // dereference first dimension and return a view on the remaining ones
    return operator()(_myid())[idx];
  }

  /**
   * allows fortran like local assignment of scalars
   * \code
   *   dash::Coarray<int> i;
   *   i = 42;
   * \endcode
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(
      __valuetype_rank == 0 &&
      !is_atomic<value_type>::value), value_type>::type
  inline operator=(const value_type & value){
    return *(_storage.lbegin()) = value;
  }

  /**
   * fortran like local assignment of scalar coarrays
   * of atomic value types
   */
  template<int __valuetype_rank = _valuetype_rank::value>
  typename std::enable_if<(
      __valuetype_rank == 0 &&
      is_atomic<value_type>::value), value_type>::type
  inline operator=(const value_type & value){
    return operator()(_myid()) = value;
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
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline operator value_type() const {
    return *(_storage.lbegin());
  }

  /**
   * conversion operator for an atomic scalar coarray to the native
   * (non-atomic) type
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline operator value_base_type() {
    return _storage[_myid()];
  }

  /**
   * convert scalar Coarray to a global reference.
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(__valuetype_rank == 0)>::type>
  explicit inline operator reference() {
    return _storage[myid()];
  }

  /**
   * Get a reference to a member of a certain type at the
   * specified offset
   */
  template<
    typename MEMTYPE,
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(__valuetype_rank == 0)>::type>
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
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(__valuetype_rank == 0)>::type>
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
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline value_type & operator +=(const value_type & value) {
    return *(_storage.lbegin()) += value;
  }

  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline value_base_type operator +=(const value_base_type & value) {
    return operator()(_myid()) += value;
  }


  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline value_type & operator -=(const value_type & value) {
    return *(_storage.lbegin()) -= value;
  }

  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline value_base_type operator -=(const value_base_type & value) {
    return operator()(_myid()) -= value;
  }


  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
   inline value_type & operator *=(const value_type & value) {
    return *(_storage.lbegin()) *= value;
  }

  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline reference operator *=(const value_type & value) {
    return operator()(_myid()) *= value;
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
   inline value_type & operator /=(const value_type & value) {
    return *(_storage.lbegin()) /= value;
  }

  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline reference operator /=(const value_type & value) {
    return operator()(_myid()) /= value;
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
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline value_type operator +(const value_type & value) const {
    return *(_storage.lbegin()) + value;
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline value_type operator -(const value_type & value) const {
    return *(_storage.lbegin()) - value;
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline value_type operator *(const value_type & value) const {
    return *(_storage.lbegin()) * value;
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
   inline value_type operator /(const value_type & value) const {
    return *(_storage.lbegin()) / value;
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
  inline value_type & operator ++() {
    return ++(*(_storage.lbegin()));
  }

  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline value_base_type operator ++() {
    return ++(operator()(_myid()));
  }
 
  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
   inline value_type operator ++(int) {
    return (*(_storage.lbegin()))++;
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
   inline value_type & operator --() {
    return --(*(_storage.lbegin()));
  }

  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        is_atomic<value_type>::value)>::type>
  inline value_base_type operator --() {
    return --(operator()(_myid()));
  }

  /**
   * allows fortran like local access of scalars
   */
  template<
    int __valuetype_rank = _valuetype_rank::value,
    typename = typename std::enable_if<(
        __valuetype_rank == 0 &&
        !is_atomic<value_type>::value)>::type>
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
