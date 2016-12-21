#ifndef DASH__EXPERIMENTAL__HALO_H__
#define DASH__EXPERIMENTAL__HALO_H__

#include <dash/GlobMem.h>
#include <dash/iterator/GlobIter.h>

#include <dash/internal/Logging.h>

#include <functional>

namespace dash {
namespace experimental {

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
 * \code
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
 * \endcode
 *
 * Example for an inner block boundary iteration space:
 *
 * \code
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
 * \endcode
 *
 */



enum class HaloRegion: std::uint8_t{
  MINUS,
  PLUS,
  COUNT //number of regions
};


template<dim_t NumDimensions>
class HaloSpec
{
public:
  using offset_t = int;
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
      _points += std::abs(offset.minus) + offset.plus;
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
      _points += std::abs(offset.minus) + offset.plus;
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
    return std::abs(offset.minus) + offset.plus;
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
  const dash::experimental::HaloSpec<NumDimensions>& halospec)
{
  std::ostringstream ss;
  ss << "dash::HaloSpec<" << NumDimensions << ">(";
  for (const auto& offset : halospec.halo_offsets()) {
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
class HaloBlockIter
: public std::iterator<
    std::random_access_iterator_tag,
    ElementT,
    typename PatternT::index_type,
    PointerT,
    ReferenceT >
{
private:
  using self_t    = HaloBlockIter<ElementT, PatternT, PointerT, ReferenceT>;
  using GlobMem_t = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;

  static const dim_t      NumDimensions = PatternT::ndim();
//static const MemArrange Arrangement   = PatternT::memory_order();

public:
  using has_view        = std::integral_constant<bool, true>;
  using pattern_type    = PatternT;

  using index_type      = typename PatternT::index_type;
  using size_type       = typename PatternT::size_type;
  using viewspec_t      = typename PatternT::viewspec_type;

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
  HaloBlockIter(GlobMem_t & globmem, const PatternT & pattern,
      const viewspec_t & halo_region, index_type pos, index_type size)
  : _globmem(globmem),
    _pattern(pattern),
    _halo_region(halo_region),
    _idx(pos),
    _size(size),
    _max_idx(_size-1),
    _myid(dash::myid()),
    _lbegin(globmem.lbegin())
    //_position_to_coords(position_mapping_func)
  {
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _idx);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _max_idx);
    DASH_LOG_TRACE_VAR("HaloBlockIter()", _size);
  }

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
  static constexpr dim_t ndim()
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
    auto local_pos = _pattern.local_index(glob_coords(idx));

    return reference(_globmem.at(local_pos.unit, local_pos.index));
  }

