#ifndef DASH__LOCAL_VIEW_ITER_H__INCLUDED
#define DASH__LOCAL_VIEW_ITER_H__INCLUDED

#include <dash/Pattern.h>
#include <dash/Allocator.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>
#include <dash/memory/GlobStaticMem.h>

#include <dash/iterator/GlobIter.h>

#include <cstddef>
#include <functional>
#include <sstream>
#include <iostream>

namespace dash {

#ifndef DOXYGEN
// Forward-declaration
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType,
  class    PointerType,
  class    ReferenceType >
class GlobIter;
// Forward-declaration
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType,
  class    PointerType,
  class    ReferenceType >
class GlobStencilIter;
#endif

/**
 * Global iterator on an index set specified by a view modifier.
 *
 * \concept{DashGlobalIteratorConcept}
 */
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType   = GlobStaticMem<
                             typename std::decay<ElementType>::type
                           >,
  class    PointerType   = ElementType *,
  class    ReferenceType = ElementType & >
class LocalViewIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           typename PatternType::index_type,
           PointerType,
           ReferenceType > {
private:
  typedef LocalViewIter<
            ElementType,
            PatternType,
            GlobMemType,
            PointerType,
            ReferenceType>
    self_t;

  typedef GlobIter<
            ElementType,
            PatternType,
            GlobMemType,
            PointerType,
            ReferenceType>
    global_type;

  typedef GlobIter<
            const ElementType,
            PatternType,
            GlobMemType,
            PointerType,
            ReferenceType>
    const_global_type;

  typedef typename std::remove_const<ElementType>::type
    nonconst_value_type;

  typedef typename PatternType::viewspec_type
    ViewSpecType;
  typedef typename PatternType::index_type
    IndexType;

public:
  typedef          ElementType                         value_type;

  typedef          ReferenceType                        reference;
  typedef typename dash::const_value_cast<ReferenceType>::type
                                                  const_reference;

  typedef          PointerType                            pointer;
  typedef typename dash::const_value_cast<PointerType>::type
                                                    const_pointer;

  typedef typename GlobMemType::local_pointer       local_pointer;
  typedef typename GlobMemType::local_pointer          local_type;

  typedef          PatternType                       pattern_type;
  typedef typename PatternType::index_type             index_type;

private:
  typedef LocalViewIter<
            const ElementType,
            PatternType,
            GlobMemType,
            const_pointer,
            const_reference >
    self_const_t;

public:
  typedef std::integral_constant<bool, true> has_view;

public:
  // For ostream output
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend std::ostream & operator<<(
      std::ostream & os,
      const LocalViewIter<T_, P_, GM_, Ptr_, Ref_> & it);

  // For conversion to GlobStencilIter
  template<
    typename T_,
    class    P_,
    class    Ptr_,
    class    GM_,
    class    Ref_ >
  friend class GlobStencilIter;

  template<
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend class LocalViewIter;

  template<
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend class GlobIter;

private:
  static const dim_t      NumDimensions = PatternType::ndim();
  static const MemArrange Arrangement   = PatternType::memory_order();

protected:
  /// Global memory used to dereference iterated values.
  GlobMemType                * _globmem;
  /// Pattern that specifies the iteration order (access pattern).
  const PatternType          * _pattern;
  /// View that specifies the iterator's index range relative to the global
  /// index range of the iterator's pattern.
  const ViewSpecType         * _viewspec;
  /// Current position of the iterator relative to the iterator's view.
  IndexType                    _idx             = 0;
  /// The iterator's view index start offset.
  IndexType                    _view_idx_offset = 0;
  /// Maximum position relative to the viewspec allowed for this iterator.
  IndexType                    _max_idx         = 0;
  /// Unit id of the active unit
  team_unit_t                  _myid;
  /// Pointer to first element in local memory
  local_pointer                _lbegin          = nullptr;

public:
  /**
   * Default constructor.
   */
  constexpr LocalViewIter()
  : _globmem(nullptr)
  , _pattern(nullptr)
  , _viewspec(nullptr)
  , _idx(0)
  , _view_idx_offset(0)
  , _max_idx(0)
  , _myid(dash::Team::All().myid())
  , _lbegin(nullptr)
  { }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern and view spec.
   */
  template <class GlobStaticMemT_>
  constexpr LocalViewIter(
    GlobStaticMemT_      * gmem,
	  const PatternType    & pat,
    const ViewSpecType   & viewspec,
	  IndexType              position          = 0,
    IndexType              view_index_offset = 0)
  : _globmem(reinterpret_cast<GlobMemType *>(gmem))
  , _pattern(&pat)
  , _viewspec(&viewspec)
  , _idx(position)
  , _view_idx_offset(view_index_offset)
  , _max_idx(viewspec.size() - 1)
  , _myid(pat.team().myid())
  , _lbegin(_globmem->lbegin())
  { }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern and view spec.
   */
  template <class GlobStaticMemT_>
  constexpr LocalViewIter(
    GlobStaticMemT_   * gmem,
	  const PatternType & pat,
	  IndexType           position          = 0,
    IndexType           view_index_offset = 0)
  : _globmem(reinterpret_cast<GlobMemType *>(gmem))
  , _pattern(&pat)
  , _viewspec(nullptr)
  , _idx(position)
  , _view_idx_offset(view_index_offset)
  , _max_idx(pat.size() - 1)
  , _myid(dash::Team::GlobalUnitID())
  , _lbegin(_globmem->lbegin())
  { }

  /**
   * Constructor, creates a global view iterator from a global iterator.
   */
  template <
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  constexpr LocalViewIter(
    const GlobIter<nonconst_value_type, P_, GM_, Ptr_, Ref_> & other,
    const ViewSpecType                                       & viewspec,
    IndexType                                                  view_offs = 0)
  : _globmem(other._globmem)
  , _pattern(other._pattern)
  , _viewspec(&viewspec)
  , _idx(other._idx)
  , _view_idx_offset(view_offs)
  , _max_idx(other._max_idx)
  , _myid(other._myid)
  , _lbegin(other._lbegin)
  { }

  /**
   * Copy constructor.
   */
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  constexpr LocalViewIter(
//  const LocalViewIter<nonconst_value_type, P_, GM_, Ptr_, Ref_> & other)
    const LocalViewIter<T_, P_, GM_, Ptr_, Ref_> & other)
  : _globmem        (other._globmem)
  , _pattern        (other._pattern)
  , _viewspec       (other._viewspec)
  , _idx            (other._idx)
  , _view_idx_offset(other._view_idx_offset)
  , _max_idx        (other._max_idx)
  , _myid           (other._myid)
  , _lbegin         (other._lbegin)
  { }

  /**
   * Move constructor.
   */
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  constexpr LocalViewIter(
//  LocalViewIter<nonconst_value_type, P_, GM_, Ptr_, Ref_> && other)
    LocalViewIter<T_, P_, GM_, Ptr_, Ref_> && other)
  : _globmem        (other._globmem)
  , _pattern        (other._pattern)
  , _viewspec       (other._viewspec)
  , _idx            (other._idx)
  , _view_idx_offset(other._view_idx_offset)
  , _max_idx        (other._max_idx)
  , _myid           (other._myid)
  , _lbegin         (other._lbegin)
  { }

  /**
   * Assignment operator.
   */
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  self_t & operator=(
    const LocalViewIter<T_, P_, GM_, Ptr_, Ref_ > & other)
  {
    _globmem         = other._globmem;
    _pattern         = other._pattern;
    _viewspec        = other._viewspec;
    _idx             = other._idx;
    _view_idx_offset = other._view_idx_offset;
    _max_idx         = other._max_idx;
    _myid            = other._myid;
    _lbegin          = other._lbegin;
  }

  /**
   * Move-assignment operator.
   */
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  self_t & operator=(
    LocalViewIter<T_, P_, GM_, Ptr_, Ref_ > && other)
  {
    _globmem         = other._globmem;
    _pattern         = other._pattern;
    _viewspec        = other._viewspec;
    _idx             = other._idx;
    _view_idx_offset = other._view_idx_offset;
    _max_idx         = other._max_idx;
    _myid            = other._myid;
    _lbegin          = other._lbegin;
    // no ownership to transfer
    return *this;
  }

  /**
   * The number of dimensions of the iterator's underlying pattern.
   */
  static dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  explicit operator const_pointer() const
  {
    return local();
  }

  /**
   * Type conversion operator to native pointer.
   *
   * \return  A global reference to the element at the iterator's position
   */
  explicit operator pointer()
  {
    return local();
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.dart_gptr()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx    = _idx;
    IndexType offset = 0;
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    // Global index to local index and unit:
    local_pos_t local_pos = lpos();
    DASH_LOG_TRACE("LocalViewIter.dart_gptr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    // Global pointer to element at given position:
    dash::GlobPtr<ElementType> gptr(
      _globmem->at(
        local_pos.unit,
        local_pos.index)
    );
    DASH_LOG_TRACE_VAR("GlobIter.dart_gptr >", gptr);
    return (gptr + offset).dart_gptr();
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  inline reference operator*()
  {
    return this->operator[](0);
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  inline const_reference operator*() const
  {
    return this->operator[](0);
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  reference operator[](
    /// The global position of the element
    index_type idx)
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.[]=()", idx);
    idx              += _idx;
    IndexType offset  = 0;
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    if (_viewspec != nullptr) {
      // Viewspec projection required:
      auto l_coords = coords(idx);
      idx = _pattern->local_memory_layout().at(l_coords);
    }
    DASH_LOG_TRACE("LocalViewIter.[]= >",
                   "globmem.lbegin:", _globmem->lbegin(),
                   "+", idx);
    // Global reference to element at given position:
    return *(_globmem->lbegin() + idx + offset);
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  const_reference operator[](
    /// The global position of the element
    index_type idx) const
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.[]()", idx);
    idx              += _idx;
    IndexType offset  = 0;
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    if (_viewspec != nullptr) {
      // Viewspec projection required:
      auto l_coords = coords(idx);
      idx = _pattern->local_memory_layout().at();
    }
    DASH_LOG_TRACE("LocalViewIter.[] >",
                   "globmem.lbegin:", _globmem->lbegin(),
                   "+ index", idx,
                   "+ tail",  offset);
    // Global reference to element at given position:
    return *(_globmem->lbegin() + idx + offset);
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  constexpr bool is_local() const
  {
    return true;
  }

  /**
   * Convert global iterator to native pointer.
   */
  local_pointer local() const
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.local=()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx    = _idx;
    IndexType offset = 0;
    DASH_LOG_TRACE_VAR("LocalViewIter.local=", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("LocalViewIter.local=", idx);
    DASH_LOG_TRACE_VAR("LocalViewIter.local=", offset);

    if (_viewspec != nullptr) {
      // Viewspec projection required:
      idx = _pattern->local_memory_layout().at(coords(idx));
    }
    DASH_LOG_TRACE_VAR("LocalViewIter.local= >", idx);
    DASH_LOG_TRACE_VAR("LocalViewIter.local= >", offset);
    return (_lbegin + idx + offset);
  }

  /**
   * Map iterator to global index domain by projecting the iterator's view.
   */
  inline const_global_type global() const
  {
    auto g_idx = gpos();
    return const_global_type(
             _globmem,
             *_pattern,
             g_idx
           );
  }

  /**
   * Map iterator to global index domain by projecting the iterator's view.
   */
  inline global_type global()
  {
    return global_type(
             _globmem,
             *_pattern,
             gpos()
           );
  }

  /**
   * Position of the iterator in its view's iteration space and the view's
   * offset in global index space.
   */
  constexpr index_type pos() const noexcept
  {
    return _idx + _view_idx_offset;
  }

  /**
   * Position of the iterator in its view's iteration space, disregarding
   * the view's offset in global index space.
   */
  inline index_type rpos() const noexcept
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   * Projects iterator position from its view spec to global index domain.
   */
  inline index_type gpos() const
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.gpos()", _idx);
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos >", _idx);
      return _idx;
    } else {
      IndexType idx    = _idx;
      IndexType offset = 0;
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos", *_viewspec);
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos", _max_idx);
      // Convert iterator position (_idx) to local index and unit.
      if (_idx > _max_idx) {
        // Global iterator pointing past the range indexed by the pattern
        // which is the case for .end() iterators.
        idx    = _max_idx;
        offset = _idx - _max_idx;
      }
      // Viewspec projection required:
      auto g_coords = coords(idx);
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos", _idx);
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos", g_coords);
      auto g_idx    = _pattern->memory_layout().at(g_coords);
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos", g_idx);
      g_idx += offset;
      DASH_LOG_TRACE_VAR("LocalViewIter.gpos >", g_idx);
      return g_idx;
    }
  }

  /**
   * Unit and local offset at the iterator's position.
   * Projects iterator position from its view spec to global index domain.
   */
  inline typename pattern_type::local_index_t lpos() const
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.lpos()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx    = _idx;
    IndexType offset = 0;
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx    = _max_idx;
      offset = _idx - _max_idx;
      DASH_LOG_TRACE_VAR("LocalViewIter.lpos", _max_idx);
      DASH_LOG_TRACE_VAR("LocalViewIter.lpos", idx);
      DASH_LOG_TRACE_VAR("LocalViewIter.lpos", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      DASH_LOG_TRACE_VAR("LocalViewIter.lpos", *_viewspec);
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    local_pos.index += offset;
    DASH_LOG_TRACE("LocalViewIter.lpos >",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    return local_pos;
  }

  /**
   * Whether the iterator's position is relative to a view.
   */
  inline bool is_relative() const noexcept
  {
    return _viewspec != nullptr;
  }

  /**
   * The view that specifies this iterator's index range.
   */
  inline ViewSpecType viewspec() const
  {
    if (_viewspec != nullptr) {
      return *_viewspec;
    }
    return ViewSpecType(_pattern->memory_layout().extents());
  }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   */
  constexpr const GlobMemType & globmem() const noexcept
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline GlobMemType & globmem() noexcept
  {
    return *_globmem;
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++() noexcept
  {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int) noexcept
  {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--() noexcept
  {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int) noexcept
  {
    self_t result = *this;
    --_idx;
    return result;
  }

  self_t & operator+=(index_type n) noexcept
  {
    _idx += n;
    return *this;
  }

  self_t & operator-=(index_type n) noexcept
  {
    _idx -= n;
    return *this;
  }

  self_t operator+(index_type n) const noexcept
  {
    if (_viewspec == nullptr) {
      self_t res(
        _globmem,
        *_pattern,
        _idx + static_cast<IndexType>(n),
        _view_idx_offset);
      return res;
    }
    self_t res(
      _globmem,
      *_pattern,
      *_viewspec,
      _idx + static_cast<IndexType>(n),
      _view_idx_offset);
    return res;
  }

  self_t operator-(index_type n) const noexcept
  {
    if (_viewspec == nullptr) {
      self_t res(
        _globmem,
        *_pattern,
        _idx - static_cast<IndexType>(n),
        _view_idx_offset);
      return res;
    }
    self_t res(
      _globmem,
      *_pattern,
      *_viewspec,
      _idx - static_cast<IndexType>(n),
      _view_idx_offset);
    return res;
  }

  index_type operator+(
    const self_t & other) const noexcept
  {
    return _idx + other._idx;
  }

  index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }

  inline bool operator<(const self_t & other) const noexcept
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::less<index_type>(),
                   std::less<pointer>());
  }

  inline bool operator<=(const self_t & other) const noexcept
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::less_equal<index_type>(),
                   std::less_equal<pointer>());
  }

  inline bool operator>(const self_t & other) const noexcept
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::greater<index_type>(),
                   std::greater<pointer>());
  }

  inline bool operator>=(const self_t & other) const noexcept
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::greater_equal<index_type>(),
                   std::greater_equal<pointer>());
  }

  inline bool operator==(const self_t & other) const noexcept
  {
    // NOTE: See comments in method compare().
    if (_viewspec == other._viewspec) {
      // Same viewspec pointer
      return _idx == other._idx;
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) &&
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return _idx == other._idx;
    }
    auto lhs_local = lpos();
    auto rhs_local = other.lpos();
    return (lhs_local.unit  == rhs_local.unit &&
            lhs_local.index == rhs_local.index);
  }

  inline bool operator!=(const self_t & other) const noexcept
  {
    // NOTE: See comments in method compare().
    if (_viewspec == other._viewspec) {
      // Same viewspec pointer
      return _idx != other._idx;
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) &&
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return _idx != other._idx;
    }
    auto lhs_local = lpos();
    auto rhs_local = other.lpos();
    return (lhs_local.unit  != rhs_local.unit ||
            lhs_local.index != rhs_local.index);
  }

  constexpr const PatternType & pattern() const noexcept
  {
    return *_pattern;
  }

  constexpr dash::Team & team() const noexcept
  {
    return _pattern->team();
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template<
    class LocalIndexCmpFun,
    class LocalPtrCmpFun >
  bool compare(
    const self_t & other,
    const LocalIndexCmpFun & lidx_cmp,
    const LocalPtrCmpFun   & lptr_cmp) const noexcept
  {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if (this == &other) {
      return true;
    }
#endif
    // NOTE:
    // Do not check _idx first, as it would never match for comparison with
    // an end iterator.
    if (_viewspec == other._viewspec) {
      // Same viewspec pointer
      return lidx_cmp(_idx, other._idx);
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) &&
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return lidx_cmp(_idx, other._idx);
    }
    // View projection at lhs and/or rhs set.
    // Convert both to GlobPtr (i.e. apply view projection) and compare.
    //
    // NOTE:
    // This conversion is quite expensive but will never be necessary
    // if both iterators have been created from the same range.
    // Example:
    //   a.block(1).begin() == a.block(1).end().
    // does not require viewspace projection while
    //   a.block(1).begin() == a.end()
    // does. The latter case should be avoided for this reason.
    const pointer lhs_ptr(dart_gptr());
    const pointer rhs_ptr(other.dart_gptr());
    return lptr_cmp(lhs_ptr, rhs_ptr);
  }

  /**
   * Convert a local offset within the local iterator's range to
   * corresponding local coordinates with respect to viewspec projection.
   *
   * This method is bounds-checked and might throw.
   *
   * \note
   * This method could be specialized for NumDimensions = 1 for performance
   * tuning.
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType l_index) const
  {
    DASH_LOG_TRACE_VAR("LocalViewIter.coords()", l_index);
    // Global cartesian coords of current iterator position:
    std::array<IndexType, NumDimensions> l_coords;
    if (_viewspec != nullptr) {
      DASH_LOG_TRACE_VAR("LocalViewIter.coords", *_viewspec);
      // Create cartesian index space from extents of view projection:
      CartesianIndexSpace<NumDimensions, Arrangement, IndexType> index_space(
        _viewspec->extents());
      // Initialize global coords with view coords (coords of iterator
      // position in view index space):
      l_coords = index_space.coords(l_index);
      // Apply offset of view projection to view coords:
      for (dim_t d = 0; d < NumDimensions; ++d) {
        l_coords[d] += _viewspec->offset(d);
      }
    } else {
      l_coords = _pattern->local_memory_layout().coords(l_index);
    }
    DASH_LOG_TRACE_VAR("LocalViewIter.coords >", l_coords);
    return l_coords;
  }

}; // class LocalViewIter

/**
 * Resolve the number of elements between two global iterators.
 *
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *   g_last == g_first + (l_last - l_first)
 * Example:
 *
 * \code
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * \endcode
 *
 * When iterating in local memory range [0,5[ of unit 0, the position of the
 * global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<
  typename ElementT,
  class    Pattern,
  class    GlobStaticMem,
  class    Pointer,
  class    Reference >
auto distance(
  const LocalViewIter<ElementT, Pattern, GlobStaticMem, Pointer, Reference> &
    first,
  /// Global iterator to the final position in the global sequence
  const LocalViewIter<ElementT, Pattern, GlobStaticMem, Pointer, Reference> &
    last)
-> typename Pattern::index_type
{
  return last - first;
}

template <
  typename ElementT,
  class    Pattern,
  class    GlobStaticMem,
  class    Pointer,
  class    Reference >
std::ostream & operator<<(
  std::ostream & os,
  const dash::LocalViewIter<
          ElementT, Pattern, GlobStaticMem, Pointer, Reference> & it)
{
  std::ostringstream ss;
  ss << "dash::LocalViewIter<"
     << typeid(ElementT).name() << ", "
     << typeid(Pointer).name()
     << ">("
     << "idx:"  << it._idx << ", "
     << "gptr:" << it.global().dart_gptr() << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__LOCAL_VIEW_ITER_H__INCLUDED
