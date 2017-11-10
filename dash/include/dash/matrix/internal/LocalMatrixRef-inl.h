 #ifndef DASH__MATRIX__INTERNAL__LOCAL_MATRIX_REF_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__LOCAL_MATRIX_REF_INL_H_INCLUDED

#include <dash/matrix/LocalMatrixRef.h>
#include <cassert>


namespace dash {

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <class T_>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::LocalMatrixRef(
  const LocalMatrixRef<T_, NumDim, CUR+1, PatternT> & previous,
  size_type coord)
  : _refview(previous._refview)
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", CUR);
  _refview._coord[_refview._dim] = coord;
  _refview._dim++;
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", _refview._dim);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", _refview._coord);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.(prev)", _refview._viewspec);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <class T_>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::LocalMatrixRef(
  Matrix<T_, NumDim, index_type, PatternT> * mat)
  : _refview(mat->_ref._refview)
{
  auto local_extents = mat->_pattern.local_extents();
  DASH_LOG_TRACE_VAR("LocalMatrixRef(mat)", local_extents);
  // Global offset to first local element is missing:
  // _refview._viewspec.resize(local_extents);
  std::array<index_type, NumDim> local_begin_coords = {{ }};
  auto local_offsets = mat->_pattern.global(local_begin_coords);
  _refview._viewspec = ViewSpec_t(local_offsets, local_extents);
  DASH_LOG_TRACE_VAR("LocalMatrixRef(mat) >", _refview._viewspec);
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
  _refview._viewspec.resize(local_extents);
}
#endif

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::block(
  const std::array<index_type, NumDim> & block_lcoords)
{
  // Note: This is equivalent to
  //   foreach (d in 0 ... NumDim):
  //     view = view.sub<d>(block_view.offset(d),
  //                        block_view.extent(d));
  //
  DASH_LOG_TRACE("LocalMatrixRef.block()", block_lcoords);
  const auto& pattern = _refview._mat->_pattern;
  auto block_lindex   = pattern.blockspec().at(block_lcoords);
  DASH_LOG_TRACE("LocalMatrixRef.block()", block_lindex);
  // Global view of local block:
  auto l_block_g_view = pattern.local_block(block_lindex);
  // Local view of local block:
  auto l_block_l_view = pattern.local_block_local(block_lindex);
  // Return a view specified by the block's viewspec:
  ViewT<NumDim> view;
  view._refview             = MatrixRefView_t(_refview._mat);
  view._refview._viewspec   = l_block_g_view;
  view._refview._l_viewspec = l_block_l_view;
  DASH_LOG_TRACE("LocalMatrixRef.block >",
                 "global:",
                 "offsets:", view._refview._viewspec.offsets(),
                 "extents:", view._refview._viewspec.extents(),
                 "local:",
                 "offsets:", view._refview._l_viewspec.offsets(),
                 "extents:", view._refview._l_viewspec.extents());
  return view;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::block(
  index_type block_lindex)
{
  // Note: This is equivalent to
  //   foreach (d in 0 ... NumDim):
  //     view = view.sub<d>(block_view.offset(d),
  //                        block_view.extent(d));
  //
  DASH_LOG_TRACE("LocalMatrixRef.block()", block_lindex);
  const auto& pattern = _refview._mat->_pattern;
  // Global view of local block:
  auto l_block_g_view = pattern.local_block(block_lindex);
  // Local view of local block:
  auto l_block_l_view = pattern.local_block_local(block_lindex);
  // Return a view specified by the block's viewspec:
  ViewT<NumDim> view;
  view._refview             = MatrixRefView_t(_refview._mat);
  view._refview._viewspec   = l_block_g_view;
  view._refview._l_viewspec = l_block_l_view;
  DASH_LOG_TRACE("LocalMatrixRef.block >",
                 "global:",
                 "offsets:", view._refview._viewspec.offsets(),
                 "extents:", view._refview._viewspec.extents(),
                 "local:",
                 "offsets:", view._refview._l_viewspec.offsets(),
                 "extents:", view._refview._l_viewspec.extents());
  return view;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator LocalMatrixRef<T, NumDim, CUR-1, PatternT> && () &&
{
  return LocalMatrixRef<T, NumDim, CUR-1, PatternT>(std::move(_refview));
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator MatrixRef<T, NumDim, CUR, PatternT> ()
{
  // Should avoid cast from MatrixRef to LocalMatrixRef.
  // Different operation semantics.
  MatrixRef<T, NumDim, CUR, PatternT>  ref;
  ref._refview = _refview;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::extent(
  dim_t dim) const noexcept
{
  if (dim >= NumDim || dim < 0) {
    DASH_LOG_ERROR(
      "LocalMatrixRef.offset(): Invalid dimension, expected 0..",
      (NumDim - 1), "got ", dim);
    assert(false);
  }
  return _refview._viewspec.extent(dim);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr std::array<typename PatternT::size_type, NumDim>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::extents() const noexcept
{
  return _refview._viewspec.extents();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::index_type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::offset(
  dim_t dim) const noexcept
{
  if(dim >= NumDim || dim < 0) {
    DASH_LOG_ERROR(
      "LocalMatrixRef.offset(): Invalid dimension, expected 0..",
      (NumDim - 1), "got ", dim);
    assert(false);
  }
  return _refview._viewspec.offset(dim);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr std::array<typename PatternT::index_type, NumDim>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::offsets() const noexcept
{
  return _refview._viewspec.offsets();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr bool
LocalMatrixRef<T, NumDim, CUR, PatternT>
::empty() const noexcept
{
  return size() == 0;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::begin() noexcept
{
  return iterator(
    _refview._mat->_glob_mem,
    _refview._mat->_pattern,
    _refview._viewspec,
    // iterator position in view index space
    0,
    // view index start offset
    _refview._mat->_pattern.local_at(
                          _refview._coord,
                          _refview._l_viewspec)
  );
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename LocalMatrixRef<T, NumDim, CUR, PatternT>::const_iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::begin() const noexcept
{
  return const_iterator(
    _refview._mat->_glob_mem,
    _refview._mat->_pattern,
    _refview._viewspec,
    // iterator position in view index space
    0,
    // view index start offset
    _refview._mat->_pattern.local_at(
                          _refview._coord,
                          _refview._l_viewspec)
  );
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename LocalMatrixRef<T, NumDim, CUR, PatternT>::iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::end() noexcept
{
  return begin() + size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr typename LocalMatrixRef<T, NumDim, CUR, PatternT>::const_iterator
LocalMatrixRef<T, NumDim, CUR, PatternT>
::end() const noexcept
{
  return begin() + size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline T *
LocalMatrixRef<T, NumDim, CUR, PatternT>
::lbegin() noexcept
{
  return begin().local();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr const T *
LocalMatrixRef<T, NumDim, CUR, PatternT>
::lbegin() const noexcept
{
  return begin().local();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline T *
LocalMatrixRef<T, NumDim, CUR, PatternT>
::lend() noexcept
{
  return end().local();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr const T *
LocalMatrixRef<T, NumDim, CUR, PatternT>
::lend() const noexcept
{
  return end().local();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename LocalMatrixRef<T, NumDim, CUR, PatternT>::size_type
constexpr LocalMatrixRef<T, NumDim, CUR, PatternT>
::size() const noexcept
{
  return _refview._viewspec.size();
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::local_at(
  size_type pos)
{
  if (!(pos < _refview._mat->local_size())) {
    throw std::out_of_range("Out of range");
  }
  DASH_LOG_TRACE("LocalMatrixRef.local_at()",
                 "pos:", pos);
  return _refview._mat->lbegin()[pos];
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
const T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::local_at(
  size_type pos) const
{
  if (!(pos < _refview._mat->local_size())) {
    throw std::out_of_range("Out of range");
  }
  DASH_LOG_TRACE("LocalMatrixRef.local_at()",
                 "pos:", pos);
  return _refview._mat->lbegin()[pos];
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
inline T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::at(
  Args... args)
{
  static_assert(sizeof...(Args) == CUR,
      "Invalid number of arguments in variadic access method!");

        auto  l_coords   = _refview._coord;
  const auto& l_viewspec = _refview._l_viewspec;
  const auto& pattern    = _refview._mat->_pattern;
  std::array<size_type, CUR> coord = {
    static_cast<size_type>(args)...
  };

  for(auto i = 0; i < CUR; ++i) {
    l_coords[NumDim - CUR + i] = coord[i];
  }
  DASH_LOG_TRACE("LocalMatrixRef.at()",
                 "ndim:",     NumDim,
                 "curdim:",   CUR,
                 "l_coords:",   _refview._coord,
                 "l_viewspec:", _refview._l_viewspec);
  return local_at(
           pattern.local_at(
             l_coords,
             l_viewspec));
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<typename ... Args>
inline T & LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator()(
  Args... args)
{
  return at(args...);
}


template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim > 0),
  LocalMatrixRef<T, NumDim, __NumViewDim, PatternT>>::type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  size_type pos)
{
  DASH_LOG_TRACE("LocalMatrixRef.[]=()",
                 "curdim:",   CUR,
                 "index:",    pos,
                 "viewspec:", _refview._viewspec);
  return LocalMatrixRef<T, NumDim, CUR-1, PatternT>(*this, pos);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim == 0), T&>::type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  size_type pos)
{
        auto  l_coords   = _refview._coord;
  const auto& l_viewspec = _refview._l_viewspec;
  const auto& pattern    = _refview._mat->_pattern;
  DASH_LOG_TRACE("LocalMatrixRef<0>.local()",
                 "coords:",         l_coords,
                 "local viewspec:", l_viewspec.extents());

  l_coords[NumDim-1] = pos;

  // Local coordinates and local viewspec to local index:
  auto local_index = pattern.local_at(
                       l_coords,
                       l_viewspec);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.local()", local_index);
  return local_at(local_index);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim > 0),
  LocalMatrixRef<const T, NumDim, __NumViewDim, PatternT>>::type
constexpr
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](size_type pos) const
{
  return LocalMatrixRef<const T, NumDim, CUR-1, PatternT>(*this, pos);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim == 0), const T&>::type
LocalMatrixRef<T, NumDim, CUR, PatternT>
::operator[](
  size_type pos) const
{
  auto  l_coords   = _refview._coord;
  const auto& l_viewspec = _refview._l_viewspec;
  const auto& pattern    = _refview._mat->_pattern;
  DASH_LOG_TRACE("LocalMatrixRef<0>.local()",
                 "coords:",         l_coords,
                 "local viewspec:", l_viewspec.extents());

  l_coords[NumDim-1] = pos;

  // Local coordinates and local viewspec to local index:
  auto local_index = pattern.local_at(
                       l_coords,
                       l_viewspec);
  DASH_LOG_TRACE_VAR("LocalMatrixRef<0>.local()", local_index);
  return local_at(local_index);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalMatrixRef<T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::sub(
  size_type n)
{
// // following Meyers, Eff. C++ p. 23, Item 3
// using RefType = LocalMatrixRef<T, NumDim, CUR, PatternT>;
// return static_cast<RefType &>(*this).sub<SubDimension>(n);

  static_assert(
      NumDim-1 > 0,
      "Dimension too low for sub()");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Illegal sub-dimension");
  dim_t target_dim = SubDimension + _refview._dim;
  DASH_LOG_TRACE("LocalMatrixRef<N>.sub(n)", "n:", n,
                 "target_dim:", target_dim, "refview.dim:", _refview._dim);

  LocalMatrixRef<T, NumDim, NumDim - 1, PatternT> ref;
  ref._refview._coord[target_dim] = 0;

  ref._refview._viewspec = _refview._viewspec;
  // Offset specified by user is relative to existing offset of the view
  // so slice offset must be applied on the view's current offset in the
  // sub-dimension:
  ref._refview._viewspec.resize_dim(
                           target_dim,
                           _refview._viewspec.offset(target_dim) + n, 1);
  ref._refview._viewspec.set_rank(NumDim-1);

  DASH_LOG_TRACE("LocalMatrixRef<N>.sub(n)", "n:", n,
                 "refview.size:", ref._refview._viewspec.size());
  ref._refview._mat = _refview._mat;
  ref._refview._dim = _refview._dim + 1;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalMatrixRef<const T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::sub(
  size_type n) const
{
  static_assert(
      NumDim-1 > 0,
      "Dimension too low for sub()");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Illegal sub-dimension");
  dim_t target_dim = SubDimension + _refview._dim;
  DASH_LOG_TRACE("LocalMatrixRef<N>.sub(n)", "n:", n,
                 "target_dim:", target_dim, "refview.dim:", _refview._dim);

  LocalMatrixRef<const T, NumDim, NumDim - 1, PatternT> ref;
  ref._refview._coord[target_dim] = 0;

  ref._refview._viewspec = _refview._viewspec;
  // Offset specified by user is relative to existing offset of the view
  // so slice offset must be applied on the view's current offset in the
  // sub-dimension:
  ref._refview._viewspec.resize_dim(
                           target_dim,
                           _refview._viewspec.offset(target_dim) + n, 1);
  ref._refview._viewspec.set_rank(NumDim-1);

  DASH_LOG_TRACE("LocalMatrixRef<N>.sub(n)", "n:", n,
                 "refview.size:", ref._refview._viewspec.size());
  ref._refview._mat = reinterpret_cast<
                        dash::Matrix<
                          const T, NumDim, index_type, pattern_type
                        > * const >(_refview._mat);
  ref._refview._dim = _refview._dim + 1;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::col(
  size_type n)
{
  return sub<1>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr LocalMatrixRef<const T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::col(
  size_type n) const
{
  return sub<1>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
LocalMatrixRef<T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>::row(
  size_type n)
{
  return sub<0>(n);
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr LocalMatrixRef<const T, NumDim, NumDim-1, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>::row(
  size_type n) const
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
// // following Meyers, Eff. C++ p. 23, Item 3
// using RefType = LocalMatrixRef<T, NumDim, CUR, PatternT>;
// return static_cast<const RefType &>(*this).sub<SubDimension>(offset, extent);

  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", SubDimension);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", offset);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", extent);
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension");
  LocalMatrixRef<T, NumDim, NumDim, PatternT> ref;
  ::std::fill(ref._refview._coord.begin(), ref._refview._coord.end(), 0);
  ref._refview._viewspec = _refview._viewspec;
  ref._refview._viewspec.resize_dim(
                            SubDimension,
                            offset,
                            extent);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub >",
                     ref._refview._viewspec.size());
  ref._refview._mat = _refview._mat;
  return ref;
}

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
template<dim_t SubDimension>
LocalMatrixRef<const T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::sub(
  size_type offset,
  size_type extent) const
{
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", SubDimension);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", offset);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub()", extent);
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension");
  LocalMatrixRef<const T, NumDim, NumDim, PatternT> ref;
  ::std::fill(ref._refview._coord.begin(), ref._refview._coord.end(), 0);
  ref._refview._viewspec = _refview._viewspec;
  ref._refview._viewspec.resize_dim(
                            SubDimension,
                            offset,
                            extent);
  DASH_LOG_TRACE_VAR("LocalMatrixRef.sub >",
                     ref._refview._viewspec.size());
  ref._refview._mat = _refview._mat;
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
constexpr LocalMatrixRef<const T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>
::rows(
  size_type offset,
  size_type extent) const
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

template<typename T, dim_t NumDim, dim_t CUR, class PatternT>
constexpr LocalMatrixRef<const T, NumDim, NumDim, PatternT>
LocalMatrixRef<T, NumDim, CUR, PatternT>::cols(
  size_type offset,
  size_type extent) const
{
  return sub<1>(offset, extent);
}

} // namespace dash

#endif  // DASH__MATRIX__INTERNAL__LOCAL_MATRIX_REF_INL_H_INCLUDED
