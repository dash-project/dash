#ifndef DASH__HALO_H__
#define DASH__HALO_H__

namespace dash {

template<dim_t NumDimensions>
class HaloSpec
{
public:
  typedef int
    offset_type;

  typedef struct {
    offset_type min;
    offset_type max;
  } offset_range_type;

private:
  /// The stencil's offset range (min, max) in every dimension.
  std::array<offset_range_type, NumDimensions> _offset_ranges;
  /// Number of points in the stencil.
  int                                          _points;

public:
  /**
   * Creates a new instance of class HaloSpec with the given offset ranges
   * (pair of minimum offset, maximum offset) in the stencil's dimensions.
   *
   * For example, a two-dimensional five-point stencil has offset ranges
   * { (-1, 1), (-1, 1) }
   * and a stencil with only north and east halo cells has offset ranges
   * { (-1, 0), ( 0, 1) }.
   */
  HaloSpec(
    const std::array<offset_range_type, NumDimensions> & offset_ranges)
  : _offset_ranges(offset_ranges)
  {
    // minimum stencil size when containing center element only:
    _points = 1;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      _points += std::abs(_offset_ranges[d].max - _offset_ranges[d].min);
    }
  }

  /**
   * Creates a new instance of class HaloSpec with the given offset ranges
   * (pair of minimum offset, maximum offset) in the stencil's dimensions.
   *
   * For example, a two-dimensional five-point stencil has offset ranges
   * { (-1, 1), (-1, 1) }
   * and a stencil with only north and east halo cells has offset ranges
   * { (-1, 0), ( 0, 1) }.
   */
  template<typename... Args>
  HaloSpec(offset_range_type arg, Args... args)
  {
    static_assert(sizeof...(Args) == NumDimensions-1,
                  "Invalid number of offset ranges");
    _offset_ranges = { arg, static_cast<offset_range_type>(args)... };
    // minimum stencil size when containing center element only:
    _points = 1;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      _points += std::abs(_offset_ranges[d].max - _offset_ranges[d].min);
    }
  }

  /**
   * Creates a new instance of class HaloSpec that only consists of the
   * center point.
   */
  HaloSpec()
  : _points(1)
  {
    // initialize offset ranges with (0,0) in all dimensions:
    for (dim_t d = 0; d < NumDimensions; ++d) {
      _offset_ranges = {{ 0, 0 }};
    }
  }

  /**
   * The stencil's number of dimensions.
   */
  static dim_t ndim() {
    return NumDimensions;
  }

  /**
   * Number of points in the stencil.
   */
  inline int npoints() const
  {
    return _points;
  }

  /**
   * Offset range (minimum and maximum offset) in the given dimension.
   */
  const offset_range_type & offset_range(dim_t dimension) const
  {
    return _offset_ranges[dimension];
  }

  /**
   * Offset ranges (minimum and maximum offset) all dimensions.
   */
  const std::array< offset_range_type, NumDimensions> & offset_ranges() const
  {
    return _offset_ranges;
  }

  /**
   * Width of the halo in the given dimension.
   */
  inline int width(dim_t dimension) const
  {
    auto offset_range = _offset_ranges[dimension];
    return std::max(std::abs(offset_range.max),
                    std::abs(offset_range.min));
  }
};

template<dim_t NumDimensions>
std::ostream & operator<<(
  std::ostream & os,
  const dash::HaloSpec<NumDimensions> & halospec)
{
  std::ostringstream ss;
  ss << "dash::HaloSpec<" << NumDimensions << ">(";
  for (dim_t d = 0; d < NumDimensions; ++d) {
    auto offset_range = halospec.offset_range(d);
    ss << "{ " << offset_range.min
       << ", " << offset_range.max
       << " }";
  }
  ss << ")";
  return operator<<(os, ss.str());
}

/**
 * Iterator on block elements in internal (boundary) or external (halo) border
 * regions.
 *
 * Example for a block boundary iteration space:
 *
 *           .-------------------------.  _
 *           |  0  1  2  3  4  5  6  7 |   \
 *           |  8  9 10 11 12 13 14 15 |    }- halo width in dimension 0
 *           `-------------------------'  _/
 *  .-------..-------------------------..-------.
 *  | 16 17 ||                         || 18 19 |
 *  :  ...  ::   inner block region    ::  ...  :
 *  | 28 29 ||                         || 30 31 |
 *  '-------''-------------------------''-------'
 *           ,-------------------------. \__ __/
 *           | 32 33 34 35 36 37 38 39 |    |
 *           | 40 41 42 43 44 45 46 47 |    '- halo width in dimension 1
 *           `-------------------------'
 */