  dart_gptr_t dart_gptr() const
  {
    return operator[](_idx).dart_gptr();
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  bool is_local() const
  {
    return (_myid == lpos().unit);
  }

  GlobIter<ElementT, PatternT> global() const
  {
    auto g_idx = gpos();
    return GlobIter<ElementT, PatternT>(&_globmem, _pattern, g_idx);
  }

  ElementT* local() const
  {
    auto local_pos = lpos();

    if(_myid != local_pos.unit)
      return nullptr;

    //
    return _lbegin + local_pos.index;
  }

  /**
   * Position of the iterator in global storage order.
   *
   * \see DashGlobalIteratorConcept
   */
  index_type pos() const
  {
    return gpos();
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
  index_type gpos() const
  {
    return _pattern.memory_layout().at(glob_coords(_idx));
  }

  typename PatternT::local_index_t lpos() const
  {
    return  _pattern.local_index(glob_coords(_idx));
  }

  viewspec_t viewspec() const
  {
    return _halo_region;
  }

  inline bool is_relative() const noexcept
  {
    return true;
  }

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

  index_type operator+(const self_t & other) const
  {
    return _idx + other._idx;
  }

  self_t operator-(index_type n) const
  {
    self_t res{*this};
    res -= n;

    return res;
  }

  index_type operator-(const self_t & other) const
  {
    return _idx - other._idx;
  }

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

  std::array<index_type, NumDimensions> glob_coords(index_type idx) const
  {
    return _pattern.memory_layout().coords(idx, _halo_region);
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem_t &                        _globmem;
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

  ElementT *                         _lbegin;
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

public:
  HaloBlockView(GlobMem_t & globmem, const PatternT & pattern, const viewspec_t &  halo_region)
  : _halo_region(halo_region), _size(_halo_region.size()),
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

  const viewspec_t & region_view() const
  {
    return _halo_region;
  }

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
  const viewspec_t &        _halo_region;
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

public:
  static constexpr dim_t NumDimensions = PatternT::ndim();

  using index_type   = typename PatternT::index_type;
  using size_type    = typename PatternT::size_type;
  using value_t      = ElementT;
  using viewspec_t   = typename PatternT::viewspec_type;
  using halospec_t   = HaloSpec<NumDimensions>;
  using block_view_t = HaloBlockView<ElementT, PatternT>;

public:
  /**
   * Creates a new instance of HaloBlock that extends a given pattern block
   * by halo semantics.
   */
  HaloBlock(GlobMem_t & globmem, const PatternT & pattern, const viewspec_t &  view,
    const halospec_t & halospec)
  : _globmem(globmem), _pattern(pattern), _view(view), _halospec(halospec)
  {
    _halo_regions.reserve(NumDimensions * 2);
    _boundary_regions.reserve(NumDimensions * 2);

    _view_outer = view;
    _view_inner = view;
    _view_save  = view;

    for (dim_t d = 0; d < NumDimensions; ++d)
    {
      auto halo_offs_minus = std::abs(halospec.halo_offset(d).minus);
      auto halo_offs_plus  = halospec.halo_offset(d).plus;


      auto region_offsets = view.offsets();
      auto region_extents = view.extents();
      auto view_offset    = view.offset(d);
      auto view_extent    = view.extent(d);

      if(halo_offs_minus == 0 || view_offset < halo_offs_minus)
      {
        _view_save.resize_dim(d, view_offset + halo_offs_minus, view_extent - halo_offs_minus);
        _view_inner.resize_dim(d, view_offset + halo_offs_minus, view_extent - halo_offs_minus);

        _boundary_regions.push_back(viewspec_t{});
        _halo_regions.push_back(viewspec_t{});
      }
      else
      {
        _view_outer.resize_dim(d, view_offset - halo_offs_minus, view_extent + halo_offs_minus);
        _view_inner.resize_dim(d, view_offset + halo_offs_minus, view_extent - halo_offs_minus);

        region_extents[d]      = halo_offs_minus;
        _boundary_regions.push_back(viewspec_t(region_offsets,  region_extents));

        setBndElems(d, region_offsets, region_extents);

        region_offsets[d]   = view_offset - halo_offs_minus;
        region_extents[d]   = halo_offs_minus;
        _halo_regions.push_back(viewspec_t(region_offsets, region_extents));
      }

      if(halo_offs_plus == 0 ||
          std::abs(view_offset + view_extent + halo_offs_plus) > _pattern.extent(d))
      {
        _view_save.resize_dim(d, _view_save.offset(d), _view_save.extent(d) - halo_offs_plus);
        _view_inner.resize_dim(d, _view_inner.offset(d), _view_inner.extent(d) - halo_offs_plus);

        _boundary_regions.push_back(viewspec_t{});
        _halo_regions.push_back(viewspec_t{});
      }
      else
      {
        _view_outer.resize_dim(d, _view_outer.offset(d), _view_outer.extent(d) + halo_offs_plus);
        _view_inner.resize_dim(d, _view_inner.offset(d), _view_inner.extent(d) - halo_offs_plus);

        region_offsets[d]  = view_offset + view_extent - halo_offs_plus;
        region_extents[d]  = halo_offs_plus;
        _boundary_regions.push_back(viewspec_t(region_offsets, region_extents));

        setBndElems(d, region_offsets, region_extents);

        region_offsets[d]  = view_offset + view_extent;
        region_extents[d]  = halo_offs_plus;
        _halo_regions.push_back(viewspec_t(region_offsets, region_extents));
      }
    }

  }

  HaloBlock() = delete;

  /**
   * Copy constructor.
   */
  HaloBlock(const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  static constexpr dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * The pattern instance that created the encapsulated block.
   */
  const PatternT & pattern() const
  {
    return _pattern;
  }

  const GlobMem_t & globmem() const
  {
    return _globmem;
  }

  const halospec_t & halospec() const
  {
    return _halospec;
  }

  /**
   * Creates view on halo region for a given dimension and halo region.
   * For example, the east halo region in a two-dimensional block
   * has (1, dash::HaloRegion::PLUS).
   */
  const block_view_t halo_region(dim_t dimension, HaloRegion region) const
  {
    auto region_index = 2 * dimension + static_cast<uint8_t>(region);

    return block_view_t(_globmem, _pattern, _halo_regions[region_index]);
  }

  /**
   * Creates view on boundary region for a given dimension and boundary region.
   * For example, the east boundary region in a two-dimensional block
   * has (1, dash::HaloRegion::PLUS).
   */
  const block_view_t boundary_region(dim_t dimension, HaloRegion region) const
  {
    auto region_index = 2 * dimension + static_cast<uint8_t>(region);

    return block_view_t(_globmem, _pattern, _boundary_regions[region_index]);
  }

  const std::vector<viewspec_t> & boundary_elements() const
  {
    return _boundary_elements;
  }

  size_type halo_size() const
  {
    return std::accumulate(_halo_regions.begin(),_halo_regions.end(), 0,
        [](size_type sum, viewspec_t region){return sum + region.size();});
  }

  size_type halo_size(dim_t dimension, HaloRegion region) const
  {
    return _halo_regions[2 * dimension + static_cast<uint8_t>(region)].size();
  }

  size_type boundary_size() const
  {
    return _size_bnd_elems;
  }

  const viewspec_t & view() const
  {
    return _view;
  }

  // TODO find useful name
  const viewspec_t & view_save() const
  {
    return _view_save;
  }

  /**
   * View specifying the inner block region.
   */
  const viewspec_t & view_inner() const
  {
    return _view_inner;
  }

  /**
   * View specifying the outer block region including halo.
   */
  const viewspec_t & view_outer() const
  {
    return _view_outer;
  }

private:
  void setBndElems(dim_t dim, std::array<index_type, NumDimensions> offsets,
      std::array<size_type, NumDimensions> extents)
  {
    if(dim == 0)
    {
      for(auto d = 0; d < NumDimensions; ++d)
      {
        auto halo_off_minus = std::abs(_halospec.halo_offset(d).minus);
        auto halo_off_plus = _halospec.halo_offset(d).plus;

        if(offsets[d] < halo_off_minus)
        {
          offsets[d] += halo_off_minus;
          extents[d] -= halo_off_minus;
        }
        if(offsets[d] + extents[d] + halo_off_plus > _pattern.extent(d))
          extents[d] -= halo_off_plus;
      }
    }
    else
    {
      for(auto d = 0; d < dim; ++d)
      {
        offsets[d] -= _halospec.halo_offset(d).minus;
        extents[d] -= _halospec.width(d);
      }
    }
    viewspec_t boundary(offsets, extents);
    _size_bnd_elems += boundary.size();
    _boundary_elements.push_back(std::move(boundary));
  }

private:
  GlobMem_t &               _globmem;

  const PatternT &          _pattern;

  const viewspec_t &        _view;

  const halospec_t &        _halospec;

  viewspec_t                _view_save;

  viewspec_t                _view_inner;

  viewspec_t                _view_outer;

  std::vector<viewspec_t>   _halo_regions;

  std::vector<viewspec_t>   _boundary_regions;

  std::vector<viewspec_t>   _boundary_elements;

  size_type                 _size_bnd_elems = 0;

}; // class HaloBlock

template<typename HaloBlockT>
class HaloMemory
{
public:
  static constexpr dim_t NumDimensions = HaloBlockT::ndim();

  using value_t    = typename HaloBlockT::value_t;

  HaloMemory(const HaloBlockT & _haloblock)
  {
    _halobuffer.resize(_haloblock.halo_size());
    _halo_offsets.resize(NumDimensions * 2, nullptr);
    value_t* offset = _halobuffer.data();
    for(auto d = 0; d < NumDimensions; ++d)
    {
      auto off_pos = d * 2;
      auto region_size = _haloblock.halo_size(d, HaloRegion::MINUS);
      if(region_size > 0)
      {
        _halo_offsets[off_pos + static_cast<uint8_t>(HaloRegion::MINUS)] = offset;
        offset += region_size;
      }

      region_size = _haloblock.halo_size(d, HaloRegion::PLUS);
      if(region_size > 0)
      {
        _halo_offsets[off_pos + static_cast<uint8_t>(HaloRegion::PLUS)] = offset;
        offset += region_size;
      }
    }
  }

  value_t * haloPos(dim_t dim, HaloRegion halo_region)
  {
    return _halo_offsets[dim * 2 + static_cast<uint8_t>(halo_region)];
  }

  value_t * startPos()
  {
    return _halobuffer.data();
  }

  const std::vector<value_t> & haloBuffer()
  {
    return _halobuffer;
  }

private:
  std::vector<value_t>    _halobuffer;
  std::vector<value_t*>   _halo_offsets;
};

} // namespace dash
} // namespace experimental

#endif // DASH__EXPERIMENTAL__HALO_H__
