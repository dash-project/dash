#ifndef DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED
#define DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Exception.h>
#include <dash/internal/Logging.h>

#include <dash/iterator/GlobIter.h>

#include <dash/Matrix.h>

#include <type_traits>


namespace dash {

template <
    typename T,
    dim_t NumDim,
    typename IndexT,
    class PatternT,
    typename LocalMemT>
inline Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::Matrix(Team& t)
  : _team(&t)
  , _size(0)
  , _lsize(0)
  , _lcapacity(0)
  , _pattern(size_spec(), distribution_spec(), *_team)
{
  DASH_LOG_TRACE("Matrix()", "default constructor");
}

template <
    typename T,
    dim_t NumDim,
    typename IndexT,
    class PatternT,
    typename LocalMemT>
inline Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::Matrix(
    const size_spec&         ss,
    const distribution_spec& ds,
    Team&                    t,
    const team_spec&         ts)
  : _team(&t)
  , _size(0)
  , _lsize(0)
  , _lcapacity(0)
  , _pattern(ss, ds, ts, t)
{
  DASH_LOG_TRACE_VAR("Matrix()", _team->myid());
  allocate(_pattern);
  DASH_LOG_TRACE("Matrix()", "Initialized");
}

template <
    typename T,
    dim_t NumDim,
    typename IndexT,
    class PatternT,
    typename LocalMemT>
inline Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::Matrix(
    const PatternT& pattern)
  : _team(&pattern.team())
  , _size(0)
  , _lsize(0)
  , _lcapacity(0)
  , _pattern(pattern)
{
  DASH_LOG_TRACE("Matrix()", "pattern instance constructor");
  allocate(_pattern);
  DASH_LOG_TRACE("Matrix()", "Initialized");
}

template <
    typename T,
    dim_t NumDim,
    typename IndexT,
    class PatternT,
    typename LocalMemT>
inline Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::Matrix(self_t&& other)
  : _team(other._team)
  , _size(other._size)
  , _lcapacity(other._lcapacity)
  , _begin(other._begin)
  , _pattern(other._pattern)
  , _glob_mem(std::move(other._glob_mem))
  , _allocator(&_glob_mem)
  , _data(other._data.release(), {_allocator, _pattern.local_size()})
  , _lbegin(other._lbegin)
  , _lend(other._lend)
  , _ref(other._ref)
{
  // do not free other globmem
  other._lbegin = nullptr;
  other._lend   = nullptr;
  other._begin  = iterator{};

  // Register team deallocator:
  _team->register_deallocator(this, std::bind(&Matrix::deallocate, this));
  DASH_LOG_TRACE("Matrix()", "Move-Constructed");
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::~Matrix()
{
  DASH_LOG_TRACE_VAR("Matrix.~Matrix()", this);
  deallocate();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline Matrix<T, NumDim, IndexT, PatternT, LocalMemT> &
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::operator= (
  Matrix<T, NumDim, IndexT, PatternT, LocalMemT> && other)
{
  if (this == &other) {
    return *this;
  }

  _pattern = std::move(other._pattern);

  DASH_ASSERT_EQ(
      _allocator.resource(), &_glob_mem, "invalid state in move assignment");

  // a) reset own data
  if (_data) {
    destruct_at_end(_lbegin);
  }
  _data.reset();
  // b) move memory resource
  _glob_mem = std::move(other._glob_mem);
  // c) move the unique_gptr from other into this
  unique_gptr_t __tmp{other._data.release(),
                      {_allocator, _pattern.local_size()}};
  _data = std::move(__tmp);

  _team      = other._team;
  _size      = other._size;
  _lcapacity = other._lcapacity;
  _begin = iterator{&_glob_mem, _pattern, other._begin.pos()};
  _lbegin    = other._lbegin;
  _lend      = other._lend;
  _ref       = other._ref;

  // Re-register team deallocator:
  _team->register_deallocator(this, std::bind(&Matrix::deallocate, this));

  DASH_LOG_TRACE("Matrix.operator=(&&)", "Move-Assigned");
  return *this;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<T, NumDim, NumDim, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<T, NumDim, NumDim, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
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

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
bool Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::allocate(
  const PatternT & pattern)
{
  DASH_LOG_TRACE("Matrix.allocate()", "pattern",
                 pattern.memory_layout().extents());
  if (&_pattern != &pattern) {
    DASH_LOG_TRACE("Matrix.allocate()", "using specified pattern");
    _pattern = pattern;
    _team = &pattern.team();
  }

  _glob_mem   = GlobMem_t{*_team};
  _allocator = allocator_type{&_glob_mem};

  // Copy sizes from pattern:
  _size      = _pattern.size();
  _team      = &(_pattern.team());
  _lsize     = _pattern.local_size();
  _lcapacity = _pattern.local_capacity();

  _data = std::move(dash::allocate_unique<value_type>(_allocator, _lsize));

  DASH_LOG_TRACE_VAR("Matrix.allocate", _size);
  DASH_LOG_TRACE_VAR("Matrix.allocate", _lsize);
  DASH_LOG_TRACE_VAR("Matrix.allocate", _lcapacity);


  _begin  = iterator(&_glob_mem, _pattern);

  if (_size) {
    DASH_ASSERT(_data);
  }

  if (_lsize) {
    auto soon_to_be_lbegin = _data.get();
    soon_to_be_lbegin.set_unit(_team->myid());
    _lbegin = soon_to_be_lbegin.local();
    DASH_ASSERT(_lbegin);
    _lend   = std::next(_lbegin, _lsize);
  }

  // Register team deallocator:
  _team->register_deallocator(
    this, std::bind(&Matrix::deallocate, this));
  // Initialize local proxy object:
  _ref._refview    = MatrixRefView_t(this);
  local            = local_type(this);
  DASH_LOG_TRACE("Matrix.allocate() finished");
  return true;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
bool Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
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
    DASH_LOG_TRACE(
        "Matrix.allocate", "initializing pattern with Team::All()");
    _team = &team;
    _pattern  = PatternT(sizespec, distribution, teamspec, team);
  }
  else {
    DASH_LOG_TRACE(
        "Matrix.allocate", "initializing pattern with initial team");
    _pattern = PatternT(sizespec, distribution, teamspec, *_team);
  }

  return allocate(_pattern);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
void Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
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

  if (_data) {
    destruct_at_end(_lbegin);
  }

  _data.reset();

  // reset the pointer
  _begin  = iterator{};
  _size   = 0;
  _lbegin = nullptr;
  _lend   = nullptr;
  _lsize  = 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
void Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::destruct_at_end(
    value_type * new_last)
{
  DASH_LOG_DEBUG_VAR("Matrix.destruct_at_end", new_last);
  if (!_data || !new_last) {
    return;
  }

  DASH_ASSERT(_lend);

  local_pointer soon_to_be_new_last = _lend;

  auto local_alloc = _glob_mem.get_allocator();
  using allocator_traits = std::allocator_traits<decltype(local_alloc)>;

  while (new_last != soon_to_be_new_last) {
    allocator_traits::destroy(local_alloc, --soon_to_be_new_last);
  }
  _lend = new_last;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr inline dash::Team & Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::team() const noexcept
{
  return *_team;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::size_type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::size() const noexcept
{
  return _size;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::size_type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::local_size() const noexcept
{
  return _lsize;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::size_type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::local_capacity() const noexcept
{
  return _lcapacity;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::size_type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::extent(
  dim_t dim) const noexcept
{
  return _pattern.extent(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::size_type,
    NumDim >
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::extents() const noexcept
{
  return _pattern.extents();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::index_type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::offset(
  dim_t dim) const noexcept
{
  return _pattern.offset(dim);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr
  std::array<
    typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::index_type,
    NumDim >
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::offsets() const noexcept
{
  // Offset of global matrix is (0,0)
  return std::array<
    typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::index_type,
    NumDim > { };
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr bool
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::empty() const noexcept
{
  return size() == 0;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline void
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::barrier() const {
  _team->barrier();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline void
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::flush() {
  if (_data)
    _glob_mem.flush(_data.get());
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline void
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::flush(dash::team_unit_t target) {
  if (_data)
    _glob_mem.flush(_data.get(), target);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline void
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::flush_local() {
  if (_data)
    _glob_mem.flush_local(_data.get());
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
inline void
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::flush_local(dash::team_unit_t target) {
  if (_data)
    _glob_mem.flush_local(_data.get(), target);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::begin() const noexcept
{
  return const_iterator(_begin);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::iterator
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::begin() noexcept
{
  return iterator(_begin);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::const_iterator
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::end() const noexcept
{
  return const_iterator(_begin + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::iterator
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::end() noexcept
{
  return iterator(_begin + _size);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::local_type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::sub_local() noexcept
{
  return local_type(this);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
T *
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::lbegin() noexcept
{
  return _lbegin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr const T *
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::lbegin() const noexcept
{
  return _lbegin;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
T *
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::lend() noexcept
{
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr const T *
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::lend() const noexcept
{
  return _lend;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim != 0),
  MatrixRef<const T, NumDim, __NumViewDim, PatternT, LocalMemT>>::type
constexpr Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::operator[](size_type pos) const
{
  return _ref.operator[](pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim == 0),
    typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::const_reference>::type
constexpr Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::operator[](size_type pos) const
{
  return _ref.at(pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim != 0),
  MatrixRef<T, NumDim, __NumViewDim, PatternT, LocalMemT>>::type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::operator[](size_type pos)
{
  return _ref.operator[](pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t __NumViewDim>
  typename std::enable_if<(__NumViewDim == 0),
    typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::reference>::type
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::operator[](size_type pos)
{
  return _ref.at(pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t SubDimension>
MatrixRef<const T, NumDim, NumDim, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::sub(
  size_type offset,
  size_type extent) const
{
  return this->_ref.template sub<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t SubDimension>
MatrixRef<T, NumDim, NumDim, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::sub(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<SubDimension>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t SubDimension>
MatrixRef<const T, NumDim, NumDim-1, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::sub(
  size_type n) const
{
  return this->_ref.template sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template<dim_t SubDimension>
MatrixRef<T, NumDim, NumDim-1, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::sub(
  size_type n)
{
  return this->_ref.template sub<SubDimension>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<const T, NumDim, NumDim-1, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::col(
  size_type n) const
{
  return this->_ref.template sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<T, NumDim, NumDim-1, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::col(
  size_type n)
{
  return this->_ref.template sub<1>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<const T, NumDim, NumDim-1, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::row(
  size_type n) const
{
  return this->_ref.template sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<T, NumDim, NumDim-1, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::row(
  size_type n)
{
  return this->_ref.template sub<0>(n);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<T, NumDim, NumDim, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::rows(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<0>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
MatrixRef<T, NumDim, NumDim, PatternT, LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::cols(
  size_type offset,
  size_type extent)
{
  return this->_ref.template sub<1>(offset, extent);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::const_reference
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::at(Args... args) const
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::reference
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::at(Args... args)
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::const_reference
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::operator()(Args... args) const
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template <typename ... Args>
typename Matrix<T, NumDim, IndexT, PatternT, LocalMemT>::reference
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::operator()(Args... args)
{
  return _ref.at(args...);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr const PatternT &
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::pattern() const
{
  return _pattern;
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
constexpr bool Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::is_local(
  size_type g_pos) const
{
  return _ref.is_local(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template <dim_t Dimension>
constexpr bool Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::is_local(
  size_type g_pos) const
{
  return _ref.is_local<Dimension>(g_pos);
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
template <int level>
dash::HView<Matrix<T, NumDim, IndexT, PatternT, LocalMemT>, level>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::hview()
{
  return _ref.template hview<level>();
}

template <typename T, dim_t NumDim, typename IndexT, class PatternT, typename LocalMemT>
Matrix<T, NumDim, IndexT, PatternT, LocalMemT>
::operator MatrixRef<T, NumDim, NumDim, PatternT, LocalMemT>()
{
  return _ref;
}

} // namespace dash

#endif  // DASH__MATRIX__INTERNAL__MATRIX_INL_H_INCLUDED