template<
  typename ElementType,
  class    PatternType,
  class    PointerType   = GlobPtr<ElementType, PatternType>,
  class    ReferenceType = GlobRef<ElementType> >
class BlockBoundaryIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           typename PatternType::index_type,
           PointerType,
           ReferenceType >
{
private:
  typedef BlockBoundaryIter<ElementType, PatternType>      self_t;

public:
  typedef PatternType                                pattern_type;
  typedef typename PatternType::index_type             index_type;
  typedef typename PatternType::size_type               size_type;
  typedef typename PatternType::viewspec_type       viewspec_type;

  typedef       ReferenceType                           reference;
  typedef const ReferenceType                     const_reference;
  typedef       PointerType                               pointer;
  typedef const PointerType                         const_pointer;

  // For ostream output
  template <
    typename T_,
    class P_,
    class Ptr_,
    class Ref_ >
  friend std::ostream & operator<<(
      std::ostream & os,
      const BlockBoundaryIter<T_, P_, Ptr_, Ref_> & it);

public:
  typedef std::integral_constant<bool, true>             has_view;

public:
  BlockBoundaryIter(
    /// Global memory used to dereference iterated values.
    GlobMem<ElementType> * globmem,
    /// The pattern that created the encapsulated block.
    const PatternType    & pattern,
    /// A block's inner or outer view. Iteration space is the view's boundary
    /// specified by the halo.
    const viewspec_type  & viewspec,
    /// The halo to apply to the block.
    const halospec_type  & halospec,
    /// The iterator's position in the block boundary's iteration space
    index_type             pos  = 0,
    /// The number of elements in the block boundary's iteration space.
    index_type             size = 0)
  : _globmem(globmem),
    _pattern(&pattern),
    _viewspec(&viewspec),
    _halospec(&halospec),
    _idx(pos),
    _size(size),
    _max_idx(_size-1),
    _myid(dash::myid()),
    _lbegin(_globmem->lbegin()),
  {
    if (_size <= 0) {
      dim_t d_v = NumDimensions - 1;
      for (dim_t d = 0; d < NumDimensions; ++d, --d_v) {
        _size += _halospec->width(d) * _viewspec->extent(d_v);
      }
      _max_idx = _size - 1;
    }
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", _idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", _max_idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", _size);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", *_viewspec);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", *_halospec);
  }

  /**
   * Copy constructor.
   */
  BlockBoundaryIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Position of the iterator in global storage order.
   *
   * \see DashGlobalIteratorConcept
   */
  inline index_type pos() const
  {
    DASH_LOG_TRACE("BlockBoundaryIter.pos()",
                   "idx:", _idx, "vs_offset:", _view_idx_offset);
    return _idx + _view_idx_offset;
  }

  /**
   * Position of the iterator in its view's iteration space, disregarding
   * the view's offset in global index space.
   *
   * \see DashViewIteratorConcept
   */
  inline index_type rpos() const
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   * Projects iterator position from its view spec to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  inline index_type gpos() const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos()", _idx);
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos >", _idx);
      return _idx;
    } else {
      index_type idx    = _idx;
      index_type offset = 0;
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos", *_viewspec);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos", _max_idx);
      // Convert iterator position (_idx) to local index and unit.
      if (_idx > _max_idx) {
        // Global iterator pointing past the range indexed by the pattern
        // which is the case for .end() iterators.
        idx    = _max_idx;
        offset = _idx - _max_idx;
      }
      // Viewspec projection required:
      auto g_coords = coords(idx);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos", _idx);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos", g_coords);
      auto g_idx    = _pattern->memory_layout().at(g_coords);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos", g_idx);
      g_idx += offset;
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.gpos >", g_idx);
      return g_idx;
    }
  }

  /**
   * Unit and local offset at the iterator's position.
   * Projects iterator position from its view spec to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  inline typename pattern_type::local_index_t lpos() const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.lpos()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx    = _max_idx;
      offset = _idx - _max_idx;
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.lpos", _max_idx);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.lpos", idx);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.lpos", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.lpos", *_viewspec);
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    local_pos.index += offset;
    DASH_LOG_TRACE("BlockBoundaryIter.lpos >",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    return local_pos;
  }

  /**
   * Whether the iterator's position is relative to a view.
   *
   * \see DashGlobalIteratorConcept
   */
  inline bool is_relative() const noexcept
  {
    return _viewspec != nullptr;
  }

  /**
   * The view that specifies this iterator's index range.
   *
   * \see DashViewIteratorConcept
   */
  inline viewspec_type viewspec() const
  {
    if (_viewspec != nullptr) {
      return *_viewspec;
    }
    return viewspec_type(_pattern->memory_layout().extents());
  }

  inline const halospec_type & halospec() const noexcept
  {
    return _halospec;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  inline const GlobMem<ElementType> & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  inline GlobMem<ElementType> & globmem()
  {
    return *_globmem;
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++()
  {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t result = *this;
    --_idx;
    return result;
  }

  self_t & operator+=(index_type n)
  {
    _idx += n;
    return *this;
  }

  self_t & operator-=(index_type n)
  {
    _idx -= n;
    return *this;
  }

  self_t operator+(index_type n) const
  {
    if (_viewspec == nullptr) {
      self_t res(
        _globmem,
        *_pattern,
        _idx + static_cast<index_type>(n),
        _view_idx_offset);
      return res;
    }
    self_t res(
      _globmem,
      *_pattern,
      *_viewspec,
      _idx + static_cast<index_type>(n),
      _view_idx_offset);
    return res;
  }

  self_t operator-(index_type n) const
  {
    if (_viewspec == nullptr) {
      self_t res(
        _globmem,
        *_pattern,
        _idx - static_cast<index_type>(n),
        _view_idx_offset);
      return res;
    }
    self_t res(
      _globmem,
      *_pattern,
      *_viewspec,
      _idx - static_cast<index_type>(n),
      _view_idx_offset);
    return res;
  }

  index_type operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }

  inline bool operator<(const self_t & other) const
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::less<index_type>(),
                   std::less<pointer>());
  }

  inline bool operator<=(const self_t & other) const
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::less_equal<index_type>(),
                   std::less_equal<pointer>());
  }

  inline bool operator>(const self_t & other) const
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::greater<index_type>(),
                   std::greater<pointer>());
  }

  inline bool operator>=(const self_t & other) const
  {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::greater_equal<index_type>(),
                   std::greater_equal<pointer>());
  }

  inline bool operator==(const self_t & other) const
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

  inline bool operator!=(const self_t & other) const
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

  inline const PatternType & pattern() const
  {
    return *_pattern;
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template<
    class GlobIndexCmpFun,
    class GlobPtrCmpFun >
  bool compare(
    const self_t & other,
    const GlobIndexCmpFun & gidx_cmp,
    const GlobPtrCmpFun   & gptr_cmp) const
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
      return gidx_cmp(_idx, other._idx);
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) &&
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return gidx_cmp(_idx, other._idx);
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
    const pointer lhs_dart_gptr(dart_gptr());
    const pointer rhs_dart_gptr(other.dart_gptr());
    return gptr_cmp(lhs_dart_gptr, rhs_dart_gptr);
  }

  /**
   * Convert the given iterator position in border iteration space to
   * coordinates in the block view.
   *
   * NOTE:
   * This method could be specialized for NumDimensions = 1 for performance
   * tuning.
   */
  std::array<index_type, NumDimensions> coords(
    index_type glob_index) const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.coords()", glob_index);
    // Global cartesian coords of current iterator position:
    std::array<index_type, NumDimensions> glob_coords;
    if (_viewspec != nullptr) {
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.coords", *_viewspec);
      // Create cartesian index space from extents of view projection:
      CartesianIndexSpace<NumDimensions, Arrangement, index_type> index_space(
        _viewspec->extents());
      // Initialize global coords with view coords (coords of iterator
      // position in view index space):
      glob_coords = index_space.coords(glob_index);
      // Apply offset of view projection to view coords:
      for (dim_t d = 0; d < NumDimensions; ++d) {
        auto dim_offset  = _viewspec->offset(d);
        glob_coords[d]  += dim_offset;
      }
    } else {
      glob_coords = _pattern->memory_layout().coords(glob_index);
    }
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.coords >", glob_coords);
    return glob_coords;
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem<ElementType> * _globmem;
  /// View specifying the block region. Iteration space contains the view
  /// elements within the boundary defined by the halo spec.
  const viewspec_type  * _viewspec = nullptr;
  /// Pattern that created the encapsulated block.
  const pattern_type   * _pattern  = nullptr;
  /// Halo to apply to the encapsulated block.
  const halospec_type  * _halospec = nullptr;
  /// Iterator's position relative to the block border's iteration space.
  index_type             _idx      = 0;
  /// Number of elements in the block border's iteration space.
  index_type             _size     = 0;
  /// Maximum iterator position in the block border's iteration space.
  index_type             _max_idx  = 0;
  /// Unit id of the active unit
  dart_unit_t            _myid;
  /// Pointer to first element in local memory
  ElementType          * _lbegin   = nullptr;
};

