#ifndef DASH__HALO_VIEW_H__
#define DASH__HALO_VIEW_H__

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
 */
template<typename PatternType>
class BlockBoundaryIter
{
public:
  typedef PatternType                                pattern_type;
  typedef typename PatternType::index_type             index_type;
  typedef typename PatternType::size_type               size_type;
  typedef typename PatternType::viewspec_type       viewspec_type;

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
    _lbegin(_globmem->lbegin()),
  {
    if (size <= 0) {
      dim_t d_v = NumDimensions - 1;
      for (dim_t d = 0; d < NumDimensions; ++d, --d_v) {
        _size += _halospec->width(d) * _viewspec->extent(d_v);
      }
    }
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", _idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", _size);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", *_viewspec);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter(gmem,p,vs,hs,idx,sz)", *_halospec);
  }

private:
  /**
   * Convert the given iterator position in border iteration space to
   * coordinates in the block view.
   */
  std::array<NumDimensions, index_type> coords(index_type pos) const
  {
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem<ElementType> * _globmem;
  /// View specifying the block region. Iteration space contains the view
  /// elements within the boundary defined by the halo spec.
  const viewspec_type  * _viewspec = nullptr;
  /// The pattern that created the encapsulated block.
  const pattern_type   * _pattern  = nullptr;
  /// The halo to apply to the encapsulated block.
  const halospec_type  * _halospec = nullptr;
  /// The iterator's position relative to the block border's iteration space.
  index_type             _idx      = 0;
  /// The number of elements in the block border's iteration space.
  index_type             _size     = 0;
};

template<typename PatternType>
class BlockBoundaryView
{
private:
  typedef BlockBoundaryView<PatternType> self_t;

public:
  typedef       BlockBoundaryIter<PatternType>           iterator;
  typedef const BlockBoundaryIter<PatternType>     const_iterator;

  typedef PatternType                                pattern_type;
  typedef typename PatternType::index_type             index_type;
  typedef typename PatternType::size_type               size_type;
  typedef typename PatternType::viewspec_type       viewspec_type;

public:
  BlockBoundaryView(
    const PatternType   & pattern,
    /// A block's inner or outer viewspec.
    const viewspec_type & viewspec,
    // The halo to apply to the block.
    const halospec_type & halospec)
  : _size(initialize_size(
            halospec,
            viewspec)),
    _beg(_pattern, _viewspec, _halospec, 0,     _size),
    _end(_pattern, _viewspec, _halospec, _size, _size)
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
 *   HaloBlock<PatternType> haloblock(
 *                            pattern,
 *                            pattern.block({ 1,2 },
 *                            halospec);
 *   // create local copy of elements in west boundary:
 *   ValueType * boundary_copy = new ValueType[haloblock.boundary().size())];
 *   dash::copy(haloblock.boundary().begin(),
 *              haloblock.boundary().end(),
 *              boundary_copy);
 * \endcode
 */
template<typename PatternType>
class HaloBlock
{
private:
  typedef HaloBlock<PatternType> self_t;

public:
  typedef PatternType                               pattern_type;
  typedef typename PatternType::index_type            index_type;
  typedef typename PatternType::size_type              size_type;
  typedef typename PatternType::viewspec_type      viewspec_type;
  typedef BlockBoundaryView<PatternType>      boundary_view_type;
  typedef BlockBoundaryView<PatternType>          halo_view_type;

private:
  static const dim_t NumDimensions = PatternType::ndim();

public:
  /**
   * Creates a new instance of HaloBlock that extends a given pattern block
   * by halo semantics.
   */
  HaloBlock(
    // Pattern that created the encapsulated block.
    const pattern_type  & pattern,
    // View specifying the inner block region.
    const viewspec_type & viewspec,
    // The halo to apply to the block.
    const halospec_type & halospec)
  : _pattern(&pattern)
    _viewspec_inner(&viewspec),
    _halospec(&halospec),
    _boundary_view(pattern, viewspec, halospec)
  {
    _viewspec_outer = _viewspec_inner;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      auto view_outer_offset_d = _halospec->offset_range(d).min;
      auto view_outer_extent_d = _viewspec_outer.extent(d) +
                                 _halospec.width(d);
      _viewspec_outer.resize_dim(d, view_outer_offset_d, view_outer_extent_d);
    }
    _halo_view = halo_view_type(*_pattern, _viewspec_outer, *_halospec);
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
  /// The pattern that created the encapsulated block.
  const pattern_type  * _pattern        = nullptr;
  /// View specifying the original internal block region and its iteration
  /// space.
  const viewspec_type * _viewspec_inner = nullptr;
  /// The halo to apply to the encapsulated block.
  const halospec_type * _halospec       = nullptr;
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
  viewspec_type         _viewspec_outer;
  boundary_view_type    _boundary_view;
  halo_view_type        _halo_view;

}; // class HaloBlock

} // namespace dash

#endif // DASH__HALO_VIEW_H__
