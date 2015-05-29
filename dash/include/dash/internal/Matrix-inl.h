#ifndef DASH_INTERNAL_MATRIX_INL_H_INCLUDED
#define DASH_INTERNAL_MATRIX_INL_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>

namespace dash {

template<typename T, size_t NumDimensions>
Matrix_RefProxy<T, NumDimensions>::Matrix_RefProxy()
: _dim(0) {
}

// Local_Ref

template<typename T, size_t NumDimensions, size_t CUR>
inline Local_Ref<T, NumDimensions, CUR>::operator
Local_Ref<T, NumDimensions, CUR - 1> && () {
  Local_Ref<T, NumDimensions, CUR - 1> ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
Local_Ref<T, NumDimensions, CUR>::operator
Matrix_Ref<T, NumDimensions, CUR> () {
  // Should avoid cast from Matrix_Ref to Local_Ref.
  // Different operation semantics.
  Matrix_Ref<T, NumDimensions, CUR>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
Local_Ref<T, NumDimensions, CUR>::Local_Ref(Matrix<T, NumDimensions> * mat) {
  _proxy = new Matrix_RefProxy < T, NumDimensions >;
  *_proxy = *(mat->_ref._proxy);

  for (int i = 0; i < NumDimensions; i++) {
    _proxy->_viewspec.begin[i] = 0;
    _proxy->_viewspec.range[i] = mat->_pattern.local_extent(i);
  }
  _proxy->_viewspec.update_size();
}

template<typename T, size_t NumDimensions, size_t CUR>
long long Local_Ref<T, NumDimensions, CUR>::extent(size_t dim) const {
  assert(dim < NumDimensions && dim >= 0);
  return _proxy->_mat->_pattern.local_extent(dim);
}

template<typename T, size_t NumDimensions, size_t CUR>
size_t Local_Ref<T, NumDimensions, CUR>::size() const {
  return _proxy->_viewspec.size();
}

template<typename T, size_t NumDimensions, size_t CUR>
T & Local_Ref<T, NumDimensions, CUR>::at_(size_type pos) {
  if (!(pos < _proxy->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return _proxy->_mat->lbegin()[pos];
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
T & Local_Ref<T, NumDimensions, CUR>::at(Args... args) {
  assert(sizeof...(Args) == NumDimensions - _proxy->_dim);
  std::array<long long, NumDimensions> coord = { args... };
  for(int i=_proxy->_dim;i<NumDimensions;i++) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return at_(
      _proxy->_mat->_pattern.local_at_(
        _proxy->_coord,
        _proxy->_viewspec));
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
T & Local_Ref<T, NumDimensions, CUR>::operator()(Args... args) {
  return at(args...);
}

template<typename T, size_t NumDimensions, size_t CUR>
Local_Ref<T, NumDimensions, CUR-1> &&
Local_Ref<T, NumDimensions, CUR>::operator[](size_t n) {
  Local_Ref<T, NumDimensions, CUR-1>  ref;
  ref._proxy = _proxy;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  _proxy->_viewspec.update_size();
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
Local_Ref<T, NumDimensions, CUR-1>
Local_Ref<T, NumDimensions, CUR>::operator[](size_t n) const {
  Local_Ref<T, NumDimensions, CUR - 1> ref;
  ref._proxy = new Matrix_RefProxy < T, NumDimensions > ;
  ref._proxy->_coord = _proxy->_coord;
  ref._proxy->_coord[_proxy->_dim] = n;
  ref._proxy->_dim = _proxy->_dim + 1;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.view_dim--;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
Local_Ref<T, NumDimensions, NumDimensions-1>
Local_Ref<T, NumDimensions, CUR>::sub(size_type n) {
  static_assert(
      NumDimensions - 1 > 0,
      "Dimension too low for sub()");
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Illegal sub-dimension");
  size_t target_dim = SubDimension + _proxy->_dim;
  Local_Ref<T, NumDimensions, NumDimensions - 1> ref;
  Matrix_RefProxy<T, NumDimensions> * proxy =
    new Matrix_RefProxy<T, NumDimensions>();
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);
  ref._proxy = proxy;
  ref._proxy->_coord[target_dim] = 0;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.begin[target_dim] = n;
  ref._proxy->_viewspec.range[target_dim] = 1;
  ref._proxy->_viewspec.view_dim--;
  ref._proxy->_viewspec.update_size();
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_dim = _proxy->_dim + 1;
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Local_Ref<T, NumDimensions, NumDimensions-1>
Local_Ref<T, NumDimensions, CUR>::col(size_type n) {
  return sub<1>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Local_Ref<T, NumDimensions, NumDimensions-1>
Local_Ref<T, NumDimensions, CUR>::row(size_type n) {
  return sub<0>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
Local_Ref<T, NumDimensions, NumDimensions> Local_Ref<T, NumDimensions, CUR>::submat(
  size_type n,
  size_type range) {
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Wrong sub-dimension");
  Local_Ref<T, NumDimensions, NumDimensions> ref;
  Matrix_RefProxy<T, NumDimensions> * proxy = new Matrix_RefProxy<T, NumDimensions>();
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.begin[SubDimension] = n;
  ref._proxy->_viewspec.range[SubDimension] = range;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
Local_Ref<T, NumDimensions, NumDimensions> Local_Ref<T, NumDimensions, CUR>::rows(
  size_type n,
  size_type range) {
  return submat<0>(n, range);
}

template<typename T, size_t NumDimensions, size_t CUR>
Local_Ref<T, NumDimensions, NumDimensions> Local_Ref<T, NumDimensions, CUR>::cols(
  size_type n,
  size_type range) {
  return submat<1>(n, range);
}

// Matrix_Ref

template<typename T, size_t NumDimensions, size_t CUR>
Matrix_Ref<T, NumDimensions, CUR>::operator
Matrix_Ref<T, NumDimensions, CUR-1> && () {
  Matrix_Ref<T, NumDimensions, CUR-1> ref =
    Matrix_Ref<T, NumDimensions, CUR-1>();
  ref._proxy = _proxy;
  return ::std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
Pattern<NumDimensions> & Matrix_Ref<T, NumDimensions, CUR>::pattern() {
  return _proxy->_mat->_pattern;
}

template<typename T, size_t NumDimensions, size_t CUR>
Team & Matrix_Ref<T, NumDimensions, CUR>::team() {
  return _proxy->_mat->_team;
}

template<typename T, size_t NumDimensions, size_t CUR>
constexpr typename Matrix_Ref<T, NumDimensions, CUR>::size_type
Matrix_Ref<T, NumDimensions, CUR>::size() const noexcept {
  return _proxy->_size;
}

template<typename T, size_t NumDimensions, size_t CUR>
typename Matrix_Ref<T, NumDimensions, CUR>::size_type
Matrix_Ref<T, NumDimensions, CUR>::extent(size_type dim) const noexcept {
  return _proxy->_viewspec.range[dim];
}

template<typename T, size_t NumDimensions, size_t CUR>
constexpr bool Matrix_Ref<T, NumDimensions, CUR>::empty() const noexcept {
  return size() == 0;
}

template<typename T, size_t NumDimensions, size_t CUR>
void Matrix_Ref<T, NumDimensions, CUR>::barrier() const {
  _proxy->_mat->_team.barrier();
}

template<typename T, size_t NumDimensions, size_t CUR>
inline void
Matrix_Ref<T, NumDimensions, CUR>::forall(
  ::std::function<void(long long)> func) {
  _proxy->_mat->_pattern.forall(func, _proxy->_viewspec);
}

template<typename T, size_t NumDimensions, size_t CUR>
Matrix_Ref<T, NumDimensions, CUR-1> &&
Matrix_Ref<T, NumDimensions, CUR>::operator[](size_t n) {
  Matrix_Ref<T, NumDimensions, CUR-1> ref;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  _proxy->_viewspec.update_size();
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
Matrix_Ref<T, NumDimensions, CUR-1>
Matrix_Ref<T, NumDimensions, CUR>::operator[](size_t n) const {
  Matrix_Ref<T, NumDimensions, CUR - 1> ref;
  ref._proxy = new Matrix_RefProxy < T, NumDimensions > ;
  ref._proxy->_coord = _proxy->_coord;
  ref._proxy->_coord[_proxy->_dim] = n;
  ref._proxy->_dim = _proxy->_dim + 1;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.view_dim--;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
Matrix_Ref<T, NumDimensions, NumDimensions-1>
Matrix_Ref<T, NumDimensions, CUR>::sub(size_type n) {
  static_assert(
      NumDimensions - 1 > 0,
      "Too low dim");
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Wrong sub-dimension for sub()");
  size_t target_dim = SubDimension + _proxy->_dim;
  Matrix_Ref<T, NumDimensions, NumDimensions - 1> ref;
  Matrix_RefProxy<T, NumDimensions> * proxy =
    new Matrix_RefProxy<T, NumDimensions>;
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

  ref._proxy = proxy;
  ref._proxy->_coord[target_dim] = 0;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.begin[target_dim] = n;
  ref._proxy->_viewspec.range[target_dim] = 1;
  ref._proxy->_viewspec.view_dim--;
  ref._proxy->_viewspec.update_size();
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_dim = _proxy->_dim + 1;
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Matrix_Ref<T, NumDimensions, NumDimensions-1>
Matrix_Ref<T, NumDimensions, CUR>::col(size_type n) {
  return sub<1>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Matrix_Ref<T, NumDimensions, NumDimensions-1>
Matrix_Ref<T, NumDimensions, CUR>::row(size_type n) {
  return sub<0>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
Matrix_Ref<T, NumDimensions, NumDimensions>
Matrix_Ref<T, NumDimensions, CUR>::submat(
  size_type n,
  size_type range) {
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Wrong sub-dimension for submat()");
  Matrix_Ref<T, NumDimensions, NumDimensions> ref;
  Matrix_RefProxy<T, NumDimensions> * proxy = new Matrix_RefProxy < T, NumDimensions > ;
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.begin[SubDimension] = n;
  ref._proxy->_viewspec.range[SubDimension] = range;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Matrix_Ref<T, NumDimensions, NumDimensions>
Matrix_Ref<T, NumDimensions, CUR>::rows(
  size_type n,
  size_type range) {
  return submat<0>(n, range);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Matrix_Ref<T, NumDimensions, NumDimensions>
Matrix_Ref<T, NumDimensions, CUR>::cols(
  size_type n,
  size_type range) {
  return submat<1>(n, range);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline typename Matrix_Ref<T, NumDimensions, CUR>::reference
Matrix_Ref<T, NumDimensions, CUR>::at_(size_type unit, size_type elem) {
  if (!(elem < _proxy->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return *(_proxy->_mat->begin());
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
inline typename Matrix_Ref<T, NumDimensions, CUR>::reference
Matrix_Ref<T, NumDimensions, CUR>::at(Args... args) {
  assert(sizeof...(Args) == NumDimensions - _proxy->_dim);
  ::std::array<long long, NumDimensions> coord = { args... };
  for(int i = _proxy->_dim; i < NumDimensions; ++i) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }   
  size_t unit = _proxy->_mat->_pattern.atunit_(
                  _proxy->_coord,
                  _proxy->_viewspec);
  size_t elem = _proxy->_mat->_pattern.at_(
                  _proxy->_coord,
                  _proxy->_viewspec);
  return at_(unit , elem);
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
inline typename Matrix_Ref<T, NumDimensions, CUR>::reference
Matrix_Ref<T, NumDimensions, CUR>::operator()(Args... args) {
  return at(args...);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Pattern<NumDimensions>
Matrix_Ref<T, NumDimensions, CUR>::pattern() const {
  return _proxy->_mat->_pattern;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline bool
Matrix_Ref<T, NumDimensions, CUR>::isLocal(size_type n) {
  return (_proxy->_mat->_pattern.index_to_unit(n, _proxy->_viewspec) ==
          _proxy->_mat->_myid);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline bool
Matrix_Ref<T, NumDimensions, CUR>::isLocal(size_t dim, size_type n) {
  return _proxy->_mat->_pattern.is_local(
           n,
           _proxy->_mat->_myid,
           dim,
           _proxy->_viewspec);
}

template<typename T, size_t NumDimensions, size_t CUR>
template <int level>
inline dash::HView<Matrix<T, NumDimensions>, level>
Matrix_Ref<T, NumDimensions, CUR>::hview() {
  return dash::HView<Matrix<T, NumDimensions>, level>(*this);
}

// Local_Ref
// Partial Specialization for value deferencing.

template <typename T, size_t NumDimensions>
inline T *
Local_Ref<T, NumDimensions, 0>::at_(size_t pos) {
  if (!(pos < _proxy->_mat->size())) {
    throw std::out_of_range("Position for Local_Ref.at_ out of range");
  }
  return &(_proxy->_mat->lbegin()[pos]);
}

template <typename T, size_t NumDimensions>
inline Local_Ref<T, NumDimensions, 0>::operator T() {
  T ret = *at_(_proxy->_mat->_pattern.local_at_(
                 _proxy->_coord,
                 _proxy->_viewspec));
  delete _proxy;
  return ret;
}

template <typename T, size_t NumDimensions>
inline T
Local_Ref<T, NumDimensions, 0>::operator=(const T value) {
  T* ref = at_(_proxy->_mat->_pattern.local_at_(
                 _proxy->_coord,
                 _proxy->_viewspec));
  *ref = value;
  delete _proxy;
  return value;
}

// Matrix_Ref
// Partial Specialization for value deferencing.

template <typename T, size_t NumDimensions>
inline GlobRef<T>
Matrix_Ref<T, NumDimensions, 0 >::at_(size_t unit, size_t elem) const {
  return *(_proxy->_mat->begin()); // .get(unit, elem);
}

template <typename T, size_t NumDimensions>
inline Matrix_Ref<T, NumDimensions, 0>::operator T() {
  GlobRef<T> ref = at_(
      _proxy->_mat->_pattern.atunit_(_proxy->_coord, _proxy->_viewspec),
      _proxy->_mat->_pattern.at_(_proxy->_coord, _proxy->_viewspec)
  );
  delete _proxy;  
  return ref;
}

template <typename T, size_t NumDimensions>
inline T
Matrix_Ref<T, NumDimensions, 0>::operator=(const T value) {
  GlobRef<T> ref = at_(_proxy->_mat->_pattern.atunit_(
                         _proxy->_coord,
                         _proxy->_viewspec),
                       _proxy->_mat->_pattern.at_(
                         _proxy->_coord,
                         _proxy->_viewspec));
  ref = value;
  delete _proxy;
  return value;
}

template <typename ElementType, size_t NumDimensions>
inline Local_Ref<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::local() const {
  return _local;
}

// Matrix
// Proxy, Matrix_Ref and Local_Ref are created at initialization.

template <typename ElementType, size_t NumDimensions>
Matrix<ElementType, NumDimensions>::Matrix(
  const dash::SizeSpec<NumDimensions> & ss,
  const dash::DistributionSpec<NumDimensions> & ds,
  Team & t,
  const TeamSpec<NumDimensions> & ts)
: _team(t),
  _pattern(ss, ds, ts, t),
  _local_mem_size(_pattern.max_elem_per_unit() * sizeof(value_type)),
  _glob_mem(_team, _local_mem_size) {
  // Matrix is a friend of class Team
  dart_team_t teamid = _team.dart_id();

  _dart_gptr = DART_GPTR_NULL;
  dart_ret_t ret = dart_team_memalloc_aligned(
      teamid,
      _local_mem_size,
      &_dart_gptr);

  _ptr = new GlobIter<value_type, Pattern<NumDimensions> >(&_glob_mem, _pattern, 0lu);
  _size = _pattern.nelem();
  _myid = _team.myid();
  _ref._proxy = new Matrix_RefProxy<value_type, NumDimensions>;
  _ref._proxy->_dim = 0;
  _ref._proxy->_mat = this;
  _ref._proxy->_viewspec = _pattern.m_viewspec;
  _local = Local_Ref<ElementType, NumDimensions, NumDimensions>(this);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix<ElementType, NumDimensions>::~Matrix() {
  dart_team_t teamid = _team.dart_id();
  dart_team_memfree(teamid, _dart_gptr);
}

template <typename ElementType, size_t NumDimensions>
inline Pattern<NumDimensions> & Matrix<ElementType, NumDimensions>::pattern() {
  return _pattern;
}

template <typename ElementType, size_t NumDimensions>
inline dash::Team & Matrix<ElementType, NumDimensions>::team() {
  return _team;
}

template <typename ElementType, size_t NumDimensions>
inline constexpr typename Matrix<ElementType, NumDimensions>::size_type
Matrix<ElementType, NumDimensions>::size() const noexcept {
  return _size;
}

template <typename ElementType, size_t NumDimensions>
inline typename Matrix<ElementType, NumDimensions>::size_type
Matrix<ElementType, NumDimensions>::extent(size_t dim) const noexcept {
  return _size;
}

template <typename ElementType, size_t NumDimensions>
inline constexpr bool
Matrix<ElementType, NumDimensions>::empty() const noexcept {
  return size() == 0;
}

template <typename ElementType, size_t NumDimensions>
inline void
Matrix<ElementType, NumDimensions>::barrier() const {
  _team.barrier();
}

template <typename ElementType, size_t NumDimensions>
inline typename Matrix<ElementType, NumDimensions>::const_pointer
Matrix<ElementType, NumDimensions>::data() const noexcept {
  return *_ptr;
}

template <typename ElementType, size_t NumDimensions>
inline typename Matrix<ElementType, NumDimensions>::iterator
Matrix<ElementType, NumDimensions>::begin() noexcept {
  return iterator(data());
}

template <typename ElementType, size_t NumDimensions>
inline typename Matrix<ElementType, NumDimensions>::iterator
Matrix<ElementType, NumDimensions>::end() noexcept {
  return iterator(data() + _size);
}

template <typename ElementType, size_t NumDimensions>
inline ElementType *
Matrix<ElementType, NumDimensions>::lbegin() noexcept {
  void * addr;
  dart_gptr_t gptr = _dart_gptr;
  dart_gptr_setunit(&gptr, _myid);
  dart_gptr_getaddr(gptr, &addr);
  return (ElementType *)(addr);
}

template <typename ElementType, size_t NumDimensions>
inline ElementType *
Matrix<ElementType, NumDimensions>::lend() noexcept {
  void * addr;
  dart_gptr_t gptr = _dart_gptr;
  dart_gptr_setunit(&gptr, _myid);
  dart_gptr_incaddr(&gptr, _local_mem_size * sizeof(ElementType));
  dart_gptr_getaddr(gptr, &addr);
  return (ElementType *)(addr);
}

template <typename ElementType, size_t NumDimensions>
inline void
Matrix<ElementType, NumDimensions>::forall(
  ::std::function<void(long long)> func) {
  _pattern.forall(func);
}

template <typename ElementType, size_t NumDimensions>
template<size_t SubDimension>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::sub(
  size_type n) {
  return _ref.sub<SubDimension>(n);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::col(
  size_type n) {
  return _ref.sub<1>(n);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::row(
  size_type n) {
  return _ref.sub<0>(n);
}

template <typename ElementType, size_t NumDimensions>
template<size_t SubDimension>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::submat(
  size_type n,
  size_type range) {
  return _ref.submat<SubDimension>(n, range);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::rows(
  size_type n,
  size_type range) {
  return _ref.submat<0>(n, range);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::cols(
  size_type n,
  size_type range) {
  return _ref.submat<1>(n, range);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix_Ref<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::operator[](size_type n) {
  return _ref.operator[](n);
}

template <typename ElementType, size_t NumDimensions>
template <typename ... Args>
inline typename Matrix<ElementType, NumDimensions>::reference
Matrix<ElementType, NumDimensions>::at(Args... args) {
  return _ref.at(args...);
}

template <typename ElementType, size_t NumDimensions>
template <typename ... Args>
inline typename Matrix<ElementType, NumDimensions>::reference
Matrix<ElementType, NumDimensions>::operator()(Args... args) {
  return _ref.at(args...);
}

template <typename ElementType, size_t NumDimensions>
inline Pattern<NumDimensions>
Matrix<ElementType, NumDimensions>::pattern() const {
  return _pattern;
}

template <typename ElementType, size_t NumDimensions>
inline bool Matrix<ElementType, NumDimensions>::isLocal(size_type n) {
  return _ref.isLocal(n);
}

template <typename ElementType, size_t NumDimensions>
inline bool Matrix<ElementType, NumDimensions>::isLocal(
  size_t dim,
  size_type n) {
  return _ref.isLocal(dim, n);
}

template <typename ElementType, size_t NumDimensions>
template <int level>
inline dash::HView<Matrix<ElementType, NumDimensions>, level>
Matrix<ElementType, NumDimensions>::hview() {
  return _ref.hview<level>();
}

template <typename ElementType, size_t NumDimensions>
inline Matrix<ElementType, NumDimensions>::operator
Matrix_Ref<ElementType, NumDimensions, NumDimensions>() {
  return _ref;
}

}  // namespace dash

#endif  // DASH_INTERNAL_MATRIX_INL_H_INCLUDED