template <
  typename ElementType,
  class    Pattern,
  class    Pointer,
  class    Reference >
std::ostream & operator<<(
  std::ostream & os,
  const dash::BlockBoundaryIter<ElementType, Pattern, Pointer, Reference> & i)
{
  std::ostringstream ss;
  dash::GlobPtr<ElementType, Pattern> ptr(i);
  ss << "dash::BlockBoundaryIter<" << typeid(ElementType).name() << ">("
     << "idx:"  << i._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

template <
  typename ElementType,
  class    Pattern,
  class    Pointer,
  class    Reference >
auto distance(
  /// Global iterator to the initial position in the global sequence
  const BlockBoundaryIter<ElementType, Pattern, Pointer, Reference> & first,
  /// Global iterator to the final position in the global sequence
  const BlockBoundaryIter<ElementType, Pattern, Pointer, Reference> & last
) -> typename Pattern::index_type
{
  return last - first;
}

template<
  typename ElementType,
  typename PatternType>
class BlockBoundaryView
{
private:
  typedef BlockBoundaryView<PatternType> self_t;

public:
  typedef       BlockBoundaryIter<ElementType, PatternType>         iterator;
  typedef const BlockBoundaryIter<ElementType, PatternType>   const_iterator;

  typedef PatternType                                           pattern_type;
  typedef typename PatternType::index_type                        index_type;
  typedef typename PatternType::size_type                          size_type;
  typedef typename PatternType::viewspec_type                  viewspec_type;

public:
  BlockBoundaryView(
    /// Global memory used to dereference iterated values.
    GlobMem<ElementType> * globmem,
    // Pattern that created the encapsulated block.
    const PatternType    & pattern,
    /// A block's inner or outer viewspec.
    const viewspec_type  & viewspec,
    // The halo to apply to the block.
    const halospec_type  & halospec)
  : _size(initialize_size(
            halospec,
            viewspec)),
    _beg(globmem, _pattern, _viewspec, _halospec, 0,     _size),
    _end(globmem, _pattern, _viewspec, _halospec, _size, _size)
  {
    dim_t d_v = NumDimensions - 1;
    for (dim_t d = 0; d < NumDimensions; ++d, --d_v) {
      _size += _halospec.width(d) * _viewspec.extent(d_v);
    }
  }

  /**
   * Copy constructor.
   */
  BlockBoundaryView(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Iterator pointing at first element in the view.
   */
  inline iterator begin() const {
    return _beg;
  }

  /**
   * Iterator pointing past the last element in the view.
   */
  inline const_iterator end() const {
    return _end;
  }

private:
  index_type initialize_size() const
  {
    index_type size = 0;
    dim_t d_v = NumDimensions - 1;
    for (dim_t d = 0; d < NumDimensions; ++d, --d_v) {
      size += halospec.width(d) * viewspec.extent(d_v);
    }
    return size;
  }

private:
  /// Iterator pointing at first element in the view.
  iterator   _beg;
  /// Iterator pointing past the last element in the view.
  iterator   _end;
  /// The number of elements in this view.
  index_type _size = 0;
};

/**
 * View type that encapsulates pattern blocks in halo semantics.
 *
 * Example:
 *
 * \code
 *   PatternType pattern(...);
 *   HaloSpec<2> halospec({ -1,1 }, { -1,1});
 *   HaloBlock<ValueType, PatternType> haloblock(
 *                                       globmem,
 *                                       pattern,
 *                                       pattern.block({ 1,2 },
 *                                       halospec);
 *   // create local copy of elements in west boundary:
 *   ValueType * boundary_copy = new ValueType[haloblock.boundary().size())];
 *   dash::copy(haloblock.boundary().begin(),
 *              haloblock.boundary().end(),
 *              boundary_copy);
 * \endcode
 */
template<
  typename ElementType,
  typename PatternType>
class HaloBlock
{
private:
  typedef HaloBlock<PatternType, ElementType>                         self_t;

public:
  typedef PatternType                                           pattern_type;
  typedef typename PatternType::index_type                        index_type;
  typedef typename PatternType::size_type                          size_type;
  typedef typename PatternType::viewspec_type                  viewspec_type;
  typedef BlockBoundaryView<PatternType, ElementType>     boundary_view_type;
  typedef BlockBoundaryView<PatternType, ElementType>         halo_view_type;

private:
  static const dim_t NumDimensions = PatternType::ndim();

public:
  /**
   * Creates a new instance of HaloBlock that extends a given pattern block
   * by halo semantics.
   */
  HaloBlock(
    /// Global memory used to dereference iterated values.
    GlobMem<ElementType> * globmem,
    // Pattern that created the encapsulated block.
    const pattern_type   & pattern,
    // View specifying the inner block region.
    const viewspec_type  & viewspec,
    // The halo to apply to the block.
    const halospec_type  & halospec)
  : _globmem(globmem),
    _pattern(&pattern)
    _viewspec_inner(&viewspec),
    _halospec(&halospec),
    _boundary_view(globmem, pattern, viewspec, halospec)
  {
    _viewspec_outer = _viewspec_inner;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      auto view_outer_offset_d = _halospec->offset_range(d).min;
      auto view_outer_extent_d = _viewspec_outer.extent(d) +
                                 _halospec.width(d);
      _viewspec_outer.resize_dim(d, view_outer_offset_d, view_outer_extent_d);
    }
    _halo_view = halo_view_type(
                   _globmem, *_pattern, _viewspec_outer, *_halospec);
  }

  /**
   * Default constructor.
   */
  HaloBlock() = default;

  /**
   * Copy constructor.
   */
  HaloBlock(const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const & self_t other) = default;

  /**
   * View specifying the inner block region.
   */
  inline const viewspec_t & inner() const
  {
    return *_viewspec_inner;
  }

  /**
   * View specifying the outer block region including halo.
   */
  inline const viewspec_t & outer() const
  {
    return _viewspec_outer;
  }

  /**
   * Proxy accessor providing iteration space of the block's boundary
   * cells.
   */
  inline const boundary_view_type & boundary() const
  {
    return _boundary_view;
  }

  /**
   * Proxy accessor providing iteration space of the block's halo cells.
   */
  inline const halo_view_type & halo() const
  {
    return _halo_view;
  }

  /**
   * The pattern instance that created the encapsulated block.
   */
  inline const PatternType & pattern() const
  {
    return *_pattern;
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem<ElementType> * globmem         = nullptr;
  /// The pattern that created the encapsulated block.
  const pattern_type   * _pattern        = nullptr;
  /// View specifying the original internal block region and its iteration
  /// space.
  const viewspec_type  * _viewspec_inner = nullptr;
  /// The halo to apply to the encapsulated block.
  const halospec_type  * _halospec       = nullptr;
  /// Offsets of the inner viewspec are used as origin reference.
  /// The outer viewspec is offset the halo's minimal neighbor offsets and
  /// its extends are enlarged by halo width in every dimension.
  /// For example, the outer view for a 9-point stencil for two-dimensional
  /// Von Neumann neighborhood has halospec ((-2,2), (-2,2)).
  /// If the inner view has offsets (12, 20) and extents (23,42), the outer
  /// view has offsets
  /// (12-2, 20-2) = (10,18)
  /// and extents
  /// (23+4, 42+4) = (27,46).
  viewspec_type          _viewspec_outer;
  boundary_view_type     _boundary_view;
  halo_view_type         _halo_view;

}; // class HaloBlock

} // namespace dash

#endif // DASH__HALO_H__
