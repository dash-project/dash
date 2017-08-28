#ifndef DASH__MATRIX__INTERNAL__MATRIX_REF_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__MATRIX_REF_INL_H_INCLUDED

#include <dash/matrix/MatrixRef.h>


namespace dash {

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <class T_>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::MatrixRef(
  const MatrixRef<T_, NumDim, CUR+1, PatternT, MSpaceC> & previous,
  index_type coord)
: _refview(previous._refview)
{
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)()", CUR);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev)", coord);
  dim_t target_dim = NumDim-(CUR+1);
  // Coordinate in active dimension is 0 as it is relative to the
  // MatrixRefView's viewspec which contains the coord as view offset:
  _refview._coord[_refview._dim] = 0;
  _refview._dim++;
  _refview._viewspec.resize_dim(
                           target_dim,
                           _refview._viewspec.offset(target_dim) + coord,
                           1);
  DASH_LOG_TRACE_VAR("MatrixRef.(MatrixRef prev) >", _refview);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <class T_>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::MatrixRef(
  const MatrixRef<T_, NumDim, CUR, PatternT, MSpaceC> & other)
: _refview(other._refview)
{ }

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::operator MatrixRef<T, NumDim, CUR-1, PatternT, MSpaceC> ()
{
  DASH_LOG_TRACE_VAR("MatrixRef.MatrixRef<NDim,NVDim-1>()", CUR);
  MatrixRef<T, NumDim, CUR-1, PatternT, MSpaceC> ref =
    MatrixRef<T, NumDim, CUR-1, PatternT, MSpaceC>();
  ref._refview = _refview;
  DASH_LOG_TRACE("MatrixRef.MatrixRef<NDim,NVDim-1> >");
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr Team &
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::team() const noexcept
{
  return *(_refview._mat->_team);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::size_type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::size() const noexcept
{
  return _refview._viewspec.size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::size_type
constexpr MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::local_size() const noexcept
{
  return dash::local(
           dash::sub<CUR>(
             std::get<CUR>(_refview._viewspec.offsets()),
             ( std::get<CUR>(_refview._viewspec.offsets()) +
               std::get<CUR>(_refview._viewspec.extents()) ),
             *(_refview._mat)
           )
         ).size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::size_type
constexpr MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::local_capacity() const noexcept
{
  return dash::local(
           dash::sub<CUR>(
             std::get<CUR>(_refview._viewspec.offsets()),
             ( std::get<CUR>(_refview._viewspec.offsets()) +
               std::get<CUR>(_refview._viewspec.extents()) ),
             *(_refview._mat)
           )
         ).size();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::size_type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::extent(
  dim_t dim) const noexcept
{
  return _refview._viewspec.extent(dim);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr std::array<
  typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::size_type,
  NumDim>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::extents() const noexcept
{
  return _refview._viewspec.extents();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::index_type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::offset(
  dim_t dim) const noexcept
{
  return _refview._viewspec.offset(dim);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr std::array<
  typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::index_type,
  NumDim >
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::offsets() const noexcept
{
  return _refview._viewspec.offsets();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr bool
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::empty() const noexcept
{
  return size() == 0;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline void
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::barrier() const
{
  _refview._mat->_team->barrier();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr const typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::pattern_type &
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::pattern() const noexcept
{
  return _refview._mat->_pattern;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::const_pointer
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::data() const noexcept
{
  return begin();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::pointer
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::data() noexcept
{
  return begin();
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::const_iterator
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::begin() const noexcept
{
  return const_iterator(
           _refview._mat->_glob_mem,
           _refview._mat->_pattern,
           _refview._viewspec,
           // Relative iterator position
           0,
           // Offset of first element in viewspec, e.g. offset
           // of first element in block:
           _refview._mat->_pattern.global_at(
                _refview._coord,
                _refview._viewspec)
         );
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::iterator
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::begin() noexcept
{
  return iterator(
           _refview._mat->_glob_mem,
           _refview._mat->_pattern,
           _refview._viewspec,
           // Relative iterator position
           0,
           // Offset of first element in viewspec, e.g. offset
           // of first element in block:
           _refview._mat->_pattern.global_at(
                _refview._coord,
                _refview._viewspec)
         );
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::const_iterator
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::end() const noexcept
{
  return const_iterator(
           _refview._mat->_glob_mem,
           _refview._mat->_pattern,
           _refview._viewspec,
           // Relative iterator position
           _refview._viewspec.size(),
           // Offset of first element in viewspec
           _refview._mat->_pattern.global_at(
                  _refview._coord,
                  _refview._viewspec)
         );
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::iterator
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::end() noexcept
{
  return iterator(
           _refview._mat->_glob_mem,
           _refview._mat->_pattern,
           _refview._viewspec,
           // Relative iterator position
           _refview._viewspec.size(),
           // Offset of first element in viewspec
           _refview._mat->_pattern.global_at(
                  _refview._coord,
                  _refview._viewspec)
         );
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::local_type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::sub_local() noexcept
{
  return local_type(this);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
T *
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
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

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
T *
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
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

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim != 0),
  MatrixRef<T, NumDim, __NumViewDim, PatternT, MSpaceC>>::type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::operator[](
  size_type pos)
{
  return MatrixRef<T, NumDim, __NumViewDim, PatternT, MSpaceC>(*this, pos);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim != 0),
  MatrixRef<const T, NumDim, __NumViewDim, PatternT, MSpaceC>>::type
constexpr MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::operator[](size_type pos) const {
  return MatrixRef<const T, NumDim, __NumViewDim, PatternT, MSpaceC>(*this, pos);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim == 0), GlobRef<T> >::type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::operator[](size_type pos)
{
  auto coords = _refview._coord;
  coords[0] = pos;
  return _refview.global_reference(coords);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
typename std::enable_if<(__NumViewDim == 0), GlobRef<const T> >::type
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::operator[](size_type pos) const
{
  auto coords = _refview._coord;
  coords[NumDim - 1] = pos;
  return _refview.global_reference(coords);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <dim_t SubDimension>
MatrixRef<const T, NumDim, NumDim-1, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::sub(
  size_type offset) const
{
  return const_cast<
           MatrixRef<T, NumDim, CUR, PatternT, MSpaceC> *
         >(this)->sub<SubDimension>(offset);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <dim_t SubDimension>
MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
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

  MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC> ref;

  ref._refview._coord[target_dim] = 0;

  ref._refview._viewspec = _refview._viewspec;
  // Offset specified by user is relative to existing offset of the view
  // so slice offset must be applied on the view's current offset in the
  // sub-dimension:
  ref._refview._viewspec.resize_dim(
                           target_dim,
                           _refview._viewspec.offset(target_dim) + offset,
                           1);
  ref._refview._viewspec.set_rank(NumDim-1);

  ref._refview._mat = _refview._mat;
  ref._refview._dim = _refview._dim + 1;

  DASH_LOG_TRACE_VAR("MatrixRef.sub >",
                     ref._refview._viewspec);
  return ref;
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::col(
  size_type column_offset)
{
  return sub<1>(column_offset);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::row(
  size_type row_offset)
{
  return sub<0>(row_offset);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <dim_t SubDimension>
MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
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
  MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC> ref;
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

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::rows(
  size_type offset,
  size_type extent)
{
  return sub<0>(offset, extent);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::cols(
  size_type offset,
  size_type extent)
{
  return sub<1>(offset, extent);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::const_reference
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::at(Args... args) const
{
  return at(std::array<index_type, NumDim> {{
              static_cast<index_type>(args)...
            }});
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::reference
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::at(Args... args)
{
  return at(std::array<index_type, NumDim> {{
              static_cast<index_type>(args)...
            }});
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::const_reference
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::at(const ::std::array<typename PatternT::index_type, NumDim> & coords) const
{
  return _refview.global_reference(coords);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
inline typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::reference
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::at(const ::std::array<typename PatternT::index_type, NumDim> & coords)
{
  return _refview.global_reference(coords);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <typename ... Args>
inline typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::reference
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::operator()(Args... args)
{
  return at(args...);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
constexpr bool
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::is_local(
  index_type g_pos) const
{
  return (_refview._mat->_pattern.unit_at(g_pos, _refview._viewspec) ==
          _refview._mat->_team->myid());
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <dim_t Dimension>
constexpr bool
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::is_local(
  index_type g_pos) const
{
  return _refview._mat->_pattern.has_local_elements(
           Dimension,
           g_pos,
           _refview._mat->_team->myid(),
           _refview._viewspec);
}

template <typename T, dim_t NumDim, dim_t CUR, class PatternT, typename MSpaceC>
template <int level>
inline dash::HView<
  Matrix<
    T,
    NumDim,
    typename MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>::index_type,
    PatternT, MSpaceC>,
  level>
MatrixRef<T, NumDim, CUR, PatternT, MSpaceC>
::hview()
{
  return dash::HView<Matrix<T, NumDim, index_type, PatternT, MSpaceC>, level>(*this);
}

} // namespace dash

#endif // DASH__MATRIX__INTERNAL__MATRIX_REF_INL_H_INCLUDED
