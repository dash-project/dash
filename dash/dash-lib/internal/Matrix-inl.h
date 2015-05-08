#ifndef DASH_INTERNAL_MATRIX_INL_H_INCLUDED
#define DASH_INTERNAL_MATRIX_INL_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "dart.h"

#include "Team.h"
#include "Pattern.h"
#include "GlobIter.h"
#include "GlobRef.h"
#include "HView.h"

namespace dash {

template<typename T, size_t DIM>
Matrix_RefProxy<T, DIM>::Matrix_RefProxy()
: _dim(0) {
}

// Local_Ref

template<typename T, size_t DIM, size_t CUR>
Local_Ref<T, DIM, CUR - 1> && Local_Ref<T, DIM, CUR>::operator() {
  Local_Ref<T, DIM, CUR - 1>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
Matrix_Ref<T, DIM, CUR> Local_Ref<T, DIM, CUR>::operator () {
  // Should avoid cast from Matrix_Ref to Local_Ref.
  // Different operation semantics.
  Matrix_Ref<T, DIM, CUR>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR>::Local_Ref<T, DIM, CUR>(Matrix<T, DIM> * mat) {
  _proxy = new Matrix_RefProxy < T, DIM >;
  *_proxy = *(mat->_ref._proxy);

  for (int i = 0; i < DIM; i++) {
    _proxy->_viewspec.begin[i] = 0;
    _proxy->_viewspec.range[i] = mat->_pattern.local_extent(i);
  }
  _proxy->_viewspec.update_size();
}

template<typename T, size_t DIM, size_t CUR>
long long Local_Ref<T, DIM, CUR>::extend(size_t dim) const {
  assert(dim < DIM && dim >= 0);
  return _proxy->_mat->_pattern.local_extent(dim);
}

template<typename T, size_t DIM, size_t CUR>
size_t Local_Ref<T, DIM, CUR>::size() const {
  return _proxy->_viewspec.size();
}

template<typename T, size_t DIM, size_t CUR>
T & Local_Ref<T, DIM, CUR>::at_(size_type pos) {
  if (!(pos < _proxy->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return _proxy->_mat->lbegin()[pos];
}

template<typename T, size_t DIM, size_t CUR>
template<typename ... Args>
T & Local_Ref<T, DIM, CUR>::at(Args... args) {
  assert(sizeof...(Args) == DIM - _proxy->_dim);
  std::array<long long, DIM> coord = { args... };
  for(int i=_proxy->_dim;i<DIM;i++) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return at_(
      _proxy->_mat->_pattern.local_at_(
        _proxy->_coord,
        _proxy->_viewspec));
}

template<typename T, size_t DIM, size_t CUR>
template<typename ... Args>
T & Local_Ref<T, DIM, CUR>::operator()(Args... args) {
  return at(args...);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> && Local_Ref<T, DIM, CUR>::operator[](size_t n) {
  Local_Ref<T, DIM, CUR-1>  ref;
  ref._proxy = _proxy;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  _proxy->_viewspec.update_size();
  return std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::operator[](size_t n) const {
  Local_Ref<T, DIM, CUR - 1> ref;
  ref._proxy = new Matrix_RefProxy < T, DIM > ;
  ref._proxy->_coord = _proxy->_coord;
  ref._proxy->_coord[_proxy->_dim] = n;
  ref._proxy->_dim = _proxy->_dim + 1;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.view_dim--;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t DIM, size_t CUR>
template<size_t SUBDIM>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::sub(size_type n) const {
  static_assert(DIM - 1 > 0, "Dimension too low for sub()");
  static_assert(SUBDIM < DIM && SUBDIM >= 0, "Illegal sub-dimension");
  size_t target_dim = SUBDIM + _proxy->_dim;
  Local_Ref<T, DIM, DIM - 1> ref;
  Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
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

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::col(size_type n) const {
  return sub<1>(n);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::row(size_type n) const {
  return sub<0>(n);
}

template<typename T, size_t DIM, size_t CUR>
template<size_t SUBDIM>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::submat(
  size_type n,
  size_type range) {
  static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong sub-dimension");
  Local_Ref<T, DIM, DIM> ref;
  Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.begin[SUBDIM] = n;
  ref._proxy->_viewspec.range[SUBDIM] = range;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::rows(
  size_type n,
  size_type range) {
  return submat<0>(n, range);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> Local_Ref<T, DIM, CUR>::cols(
  size_type n,
  size_type range) {
  return submat<1>(n, range);
}

// Matrix_Ref

template<typename T, size_t DIM, size_t CUR>
Matrix_Ref<T, DIM, CUR-1> && Matrix_Ref<T, DIM, CUR>::operator() {
  Matrix_Ref<T, DIM, CUR-1> ref =  Matrix_Ref<T, DIM, CUR-1>();
  ref._proxy = _proxy;
  return ::std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
Pattern<DIM> & Matrix_Ref<T, DIM, CUR>::pattern() {
  return _proxy->_mat->_pattern;
}

template<typename T, size_t DIM, size_t CUR>
Team & Matrix_Ref<T, DIM, CUR>::team() {
  return _proxy->_mat->_team;
}

template<typename T, size_t DIM, size_t CUR>
constexpr size_type Matrix_Ref<T, DIM, CUR>::size() const noexcept {
  return _proxy->_size;
}

template<typename T, size_t DIM, size_t CUR>
size_type Matrix_Ref<T, DIM, CUR>::extent(size_type dim) const noexcept {
  return _proxy->_viewspec.range[dim];
}

template<typename T, size_t DIM, size_t CUR>
constexpr bool Matrix_Ref<T, DIM, CUR>::empty() const noexcept {
  return size() == 0;
}

template<typename T, size_t DIM, size_t CUR>
void Matrix_Ref<T, DIM, CUR>::barrier() const {
  _proxy->_mat->_team.barrier();
}

template<typename T, size_t DIM, size_t CUR>
void Matrix_Ref<T, DIM, CUR>::forall(::std::function<void(long, long)> func) {
  _proxy->_mat->_pattern.forall(func, _proxy->_viewspec);
}

template<typename T, size_t DIM, size_t CUR>
Matrix_Ref<T, DIM, CUR-1> && Matrix_Ref<T, DIM, CUR>::operator[](size_t n) {
  Matrix_Ref<T, DIM, CUR-1> ref;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  _proxy->_viewspec.update_size();
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::operator[](size_t n) const {
  Matrix_Ref<T, DIM, CUR - 1> ref;
  ref._proxy = new Matrix_RefProxy < T, DIM > ;
  ref._proxy->_coord = _proxy->_coord;
  ref._proxy->_coord[_proxy->_dim] = n;
  ref._proxy->_dim = _proxy->_dim + 1;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.view_dim--;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t DIM, size_t CUR>
template<size_t SUBDIM>
Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::sub(size_type n) {
  static_assert(DIM - 1 > 0, "Too low dim");
  static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong sub-dimension for sub()");
  size_t target_dim = SUBDIM + _proxy->_dim;
  Matrix_Ref<T, DIM, DIM - 1> ref;
  Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy<T, DIM>;
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

template<typename T, size_t DIM, size_t CUR>
inline Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::col(size_type n) {
  return sub<1>(n);
}

template<typename T, size_t DIM, size_t CUR>
inline Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::row(size_type n) {
  return sub<0>(n);
}

template<typename T, size_t DIM, size_t CUR>
template<size_t SUBDIM>
Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::submat(
  size_type n,
  size_type range) {
  static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong sub-dimension for submat()");
  Matrix_Ref<T, DIM, DIM> ref;
  Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.begin[SUBDIM] = n;
  ref._proxy->_viewspec.range[SUBDIM] = range;
  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_viewspec.update_size();
  return ref;
}

template<typename T, size_t DIM, size_t CUR>
inline Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::cols(
  size_type n,
  size_type range) {
  return submat<1>(n, range);
}

template<typename T, size_t DIM, size_t CUR>
inline Matrix_Ref<T, DIM, CUR-1> Matrix_Ref<T, DIM, CUR>::rows(
  size_type n,
  size_type range) {
  return submat<0>(n, range);
}

template<typename T, size_t DIM, size_t CUR>
template<typename ... Args>
inline reference Matrix_Ref<T, DIM, CUR>::at(Args... args) {
  assert(sizeof...(Args) == DIM - _proxy->_dim);
  ::std::array<long long, DIM> coord = { args... };
  for(int i = _proxy->_dim; i < DIM; ++i) {
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

template<typename T, size_t DIM, size_t CUR>
template<typename ... Args>
inline reference Matrix_Ref<T, DIM, CUR>::operator()(Args... args) {
  return at(args...);
}

template<typename T, size_t DIM, size_t CUR>
inline Pattern<DIM> Matrix_Ref<T, DIM, CUR>::pattern() const {
  return _proxy->_mat->_pattern;
}

template<typename T, size_t DIM, size_t CUR>
inline bool Matrix_Ref<T, DIM, CUR>::isLocal(size_type n) {
  return (_proxy->_mat->_pattern.index_to_unit(n, _proxy->_viewspec) ==
          _proxy->_mat->_myid);
}

template<typename T, size_t DIM, size_t CUR>
inline bool Matrix_Ref<T, DIM, CUR>::isLocal(size_t dim, size_type n) {
  return _proxy->_mat->_pattern.is_local(
           n,
           _proxy->_mat->_myid,
           dim,
           _proxy->_viewspec);
}

template<typename T, size_t DIM, size_t CUR>
template <int level>
inline dash::HView<Matrix<T, DIM>, level, DIM> Matrix_Ref<T, DIM, CUR>::hview() {
  return dash::HView<Matrix<T, DIM>, level, DIM>(*this);
}

// Local_Ref
// Partial Specialization for value deferencing.

template <typename T, size_t DIM>
inline T * Local_Ref<T, DIM, 0>::at_(size_t pos) {
  if (!(pos < _proxy->_mat->size())) {
    throw std::out_of_range("Position for Local_Ref.at_ out of range");
  }
  return &(_proxy->_mat->lbegin()[pos]);
}

template <typename T, size_t DIM>
inline T Local_Ref<T, DIM, 0>::operator() {
  T ret = *at_(_proxy->_mat->_pattern.local_at_(_proxy->_coord, _proxy->_viewspec));
  delete _proxy;
  return ret;
}

template <typename T, size_t DIM>
inline T Local_Ref<T, DIM, 0>::operator=(const T value) {
  T* ref = at_(_proxy->_mat->_pattern.local_at_(_proxy->_coord, _proxy->_viewspec));
  *ref = value;
  delete _proxy;
  return value;
}

// Matrix_Ref
// Partial Specialization for value deferencing.
template <typename T, size_t DIM>
inline GlobRef<T> Matrix_Ref<T, DIM, 0 >::at_(size_t unit, size_t elem) const {
  return _proxy->_mat->begin().get(unit, elem);
}

template <typename T, size_t DIM>
inline Matrix_Ref<T, DIM, 0>::operator T() {
  GlobRef<T> ref = at_(
      _proxy->_mat->_pattern.atunit_(_proxy->_coord, _proxy->_viewspec),
      _proxy->_mat->_pattern.at_(_proxy->_coord, _proxy->_viewspec)
  );
  delete _proxy;  
  return ref;
}

template <typename T, size_t DIM>
inline T Matrix_Ref<T, DIM, 0>::operator=(const T value) {
  GlobRef<T> ref = at_(_proxy->_mat->_pattern.atunit_(_proxy->_coord, _proxy->_viewspec),
                       _proxy->_mat->_pattern.at_(_proxy->_coord, _proxy->_viewspec));
  ref = value;
  delete _proxy;
  return value;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Local_Ref<ELEMENT_TYPE, DIM, DIM> Matrix<ELEMENT_TYPE, DIM>::local() const {
  return _local;
}

// Matrix
// Proxy, Matrix_Ref and Local_Ref are created at initialization.

template <typename ELEMENT_TYPE, size_t DIM>
Matrix<ELEMENT_TYPE, DIM>::Matrix(
  const dash::SizeSpec<DIM> & ss,
  const dash::DistSpec<DIM> & ds,
  Team & t,
  const TeamSpec<DIM> & ts)
: _team(t),
  _pattern(ss, ds, ts, t) {
  // Matrix is a friend of class Team
  dart_tea_t teamid = _team._dartid;
  size_t lelem = _pattern.max_ele_per_unit();
  size_t lsize = lelem * sizeof(value_type);

  _dart_gptr = DART_GPTR_NULL;
  dart_ret_t ret = dart_tea_memalloc_aligned(teamid, lsize, &_dart_gptr);

  _ptr = new GlobIter<value_type, DIM>(_pattern, _dart_gptr, 0);
  _size = _pattern.nelem();
  _lsize = lelem;
  _myid = _team.myid();
  _ref._proxy = new Matrix_RefProxy < value_type, DIM > ;
  _ref._proxy->_dim = 0;
  _ref._proxy->_mat = this;
  _ref._proxy->_viewspec = _pattern._viewspec;
  _local = Local_Ref<ELEMENT_TYPE, DIM, DIM>(this);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix<ELEMENT_TYPE, DIM>::~Matrix() {
  dart_tea_t teamid = _team._dartid;
  dart_tea_memfree(teamid, _dart_gptr);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Pattern<DIM> & Matrix<ELEMENT_TYPE, DIM>::pattern() {
  return _pattern;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Team<DIM> & Matrix<ELEMENT_TYPE, DIM>::team() {
  return _team;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline constexpr size_type Matrix<ELEMENT_TYPE, DIM>::size() const noexcept {
  return _size;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline size_type Matrix<ELEMENT_TYPE, DIM>::extent(size_t dim) const noexcept {
  return _size;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline constexpr bool Matrix<ELEMENT_TYPE, DIM>::empty() const noexcept {
  return size() == 0;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline void Matrix<ELEMENT_TYPE, DIM>::barrier() const {
  _team.barrier();
}

template <typename ELEMENT_TYPE, size_t DIM>
inline const_pointer Matrix<ELEMENT_TYPE, DIM>::data() const noexcept {
  return *_ptr;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline iterator Matrix<ELEMENT_TYPE, DIM>::begin() noexcept {
  return iterator(data());
}

template <typename ELEMENT_TYPE, size_t DIM>
inline iterator Matrix<ELEMENT_TYPE, DIM>::end() noexcept {
  return iterator(data() + _size);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline ELEMENT_TYPE * Matrix<ELEMENT_TYPE, DIM>::lbegin() noexcept {
  void * addr;
  dart_gptr_t gptr = _dart_gptr;
  dart_gptr_setunit(&gptr, _myid);
  dart_gptr_getaddr(gptr, &addr);
  return (ELEMENT_TYPE *)(addr);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline ELEMENT_TYPE * Matrix<ELEMENT_TYPE, DIM>::lend() noexcept {
  void * addr;
  dart_gptr_t gptr = _dart_gptr;
  dart_gptr_setunit(&gptr, _myid);
  dart_gptr_incaddr(&gptr, _lsize * sizeof(ELEMENT_TYPE));
  dart_gptr_getaddr(gptr, &addr);
  return (ELEMENT_TYPE *)(addr);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline void Matrix<ELEMENT_TYPE, DIM>::forall(
  ::std::function<void(long long) func) {
  _pattern.forall(func);
}

template <typename ELEMENT_TYPE, size_t DIM>
template<size_t SUBDIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM-1
Matrix<ELEMENT_TYPE, DIM>::sub(
  size_type n) {
  return _ref.sub<SUBDIM>(n);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM-1
Matrix<ELEMENT_TYPE, DIM>::col(
  size_type n) {
  return _ref.sub<1>(n);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM-1
Matrix<ELEMENT_TYPE, DIM>::row(
  size_type n) {
  return _ref.sub<0>(n);
}

template <typename ELEMENT_TYPE, size_t DIM>
template<size_t SUBDIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM>
Matrix<ELEMENT_TYPE, DIM>::submat(
  size_type n,
  size_type range) {
  return _ref.submat<SUBDIM>(n, range);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM>
Matrix<ELEMENT_TYPE, DIM>::rows(
  size_type n,
  size_type range) {
  return _ref.submat<0>(n, range);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM>
Matrix<ELEMENT_TYPE, DIM>::cols(
  size_type n,
  size_type range) {
  return _ref.submat<1>(n, range);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM-1>
Matrix<ELEMENT_TYPE, DIM>::operator[](size_type n) {
  return _ref.operator[](n);
}

template <typename ELEMENT_TYPE, size_t DIM>
template <typename ... Args>
inline reference
Matrix<ELEMENT_TYPE, DIM>::at(Args... args) {
  return _ref.at(args...);
}

template <typename ELEMENT_TYPE, size_t DIM>
template <typename ... Args>
inline reference
Matrix<ELEMENT_TYPE, DIM>::operator()(Args... args) {
  return _ref.at(args...);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Pattern<DIM>
Matrix<ELEMENT_TYPE, DIM>::pattern() const {
  return _pattern;
}

template <typename ELEMENT_TYPE, size_t DIM>
inline bool Matrix<ELEMENT_TYPE, DIM>::isLocal(size_type n) {
  return _ref.islocal(n);
}

template <typename ELEMENT_TYPE, size_t DIM>
inline bool Matrix<ELEMENT_TYPE, DIM>::isLocal(
  size_t dim,
  size_type n) {
  return _ref.islocal(dim, n);
}

template <typename ELEMENT_TYPE, size_t DIM>
template <int level>
inline dash::HView<Matrix<ELEMENT_TYPE, DIM>, level, DIM>
Matrix<ELEMENT_TYPE, DIM>::hview() {
  return _ref.hview<level>();
}

template <typename ELEMENT_TYPE, size_t DIM>
inline Matrix<ELEMENT_TYPE, DIM>::operator Matrix_Ref<ELEMENT_TYPE, DIM, DIM>() {
  return _ref;
}

}  // namespace dash

#endif  // DASH_INTERNAL_MATRIX_INL_H_INCLUDED
