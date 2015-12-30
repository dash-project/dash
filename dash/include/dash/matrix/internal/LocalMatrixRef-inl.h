#ifndef DASH__MATRIX__INTERNAL__LOCAL_MATRIX_REF_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__LOCAL_MATRIX_REF_INL_H_INCLUDED

#include <dash/matrix/LocalMatrixRef.h>


namespace dash {

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::LocalMatrixRef(
  const LocalMatrixRef<T, NumDim, CUR+1, PatternT> & previous,
  index_type coord)
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", CUR);
  // Copy proxy of MatrixRef from last dimension:
  _refview = new MatrixRefView<T, NumDim, PatternT>(
                   *(previous._refview));
  _refview->_coord[_refview->_dim] = coord;
  _refview->_dim++;
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", _refview->_dim);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", _refview->_coord);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", _refview->_viewspec);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::LocalMatrixRef(
  Matrix<T, NumDim, index_type, PatternT> * mat)
{
  _refview = new MatrixRefView<T, NumDim, PatternT>(
                   *(mat->_ref._refview));
  auto local_extents = mat->_pattern.local_extents();
  DASH_LOG_TRACE_VAR("LocalMatrixRef(mat)", local_extents);
  _refview->_viewspec.resize(local_extents);
  DASH_LOG_TRACE_VAR("LocalMatrixRef(mat)", _refview->_viewspec);
}

#if 0
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
#endif

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
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::extent(
  dim_t dim) const noexcept
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
inline
  std::array<
    typename PatternT::size_type,
    NumDim>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::extents() const noexcept
{
  return _refview->_viewspec.extents();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline bool
LocalMatrixRef<T, NumDim, CUR, PatternT>
::empty() const noexcept
{
  return size() == 0;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::const_pointer
LocalMatrixRef<T, NumDim, CUR, PatternT>
::data() const noexcept
{
  // Pointer to first local element disregarding viewspec:
  auto l_begin    =  _refview->_mat->lbegin();
  auto l_vs_first =  _refview->_mat->_pattern.local_at(
                       _refview->_coord,
                       _refview->_viewspec);
  // Pointer to first local element in viewspec:
  return (l_begin + l_vs_first);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::begin() noexcept
{
  // Offset of first local element in viewspec:
  // auto l_vs_first =  _refview->_mat->_pattern.local_at(
  //                      _refview->_coord,
  //                      _refview->_viewspec);
  return iterator(
           _refview->_mat->_glob_mem,
           _refview->_mat->_pattern,
           _refview->_viewspec);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::const_iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::begin() const noexcept
{
  // Offset of first local element in viewspec:
  // auto l_vs_first =  _refview->_mat->_pattern.local_at(
  //                      _refview->_coord,
  //                      _refview->_viewspec);
  return const_iterator(
           _refview->_mat->_glob_mem,
           _refview->_mat->_pattern,
           _refview->_viewspec);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::end() noexcept
{
  return begin() + size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::const_iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::end() const noexcept
{
  return begin() + size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type
inline LocalMatrixRef<T, NumDim, CUR, PatternT>
::size() const noexcept
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
LocalMatrixRef<T, NumDim, CUR-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  index_type pos)
{
  DASH_LOG_TRACE("LocalMatrixRef.[]=()",
                 "curdim:",   CUR,
                 "index:",    pos,
                 "viewspec:", _refview->_viewspec);
  LocalMatrixRef<T, NumDim, CUR-1, PatternT> ref(*this, pos);
  return ref;
#if 0
  DASH_LOG_TRACE("LocalMatrixRef.[]=",
                 "current refview:",
                 "refview.coord:",         _refview->_coord,
                 "refview.dim:",           _refview->_dim,
                 "refview.viewspec.rank:", _refview->_viewspec.rank());
  LocalMatrixRef<T, NumDim, CUR-1, PatternT> ref;
  // Transfer ownership of _refview:
  ref._refview = _refview;
  ref._refview->_coord[_refview->_dim] = n;
  ref._refview->_dim++;
  ref._refview->_viewspec.set_rank(ref._refview->_dim);
  DASH_LOG_TRACE("LocalMatrixRef.[]= >",
                 "refview coords:", ref._refview->_coord);
  return ref;
#endif
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR-1, PatternT>
const LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  index_type pos) const
{
  DASH_LOG_TRACE("LocalMatrixRef.[]()",
                 "curdim:",   CUR,
                 "index:",    pos,
                 "viewspec:", _refview->_viewspec);
  LocalMatrixRef<T, NumDim, CUR-1, PatternT> ref(*this, pos);
  return ref;
#if 0
  LocalMatrixRef<T, NumDim, CUR-1, PatternT> ref;
  ref._refview = new MatrixRefView<T, NumDim, PatternT>(*_refview);
  ref._refview->_coord[_refview->_dim] = n;
  ref._refview->_dim++;
  ref._refview->_viewspec.set_rank(ref._refview->_dim);
  DASH_LOG_TRACE("LocalMatrixRef.[] >",
                 "refview coords:", ref._refview->_coord);
  return ref;
#endif
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
::sub(
  size_type offset,
  size_type extent)
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", SubDimension);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", offset);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", extent);
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
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub >",
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
  return sub<0>(offset, extent);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>::cols(
  size_type offset,
  size_type extent)
{
  return sub<1>(offset, extent);
}

// LocalMatrixRef<T, NumDim, 0>
// Partial Specialization for value deferencing.

template <typename T, dim_t NumDim, class PatternT>
LocalMatrixRef<T, NumDim, 0, PatternT>
::LocalMatrixRef(
  const LocalMatrixRef<T, NumDim, 1, PatternT> & previous,
  typename PatternT::index_type                  coord)
{
  DASH_LOG_TRACE("LocalMatrixRef<0>.(prev)");
  // Copy proxy of MatrixRef from last dimension:
  _refview = new MatrixRefView<T, NumDim, PatternT>(
                   *(previous._refview));
  _refview->_coord[_refview->_dim] = coord;
  _refview->_dim++;
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.(prev)", _refview->_coord);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.(prev)", _refview->_dim);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.(prev)", _refview->_viewspec);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.(prev)", _refview->_l_viewspec);
}

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
  auto l_coords   = _refview->_coord;
  auto l_viewspec = _refview->_l_viewspec;
  auto pattern    = _refview->_mat->_pattern;
  DASH_LOG_TRACE("LocalMatrixRef<0>.T()",
                 "coords:",         l_coords,
                 "local viewspec:", l_viewspec.extents());
  // Local coordinates and local viewspec to local index:
  auto local_index = pattern.local_at(
                       l_coords,
                       l_viewspec);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.T()", local_index);
  T ret = *local_at(local_index);
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
  auto l_coords   = _refview->_coord;
  auto l_viewspec = _refview->_l_viewspec;
  auto pattern    = _refview->_mat->_pattern;
  DASH_LOG_TRACE("LocalMatrixRef<0>.=()",
                 "coords:",         l_coords,
                 "local viewspec:", l_viewspec,
                 "value:",          value);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.=()", l_viewspec);
  // Local coordinates and local viewspec to local index:
  auto local_index = pattern.local_at(
                       l_coords,
                       l_viewspec);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.=", local_index);
  T* ref = local_at(local_index);
  *ref   = value;
  DASH_LOG_TRACE("LocalMatrixRef<0>.=", "delete _refview");
  delete _refview;
  return value;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
LocalMatrixRef<T, NumDim, 0, PatternT>
::operator+=(
  const T & value)
{
  auto l_coords   = _refview->_coord;
  auto l_viewspec = _refview->_l_viewspec;
  auto pattern    = _refview->_mat->_pattern;
  DASH_LOG_TRACE("LocalMatrixRef<0>.+=()",
                 "coords:",         l_coords,
                 "local viewspec:", l_viewspec,
                 "value:",          value);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.+=()", l_viewspec);
  // Local coordinates and local viewspec to local index:
  auto local_index = pattern.local_at(
                       l_coords,
                       l_viewspec);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.=", local_index);
  T* ref  = local_at(local_index);
  *ref   += value;
  DASH_LOG_TRACE("LocalMatrixRef<0>.+=", "delete _refview");
  delete _refview;
  return value;
}

} // namespace dash

#endif  // DASH__MATRIX__INTERNAL__LOCAL_MATRIX_REF_INL_H_INCLUDED
