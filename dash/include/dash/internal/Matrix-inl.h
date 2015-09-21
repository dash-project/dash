#ifndef DASH_INTERNAL_MATRIX_INL_H_INCLUDED
#define DASH_INTERNAL_MATRIX_INL_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

namespace dash {

template<typename T, dim_t NumDim, class PatternT>
MatrixRefProxy<T, NumDim, PatternT>::MatrixRefProxy()
: _dim(0)
{
  DASH_LOG_TRACE("MatrixRefProxy()");
}

template<typename T, dim_t NumDim, class PatternT>
MatrixRefProxy<T, NumDim, PatternT>::MatrixRefProxy(
  Matrix<T, NumDim, index_type, PatternT> * matrix)
: _dim(0),
  _mat(matrix),
  _viewspec(matrix->extents())
{
  DASH_LOG_TRACE_VAR("MatrixRefProxy(matrix)", matrix);
}

template<typename T, dim_t NumDim, class PatternT>
MatrixRefProxy<T, NumDim, PatternT>::MatrixRefProxy(
  const MatrixRefProxy<T, NumDim, PatternT> & other)
: _dim(other._dim),
  _mat(other._mat),
  _coord(other._coord),
  _viewspec(other._viewspec)
{
  DASH_LOG_TRACE_VAR("MatrixRefProxy()", other._mat);
  DASH_LOG_TRACE_VAR("MatrixRefProxy()", other._coord);
  DASH_LOG_TRACE_VAR("MatrixRefProxy()", other._viewspec);
}

template<typename T, dim_t NumDim, class PatternT>
GlobRef<T>
MatrixRefProxy<T, NumDim, PatternT>::global_reference() const
{
  DASH_LOG_TRACE_VAR("MatrixRefProxy.global_reference()", _coord);
  auto pattern       = _mat->pattern();
  auto memory_layout = pattern.memory_layout();
  auto global_index  = memory_layout.at(_coord, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefProxy.global_reference", global_index);
  auto global_begin  = _mat->begin();
  GlobRef<T> ref     = global_begin[global_index];
  DASH_LOG_TRACE_VAR("MatrixRefProxy.global_reference >", (T)ref);
  return ref;
}

////////////////////////////////////////////////////////////////////////
// LocalRef
////////////////////////////////////////////////////////////////////////

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalRef<T, NumDim, CUR, PatternT>::operator
LocalRef<T, NumDim, CUR - 1, PatternT> && ()
{
  LocalRef<T, NumDim, CUR - 1, PatternT> ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalRef<T, NumDim, CUR, PatternT>::operator
MatrixRef<T, NumDim, CUR, PatternT> ()
{
  // Should avoid cast from MatrixRef to LocalRef.
  // Different operation semantics.
  MatrixRef<T, NumDim, CUR, PatternT>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalRef<T, NumDim, CUR, PatternT>::LocalRef(
  Matrix<T, NumDim, index_type, PatternT> * mat)
{
  _proxy = new MatrixRefProxy<T, NumDim, PatternT>(
             *(mat->_ref._proxy));
  std::array<size_t, NumDim> local_extents;
  for (int d = 0; d < NumDim; ++d) {
    local_extents[d] = mat->_pattern.local_extent(d);
  }
  DASH_LOG_TRACE_VAR("LocalRef()", local_extents);
  _proxy->_viewspec.resize(local_extents);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename LocalRef<T, NumDim, CUR, PatternT>::size_type
LocalRef<T, NumDim, CUR, PatternT>::extent(
  dim_t dim) const
{
  if(dim >= NumDim || dim == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LocalRef.extent(): Invalid dimension, " <<
      "expected 0.." << (NumDim - 1) << " " <<
      "got " << dim);
  }
  return _proxy->_mat->_pattern.local_extent(dim);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline
  std::array<
    typename LocalRef<T, NumDim, CUR, PatternT>::size_type,
    NumDim>
LocalRef<T, NumDim, CUR, PatternT>::extents() const noexcept
{
  return _proxy->_mat->_pattern.local_extents();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename LocalRef<T, NumDim, CUR, PatternT>::size_type
LocalRef<T, NumDim, CUR, PatternT>::size() const
{
  return _proxy->_viewspec.size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
T & LocalRef<T, NumDim, CUR, PatternT>::local_at(
  size_type pos)
{
  if (!(pos < _proxy->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return _proxy->_mat->lbegin()[pos];
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
T & LocalRef<T, NumDim, CUR, PatternT>::at(
  Args... args)
{
  if(sizeof...(Args) != (NumDim - _proxy->_dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LocalRef.at(): Invalid number of arguments " <<
      "expected " << (NumDim - _proxy->_dim) << " " <<
      "got " << sizeof...(Args));
  }
  std::array<long long, NumDim> coord = { args... };
  for(auto i = _proxy->_dim; i < NumDim; ++i) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return local_at(
           _proxy->_mat->_pattern.local_at(
             _proxy->_coord,
             _proxy->_viewspec));
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
T & LocalRef<T, NumDim, CUR, PatternT>::operator()(
  Args... args)
{
  return at(args...);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalRef<T, NumDim, CUR-1, PatternT> &&
LocalRef<T, NumDim, CUR, PatternT>::operator[](
  index_type n)
{
  DASH_LOG_TRACE_VAR("LocalRef.[]()", n);
  LocalRef<T, NumDim, CUR-1, PatternT> ref;
  ref._proxy = _proxy;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  return std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalRef<T, NumDim, CUR-1, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::operator[](
  index_type pos) const
{
  DASH_LOG_TRACE_VAR("LocalRef.[]()", pos);
  LocalRef<T, NumDim, CUR-1, PatternT> ref;
  ref._proxy = new MatrixRefProxy<T, NumDim, PatternT>(*_proxy);
  ref._proxy->_coord[_proxy->_dim] = pos;
  ref._proxy->_dim                 =  _proxy->_dim + 1;
  ref._proxy->_viewspec.set_rank(CUR);
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalRef<T, NumDim, NumDim-1, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::sub(
  size_type n)
{
  static_assert(
      NumDim-1 > 0,
      "Dimension too low for sub()");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Illegal sub-dimension");
  dim_t target_dim = SubDimension + _proxy->_dim;
  LocalRef<T, NumDim, NumDim - 1, PatternT> ref;
  MatrixRefProxy<T, NumDim, PatternT> * proxy =
    new MatrixRefProxy<T, NumDim, PatternT>();
  ref._proxy = proxy;
  ref._proxy->_coord[target_dim] = 0;

  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(target_dim, 1, n);
  ref._proxy->_viewspec.set_rank(NumDim-1);

  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_dim = _proxy->_dim + 1;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalRef<T, NumDim, NumDim-1, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::col(
  size_type n)
{
  return sub<1>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalRef<T, NumDim, NumDim-1, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::row(
  size_type n)
{
  return sub<0>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalRef<T, NumDim, NumDim, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::submat(
  size_type offset,
  size_type extent)
{
  DASH_LOG_TRACE_VAR("LocalRef.submat()", SubDimension);
  DASH_LOG_TRACE_VAR("LocalRef.submat()", offset);
  DASH_LOG_TRACE_VAR("LocalRef.submat()", extent);
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension");
  LocalRef<T, NumDim, NumDim, PatternT> ref;
  MatrixRefProxy<T, NumDim, PatternT> * proxy =
    new MatrixRefProxy<T, NumDim, PatternT>();
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);
  ref._proxy = proxy;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(
                          SubDimension,
                          extent,
                          offset);
  DASH_LOG_TRACE_VAR("LocalRef.submat >",
                     ref._proxy->_viewspec.size());
  ref._proxy->_mat = _proxy->_mat;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalRef<T, NumDim, NumDim, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::rows(
  size_type offset,
  size_type extent)
{
  return submat<0>(offset, extent);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalRef<T, NumDim, NumDim, PatternT>
LocalRef<T, NumDim, CUR, PatternT>::cols(
  size_type offset,
  size_type extent)
{
  return submat<1>(offset, extent);
}

// LocalRef<T, NumDim, 0>
// Partial Specialization for value deferencing.

template <typename T, dim_t NumDim, class PatternT>
inline T *
LocalRef<T, NumDim, 0, PatternT>::local_at(
  index_type pos)
{
  if (!(pos < _proxy->_mat->size())) {
    DASH_THROW(
      dash::exception::OutOfRange,
      "Position for LocalRef<0>.local_at out of range");
  }
  return &(_proxy->_mat->lbegin()[pos]);
}

template <typename T, dim_t NumDim, class PatternT>
inline LocalRef<T, NumDim, 0, PatternT>::operator T() {
  T ret = *local_at(_proxy->_mat->_pattern.local_at(
                      _proxy->_coord,
                      _proxy->_viewspec));
  DASH_LOG_TRACE("LocalRef<0>.T()", "delete _proxy");
  delete _proxy;
  return ret;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
LocalRef<T, NumDim, 0, PatternT>::operator=(
  const T & value)
{
  T* ref = local_at(_proxy->_mat->_pattern.local_at(
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

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::MatrixRef(
  const MatrixRef<T, NumDim, CUR+1, PatternT> & previous,
  index_type coord)
{
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", CUR);
  // Copy proxy of MatrixRef from last dimension:
  _proxy = new MatrixRefProxy<T, NumDim, PatternT>(*(previous._proxy));
  _proxy->_coord[_proxy->_dim] = coord;
  _proxy->_dim                 = _proxy->_dim + 1;
  _proxy->_viewspec.set_rank(CUR+1);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _proxy->_dim);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _proxy->_coord);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::operator
MatrixRef<T, NumDim, CUR-1, PatternT> && ()
{
  DASH_LOG_TRACE_VAR("MatrixRef.() &&", CUR);
  MatrixRef<T, NumDim, CUR-1, PatternT> ref =
    MatrixRef<T, NumDim, CUR-1, PatternT>();
  ref._proxy = _proxy;
  return ::std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename MatrixRef<T, NumDim, CUR, PatternT>::pattern_type &
MatrixRef<T, NumDim, CUR, PatternT>::pattern()
{
  // TODO: Should return const ref?
  return _proxy->_mat->_pattern;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
Team &
MatrixRef<T, NumDim, CUR, PatternT>::team()
{
  return _proxy->_mat->_team;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>::size() const noexcept
{
  return _proxy->_viewspec.size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>::extent(
  size_type dim) const noexcept
{
  return _proxy->_viewspec.range[dim];
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
std::array<
  typename MatrixRef<T, NumDim, CUR, PatternT>::size_type,
  NumDim>
MatrixRef<T, NumDim, CUR, PatternT>::extents() const noexcept
{
  return _proxy->_viewspec.range;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr bool
MatrixRef<T, NumDim, CUR, PatternT>::empty() const noexcept
{
  return size() == 0;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
void
MatrixRef<T, NumDim, CUR, PatternT>::barrier() const
{
  _proxy->_mat->_team.barrier();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::operator[](
  index_type pos) const
{
  DASH_LOG_TRACE_VAR("MatrixRef.[]()", pos);
  DASH_LOG_TRACE_VAR("MatrixRef.[]", CUR);
  MatrixRef<T, NumDim, CUR-1, PatternT> ref(*this, pos);
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::sub(
  size_type n)
{
  static_assert(
      NumDim - 1 > 0,
      "Too low dim");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension for sub()");
  dim_t target_dim = SubDimension + _proxy->_dim;

  MatrixRef<T, NumDim, NumDim - 1, PatternT> ref;
  MatrixRefProxy<T, NumDim, PatternT> * proxy =
    new MatrixRefProxy<T, NumDim, PatternT>;

  ref._proxy = proxy;
  ref._proxy->_coord[target_dim] = 0;

  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(target_dim, 1, n);
  ref._proxy->_viewspec.set_rank(NumDim-1);

  ref._proxy->_mat = _proxy->_mat;
  ref._proxy->_dim = _proxy->_dim + 1;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::col(
  size_type n)
{
  return sub<1>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::row(
  size_type n)
{
  return sub<0>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::submat(
  size_type offset,
  size_type extent)
{
  DASH_LOG_TRACE_VAR("MatrixRef.submat()", SubDimension);
  DASH_LOG_TRACE_VAR("MatrixRef.submat()", offset);
  DASH_LOG_TRACE_VAR("MatrixRef.submat()", extent);
  static_assert(
    SubDimension < NumDim && SubDimension >= 0,
    "Wrong sub-dimension for submat()");
  MatrixRef<T, NumDim, NumDim, PatternT> ref;
  MatrixRefProxy<T, NumDim, PatternT> * proxy =
    new MatrixRefProxy<T, NumDim, PatternT>();
  ref._proxy            = proxy;
  ref._proxy->_mat      = _proxy->_mat;
  ref._proxy->_viewspec = _proxy->_viewspec;
  ref._proxy->_viewspec.resize_dim(
                          SubDimension,
                          extent,
                          offset);
  DASH_LOG_TRACE_VAR("MatrixRef.submat >",
                     ref._proxy->_viewspec.size());
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::rows(
  size_type offset,
  size_type extent)
{
  return submat<0>(offset, extent);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>::cols(
  size_type offset,
  size_type extent)
{
  return submat<1>(offset, extent);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::reference
MatrixRef<T, NumDim, CUR, PatternT>::at(Args... args)
{
  if(sizeof...(Args) != (NumDim - _proxy->_dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "MatrixRef.at(): Invalid number of arguments " <<
      "expected " << (NumDim - _proxy->_dim) << " " <<
      "got " << sizeof...(Args));
  }
  ::std::array<index_type, NumDim> coord = { args... };
  for(auto i = _proxy->_dim; i < NumDim; ++i) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return _proxy->global_reference();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::reference
MatrixRef<T, NumDim, CUR, PatternT>::operator()(Args... args)
{
  return at(args...);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::pattern_type
MatrixRef<T, NumDim, CUR, PatternT>::pattern() const
{
  // TODO: Should return const ref?
  return _proxy->_mat->_pattern;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline bool
MatrixRef<T, NumDim, CUR, PatternT>::is_local(
  index_type g_pos) const
{
  return (_proxy->_mat->_pattern.unit_at(g_pos, _proxy->_viewspec) ==
          _proxy->_mat->_myid);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <dim_t Dimension>
inline bool
MatrixRef<T, NumDim, CUR, PatternT>::is_local(
  index_type g_pos) const
{
  return _proxy->_mat->_pattern.has_local_elements(
           Dimension,
           g_pos,
           _proxy->_mat->_myid,
           _proxy->_viewspec);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <int level>
inline dash::HView<
  Matrix<
    T,
    NumDim,
    typename MatrixRef<T, NumDim, CUR, PatternT>::index_type,
    PatternT>,
  level>
MatrixRef<T, NumDim, CUR, PatternT>::hview()
{
  return dash::HView<Matrix<T, NumDim, index_type, PatternT>, level>(*this);
}

// MatrixRef<T, NumDim, 0>
// Partial Specialization for value deferencing.

template <typename T, dim_t NumDim, class PatternT>
MatrixRef<T, NumDim, 0, PatternT>::MatrixRef(
  const MatrixRef<T, NumDim, 1, PatternT> & previous,
  typename PatternT::index_type coord)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", 0);
  // Copy proxy of MatrixRef from last dimension:
  _proxy = new MatrixRefProxy<T, NumDim, PatternT>(
             *(previous._proxy));
  _proxy->_coord[_proxy->_dim] = coord;
  _proxy->_dim                 = _proxy->_dim + 1;
  _proxy->_viewspec.set_rank(1);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _proxy->_coord);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _proxy->_dim);
}

template <typename T, dim_t NumDim, class PatternT>
inline MatrixRef<T, NumDim, 0, PatternT>::operator T()
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T()", _proxy->_coord);
  GlobRef<T> ref = _proxy->global_reference();
  DASH_LOG_TRACE("MatrixRef<0>.T()", "delete _proxy");
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T() delete", _proxy);
  delete _proxy;
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T() >", (T)ref);
  return ref;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>::operator=(
  const T & value)
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

////////////////////////////////////////////////////////////////////////
// Matrix
// Proxy, MatrixRef and LocalRef are created at initialization.
////////////////////////////////////////////////////////////////////////

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>::Matrix(
  Team & t)
: _team(t),
  _size(0),
  _lsize(0),
  _lcapacity(0)
{
  DASH_LOG_TRACE("Matrix()", "default constructor");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>::Matrix(
  const SizeSpec_t & ss,
  const DistributionSpec_t & ds,
  Team & t,
  const TeamSpec_t & ts)
: _team(t),
  _myid(_team.myid()),
  _size(0),
  _lsize(0),
  _lcapacity(0),
  _pattern(ss, ds, ts, t)
{
  DASH_LOG_TRACE_VAR("Matrix()", _myid);
  allocate(_pattern);
  DASH_LOG_TRACE("Matrix()", "Initialized");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>::Matrix(
  const PatternT & pattern)
: _team(pattern.team()),
  _myid(_team.myid()),
  _size(0),
  _lsize(0),
  _lcapacity(0),
  _pattern(pattern)
{
  DASH_LOG_TRACE("Matrix()", "pattern instance constructor");
  allocate(_pattern);
  DASH_LOG_TRACE("Matrix()", "Initialized");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>::~Matrix()
{
  DASH_LOG_TRACE_VAR("Array.~Matrix()", this);
  deallocate();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
bool Matrix<T, NumDim, IndexT, PatternT>::allocate(
  const PatternT & pattern)
{
  DASH_LOG_TRACE("Matrix.allocate()", "pattern", 
                 pattern.memory_layout().extents());
  // Copy sizes from pattern:
  _size            = _pattern.size();
  _lsize           = _pattern.local_size();
  _lcapacity       = _pattern.local_capacity();
  DASH_LOG_TRACE_VAR("Matrix.allocate", _size);
  DASH_LOG_TRACE_VAR("Matrix.allocate", _lsize);
  DASH_LOG_TRACE_VAR("Matrix.allocate", _lcapacity);
  // Allocate and initialize memory ranges:
  _ref._proxy      = new MatrixRefProxy_t(this);
  _glob_mem        = new GlobMem_t(_team, _lcapacity);
  _begin           = GlobIter_t(_glob_mem, _pattern);
  _lbegin          = _glob_mem->lbegin();
  _lend            = _glob_mem->lend();
  // Register team deallocator:
  _team.register_deallocator(
    this, std::bind(&Matrix::deallocate, this));
  // Initialize local proxy object:
  local            = local_type(this);
  DASH_LOG_TRACE("Matrix.allocate() finished");
  return true;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
bool Matrix<T, NumDim, IndexT, PatternT>::allocate(
  size_type nelem,
  dash::DistributionSpec<1> distribution,
  dash::Team & team)
{
  DASH_LOG_TRACE("Matrix.allocate()", nelem);
  // Check requested capacity:
  if (nelem == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "Tried to allocate dash::Matrix with size 0");
  }
  if (_team == dash::Team::Null()) {
    DASH_LOG_TRACE("Matrix.allocate",
                   "initializing pattern with Team::All()");
    _pattern = PatternT(nelem, distribution, team);
  } else {
    DASH_LOG_TRACE("Matrix.allocate",
                   "initializing pattern with initial team");
    _pattern = PatternT(nelem, distribution, _team);
  }
  return allocate(_pattern);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
void Matrix<T, NumDim, IndexT, PatternT>::deallocate()
{
  if (_size == 0) {
    return;
  }
  DASH_LOG_TRACE_VAR("Matrix.deallocate()", this);
  // Remove this function from team deallocator list to avoid
  // double-free:
  _team.unregister_deallocator(
    this, std::bind(&Matrix::deallocate, this));
  // Actual destruction of the array instance:
  delete _glob_mem;
  _size = 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline dash::Team & Matrix<T, NumDim, IndexT, PatternT>::team() {
  return _team;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>::size() const noexcept {
  return _size;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>::local_size() const noexcept {
  return _lsize;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>::local_capacity() const noexcept {
  return _lcapacity;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>::extent(dim_t dim) const noexcept {
  return _pattern.extent(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT>::size_type,
    NumDim>
Matrix<T, NumDim, IndexT, PatternT>::extents() const noexcept {
  return _pattern.extents();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr bool
Matrix<T, NumDim, IndexT, PatternT>::empty() const noexcept {
  return size() == 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline void
Matrix<T, NumDim, IndexT, PatternT>::barrier() const {
  _team.barrier();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_pointer
Matrix<T, NumDim, IndexT, PatternT>::data() const noexcept {
  return _begin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::iterator
Matrix<T, NumDim, IndexT, PatternT>::begin() noexcept {
  return iterator(data());
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT>::begin() const noexcept {
  return const_iterator(data());
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT>::end() const noexcept {
  return const_iterator(data() + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::iterator
Matrix<T, NumDim, IndexT, PatternT>::end() noexcept {
  return iterator(data() + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline T *
Matrix<T, NumDim, IndexT, PatternT>::lbegin() noexcept {
  return _lbegin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline T *
Matrix<T, NumDim, IndexT, PatternT>::lend() noexcept {
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::operator[](size_type pos) {
  DASH_LOG_TRACE_VAR("Matrix.[]()", pos);
  return _ref.operator[](pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template<dim_t SubDimension>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::sub(
  size_type n) {
  return _ref.sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::col(
  size_type n) {
  return _ref.sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::row(
  size_type n) {
  return _ref.sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template<dim_t SubDimension>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::submat(
  size_type offset,
  size_type extent) {
  return _ref.submat<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::rows(
  size_type offset,
  size_type extent) {
  return _ref.submat<0>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>::cols(
  size_type offset,
  size_type extent) {
  return _ref.submat<1>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <typename ... Args>
inline typename Matrix<T, NumDim, IndexT, PatternT>::reference
Matrix<T, NumDim, IndexT, PatternT>::at(Args... args) {
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <typename ... Args>
inline typename Matrix<T, NumDim, IndexT, PatternT>::reference
Matrix<T, NumDim, IndexT, PatternT>::operator()(Args... args) {
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline const PatternT &
Matrix<T, NumDim, IndexT, PatternT>::pattern() const {
  return _pattern;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline bool Matrix<T, NumDim, IndexT, PatternT>::is_local(
  size_type g_pos) const {
  return _ref.is_local(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <dim_t Dimension>
inline bool Matrix<T, NumDim, IndexT, PatternT>::is_local(
  size_type g_pos) const {
  return _ref.is_local<Dimension>(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <int level>
inline dash::HView<Matrix<T, NumDim, IndexT, PatternT>, level>
Matrix<T, NumDim, IndexT, PatternT>::hview() {
  return _ref.hview<level>();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>::operator
MatrixRef<T, NumDim, NumDim, PatternT>() {
  return _ref;
}

}  // namespace dash

#endif  // DASH_INTERNAL_MATRIX_INL_H_INCLUDED
