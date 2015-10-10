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
MatrixRefView<T, NumDim, PatternT>::MatrixRefView()
: _dim(0)
{
  DASH_LOG_TRACE("MatrixRefView()");
}

template<typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>::MatrixRefView(
  Matrix<T, NumDim, index_type, PatternT> * matrix)
: _dim(0),
  _mat(matrix),
  _viewspec(matrix->extents())
{
  DASH_LOG_TRACE_VAR("MatrixRefView(matrix)", matrix);
}

template<typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>::MatrixRefView(
  const MatrixRefView<T, NumDim, PatternT> & other)
: _dim(other._dim),
  _mat(other._mat),
  _coord(other._coord),
  _viewspec(other._viewspec)
{
  DASH_LOG_TRACE_VAR("MatrixRefView()", other._mat);
  DASH_LOG_TRACE_VAR("MatrixRefView()", other._coord);
  DASH_LOG_TRACE_VAR("MatrixRefView()", other._viewspec);
}

template<typename T, dim_t NumDim, class PatternT>
GlobRef<T>
MatrixRefView<T, NumDim, PatternT>::global_reference() const
{
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference()", _coord);
  auto pattern       = _mat->pattern();
  auto memory_layout = pattern.memory_layout();
  // MatrixRef coordinate and viewspec to global linear index:
  auto global_index  = memory_layout.at(_coord, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference", global_index);
  auto global_begin  = _mat->begin();
  // Global reference at global linear index:
  GlobRef<T> ref     = global_begin[global_index];
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference >", (T)ref);
  return ref;
}

////////////////////////////////////////////////////////////////////////
// LocalMatrixRef
////////////////////////////////////////////////////////////////////////

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::LocalMatrixRef(
  Matrix<T, NumDim, index_type, PatternT> * mat)
{
  _refview = new MatrixRefView<T, NumDim, PatternT>(
             *(mat->_ref._refview));
  std::array<size_t, NumDim> local_extents;
  for (int d = 0; d < NumDim; ++d) {
    local_extents[d] = mat->_pattern.local_extent(d);
  }
  DASH_LOG_TRACE_VAR("LocalMatrixRef(mat)", local_extents);
  _refview->_viewspec.resize(local_extents);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::LocalMatrixRef(
  MatrixRef<T, NumDim, CUR, PatternT> * matref)
{
  DASH_THROW(
    dash::exception::NotImplemented,
    "Matrix view projection order matrix.sub().local() "
    "is not supported, yet. Use matrix.local().sub().");

  _refview = new MatrixRefView<T, NumDim, PatternT>(
             *(matref->_refview));
  std::array<size_t, NumDim> local_extents;
  for (int d = 0; d < NumDim; ++d) {
    // TODO: Workaround here, not implemented!
    local_extents[d] = matref->pattern().local_extent(d);
  }
  DASH_LOG_TRACE_VAR("LocalMatrixRef(matref)", local_extents);
  _refview->_viewspec.resize(local_extents);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator LocalMatrixRef<T, NumDim, CUR - 1, PatternT> && ()
{
  LocalMatrixRef<T, NumDim, CUR - 1, PatternT> ref;
  ref._refview = _refview;
  DASH_LOG_TRACE("LocalMatrixRef.&& move");
  return ::std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator MatrixRef<T, NumDim, CUR, PatternT> ()
{
  // Should avoid cast from MatrixRef to LocalMatrixRef.
  // Different operation semantics.
  MatrixRef<T, NumDim, CUR, PatternT>  ref;
  ref._refview = _refview;
  DASH_LOG_TRACE("LocalMatrixRef.MatrixRef move");
  return ::std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline constexpr typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::extent(
  dim_t dim) const
{
  if(dim >= NumDim || dim == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LocalMatrixRef.extent(): Invalid dimension, " <<
      "expected 0.." << (NumDim - 1) << " " <<
      "got " << dim);
  }
  return _refview->_mat->_pattern.local_extent(dim);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline constexpr
  std::array<
    typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type,
    NumDim>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::extents() const noexcept
{
  return _refview->_mat->_pattern.local_extents();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::size() const
{
  return _refview->_viewspec.size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::local_at(
  size_type pos)
{
  if (!(pos < _refview->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return _refview->_mat->lbegin()[pos];
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::at(
  Args... args)
{
  if(sizeof...(Args) != (NumDim - _refview->_dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LocalMatrixRef.at(): Invalid number of arguments " <<
      "expected " << (NumDim - _refview->_dim) << " " <<
      "got " << sizeof...(Args));
  }
  std::array<long long, NumDim> coord = { args... };
  for(auto i = _refview->_dim; i < NumDim; ++i) {
    _refview->_coord[i] = coord[i-_refview->_dim];
  }
  return local_at(
           _refview->_mat->_pattern.local_at(
             _refview->_coord,
             _refview->_viewspec));
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator()(
  Args... args)
{
  return at(args...);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR-1, PatternT> &&
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  index_type n)
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.[]()", n);
  LocalMatrixRef<T, NumDim, CUR-1, PatternT> ref;
  ref._refview = _refview;
  _refview->_coord[_refview->_dim] = n;
  _refview->_dim++;
  DASH_LOG_TRACE("LocalMatrixRef.[]", "move");
  return std::move(ref);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  index_type pos) const
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.[]()", pos);
  LocalMatrixRef<T, NumDim, CUR-1, PatternT> ref;
  ref._refview = new MatrixRefView<T, NumDim, PatternT>(*_refview);
  ref._refview->_coord[_refview->_dim] = pos;
  ref._refview->_dim++;
  ref._refview->_viewspec.set_rank(CUR);
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalMatrixRef<T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::sub(
  size_type n)
{
  static_assert(
      NumDim-1 > 0,
      "Dimension too low for sub()");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Illegal sub-dimension");
  dim_t target_dim = SubDimension + _refview->_dim;
  LocalMatrixRef<T, NumDim, NumDim - 1, PatternT> ref;
  MatrixRefView<T, NumDim, PatternT> * proxy =
    new MatrixRefView<T, NumDim, PatternT>();
  ref._refview = proxy;
  ref._refview->_coord[target_dim] = 0;

  ref._refview->_viewspec = _refview->_viewspec;
  ref._refview->_viewspec.resize_dim(target_dim, n, 1);
  ref._refview->_viewspec.set_rank(NumDim-1);

  ref._refview->_mat = _refview->_mat;
  ref._refview->_dim = _refview->_dim + 1;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalMatrixRef<T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::col(
  size_type n)
{
  return sub<1>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalMatrixRef<T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>::row(
  size_type n)
{
  return sub<0>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalMatrixRef<T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::submat(
  size_type offset,
  size_type extent)
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.submat()", SubDimension);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.submat()", offset);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.submat()", extent);
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension");
  LocalMatrixRef<T, NumDim, NumDim, PatternT> ref;
  MatrixRefView<T, NumDim, PatternT> * proxy =
    new MatrixRefView<T, NumDim, PatternT>();
  ::std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);
  ref._refview = proxy;
  ref._refview->_viewspec = _refview->_viewspec;
  ref._refview->_viewspec.resize_dim(
                            SubDimension,
                            offset,
                            extent);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.submat >",
                     ref._refview->_viewspec.size());
  ref._refview->_mat = _refview->_mat;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::rows(
  size_type offset,
  size_type extent)
{
  return submat<0>(offset, extent);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>::cols(
  size_type offset,
  size_type extent)
{
  return submat<1>(offset, extent);
}

// LocalMatrixRef<T, NumDim, 0>
// Partial Specialization for value deferencing.

template <typename T, dim_t NumDim, class PatternT>
inline T *
LocalMatrixRef<T, NumDim, 0, PatternT>
::local_at(
  index_type pos)
{
  if (!(pos < _refview->_mat->size())) {
    DASH_THROW(
      dash::exception::OutOfRange,
      "Position for LocalMatrixRef<0>.local_at out of range");
  }
  return &(_refview->_mat->lbegin()[pos]);
}

template <typename T, dim_t NumDim, class PatternT>
inline LocalMatrixRef<T, NumDim, 0, PatternT>
::operator T()
{
  T ret = *local_at(_refview->_mat->_pattern.local_at(
                      _refview->_coord,
                      _refview->_viewspec));
  DASH_LOG_TRACE("LocalMatrixRef<0>.T()", "delete _refview");
  delete _refview;
  return ret;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
LocalMatrixRef<T, NumDim, 0, PatternT>
::operator=(
  const T & value)
{
  T* ref = local_at(_refview->_mat->_pattern.local_at(
                      _refview->_coord,
                      _refview->_viewspec));
  *ref = value;
  DASH_LOG_TRACE("LocalMatrixRef<0>.=", "delete _refview");
  delete _refview;
  return value;
}

////////////////////////////////////////////////////////////////////////
// MatrixRef
////////////////////////////////////////////////////////////////////////

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::MatrixRef(
  const MatrixRef<T, NumDim, CUR+1, PatternT> & previous,
  index_type coord)
{
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", CUR);
  // Copy proxy of MatrixRef from last dimension:
  _refview = new MatrixRefView<T, NumDim, PatternT>(*(previous._refview));
  _refview->_coord[_refview->_dim] = coord;
  _refview->_dim++;
  _refview->_viewspec.set_rank(CUR+1);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _refview->_dim);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _refview->_coord);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::operator MatrixRef<T, NumDim, CUR-1, PatternT> && ()
{
  DASH_LOG_TRACE_VAR("MatrixRef.() &&", CUR);
  MatrixRef<T, NumDim, CUR-1, PatternT> ref =
    MatrixRef<T, NumDim, CUR-1, PatternT>();
  ref._refview = _refview;
  DASH_LOG_TRACE("MatrixRef.&&", "move");
  return ::std::move(ref);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
Team &
MatrixRef<T, NumDim, CUR, PatternT>
::team()
{
  return _refview->_mat->_team;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>
::size() const noexcept
{
  return _refview->_viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>
::local_size() const noexcept
{
  // TODO: Should be
  //   sub_local().size();
  DASH_THROW(
    dash::exception::NotImplemented,
    "MatrixRef.local_size: Matrix view projection order "
    "matrix.sub().local() is not supported, yet. Use matrix.local().sub().");
  return _refview->_viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>
::local_capacity() const noexcept
{
  // TODO: Should be
  //   sub_local().capacity();
  DASH_THROW(
    dash::exception::NotImplemented,
    "MatrixRef.local_capacity: Matrix view projection order "
    "matrix.sub().local() is not supported, yet. Use matrix.local().sub().");
  return _refview->_viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>
::extent(
  dim_t dim) const noexcept
{
  return _refview->_viewspec.range[dim];
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr std::array<
  typename MatrixRef<T, NumDim, CUR, PatternT>::size_type,
  NumDim>
MatrixRef<T, NumDim, CUR, PatternT>
::extents() const noexcept
{
  return _refview->_viewspec.range;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr bool
MatrixRef<T, NumDim, CUR, PatternT>
::empty() const noexcept
{
  return size() == 0;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline void
MatrixRef<T, NumDim, CUR, PatternT>
::barrier() const
{
  _refview->_mat->_team.barrier();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline const typename MatrixRef<T, NumDim, CUR, PatternT>::pattern_type &
MatrixRef<T, NumDim, CUR, PatternT>
::pattern() const
{
  // TODO:
  // Should return pattern projected to cartesian space of this view?
  return _refview->_mat->_pattern;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::const_pointer
MatrixRef<T, NumDim, CUR, PatternT>
::data() const noexcept
{
  DASH_LOG_TRACE_VAR("MatrixRef.data()", _refview->_viewspec.extents());
  return GlobIter_t(
           _refview->_mat->_glob_mem,
           _refview->_mat->_pattern,
           _refview->_viewspec);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::const_iterator
MatrixRef<T, NumDim, CUR, PatternT>
::begin() const noexcept
{
  return data();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::iterator
MatrixRef<T, NumDim, CUR, PatternT>
::begin() noexcept
{
  // const to non-const
  return iterator(data());
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::const_iterator
MatrixRef<T, NumDim, CUR, PatternT>
::end() const noexcept
{
  return data() + _refview->_viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::iterator
MatrixRef<T, NumDim, CUR, PatternT>
::end() noexcept
{
  // const to non-const
  return iterator(data() + _refview->_viewspec.size());
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::local_type
MatrixRef<T, NumDim, CUR, PatternT>
::sub_local() noexcept
{
  return local_type(this);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline T *
MatrixRef<T, NumDim, CUR, PatternT>
::lbegin() noexcept
{
  // TODO:
  // Expensive as a new LocalMatrixRef object is created.
  // Note: Not equivalent to
  //
  //   _mat->local.view(_refview)
  //
  // ... as order of projections (slice + local vs. local + slice) matters.
  return sub_local().begin();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline T *
MatrixRef<T, NumDim, CUR, PatternT>
::lend() noexcept
{
  // TODO:
  // Expensive as a new LocalMatrixRef object is created.
  // Note: Not equivalent to
  //
  //   _mat->local.view(_refview)
  //
  // ... as order of projections (slice + local vs. local + slice) matters.
  return sub_local().end();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  index_type pos) const
{
  DASH_LOG_TRACE_VAR("MatrixRef.[]()", pos);
  DASH_LOG_TRACE_VAR("MatrixRef.[]", CUR);
  MatrixRef<T, NumDim, CUR-1, PatternT> ref(*this, pos);
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <dim_t SubDimension>
MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::sub(
  size_type n)
{
  static_assert(
      NumDim - 1 > 0,
      "Too low dim");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension for sub()");
  dim_t target_dim = SubDimension + _refview->_dim;

  MatrixRef<T, NumDim, NumDim - 1, PatternT> ref;
  MatrixRefView<T, NumDim, PatternT> * proxy =
    new MatrixRefView<T, NumDim, PatternT>;

  ref._refview = proxy;
  ref._refview->_coord[target_dim] = 0;

  ref._refview->_viewspec = _refview->_viewspec;
  ref._refview->_viewspec.resize_dim(target_dim, n, 1);
  ref._refview->_viewspec.set_rank(NumDim-1);

  ref._refview->_mat = _refview->_mat;
  ref._refview->_dim = _refview->_dim + 1;
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::col(
  size_type n)
{
  return sub<1>(n);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::row(
  size_type n)
{
  return sub<0>(n);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <dim_t SubDimension>
MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::submat(
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
  MatrixRefView<T, NumDim, PatternT> * proxy =
    new MatrixRefView<T, NumDim, PatternT>();
  ref._refview            = proxy;
  ref._refview->_mat      = _refview->_mat;
  ref._refview->_viewspec = _refview->_viewspec;
  ref._refview->_viewspec.resize_dim(
                            SubDimension,
                            offset,
                            extent);
  DASH_LOG_TRACE_VAR("MatrixRef.submat >",
                     ref._refview->_viewspec.size());
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::rows(
  size_type offset,
  size_type extent)
{
  return submat<0>(offset, extent);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::cols(
  size_type offset,
  size_type extent)
{
  return submat<1>(offset, extent);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::reference
MatrixRef<T, NumDim, CUR, PatternT>
::at(Args... args)
{
  if(sizeof...(Args) != (NumDim - _refview->_dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "MatrixRef.at(): Invalid number of arguments " <<
      "expected " << (NumDim - _refview->_dim) << " " <<
      "got " << sizeof...(Args));
  }
  ::std::array<index_type, NumDim> coord = { args... };
  for(auto i = _refview->_dim; i < NumDim; ++i) {
    _refview->_coord[i] = coord[i-_refview->_dim];
  }
  return _refview->global_reference();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::reference
MatrixRef<T, NumDim, CUR, PatternT>
::operator()(Args... args)
{
  return at(args...);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline bool
MatrixRef<T, NumDim, CUR, PatternT>
::is_local(
  index_type g_pos) const
{
  return (_refview->_mat->_pattern.unit_at(g_pos, _refview->_viewspec) ==
          _refview->_mat->_myid);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <dim_t Dimension>
inline bool
MatrixRef<T, NumDim, CUR, PatternT>
::is_local(
  index_type g_pos) const
{
  return _refview->_mat->_pattern.has_local_elements(
           Dimension,
           g_pos,
           _refview->_mat->_myid,
           _refview->_viewspec);
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
MatrixRef<T, NumDim, CUR, PatternT>
::hview()
{
  return dash::HView<Matrix<T, NumDim, index_type, PatternT>, level>(*this);
}

// MatrixRef<T, NumDim, 0>
// Partial Specialization for value deferencing.

template <typename T, dim_t NumDim, class PatternT>
MatrixRef<T, NumDim, 0, PatternT>
::MatrixRef(
  const MatrixRef<T, NumDim, 1, PatternT> & previous,
  typename PatternT::index_type             coord)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", 0);
  // Copy proxy of MatrixRef from last dimension:
  _refview = new MatrixRefView<T, NumDim, PatternT>(
             *(previous._refview));
  _refview->_coord[_refview->_dim] = coord;
  _refview->_dim                   = _refview->_dim + 1;
  _refview->_viewspec.set_rank(1);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _refview->_coord);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _refview->_dim);
}

template <typename T, dim_t NumDim, class PatternT>
inline MatrixRef<T, NumDim, 0, PatternT>
::operator T()
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T()", _refview->_coord);
  GlobRef<T> ref = _refview->global_reference();
  DASH_LOG_TRACE("MatrixRef<0>.T()", "delete _refview");
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T() delete", _refview);
  delete _refview;
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T() >", (T)ref);
  return ref;
}

template <typename T, dim_t NumDim, class PatternT>
inline MatrixRef<T, NumDim, 0, PatternT>
::operator GlobPtr<T>()
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.GlobPtr()", _refview->_coord);
  GlobRef<T> ref = _refview->global_reference();
  return ref.gptr();
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator=(
  const T & value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.=", _refview->_coord);
  GlobRef<T> ref = _refview->global_reference();
  ref = value;
  DASH_LOG_TRACE("MatrixRef<0>.=", "delete _refview");
  DASH_LOG_TRACE_VAR("MatrixRef<0>.= delete", _refview);
  delete _refview;
  return value;
}

////////////////////////////////////////////////////////////////////////
// Matrix
// Proxy, MatrixRef and LocalMatrixRef are created at initialization.
////////////////////////////////////////////////////////////////////////

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::Matrix(
  Team & t)
: _team(t),
  _size(0),
  _lsize(0),
  _lcapacity(0)
{
  DASH_LOG_TRACE("Matrix()", "default constructor");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::Matrix(
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
inline Matrix<T, NumDim, IndexT, PatternT>
::Matrix(
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
inline Matrix<T, NumDim, IndexT, PatternT>
::~Matrix()
{
  DASH_LOG_TRACE_VAR("Array.~Matrix()", this);
  deallocate();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
bool Matrix<T, NumDim, IndexT, PatternT>
::allocate(
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
  _ref._refview    = new MatrixRefView_t(this);
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
bool Matrix<T, NumDim, IndexT, PatternT>
::allocate(
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
void Matrix<T, NumDim, IndexT, PatternT>
::deallocate()
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
inline dash::Team & Matrix<T, NumDim, IndexT, PatternT>
::team() {
  return _team;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::size() const noexcept
{
  return _size;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::local_size() const noexcept
{
  return _lsize;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::local_capacity() const noexcept
{
  return _lcapacity;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::extent(
  dim_t dim) const noexcept
{
  return _pattern.extent(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline constexpr
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT>::size_type,
    NumDim>
Matrix<T, NumDim, IndexT, PatternT>
::extents() const noexcept
{
  return _pattern.extents();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
constexpr bool
Matrix<T, NumDim, IndexT, PatternT>
::empty() const noexcept
{
  return size() == 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline void
Matrix<T, NumDim, IndexT, PatternT>
::barrier() const {
  _team.barrier();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_pointer
Matrix<T, NumDim, IndexT, PatternT>
::data() const noexcept
{
  return _begin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::iterator
Matrix<T, NumDim, IndexT, PatternT>
::begin() noexcept
{
  return iterator(data());
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT>
::begin() const noexcept
{
  return const_iterator(data());
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT>
::end() const noexcept
{
  return const_iterator(data() + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::iterator
Matrix<T, NumDim, IndexT, PatternT>
::end() noexcept
{
  return iterator(data() + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::local_type
Matrix<T, NumDim, IndexT, PatternT>
::sub_local() noexcept
{
  return local_type(this);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline T *
Matrix<T, NumDim, IndexT, PatternT>
::lbegin() noexcept
{
  return _lbegin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline T *
Matrix<T, NumDim, IndexT, PatternT>
::lend() noexcept
{
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::operator[](size_type pos)
{
  DASH_LOG_TRACE_VAR("Matrix.[]()", pos);
  return _ref.operator[](pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template<dim_t SubDimension>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::sub(
  size_type n)
{
  return _ref.sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::col(
  size_type n)
{
  return _ref.sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::row(
  size_type n)
{
  return _ref.sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template<dim_t SubDimension>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::submat(
  size_type offset,
  size_type extent)
{
  return _ref.submat<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::rows(
  size_type offset,
  size_type extent)
{
  return _ref.submat<0>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::cols(
  size_type offset,
  size_type extent)
{
  return _ref.submat<1>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <typename ... Args>
inline typename Matrix<T, NumDim, IndexT, PatternT>::reference
Matrix<T, NumDim, IndexT, PatternT>
::at(Args... args)
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <typename ... Args>
inline typename Matrix<T, NumDim, IndexT, PatternT>::reference
Matrix<T, NumDim, IndexT, PatternT>
::operator()(Args... args)
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline const PatternT &
Matrix<T, NumDim, IndexT, PatternT>
::pattern() const
{
  return _pattern;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline bool Matrix<T, NumDim, IndexT, PatternT>
::is_local(
  size_type g_pos) const
{
  return _ref.is_local(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <dim_t Dimension>
inline bool Matrix<T, NumDim, IndexT, PatternT>
::is_local(
  size_type g_pos) const
{
  return _ref.is_local<Dimension>(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template <int level>
inline dash::HView<Matrix<T, NumDim, IndexT, PatternT>, level>
Matrix<T, NumDim, IndexT, PatternT>
::hview()
{
  return _ref.hview<level>();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::operator MatrixRef<T, NumDim, NumDim, PatternT>()
{
  return _ref;
}

}  // namespace dash

#endif  // DASH_INTERNAL_MATRIX_INL_H_INCLUDED
