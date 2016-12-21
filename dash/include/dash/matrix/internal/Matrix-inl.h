#ifndef DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobMem.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

#include <dash/iterator/GlobIter.h>

#include <dash/Matrix.h>

#include <type_traits>


namespace dash {

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::Matrix(
  Team & t)
: _team(&t),
  _size(0),
  _lsize(0),
  _lcapacity(0),
  _pattern(
    SizeSpec_t(),
    DistributionSpec_t(),
    *_team),
  _glob_mem(nullptr),
  _lbegin(nullptr),
  _lend(nullptr)
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
: _team(&t),
  _size(0),
  _lsize(0),
  _lcapacity(0),
  _pattern(ss, ds, ts, t),
  _glob_mem(nullptr),
  _lbegin(nullptr),
  _lend(nullptr)
{
  DASH_LOG_TRACE_VAR("Matrix()", _team->myid());
  allocate(_pattern);
  DASH_LOG_TRACE("Matrix()", "Initialized");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::Matrix(
  const PatternT & pattern)
: _team(&pattern.team()),
  _size(0),
  _lsize(0),
  _lcapacity(0),
  _pattern(pattern),
  _glob_mem(nullptr),
  _lbegin(nullptr),
  _lend(nullptr)
{
  DASH_LOG_TRACE("Matrix()", "pattern instance constructor");
  allocate(_pattern);
  DASH_LOG_TRACE("Matrix()", "Initialized");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::~Matrix()
{
  DASH_LOG_TRACE_VAR("Matrix.~Matrix()", this);
  deallocate();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::block(
  const std::array<index_type, NumDim> & block_gcoords)
{
  DASH_LOG_TRACE("Matrix.block()", "gcoords:", block_gcoords);
  // Note: This is equivalent to
  //   foreach (d in 0 ... NumDimensions):
  //     view = view.sub<d>(block_view.offset(d),
  //                        block_view.extent(d));
  //
  auto block_gindex = pattern().blockspec().at(block_gcoords);
  DASH_LOG_TRACE_VAR("Matrix.block()",  block_gindex);
  // Resolve the block's viewspec:
  auto block_view = pattern().block(block_gindex);
  // Return a view specified by the block's viewspec:
  view_type<NumDim> view;
  view._refview = MatrixRefView_t(this);
  view._refview._viewspec = block_view;
  DASH_LOG_TRACE("Matrix.block >", block_view);
  return view;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::block(
  index_type block_gindex)
{
  // Note: This is equivalent to
  //   foreach (d in 0 ... NumDimensions):
  //     view = view.sub<d>(block_view.offset(d),
  //                        block_view.extent(d));
  //
  DASH_LOG_TRACE("Matrix.block()", "gindex:", block_gindex);
  // Resolve the block's viewspec:
  auto block_view = pattern().block(block_gindex);
  // Return a view specified by the block's viewspec:
  view_type<NumDim> view;
  view._refview = MatrixRefView_t(this);
  view._refview._viewspec = block_view;
  DASH_LOG_TRACE("Matrix.block >", block_view);
  return view;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
bool Matrix<T, NumDim, IndexT, PatternT>
::allocate(
  const PatternT & pattern)
{
  DASH_LOG_TRACE("Matrix.allocate()", "pattern",
                 pattern.memory_layout().extents());
  if (&_pattern != &pattern) {
    DASH_LOG_TRACE("Matrix.allocate()", "using specified pattern");
    _pattern = pattern;
  }
  // Copy sizes from pattern:
  _size            = _pattern.size();
  _team            = &(_pattern.team());
  _lsize           = _pattern.local_size();
  _lcapacity       = _pattern.local_capacity();
  DASH_LOG_TRACE_VAR("Matrix.allocate", _size);
  DASH_LOG_TRACE_VAR("Matrix.allocate", _lsize);
  DASH_LOG_TRACE_VAR("Matrix.allocate", _lcapacity);
  // Allocate and initialize memory
  _glob_mem        = new GlobMem_t(_lcapacity, _pattern.team());
  _begin           = GlobIter_t(_glob_mem, _pattern);
  _lbegin          = _glob_mem->lbegin();
  _lend            = _glob_mem->lend();
  // Register team deallocator:
  _team->register_deallocator(
    this, std::bind(&Matrix::deallocate, this));
  // Initialize local proxy object:
  _ref._refview    = MatrixRefView_t(this);
  local            = local_type(this);
  DASH_LOG_TRACE("Matrix.allocate() finished");
  return true;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
bool Matrix<T, NumDim, IndexT, PatternT>
::allocate(
  const SizeSpec<NumDim, typename PatternT::size_type>  & sizespec,
  const DistributionSpec<NumDim>                        & distribution,
  const TeamSpec<NumDim, typename PatternT::index_type> & teamspec,
  dash::Team                                            & team)
{
  DASH_LOG_TRACE("Matrix.allocate()", sizespec.extents());
  // Check requested capacity:
  if (sizespec.size() == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "Tried to allocate dash::Matrix with size 0");
  }
  if (_team == nullptr || *_team == dash::Team::Null()) {
    DASH_LOG_TRACE("Matrix.allocate",
                   "initializing pattern with Team::All()");
    _team    = &team;
    _pattern = PatternT(sizespec, distribution, teamspec, team);
  } else {
    DASH_LOG_TRACE("Matrix.allocate",
                   "initializing pattern with initial team");
    _pattern = PatternT(sizespec, distribution, teamspec, *_team);
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
  // Assure all units are synchronized before deallocation, otherwise
  // other units might still be working on the matrix:
  if (dash::is_initialized()) {
    barrier();
  }
  // Remove this function from team deallocator list to avoid
  // double-free:
  _team->unregister_deallocator(
    this, std::bind(&Matrix::deallocate, this));
  // Actual destruction of the array instance:
  delete _glob_mem;
  _size = 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline dash::Team & Matrix<T, NumDim, IndexT, PatternT>
::team() {
  return *_team;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::size() const noexcept
{
  return _size;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::local_size() const noexcept
{
  return _lsize;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::local_capacity() const noexcept
{
  return _lcapacity;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::size_type
Matrix<T, NumDim, IndexT, PatternT>
::extent(
  dim_t dim) const noexcept
{
  return _pattern.extent(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT>::size_type,
    NumDim>
Matrix<T, NumDim, IndexT, PatternT>
::extents() const noexcept
{
  return _pattern.extents();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline bool
Matrix<T, NumDim, IndexT, PatternT>
::empty() const noexcept
{
  return size() == 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline void
Matrix<T, NumDim, IndexT, PatternT>
::barrier() const {
  _team->barrier();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::iterator
Matrix<T, NumDim, IndexT, PatternT>
::begin() noexcept
{
  return iterator(_begin);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT>
::begin() const noexcept
{
  return const_iterator(_begin);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT>
::end() const noexcept
{
  return const_iterator(_begin + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline typename Matrix<T, NumDim, IndexT, PatternT>::iterator
Matrix<T, NumDim, IndexT, PatternT>
::end() noexcept
{
  return iterator(_begin + _size);
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
inline const T *
Matrix<T, NumDim, IndexT, PatternT>
::lbegin() const noexcept
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
inline const T *
Matrix<T, NumDim, IndexT, PatternT>
::lend() const noexcept
{
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline const MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::operator[](size_type pos) const
{
  DASH_LOG_TRACE_VAR("Matrix.[]()", pos);
  return _ref.operator[](pos);
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
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::sub(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
template<dim_t SubDimension>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::sub(
  size_type n)
{
  return this->_ref.template sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::col(
  size_type n)
{
  return this->_ref.template sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim-1, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::row(
  size_type n)
{
  return this->_ref.template sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::rows(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<0>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline MatrixRef<T, NumDim, NumDim, PatternT>
Matrix<T, NumDim, IndexT, PatternT>
::cols(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<1>(offset, extent);
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
  return _ref.template hview<level>();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT>
inline Matrix<T, NumDim, IndexT, PatternT>
::operator MatrixRef<T, NumDim, NumDim, PatternT>()
{
  return _ref;
}

} // namespace dash

#endif  // DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED
