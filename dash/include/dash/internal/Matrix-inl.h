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
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

namespace dash {

template<typename T, size_t NumDimensions>
MatrixRefProxy<T, NumDimensions>::MatrixRefProxy()
: _dim(0)
{
}

template<typename T, size_t NumDimensions>
MatrixRefProxy<T, NumDimensions>::MatrixRefProxy(
  Matrix<T, NumDimensions> * matrix)
: _dim(0),
  _mat(matrix),
  _viewspec(matrix->pattern().viewspec()) 
{
}

template<typename T, size_t NumDimensions>
MatrixRefProxy<T, NumDimensions>::MatrixRefProxy(
  const MatrixRefProxy<T, NumDimensions> & other)
: _dim(other._dim),
  _mat(other._mat),
  _coord(other._coord),
  _viewspec(other._viewspec)
{
}

template<typename T, size_t NumDimensions>
GlobRef<T>
MatrixRefProxy<T, NumDimensions>::global_reference() const {
  DASH_LOG_TRACE_VAR("MatrixRefProxy.global_reference()", _coord);
  auto pattern       = _mat->pattern();
  auto memory_layout = pattern.memory_layout();
  auto global_index  = memory_layout.at(_coord, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefProxy.global_reference()", global_index);
  auto global_begin  = _mat->begin();
  GlobRef<T> ref     = global_begin[global_index];
  return ref;
}

////////////////////////////////////////////////////////////////////////
// LocalRef
////////////////////////////////////////////////////////////////////////

template<typename T, size_t NumDimensions, size_t CUR>
inline LocalRef<T, NumDimensions, CUR>::operator
LocalRef<T, NumDimensions, CUR - 1> && () {
  LocalRef<T, NumDimensions, CUR - 1> ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
LocalRef<T, NumDimensions, CUR>::operator
MatrixRef<T, NumDimensions, CUR> () {
  // Should avoid cast from MatrixRef to LocalRef.
  // Different operation semantics.
  MatrixRef<T, NumDimensions, CUR>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
LocalRef<T, NumDimensions, CUR>::LocalRef(
  Matrix<T, NumDimensions> * mat) {
  _proxy = new MatrixRefProxy < T, NumDimensions >;
  *_proxy = *(mat->_ref._proxy);

  std::array<size_t, NumDimensions> local_extents;
  for (int d = 0; d < NumDimensions; ++d) {
    local_extents[d] = mat->_pattern.local_extent(d);
  }
  _proxy->_viewspec.resize(local_extents);
}

template<typename T, size_t NumDimensions, size_t CUR>
long long LocalRef<T, NumDimensions, CUR>::extent(
  size_t dim) const
{
  if(dim >= NumDimensions || dim == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LocalRef.extent(): Invalid dimension, " <<
      "expected 0.." << (NumDimensions - 1) << " " <<
      "got " << dim);
  }
  return _proxy->_mat->_pattern.local_extent(dim);
}

template<typename T, size_t NumDimensions, size_t CUR>
size_t LocalRef<T, NumDimensions, CUR>::size() const
{
  return _proxy->_viewspec.size();
}

template<typename T, size_t NumDimensions, size_t CUR>
T & LocalRef<T, NumDimensions, CUR>::at_(
  size_type pos)
{
  if (!(pos < _proxy->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return _proxy->_mat->lbegin()[pos];
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
T & LocalRef<T, NumDimensions, CUR>::at(
  Args... args)
{
  if(sizeof...(Args) != (NumDimensions - _proxy->_dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LocalRef.at(): Invalid number of arguments " <<
      "expected " << (NumDimensions - _proxy->_dim) << " " <<
      "got " << sizeof...(Args));
  }
  std::array<long long, NumDimensions> coord = { args... };
  for(int i=_proxy->_dim;i<NumDimensions;i++) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return at_(
      _proxy->_mat->_pattern.local_at(
        _proxy->_coord,
        _proxy->_viewspec));
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
T & LocalRef<T, NumDimensions, CUR>::operator()(
  Args... args)
{
  return at(args...);
}

template<typename T, size_t NumDimensions, size_t CUR>
LocalRef<T, NumDimensions, CUR-1> &&
LocalRef<T, NumDimensions, CUR>::operator[](
  size_t n)
{
  LocalRef<T, NumDimensions, CUR-1>  ref;
  ref._proxy = _proxy;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  return std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
LocalRef<T, NumDimensions, CUR-1>
LocalRef<T, NumDimensions, CUR>::operator[](
  size_t pos) const
{
  LocalRef<T, NumDimensions, CUR-1> ref;
  ref._proxy = new MatrixRefProxy<T, NumDimensions>(*_proxy);
  ref._proxy->_coord[_proxy->_dim] = pos;
  ref._proxy->_dim                 =  _proxy->_dim + 1;
  ref._proxy->_viewspec.set_rank(CUR);
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
LocalRef<T, NumDimensions, NumDimensions-1>
LocalRef<T, NumDimensions, CUR>::sub(
  size_type n)
{
  static_assert(
      NumDimensions - 1 > 0,
      "Dimension too low for sub()");
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Illegal sub-dimension");
  size_t target_dim = SubDimension + _proxy->_dim;
  LocalRef<T, NumDimensions, NumDimensions - 1> ref;
  MatrixRefProxy<T, NumDimensions> * proxy =
    new MatrixRefProxy<T, NumDimensions>();
  ref._proxy = proxy;
  ref._proxy->_coord[target_dim] = 0;

  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(target_dim, 1, n);
  ref._proxy->_viewspec.set_rank(NumDimensions-1);

  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_dim = _proxy->_dim + 1;
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline LocalRef<T, NumDimensions, NumDimensions-1>
LocalRef<T, NumDimensions, CUR>::col(
  size_type n)
{
  return sub<1>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline LocalRef<T, NumDimensions, NumDimensions-1>
LocalRef<T, NumDimensions, CUR>::row(
  size_type n)
{
  return sub<0>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
LocalRef<T, NumDimensions, NumDimensions>
LocalRef<T, NumDimensions, CUR>::submat(
  size_type n,
  size_type range)
{
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Wrong sub-dimension");
  LocalRef<T, NumDimensions, NumDimensions> ref;
  MatrixRefProxy<T, NumDimensions> * proxy =
    new MatrixRefProxy<T, NumDimensions>();
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);
  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(SubDimension, range, n);
  ref._proxy->_mat = _proxy->_mat;
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
LocalRef<T, NumDimensions, NumDimensions>
LocalRef<T, NumDimensions, CUR>::rows(
  size_type n,
  size_type range)
{
  return submat<0>(n, range);
}

template<typename T, size_t NumDimensions, size_t CUR>
LocalRef<T, NumDimensions, NumDimensions>
LocalRef<T, NumDimensions, CUR>::cols(
  size_type n,
  size_type range)
{
  return submat<1>(n, range);
}

// LocalRef<T, NumDimensions, 0>
// Partial Specialization for value deferencing.

template <typename T, size_t NumDimensions>
inline T *
LocalRef<T, NumDimensions, 0>::at_(
  size_t pos)
{
  if (!(pos < _proxy->_mat->size())) {
    DASH_THROW(
      dash::exception::OutOfRange,
      "Position for LocalRef<0>.at_ out of range");
  }
  return &(_proxy->_mat->lbegin()[pos]);
}

template <typename T, size_t NumDimensions>
inline LocalRef<T, NumDimensions, 0>::operator T() {
  T ret = *at_(_proxy->_mat->_pattern.local_at(
                 _proxy->_coord,
                 _proxy->_viewspec));
  DASH_LOG_TRACE("LocalRef<0>.T()", "delete _proxy");
  delete _proxy;
  return ret;
}

template <typename T, size_t NumDimensions>
inline T
LocalRef<T, NumDimensions, 0>::operator=(
  const T value)
{
  T* ref = at_(_proxy->_mat->_pattern.local_at(
                 _proxy->_coord,
                 _proxy->_viewspec));
  *ref = value;
  DASH_LOG_TRACE("LocalRef<0>.=", "delete _proxy");
  delete _proxy;
  return value;
}

////////////////////////////////////////////////////////////////////////
// MatrixRef
////////////////////////////////////////////////////////////////////////

template<typename T, size_t NumDimensions, size_t CUR>
MatrixRef<T, NumDimensions, CUR>::MatrixRef(
  const MatrixRef<T, NumDimensions, CUR+1> & previous,
  long long coord)
{
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", CUR);
  // Copy proxy of MatrixRef from last dimension:
  _proxy = new MatrixRefProxy<T, NumDimensions>(*(previous._proxy));
  _proxy->_coord[_proxy->_dim] = coord;
  _proxy->_dim                 = _proxy->_dim + 1;
  _proxy->_viewspec.set_rank(CUR+1);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _proxy->_dim);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _proxy->_coord);
}

template<typename T, size_t NumDimensions, size_t CUR>
MatrixRef<T, NumDimensions, CUR>::operator
MatrixRef<T, NumDimensions, CUR-1> && ()
{
  DASH_LOG_TRACE_VAR("MatrixRef.() &&", CUR);
  MatrixRef<T, NumDimensions, CUR-1> ref =
    MatrixRef<T, NumDimensions, CUR-1>();
  ref._proxy = _proxy;
  return ::std::move(ref);
}

template<typename T, size_t NumDimensions, size_t CUR>
Pattern<NumDimensions> &
MatrixRef<T, NumDimensions, CUR>::pattern()
{
  return _proxy->_mat->_pattern;
}

template<typename T, size_t NumDimensions, size_t CUR>
Team &
MatrixRef<T, NumDimensions, CUR>::team()
{
  return _proxy->_mat->_team;
}

template<typename T, size_t NumDimensions, size_t CUR>
constexpr typename MatrixRef<T, NumDimensions, CUR>::size_type
MatrixRef<T, NumDimensions, CUR>::size() const noexcept
{
  return _proxy->_size;
}

template<typename T, size_t NumDimensions, size_t CUR>
typename MatrixRef<T, NumDimensions, CUR>::size_type
MatrixRef<T, NumDimensions, CUR>::extent(
  size_type dim) const noexcept
{
  return _proxy->_viewspec.range[dim];
}

template<typename T, size_t NumDimensions, size_t CUR>
constexpr bool MatrixRef<T, NumDimensions, CUR>::empty() const noexcept
{
  return size() == 0;
}

template<typename T, size_t NumDimensions, size_t CUR>
void MatrixRef<T, NumDimensions, CUR>::barrier() const
{
  _proxy->_mat->_team.barrier();
}

/*
template<typename T, size_t NumDimensions, size_t CUR>
MatrixRef<T, NumDimensions, CUR-1> &&
MatrixRef<T, NumDimensions, CUR>::operator[](
  size_t pos)
{
  DASH_LOG_TRACE_VAR("MatrixRef.[]() &&", pos);
  DASH_LOG_TRACE_VAR("MatrixRef.[] &&", CUR);

  DASH_LOG_TRACE_VAR("MatrixRef.[] &&", _proxy);
  _proxy->_coord[_proxy->_dim] = pos;
  DASH_LOG_TRACE_VAR("MatrixRef.[] &&", _proxy->_coord);
  _proxy->_dim++;
  DASH_LOG_TRACE_VAR("MatrixRef.[] &&", _proxy->_dim);

  MatrixRef<T, NumDimensions, CUR-1> ref;
  ref._proxy = _proxy;
  return std::move(ref);
}
*/

template<typename T, size_t NumDimensions, size_t CUR>
MatrixRef<T, NumDimensions, CUR-1>
MatrixRef<T, NumDimensions, CUR>::operator[](
  size_t pos) const
{
  DASH_LOG_TRACE_VAR("MatrixRef<D>.[]()", pos);
  DASH_LOG_TRACE_VAR("MatrixRef<D>.[]", CUR);
  MatrixRef<T, NumDimensions, CUR-1> ref(*this, pos);
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
MatrixRef<T, NumDimensions, NumDimensions-1>
MatrixRef<T, NumDimensions, CUR>::sub(
  size_type n)
{
  static_assert(
      NumDimensions - 1 > 0,
      "Too low dim");
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Wrong sub-dimension for sub()");
  size_t target_dim = SubDimension + _proxy->_dim;

  MatrixRef<T, NumDimensions, NumDimensions - 1> ref;
  MatrixRefProxy<T, NumDimensions> * proxy =
    new MatrixRefProxy<T, NumDimensions>;

  ref._proxy = proxy;
  ref._proxy->_coord[target_dim] = 0;

  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(target_dim, 1, n);
  ref._proxy->_viewspec.set_rank(NumDimensions-1);

  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_dim = _proxy->_dim + 1;
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline MatrixRef<T, NumDimensions, NumDimensions-1>
MatrixRef<T, NumDimensions, CUR>::col(
  size_type n)
{
  return sub<1>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline MatrixRef<T, NumDimensions, NumDimensions-1>
MatrixRef<T, NumDimensions, CUR>::row(
  size_type n)
{
  return sub<0>(n);
}

template<typename T, size_t NumDimensions, size_t CUR>
template<size_t SubDimension>
MatrixRef<T, NumDimensions, NumDimensions>
MatrixRef<T, NumDimensions, CUR>::submat(
  size_type n,
  size_type range)
{
  static_assert(
      SubDimension < NumDimensions && SubDimension >= 0,
      "Wrong sub-dimension for submat()");
  MatrixRef<T, NumDimensions, NumDimensions> ref;
  MatrixRefProxy<T, NumDimensions> * proxy =
    new MatrixRefProxy < T, NumDimensions >();

  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(SubDimension, range, n);
  ref._proxy->_mat = _proxy->_mat;
  return ref;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline MatrixRef<T, NumDimensions, NumDimensions>
MatrixRef<T, NumDimensions, CUR>::rows(
  size_type n,
  size_type range)
{
  return submat<0>(n, range);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline MatrixRef<T, NumDimensions, NumDimensions>
MatrixRef<T, NumDimensions, CUR>::cols(
  size_type n,
  size_type range)
{
  return submat<1>(n, range);
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
inline typename MatrixRef<T, NumDimensions, CUR>::reference
MatrixRef<T, NumDimensions, CUR>::at(Args... args)
{
  if(sizeof...(Args) != (NumDimensions - _proxy->_dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "MatrixRef.at(): Invalid number of arguments " <<
      "expected " << (NumDimensions - _proxy->_dim) << " " <<
      "got " << sizeof...(Args));
  }
  ::std::array<long long, NumDimensions> coord = { args... };
  for(int i = _proxy->_dim; i < NumDimensions; ++i) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return _proxy->global_reference();
}

template<typename T, size_t NumDimensions, size_t CUR>
template<typename ... Args>
inline typename MatrixRef<T, NumDimensions, CUR>::reference
MatrixRef<T, NumDimensions, CUR>::operator()(Args... args)
{
  return at(args...);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline Pattern<NumDimensions>
MatrixRef<T, NumDimensions, CUR>::pattern() const
{
  return _proxy->_mat->_pattern;
}

template<typename T, size_t NumDimensions, size_t CUR>
inline bool
MatrixRef<T, NumDimensions, CUR>::is_local(
  size_type pos)
{
  return (_proxy->_mat->_pattern.unit_at(pos, _proxy->_viewspec) ==
          _proxy->_mat->_myid);
}

template<typename T, size_t NumDimensions, size_t CUR>
inline bool
MatrixRef<T, NumDimensions, CUR>::is_local(
  size_t dim,
  size_type pos)
{
  return _proxy->_mat->_pattern.is_local(
           pos,
           _proxy->_mat->_myid,
           dim,
           _proxy->_viewspec);
}

template<typename T, size_t NumDimensions, size_t CUR>
template <int level>
inline dash::HView<Matrix<T, NumDimensions>, level>
MatrixRef<T, NumDimensions, CUR>::hview()
{
  return dash::HView<Matrix<T, NumDimensions>, level>(*this);
}

// MatrixRef<T, NumDimensions, 0>
// Partial Specialization for value deferencing.

template<typename T, size_t NumDimensions>
MatrixRef<T, NumDimensions, 0>::MatrixRef(
  const MatrixRef<T, NumDimensions, 1> & previous,
  long long coord)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", 0);
  // Copy proxy of MatrixRef from last dimension:
  _proxy = new MatrixRefProxy<T, NumDimensions>(*(previous._proxy));
  _proxy->_coord[_proxy->_dim] = coord;
  _proxy->_dim                 = _proxy->_dim + 1;
  _proxy->_viewspec.set_rank(1);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _proxy->_coord);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _proxy->_dim);
}

template <typename T, size_t NumDimensions>
inline MatrixRef<T, NumDimensions, 0>::operator T()
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T()", _proxy->_coord);
  GlobRef<T> ref = _proxy->global_reference();
  DASH_LOG_TRACE("MatrixRef<0>.T()", "delete _proxy");
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T() delete", _proxy);
  delete _proxy;
  return ref;
}

template <typename T, size_t NumDimensions>
inline T
MatrixRef<T, NumDimensions, 0>::operator=(
  const T value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.=", _proxy->_coord);
  GlobRef<T> ref = _proxy->global_reference();
  ref = value;
  DASH_LOG_TRACE("MatrixRef<0>.=", "delete _proxy");
  DASH_LOG_TRACE_VAR("MatrixRef<0>.= delete", _proxy);
  delete _proxy;
  return value;
}

template <typename ElementType, size_t NumDimensions>
inline LocalRef<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::local() const
{
  return _local;
}

////////////////////////////////////////////////////////////////////////
// Matrix
// Proxy, MatrixRef and LocalRef are created at initialization.
////////////////////////////////////////////////////////////////////////

template <typename ElementType, size_t NumDimensions>
Matrix<ElementType, NumDimensions>::Matrix(
  const dash::SizeSpec<NumDimensions> & ss,
  const dash::DistributionSpec<NumDimensions> & ds,
  Team & t,
  const TeamSpec<NumDimensions> & ts)
: _team(t),
  _pattern(ss, ds, ts, t),
  _local_mem_size(_pattern.local_capacity() * sizeof(value_type)),
  _glob_mem(_team, _local_mem_size)
{
  dart_team_t teamid = _team.dart_id();
  DASH_LOG_TRACE_VAR("Matrix()", teamid);
  DASH_LOG_TRACE_VAR("Matrix()", _local_mem_size);
  _dart_gptr     = DART_GPTR_NULL;
  dart_ret_t ret = dart_team_memalloc_aligned(
                     teamid,
                     _local_mem_size,
                     &_dart_gptr);
  _size          = _pattern.capacity();
  _myid          = _team.myid();
  DASH_LOG_TRACE_VAR("Matrix()", _size);
  _ptr           = new GlobIter_t(&_glob_mem, _pattern, 0lu);
  _ref._proxy    = new MatrixRefProxy_t(this);
  _local         = LocalRef_t(this);
}

template <typename ElementType, size_t NumDimensions>
inline Matrix<ElementType, NumDimensions>::~Matrix() {
  dart_team_t teamid = _team.dart_id();
  dart_team_memfree(teamid, _dart_gptr);
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
  return _pattern.extent(dim);
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
inline typename Matrix<ElementType, NumDimensions>::const_iterator
Matrix<ElementType, NumDimensions>::begin() const noexcept {
  return const_iterator(data());
}

template <typename ElementType, size_t NumDimensions>
inline typename Matrix<ElementType, NumDimensions>::const_iterator
Matrix<ElementType, NumDimensions>::end() const noexcept {
  return const_iterator(data() + _size);
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
template<size_t SubDimension>
inline MatrixRef<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::sub(
  size_type n) {
  return _ref.sub<SubDimension>(n);
}

template <typename ElementType, size_t NumDimensions>
inline MatrixRef<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::col(
  size_type n) {
  return _ref.sub<1>(n);
}

template <typename ElementType, size_t NumDimensions>
inline MatrixRef<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::row(
  size_type n) {
  return _ref.sub<0>(n);
}

template <typename ElementType, size_t NumDimensions>
template<size_t SubDimension>
inline MatrixRef<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::submat(
  size_type n,
  size_type range) {
  return _ref.submat<SubDimension>(n, range);
}

template <typename ElementType, size_t NumDimensions>
inline MatrixRef<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::rows(
  size_type n,
  size_type range) {
  return _ref.submat<0>(n, range);
}

template <typename ElementType, size_t NumDimensions>
inline MatrixRef<ElementType, NumDimensions, NumDimensions>
Matrix<ElementType, NumDimensions>::cols(
  size_type n,
  size_type range) {
  return _ref.submat<1>(n, range);
}

template <typename ElementType, size_t NumDimensions>
inline MatrixRef<ElementType, NumDimensions, NumDimensions-1>
Matrix<ElementType, NumDimensions>::operator[](size_type pos) {
  DASH_LOG_TRACE_VAR("Matrix.[]()", pos);
  return _ref.operator[](pos);
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
inline const Pattern<NumDimensions> &
Matrix<ElementType, NumDimensions>::pattern() const {
  return _pattern;
}

template <typename ElementType, size_t NumDimensions>
inline bool Matrix<ElementType, NumDimensions>::is_local(size_type n) {
  return _ref.is_local(n);
}

template <typename ElementType, size_t NumDimensions>
inline bool Matrix<ElementType, NumDimensions>::is_local(
  size_t dim,
  size_type n) {
  return _ref.is_local(dim, n);
}

template <typename ElementType, size_t NumDimensions>
template <int level>
inline dash::HView<Matrix<ElementType, NumDimensions>, level>
Matrix<ElementType, NumDimensions>::hview() {
  return _ref.hview<level>();
}

template <typename ElementType, size_t NumDimensions>
inline Matrix<ElementType, NumDimensions>::operator
MatrixRef<ElementType, NumDimensions, NumDimensions>() {
  return _ref;
}

}  // namespace dash

#endif  // DASH_INTERNAL_MATRIX_INL_H_INCLUDED
