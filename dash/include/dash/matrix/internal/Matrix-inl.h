#ifndef DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/memory/GlobStaticMem.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

#include <dash/iterator/GlobIter.h>

#include <dash/Matrix.h>

#include <type_traits>


namespace dash {

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::Matrix(
  Team & t)
: _team(&t),
  _size(0),
  _lsize(0),
  _lcapacity(0),
  _pattern(
    size_spec(),
    distribution_spec(),
    *_team),
  _glob_mem(nullptr),
  _lbegin(nullptr),
  _lend(nullptr)
{
  DASH_LOG_TRACE("Matrix()", "default constructor");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::Matrix(
  const size_spec & ss,
  const distribution_spec & ds,
  Team & t,
  const team_spec & ts)
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::Matrix(
  self_t && other)
: _team(other._team),
  _size(other._size),
  _lcapacity(other._lcapacity),
  _begin(other._begin),
  _pattern(other._pattern),
  _glob_mem(other._glob_mem),
  _lbegin(other._lbegin),
  _lend(other._lend),
  _ref(other._ref)
{
    // do not free other globmem
    other._glob_mem = nullptr;
    other._lbegin   = nullptr;
    other._lend     = nullptr;
    DASH_LOG_TRACE("Matrix()", "Move-Constructed");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::~Matrix()
{
  DASH_LOG_TRACE_VAR("Matrix.~Matrix()", this);
  deallocate();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline Matrix<T, NumDim, IndexT, PatternT, MSpaceC> &
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::operator= (
  Matrix<T, NumDim, IndexT, PatternT, MSpaceC> && other)
{
  deallocate();
  _team      = other._team;
  _size      = other._size;
  _lcapacity = other._lcapacity;
  _begin     = other._begin;
  _pattern   = other._pattern;
  _glob_mem  = other._glob_mem;
  _lbegin    = other._lbegin;
  _lend      = other._lend;
  _ref       = other._ref;

  // do not free other globmem
  other._glob_mem = nullptr;
  other._lbegin   = nullptr;
  other._lend     = nullptr;
  DASH_LOG_TRACE("Matrix.operator=(&&)", "Move-Assigned");
  return *this;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
bool Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
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
  // use _lcapacity as the collective allocator requires symmetric allocations
  _glob_mem        = new GlobMem_t(_lcapacity, _pattern.team());
  _begin           = iterator(_glob_mem, _pattern);
  _lbegin          = _glob_mem->lbegin();
  _lend            = _lbegin + _lsize;
  // Register team deallocator:
  _team->register_deallocator(
    this, std::bind(&Matrix::deallocate, this));
  // Initialize local proxy object:
  _ref._refview    = MatrixRefView_t(this);
  local            = local_type(this);
  DASH_LOG_TRACE("Matrix.allocate() finished");
  return true;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
bool Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
void Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::deallocate()
{
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

  //TODO: this is not thread safe and bad style
  //use std shared_ptr instead
  delete _glob_mem;
  //avoid double free
  _glob_mem = nullptr;

  _size = 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr inline dash::Team & Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::team() const noexcept
{
  return *_team;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::size_type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::size() const noexcept
{
  return _size;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::size_type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::local_size() const noexcept
{
  return _lsize;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::size_type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::local_capacity() const noexcept
{
  return _lcapacity;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::size_type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::extent(
  dim_t dim) const noexcept
{
  return _pattern.extent(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::size_type,
    NumDim >
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::extents() const noexcept
{
  return _pattern.extents();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::index_type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::offset(
  dim_t dim) const noexcept
{
  return _pattern.offset(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::index_type,
    NumDim >
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::offsets() const noexcept
{
  // Offset of global matrix is (0,0)
  return std::array<
    typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::index_type,
    NumDim > { };
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr bool
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::empty() const noexcept
{
  return size() == 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline void
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::barrier() const {
  _team->barrier();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline void
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::flush() {
  _glob_mem->flush();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline void
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::flush(dash::team_unit_t target) {
  _glob_mem->flush(target);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline void
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::flush_local() {
  _glob_mem->flush_local();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
inline void
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::flush_local(dash::team_unit_t target) {
  _glob_mem->flush_local(target);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::const_iterator
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::begin() const noexcept
{
  return const_iterator(_begin);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::iterator
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::begin() noexcept
{
  return iterator(_begin);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::const_iterator
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::end() const noexcept
{
  return const_iterator(_begin + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::iterator
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::end() noexcept
{
  return iterator(_begin + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::local_type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::sub_local() noexcept
{
  return local_type(this);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
T *
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::lbegin() noexcept
{
  return _lbegin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr const T *
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::lbegin() const noexcept
{
  return _lbegin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
T *
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::lend() noexcept
{
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr const T *
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::lend() const noexcept
{
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim != 0),
  MatrixRef<const T, NumDim, __NumViewDim, PatternT, MSpaceC>>::type
constexpr Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::operator[](size_type pos) const
{
  return _ref.operator[](pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim == 0),
  GlobRef<const T>>::type
constexpr Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::operator[](size_type pos) const
{
  return _ref.at(pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim != 0),
  MatrixRef<T, NumDim, __NumViewDim, PatternT, MSpaceC>>::type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::operator[](size_type pos)
{
  return _ref.operator[](pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim == 0),
  GlobRef<T>>::type
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::operator[](size_type pos)
{
  return _ref.at(pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t SubDimension>
MatrixRef<const T, NumDim, NumDim, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::sub(
  size_type offset,
  size_type extent) const
{
  return this->_ref.template sub<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t SubDimension>
MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::sub(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t SubDimension>
MatrixRef<const T, NumDim, NumDim-1, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::sub(
  size_type n) const
{
  return this->_ref.template sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template<dim_t SubDimension>
MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::sub(
  size_type n)
{
  return this->_ref.template sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<const T, NumDim, NumDim-1, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::col(
  size_type n) const
{
  return this->_ref.template sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::col(
  size_type n)
{
  return this->_ref.template sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<const T, NumDim, NumDim-1, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::row(
  size_type n) const
{
  return this->_ref.template sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, NumDim-1, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::row(
  size_type n)
{
  return this->_ref.template sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::rows(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<0>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::cols(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<1>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::const_reference
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::at(Args... args) const
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::reference
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::at(Args... args)
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::const_reference
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::operator()(Args... args) const
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, MSpaceC>::reference
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::operator()(Args... args)
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr const PatternT &
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::pattern() const
{
  return _pattern;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
constexpr bool Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::is_local(
  size_type g_pos) const
{
  return _ref.is_local(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template <dim_t Dimension>
constexpr bool Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::is_local(
  size_type g_pos) const
{
  return _ref.is_local<Dimension>(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
template <int level>
dash::HView<Matrix<T, NumDim, IndexT, PatternT, MSpaceC>, level>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::hview()
{
  return _ref.template hview<level>();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename MSpaceC>
Matrix<T, NumDim, IndexT, PatternT, MSpaceC>
::operator MatrixRef<T, NumDim, NumDim, PatternT, MSpaceC>()
{
  return _ref;
}

} // namespace dash

#endif  // DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED
