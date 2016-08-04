#ifndef DASH__HALO_H__
#define DASH__HALO_H__

#include <dash/GlobMem.h>

#include <dash/internal/Logging.h>

#include <functional>

namespace dash {

/**
 * The concepts defined in the following extend the abstraction of
 * multidimensional block and views in DASH by halo- and stencil capabilities.
 * The \c HaloBlock type acts as a wrapper of blocks reminussented by any
 * implementation of the \c ViewSpec concept and extends these by boundary-
 * and halo regions.
 *
 * As known from classic stencil algorithms, *boundaries* are the outermost
 * elements within a block that are requested by neighoring units.
 * *Halos* reminussent additional outer regions of a block that contain ghost
 * cells with values copied from adjacent units' boundary regions.
 *
 * For this, halo blocks require the following index spaces:
 * - the conventional *iteration space* over the block elements
 * - the *allocation space* that includes block elements and the block's halo
 *   regions
 * - the *boundary space* for iterating elements in all or singular block
 *   boundary regions
 * - the *halo space* for iterating elements in all or singular block
 *   halo regions
 *
 * Example for an outer block boundary iteration space (halo regions):
 *
 *                               .-- halo region 0
 *                              /
 *                .-------------------------. -.
 *                |  0  1  2  3  4  5  6  7 |  |
 *                |  8  9 10 11 12 13 14 15 |  |-- halo width in dimension 0
 *                |  8  9 10 11 12 13 14 15 |  |
 *                `-------------------------' -'
 *       .-------..-------------------------..-------.
 *       | 16 17 ||                         || 30 31 |
 *       :  ...  ::          block          ::  ...  : --- halo region 3
 *       | 28 29 ||                         || 42 43 |
 *       '-------''-------------------------''-------'
 *           :    .-------------------------.:       :
 *           |    | 44 45 46 47 48 49 50 51 |'---.---'
 *           |    | 52 53 54 55 56 57 58 59 |    :
 *           |    `-------------------------'    '- halo width in dimension 1
 *           '                  \
 *     halo region 2             '- halo region 1
 *
 *
 * Example for an inner block boundary iteration space:
 *
 *                      boundary region 0
 *                              :
 *          .-------------------'--------------------.
 *         |                                         |
 *       _ .-------.-------------------------.-------. _  __
 *      |  |  0  1 |  3  4  5  6  7  8  9 10 | 12 13 |  |   |   halo width in
 *      |  | 14 15 | 17 18 19 20 21 22 23 24 | 26 27 |  |   +-- dimension 0
 *      |  | 28 29 | 31 32 33 34 35 36 37 38 | 40 41 |  |   |
 *      |  :-------+-------------------------+-------:  | --'
 *      |  | 42 43 |                         | 56 57 |  |
 *    .-|  :  ...  :   inner block region    :  ...  :  +- boundary
 *    | |  | 54 55 |                         | 68 69 |  |  region 3
 *    | |  :-------+-------------------------+-------:  |
 *    | |  | 70 71 | 73 74 75 76 77 78 79 80 |       |  |
 *    | |  | 70 71 | 73 74 75 76 77 78 79 80 |  ...  |  |
 *    | |  | 84 85 | 87 88 89 90 91 92 93 94 |       |  |
 *    | '- `-------'-------------------------'-------' -'
 *    |    |                                         |
 *    |    `--------------------.------------+-------:
 *    :                         :            '---.---'
 *  boundary region 2   boundary region 1        '-------- halo width in
 *                                                         dimension 1
 */

enum class HaloRegion: std::uint8_t{
  minus,
  plus
};


template<dim_t NumDimensions>
class HaloSpec
{
public:
  using offset_t = unsigned int;
  using points_size_t = unsigned int;

  struct halo_offset_pair_t
  {
    offset_t minus;
    offset_t plus;
  };

private:
  using halo_offsets_t = std::array<halo_offset_pair_t, NumDimensions>;

public:
  /**
   * Creates a new instance of class HaloSpec with the given offset ranges
   * (pair of minus offset, plus offset) in the stencil's dimensions.
   *
   * For example, a two-dimensional five-point stencil has offset ranges
   * { (1, 1), (1, 1) }
   * and a stencil with only north and east halo cells has offset ranges
   * { (1, 0), ( 0, 1) }.
   */
  HaloSpec(const halo_offsets_t& halo_offsets)
  : _halo_offsets(halo_offsets)
  {
    // minimum stencil size when containing center element only:
    _points = 1;
    for(const auto& offset : _halo_offsets)
      _points += offset.minus + offset.plus;
  }

  /**
   * Creates a new instance of class HaloSpec with the given offset ranges
   * (pair of minus offset, plus offset) in the stencil's dimensions.
   *
   * For example, a two-dimensional five-point stencil has offset ranges
   * { (1, 1), (1, 1) }
   * and a stencil with only north and east halo cells has offset ranges
   * { (1, 0), ( 0, 1) }.
   */
  template<typename... Args>
  HaloSpec(halo_offset_pair_t arg, Args... args)
  {
    static_assert(sizeof...(Args) == NumDimensions-1,
                  "Invalid number of offset ranges");
    _halo_offsets = { arg, static_cast<halo_offset_pair_t>(args)... };
    // minimum stencil size when containing center element only:
    _points = 1;
    for(const auto& offset : _halo_offsets)
    {
      _points += offset.minus + offset.plus;
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
    for(auto& offset : _halo_offsets)
      offset = {0, 0};
  }

  /**
   * The stencil's number of dimensions.
   */
  static constexpr dim_t ndim()
  {
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
  const halo_offset_pair_t & halo_offset(dim_t dimension) const
  {
    return _halo_offsets[dimension];
  }

  /**
   * Offset ranges (minimum and maximum offset) all dimensions.
   */
  const halo_offsets_t & halo_offsets() const
  {
    return _halo_offsets;
  }

  /**
   * Width of the halo in the given dimension.
   */
  offset_t width(dim_t dimension) const
  {
    auto offset = _halo_offsets[dimension];
    return offset.minus + offset.plus;
  }

private:
  /// The stencil's offset range (min, max) in every dimension.
  halo_offsets_t                                _halo_offsets;
  /// Number of points in the stencil.
  points_size_t                                 _points;

};

template<dim_t NumDimensions>
std::ostream & operator<<(
  std::ostream & os,
  const dash::HaloSpec<NumDimensions>& halospec)
{
  std::ostringstream ss;
  ss << "dash::HaloSpec<" << NumDimensions << ">(";
  for (const auto& offset : halospec.halo_offsets) {
    ss << "{ " << offset.minus
       << ", " << offset.plus
       << " }";
  }
  ss << ")";
  return operator<<(os, ss.str());
}

template< typename ElementT, typename PatternT,
  typename PointerT   = GlobPtr<ElementT, PatternT>,
  typename ReferenceT = GlobRef<ElementT> >
class HaloBlockIter : public std::iterator< std::random_access_iterator_tag, ElementT,
           typename PatternT::index_type, PointerT, ReferenceT >
{
private:
  using self_t = HaloBlockIter<ElementT, PatternT, PointerT, ReferenceT>;
  using GlobMem_t = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;

  static const dim_t      NumDimensions = PatternT::ndim();
  //static const MemArrange Arrangement   = PatternT::memory_order();

public:
  using index_type  = typename PatternT::index_type;
  using size_type   = typename PatternT::size_type;
  using viewspec_t  = typename PatternT::viewspec_type;

  using reference       = ReferenceT;
  using const_reference = const reference;
  using pointer         = PointerT;
  using const_pointer   = const pointer;

  //using  position_mapping_function =
  //  std::function<std::array<index_type, NumDimensions>(index_type)>;

public:
  /**
   * Constructor, creates a block boundary iterator on multiple boundary
   * regions.
   */
  HaloBlockIter(const GlobMem_t & globmem, const PatternT & pattern,
      const viewspec_t & halo_region, index_type pos, index_type size)
  : _globmem(globmem),
    _pattern(pattern),
    _halo_region(halo_region),
    _idx(pos),
    _size(size),
    _max_idx(_size-1),
    _myid(dash::myid())
    //_position_to_coords(position_mapping_func)
  {
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _idx);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _max_idx);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _size);
  }

  /**
   * Constructor, creates a block boundary iterator using a custom function
   * for mapping iterator position to global coordinate space.
   */
  /*HaloBlockIter(const GlobMem_t & globmem, const PatternT & pattern,
      const std::vector<viewspec_t> & boundary_regions,
      const position_mapping_function & position_mapping_func,
      index_type pos, index_type size)
  : _globmem(globmem),
    _pattern(pattern),
    _idx(pos),
    _size(size),
    _max_idx(_size-1),
    _myid(dash::myid()),
    _position_to_coords(position_mapping_func)
  {
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _idx);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _max_idx);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _size);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", *_viewspec);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", *_halospec);
  }*/

  /**
   * Copy constructor.
   */
  HaloBlockIter(const self_t & other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * The number of dimensions of the iterator's underlying pattern.
   *
   * \see DashGlobalIteratorConcept
   */
  static dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const
  {
    DASH_LOG_TRACE("GlobStencilIter.*", _idx, _view_idx_offset);

    return operator[](_idx);
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  reference operator[](index_type idx) const
  {
    const auto & layout = _pattern.memory_layout();
    auto coords = layout.coords(idx, _halo_region);
    auto local_pos = _pattern.local_index(coords);

    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.index);
    // Global reference to element at given position:
    return reference(_globmem.at(local_pos.unit, local_pos.index));
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  /*bool is_local() const
  {
    return (_myid == lpos().unit);
  }*/

  /**
   * Position of the iterator in global storage order.
   *
   * \see DashGlobalIteratorConcept
   */
  index_type pos() const
  {
    DASH_LOG_TRACE("HaloBlockIter.pos()",
                   "idx:", _idx);
    return _idx;
  }

  /**
   * Position of the iterator in its view's iteration space, disregarding
   * the view's offset in global index space.
   *
   * \see DashViewIteratorConcept
   */
  index_type rpos() const
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   * Projects iterator position from its view spec to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  /*index_type gpos() const
  {
    DASH_LOG_TRACE_VAR("HaloBlockIter.gpos()", _idx);
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos >", _idx);
      return _idx;
    } else {
      index_type idx    = _idx;
      index_type offset = 0;
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos", *_viewspec);
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos", _max_idx);
      // Convert iterator position (_idx) to local index and unit.
      if (_idx > _max_idx) {
        // Global iterator pointing past the range indexed by the pattern
        // which is the case for .end() iterators.
        idx    = _max_idx;
        offset = _idx - _max_idx;
      }
      // Viewspec projection required:
      auto g_coords = _position_to_coords(idx);
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos", _idx);
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos", g_coords);
      auto g_idx    = _pattern->memory_layout().at(g_coords);
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos", g_idx);
      g_idx += offset;
      DASH_LOG_TRACE_VAR("HaloBlockIter.gpos >", g_idx);
      return g_idx;
    }
  }*/

  /**
   * Unit and local offset at the iterator's position.
   * Projects iterator position from its view spec to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  /*typename pattern_type::local_index_t lpos() const
  {
    DASH_LOG_TRACE_VAR("HaloBlockIter.lpos()", _idx);
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
      DASH_LOG_TRACE_VAR("HaloBlockIter.lpos", _max_idx);
      DASH_LOG_TRACE_VAR("HaloBlockIter.lpos", idx);
      DASH_LOG_TRACE_VAR("HaloBlockIter.lpos", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      DASH_LOG_TRACE_VAR("HaloBlockIter.lpos", *_viewspec);
      auto glob_coords = _position_to_coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    local_pos.index += offset;
    DASH_LOG_TRACE("HaloBlockIter.lpos >",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    return local_pos;
  }*/

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  const GlobMem_t & globmem() const
  {
    return _globmem;
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
    self_t res{*this};
    res += n;

    return res;
  }

  self_t operator-(index_type n) const
  {
    self_t res{*this};
    res -= n;

    return res;
  }

  /*index_type operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }*/

  bool operator<(const self_t & other) const
  {
    return compare(other, std::less<index_type>());
  }

  bool operator<=(const self_t & other) const
  {
    return compare(other, std::less_equal<index_type>());
  }

  bool operator>(const self_t & other) const
  {
    return compare(other, std::greater<index_type>());
  }

  bool operator>=(const self_t & other) const
  {
    return compare(other, std::greater_equal<index_type>());
  }

  bool operator==(const self_t & other) const
  {
    return compare(other, std::equal_to<index_type>());
  }

  bool operator!=(const self_t & other) const
  {
    return compare(other, std::not_equal_to<index_type>());
  }

  const PatternT & pattern() const
  {
    return _pattern;
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template<typename GlobIndexCmpFunc>
  bool compare( const self_t & other, const GlobIndexCmpFunc & gidx_cmp) const
  {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if (this == &other) {
      return true;
    }
#endif
    if (&_halo_region == &(other._halo_region) ||
        _halo_region == other._halo_region)
    {
      return gidx_cmp(_idx, other._idx);
    }
  // TODO not the best solution
    return false;
  }

  /**
   * Convert the given iterator position in border iteration space to
   * coordinates in the block view.
   *
   * NOTE:
   * This method could be specialized for NumDimensions = 1 for performance
   * tuning.
   */
  /*std::array<index_type, NumDimensions> coords(const std::vector<viewspec_type> * boundary_regions,
    index_type boundary_pos) const
  {
    DASH_LOG_TRACE_VAR("HaloBlockIter.coords<OUTER>()", boundary_pos);
    // _halo_regions contains views of the boundary's halo regions in their
    // canonical storage order. Subtract halo region size from boundary
    // position until it is smaller than the current halo region. This
    // yields the referenced helo and maps the boundary position to its
    // offset in the halo region in the same pass.
    // Some overhead for bookkeeping the halo regions, but this method also
    // works for irregular halos.
    int ri = 0;
    // offset of boundary position in halo region (= phase):
    auto halo_region_pos = boundary_pos;
    for (; ri < boundary_regions->size(); ++ri) {
      auto halo_region_size = (*boundary_regions)[ri].size();
      if (halo_region_pos < halo_region_size) {
        break;
      }
      halo_region_pos -= halo_region_size;
    }
    auto halo_region        = (*boundary_regions)[ri];
    // resolve coordinate in halo region from iterator boundary position:
    auto halo_region_coords = CartesianIndexSpace<NumDimensions>(
                                halo_region.extents())
                              .coords(halo_region_pos);
    // apply view offsets to resolve global cartesian coords of current
    // iterator position:
    std::array<index_type, NumDimensions> glob_coords = halo_region_coords;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      glob_coords[d] += halo_region.offsets()[d];
    }
    DASH_LOG_TRACE_VAR("HaloBlockIter.coords >", glob_coords);
    return glob_coords;
  }*/

private:
  /// Global memory used to dereference iterated values.
  const GlobMem_t &                  _globmem;
  /// Pattern that created the encapsulated block.
  const PatternT &                   _pattern;

  const viewspec_t &                 _halo_region;
  /// Iterator's position relative to the block border's iteration space.
  index_type                         _idx{0};
  /// Number of elements in the block border's iteration space.
  index_type                         _size{0};
  /// Maximum iterator position in the block border's iteration space.
  index_type                         _max_idx{0};
  /// Unit id of the active unit
  dart_unit_t                        _myid;
  /// Function implementing mapping of iterator position to global element
  /// coordinates.
  //position_mapping_function          _position_to_coords;

}; // class HaloBlockIter

template <typename ElementT, typename PatternT, typename PointerT, typename ReferenceT>
std::ostream & operator<<(std::ostream & os,
    const HaloBlockIter<ElementT, PatternT, PointerT, ReferenceT> & i)
{
  std::ostringstream ss;
  dash::GlobPtr<ElementT, PatternT> ptr(i);
  ss << "dash::HaloBlockIter<" << typeid(ElementT).name() << ">("
     << "idx:"  << i._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

template <typename ElementT, typename PatternT, typename PointerT, typename ReferenceT>
auto distance(
  /// Global iterator to the initial position in the global sequence
  const HaloBlockIter<ElementT, PatternT, PointerT, ReferenceT> & first,
  /// Global iterator to the final position in the global sequence
  const HaloBlockIter<ElementT, PatternT, PointerT, ReferenceT> & last
) -> typename PatternT::index_type
{
  return last - first;
}

template<typename ElementT, typename PatternT>
class HaloBlockView
{
private:
  using self_t = HaloBlockView<ElementT, PatternT>;
  using GlobMem_t = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;

  static const dim_t NumDimensions = PatternT::ndim();

public:
  using iterator       = HaloBlockIter<ElementT, PatternT>;
  using iterator_const = const iterator;

  using index_type  = typename PatternT::index_type;
  using size_type   = typename PatternT::size_type;
  using viewspec_t  = typename PatternT::viewspec_type;

  //using position_mapping_function =
  //  std::function<std::array<index_type, NumDimensions>(index_type)>;

public:
  /*BlockBoundaryView(const GlobMem_t & globmem, const PatternT & pattern,
    const std::vector<viewspec_type> & boundary_regions)
  : _size(
      std::accumulate(boundary_regions.begin(), boundary_regions.end(), 0,
        [](const size_type& size, const auto& view){ return view.size() + size })),
    _size([&boundary_regions]()
      {
        index_type size = 0;
        for(auto view : *boundary_regions)
          size += view.size();

        return size;
      }()),
    _beg(globmem, pattern, boundary_regions,     0, _size, _position_coords),
    _end(globmem, pattern, boundary_regions, _size, _size, _position_coords)
  {}*/

  HaloBlockView(const GlobMem_t & globmem, const PatternT & pattern,
    const viewspec_t &  halo_region)
  : _size(halo_region.size()),
    /*_position_coords([&boundary_region,this](index_type i_type)
      {
        return boundary_coords(boundary_region, i_type);
      }),*/
    _beg(globmem, pattern, halo_region,     0, _size),
    _end(globmem, pattern, halo_region, _size, _size)
  {}

  /**
   * Copy constructor.
   */
  HaloBlockView(const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * Iterator pointing at first element in the view.
   */
  iterator begin() const
  {
    return _beg;
  }

  /**
   * Iterator pointing past the last element in the view.
   */
  iterator_const end() const
  {
    return _end;
  }

  /**
   * The number of elements in the view.
   */
  const size_type size() const
  {
    return _size;
  }

private:
  /**
   * Convert the given iterator position in border iteration space to
   * coordinates in the block view.
   */
  /*std::array<index_type, NumDimensions> boundary_coords(
    const viewspec_t &    boundary_region,
    index_type            boundary_pos) const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryView.boundary_coords()", boundary_pos);
    // resolve coordinate in halo region from iterator boundary position:
    auto glob_coords = CartesianIndexSpace<NumDimensions>(
                           boundary_region.extents())
                         .coords(boundary_pos);

    for (dim_t d = 0; d < NumDimensions; ++d)
      glob_coords[d] += boundary_region.offsets()[d];

    DASH_LOG_TRACE_VAR("BlockBoundaryView.boundary_coords >", glob_coords);
    return glob_coords;
  }*/

private:
  /// The number of elements in this view.
  size_type                 _size{0};
  /// Function mapping iterator position to global coordinates.
  //position_mapping_function _position_coords;
  /// Iterator pointing at first element in the view.
  iterator                  _beg;
  /// Iterator pointing past the last element in the view.
  iterator                  _end;
}; // class BlockBoundaryView

template<typename ElementT, typename PatternT>
class HaloBlock
{
private:
  using self_t = HaloBlock<ElementT, PatternT>;
  using GlobMem_t = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;

  static const dim_t NumDimensions = PatternT::ndim();


public:
  using index_type = typename PatternT::index_type;
  using size_type = typename PatternT::size_type;
  using viewspec_t = typename PatternT::viewspec_type;
  using halospec_t = HaloSpec<NumDimensions>;
  using block_view_t = HaloBlockView<ElementT, PatternT>;

public:
  /**
   * Creates a new instance of HaloBlock that extends a given pattern block
   * by halo semantics.
   */
  HaloBlock(const GlobMem_t & globmem, const PatternT & pattern, const viewspec_t &  viewspec,
    const halospec_t & halospec)
  : _globmem(globmem), _pattern(pattern), _viewspec_inner(viewspec), _halospec(halospec)
  {
    _viewspec_outer = viewspec;
    _halo_regions.reserve(NumDimensions * 2);
    _boundary_regions.reserve(NumDimensions * 2);

    auto view_outer_offsets = viewspec.offsets();
    auto view_outer_extents = viewspec.extents();

    for (dim_t d = 0; d < NumDimensions; ++d)
    {
      auto idx = d * 2;
      auto halo_offs_minus   = halospec.halo_offset(d).minus;
      auto halo_offs_plus  = halospec.halo_offset(d).plus;

      view_outer_offsets[d] -= halo_offs_minus;
      view_outer_extents[d] += halo_offs_minus + halo_offs_plus;

      /* initializes boundary views (including empty once) */
      auto bnd_region_offsets  = viewspec.offsets();
      auto bnd_region_extents  = viewspec.extents();

      /* initializes halo views (including empty once) */
      auto halo_region_offsets = viewspec.offsets();
      auto halo_region_extents = viewspec.extents();


      bnd_region_extents[d]  = halo_offs_minus;
      _boundary_regions[idx] = viewspec_t(bnd_region_offsets,  bnd_region_extents);

      bnd_region_offsets[d]    = viewspec.offset(d) + bnd_region_extents[d] - halo_offs_minus;
      bnd_region_extents[d]    = halo_offs_plus;
      _boundary_regions[idx+1] = viewspec_t(bnd_region_offsets, bnd_region_extents);

      // halo extends to negative direction, e.g. west or north:
      halo_region_offsets[d]  -= halo_offs_minus;
      halo_region_extents[d]   = halo_offs_minus;
      _halo_regions[idx] = viewspec_t(halo_region_offsets, halo_region_extents);
      // halo extends to positive direction, e.g. east or south:
      halo_region_offsets[d]  = viewspec.offset(d) + viewspec.extent(d);
      halo_region_extents[d]  = halo_offs_plus;
      _halo_regions[idx+1] = viewspec_t(halo_region_offsets, halo_region_extents);
    }

    _viewspec_outer = viewspec_t(view_outer_offsets, view_outer_extents);
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
  self_t & operator=(const self_t & other) = default;

  /**
   * The pattern instance that created the encapsulated block.
   */
  const PatternT & pattern() const
  {
    return _pattern;
  }

  const halospec_t & halospec() const
  {
    return _halospec;
  }

  /**
   * Creates view on halo region for a given dimension and halo region.
   * For example, the east halo region in a two-dimensional block
   * has (1, dash::HaloRegion::plus).
   */
  //TODO Which value the first dimension has? 0 or 1?
  block_view_t halo_region(dim_t dimension, HaloRegion region)
  {
    auto region_index = 2 * dimension + static_cast<char>(region);

    return block_view_t(_globmem, _pattern, _halo_regions[region_index]);
  }

  /**
   * Creates view on boundary region for a given dimension and boundary region.
   * For example, the east boundary region in a two-dimensional block
   * has (1, dash::HaloRegion::plus).
   */
  //TODO Which value the first dimension has? 0 or 1?
  block_view_t boundary_region(dim_t dimension, HaloRegion region)
  {
    auto region_index = 2 * dimension + static_cast<char>(region);

    return block_view_t(_globmem, _pattern, _boundary_regions[region_index]);
  }

  /**
   * View specifying the inner block region.
   */
  const viewspec_t & inner() const {
    return _viewspec_inner;
  }

  /**
   * View specifying the outer block region including halo.
   */
  const viewspec_t & outer() const {
    return _viewspec_outer;
  }

private:
  const GlobMem_t &         _globmem;
  /// The pattern that created the encapsulated block.
  const PatternT &          _pattern;

  /// View specifying the original internal block region and its iteration
  /// space.
  const viewspec_t &        _viewspec_inner;

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
  viewspec_t              _viewspec_outer;

  /// The halo to apply to the encapsulated block.
  const halospec_t &      _halospec;

  /// Viewspecs for all contiguous halo regions in the halo block.
  std::vector<viewspec_t> _halo_regions;

  std::vector<viewspec_t> _boundary_regions;

}; // class HaloBlock


}//namespace dash
#endif // DASH__HALO_H__
