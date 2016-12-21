#ifndef DASH__MATRIX__INTERNAL__MATRIX_REF_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__MATRIX_REF_INL_H_INCLUDED

#include <dash/matrix/MatrixRef.h>


namespace dash {

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::MatrixRef(
  const MatrixRef<T, NumDim, CUR+1, PatternT> & previous,
  index_type coord)
  : _refview(previous._refview)
{
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", CUR);
  _refview._coord[_refview._dim] = coord;
  _refview._dim++;
  _refview._viewspec.set_rank(_refview._dim);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _refview._dim);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", _refview._coord);
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
  return _refview._mat->_team;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>
::size() const noexcept
{
  return _refview._viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
inline MatrixRef<T, NumDim, CUR, PatternT>
::local_size() const noexcept
{
  // TODO: Should be
  //   sub_local().size();
  DASH_THROW(
    dash::exception::NotImplemented,
    "MatrixRef.local_size: Matrix view projection order "
    "matrix.sub().local() is not supported, yet. Use matrix.local().sub().");
  return _refview._viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
inline MatrixRef<T, NumDim, CUR, PatternT>
::local_capacity() const noexcept
{
  // TODO: Should be
  //   sub_local().capacity();
  DASH_THROW(
    dash::exception::NotImplemented,
    "MatrixRef.local_capacity: Matrix view projection order "
    "matrix.sub().local() is not supported, yet. Use matrix.local().sub().");
  return _refview._viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::size_type
MatrixRef<T, NumDim, CUR, PatternT>
::extent(
  dim_t dim) const noexcept
{
  return _refview._viewspec.extent(dim);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline std::array<
  typename MatrixRef<T, NumDim, CUR, PatternT>::size_type,
  NumDim>
MatrixRef<T, NumDim, CUR, PatternT>
::extents() const noexcept
{
  return _refview._viewspec.extents();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline bool
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
  _refview._mat->_team.barrier();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline const typename MatrixRef<T, NumDim, CUR, PatternT>::pattern_type &
MatrixRef<T, NumDim, CUR, PatternT>
::pattern() const
{
  return _refview._mat->_pattern;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::const_pointer
MatrixRef<T, NumDim, CUR, PatternT>
::data() const noexcept
{
  return begin();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::const_iterator
MatrixRef<T, NumDim, CUR, PatternT>
::begin() const noexcept
{
  DASH_LOG_TRACE("MatrixRef.begin()",
                 "viewspec:", _refview._viewspec,
                 "coord:",    _refview._coord);
  // Offset of first element in viewspec, e.g. offset of first element in
  // block:
  auto g_vs_begin_idx = _refview._mat->_pattern.global_at(
                          _refview._coord,
                          _refview._viewspec);
  DASH_LOG_TRACE("MatrixRef.begin",
                 "iterator offset:", g_vs_begin_idx);
  auto git_begin = GlobViewIter_t(
                     _refview._mat->_glob_mem,
                     _refview._mat->_pattern,
                     _refview._viewspec,
                     0,                        // relative iterator position
                     g_vs_begin_idx            // view index offset 
                   );
  DASH_LOG_TRACE("MatrixRef.begin= >", git_begin);
  return git_begin;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::iterator
MatrixRef<T, NumDim, CUR, PatternT>
::begin() noexcept
{
  DASH_LOG_TRACE("MatrixRef.begin=()",
                 "viewspec:", _refview._viewspec,
                 "coord:",    _refview._coord);
  // Offset of first element in viewspec, e.g. offset of first element in
  // block:
  auto g_vs_begin_idx = _refview._mat->_pattern.global_at(
                          _refview._coord,
                          _refview._viewspec);
  auto git_begin = GlobViewIter_t(
                     _refview._mat->_glob_mem,
                     _refview._mat->_pattern,
                     _refview._viewspec,
                     0,                         // relative iterator position
                     g_vs_begin_idx             // view index offset 
                   );
  DASH_LOG_TRACE("MatrixRef.begin= >", git_begin);
  return git_begin;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::const_iterator
MatrixRef<T, NumDim, CUR, PatternT>
::end() const noexcept
{
  DASH_LOG_TRACE("MatrixRef.end()",
                 "viewspec:", _refview._viewspec,
                 "coord:",    _refview._coord);
  // Offset of first element in viewspec, e.g. offset of first element in
  // block:
  auto g_vs_begin_idx = _refview._mat->_pattern.global_at(
                          _refview._coord,
                          _refview._viewspec);
  auto git_end = GlobViewIter_t(
                   _refview._mat->_glob_mem,
                   _refview._mat->_pattern,
                   _refview._viewspec,
                   _refview._viewspec.size(), // relative iterator position
                   g_vs_begin_idx              // view index offset
                 );
  DASH_LOG_TRACE("MatrixRef.end >", git_end);
  return git_end;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::iterator
MatrixRef<T, NumDim, CUR, PatternT>
::end() noexcept
{
  DASH_LOG_TRACE("MatrixRef.end=()",
                 "viewspec:", _refview._viewspec,
                 "coord:",    _refview._coord);
  // Offset of first element in viewspec, e.g. offset of first element in
  // block:
  auto g_vs_begin_idx = _refview._mat->_pattern.global_at(
                          _refview._coord,
                          _refview._viewspec);
  auto git_end = GlobViewIter_t(
                   _refview._mat->_glob_mem,
                   _refview._mat->_pattern,
                   _refview._viewspec,
                   _refview._viewspec.size(), // relative iterator position
                   g_vs_begin_idx              // view index offset
                 );
  DASH_LOG_TRACE("MatrixRef.end= >", git_end);
  return git_end;
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
  index_type pos)
{
  DASH_LOG_TRACE_VAR("MatrixRef.[]=()", pos);
  DASH_LOG_TRACE_VAR("MatrixRef.[]=", CUR);
  MatrixRef<T, NumDim, CUR-1, PatternT> ref(*this, pos);
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
const MatrixRef<T, NumDim, CUR-1, PatternT>
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
  size_type offset)
{
  static_assert(
      NumDim - 1 > 0,
      "Too low dim");
  static_assert(
      SubDimension < NumDim && SubDimension >= 0,
      "Wrong sub-dimension for sub()");
  DASH_LOG_TRACE("MatrixRef.sub()",
                 "dim:",    SubDimension,
                 "offset:", offset);
  dim_t target_dim = SubDimension + _refview._dim;
  DASH_LOG_TRACE("MatrixRef<N>.sub(n)", "n:", offset,
                 "target_dim:", target_dim, "refview.dim:", _refview._dim);

  MatrixRef<T, NumDim, NumDim-1, PatternT> ref;

  ref._refview._coord[target_dim] = 0;

  ref._refview._viewspec = _refview._viewspec;
  // Offset specified by user is relative to existing offset of the view
  // so slice offset must be applied on the view's current offset in the
  // sub-dimension:
  ref._refview._viewspec.resize_dim(
                           target_dim,
                           _refview._viewspec.offset(target_dim) + offset, 1);
  ref._refview._viewspec.set_rank(NumDim-1);

  ref._refview._mat = _refview._mat;
  ref._refview._dim = _refview._dim + 1;

  DASH_LOG_TRACE_VAR("MatrixRef.sub >",
                     ref._refview._viewspec);
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::col(
  size_type column_offset)
{
  return sub<1>(column_offset);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::row(
  size_type row_offset)
{
  return sub<0>(row_offset);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <dim_t SubDimension>
MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::sub(
  size_type offset,
  size_type extent)
{
  DASH_LOG_TRACE("MatrixRef.sub()",
                 "dim:",    SubDimension,
                 "offset:", offset,
                 "extent:", extent);
  static_assert(
    SubDimension < NumDim && SubDimension >= 0,
    "Wrong sub-dimension for sub()");
  MatrixRef<T, NumDim, NumDim, PatternT> ref;
  ref._refview._mat      = _refview._mat;
  ref._refview._viewspec = _refview._viewspec;
  ref._refview._viewspec.resize_dim(
                            SubDimension,
                            offset,
                            extent);
  DASH_LOG_TRACE_VAR("MatrixRef.sub >",
                     ref._refview._viewspec);
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::rows(
  size_type offset,
  size_type extent)
{
  return sub<0>(offset, extent);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
MatrixRef<T, NumDim, CUR, PatternT>
::cols(
  size_type offset,
  size_type extent)
{
  return sub<1>(offset, extent);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::reference
MatrixRef<T, NumDim, CUR, PatternT>
::at(Args... args)
{
  if(sizeof...(Args) != (NumDim - _refview._dim)) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "MatrixRef.at(): Invalid number of arguments " <<
      "expected " << (NumDim - _refview._dim) << " " <<
      "got " << sizeof...(Args));
  }
  ::std::array<index_type, NumDim> coords = {{ static_cast<index_type>(args)... }};
  return at(coords);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
inline typename MatrixRef<T, NumDim, CUR, PatternT>::reference
MatrixRef<T, NumDim, CUR, PatternT>
::at(const ::std::array<typename PatternT::index_type, NumDim> & coords)
{
  return _refview.global_reference(coords);
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
  return (_refview._mat->_pattern.unit_at(g_pos, _refview._viewspec) ==
          _refview._mat->_team->myid());
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT>
template <dim_t Dimension>
inline bool
MatrixRef<T, NumDim, CUR, PatternT>
::is_local(
  index_type g_pos) const
{
  return _refview._mat->_pattern.has_local_elements(
           Dimension,
           g_pos,
           _refview._mat->_team->myid(),
           _refview._viewspec);
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
  : _refview(previous._refview)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", 0);
  // Copy proxy of MatrixRef from last dimension:
  _refview._coord[_refview._dim] = coord;
  _refview._dim++;
  _refview._viewspec.set_rank(NumDim);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _refview._coord);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.(MatrixRef prev)", _refview._dim);
}

template <typename T, dim_t NumDim, class PatternT>
inline bool
MatrixRef<T, NumDim, 0, PatternT>
::is_local() const
{
  return (_refview._mat->_pattern.unit_at(
                                     _refview._coord,
                                     _refview._viewspec) ==
          _refview._mat->_team->myid());
}

template <typename T, dim_t NumDim, class PatternT>
inline MatrixRef<T, NumDim, 0, PatternT>
::operator T() const
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T()", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  DASH_LOG_TRACE_VAR("MatrixRef<0>.T() >", ref);
  return ref;
}

template <typename T, dim_t NumDim, class PatternT>
inline MatrixRef<T, NumDim, 0, PatternT>
::operator GlobPtr<T, PatternT>() const
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.GlobPtr()", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  return GlobPtr<T, PatternT>(ref.dart_gptr());
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator=(
  const T & value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.=", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  ref = value;
  return value;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator+=(
  const T & value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.+=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.+=", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  ref += value;
  return value;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator+(
  const T & value)
{
  auto res  = self_t(*this);
  res      += value;
  return res;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator-=(
  const T & value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.-=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.-=", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  ref -= value;
  return value;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator-(
  const T & value)
{
  auto res  = self_t(*this);
  res      -= value;
  return res;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator*=(
  const T & value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>.*=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>.*=", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  ref *= value;
  return value;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator*(
  const T & value)
{
  auto res  = self_t(*this);
  res      *= value;
  return res;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator/=(
  const T & value)
{
  DASH_LOG_TRACE_VAR("MatrixRef<0>./=()", value);
  DASH_LOG_TRACE_VAR("MatrixRef<0>./=", _refview._coord);
  GlobRef<T> ref = _refview.global_reference();
  ref /= value;
  return value;
}

template <typename T, dim_t NumDim, class PatternT>
inline T
MatrixRef<T, NumDim, 0, PatternT>
::operator/(
  const T & value)
{
  auto res  = self_t(*this);
  res      /= value;
  return res;
}

} // namespace dash

#endif // DASH__MATRIX__INTERNAL__MATRIX_REF_INL_H_INCLUDED
