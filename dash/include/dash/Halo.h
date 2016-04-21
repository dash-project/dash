#ifndef DASH__HALO_H__
#define DASH__HALO_H__

#include <functional>

#include <dash/Allocator.h>
#include <dash/GlobMem.h>
#include <dash/internal/Logging.h>

namespace dash {

/**
 * The concepts defined in the following extend the abstraction of
 * multidimensional block and views in DASH by halo- and stencil capabilities.
 * The \c HaloBlock type acts as a wrapper of blocks represented by any
 * implementation of the \c ViewSpec concept and extends these by boundary-
 * and halo regions.
 *
 * As known from classic stencil algorithms, *boundaries* are the outermost
 * elements within a block that are requested by neighoring units.
 * *Halos* represent additional outer regions of a block that contain ghost
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
  static dim_t ndim()
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

// Forward-declaration
template<
  typename ElementType,
  typename PatternType>
class HaloBlock;

/**
 * Iterator on block elements in internal (boundary) or external (halo)
 * border regions.
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
  typedef BlockBoundaryIter<
            ElementType,
            PatternType,
            PointerType,
            ReferenceType>
    self_t;

  typedef GlobMem<
            ElementType,
            dash::allocator::CollectiveAllocator<ElementType> >
    GlobMem_t;

private:
  static const dim_t      NumDimensions = PatternType::ndim();
  static const MemArrange Arrangement   = PatternType::memory_order();

public:
  typedef PatternType                                pattern_type;
  typedef typename PatternType::index_type             index_type;
  typedef typename PatternType::size_type               size_type;
  typedef typename PatternType::viewspec_type       viewspec_type;
  typedef dash::HaloSpec<NumDimensions>             halospec_type;
  typedef HaloBlock<ElementType, PatternType>     halo_block_type;

  typedef       ReferenceType                           reference;
  typedef const ReferenceType                     const_reference;
  typedef       PointerType                               pointer;
  typedef const PointerType                         const_pointer;

  typedef std::function<std::array<index_type, NumDimensions>(index_type)>
    position_mapping_function;

  typedef enum {
    INNER,
    OUTER
  } boundary_scope;

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
  typedef std::integral_constant<bool, true> has_view;

public:
  /**
   * Constructor, creates a block boundary iterator on multiple boundary
   * regions.
   */
  BlockBoundaryIter(
    halo_block_type                  & halo_block,
    /// A block's inner or outer view. Iteration space is the view's boundary
    /// specified by the halo.
    const viewspec_type              & viewspec,
    /// Function mapping iterator position to global coordintate space.
    const position_mapping_function  & position_mapping_fun,
    /// The iterator's position in the block boundary's iteration space
    index_type                         pos,
    /// The number of elements in the block boundary's iteration space.
    index_type                         size,
    /// The iterator's view index start offset in memory storage order.
    index_type                         view_index_offset)
  : _globmem(&halo_block.globmem()),
    _viewspec(&viewspec),
    _pattern(&halo_block.pattern()),
    _halospec(&halo_block.halospec()),
    _idx(pos),
    _view_idx_offset(view_index_offset),
    _size(size),
    _max_idx(_size-1),
    _myid(dash::myid()),
    _lbegin(_globmem->lbegin()),
    _position_to_coords(position_mapping_fun)
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", _idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", _max_idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", _size);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", *_viewspec);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", *_halospec);
  }

  /**
   * Constructor, creates a block boundary iterator using a custom function
   * for mapping iterator position to global coordinate space.
   */
  BlockBoundaryIter(
    halo_block_type                  & halo_block,
    /// A block's inner or outer view. Iteration space is the view's boundary
    /// specified by the halo.
    const viewspec_type              & viewspec,
    /// Views of the block's adjacent boundary regions.
    const std::vector<viewspec_type> * boundary_regions,
    /// The iterator's position in the block boundary's iteration space
    index_type                         pos,
    /// The number of elements in the block boundary's iteration space.
    index_type                         size,
    /// The iterator's view index start offset in memory storage order.
    index_type                         view_index_offset)
  : _globmem(&halo_block.globmem()),
    _viewspec(&viewspec),
    _pattern(&halo_block.pattern()),
    _halospec(&halo_block.halospec()),
    _boundary_regions(boundary_regions),
    _idx(pos),
    _view_idx_offset(view_index_offset),
    _size(size),
    _max_idx(_size-1),
    _myid(dash::myid()),
    _lbegin(_globmem->lbegin()),
    _position_to_coords([boundary_regions,this](index_type i_type)
      {
        return coords(boundary_regions,i_type);
      })
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", _idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", _max_idx);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", _size);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", *_viewspec);
    DASH_LOG_TRACE_VAR("BlockBoundaryIter()", *_halospec);
  }

  /**
   * Default constructor.
   */
  BlockBoundaryIter() = default;

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
   * The number of dimensions of the iterator's underlying pattern.
   *
   * \see DashGlobalIteratorConcept
   */
  inline static dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \see DashGlobalIteratorConcept
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.dart_gptr()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.dart_gptr", _max_idx);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.dart_gptr", idx);
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.dart_gptr", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      DASH_LOG_TRACE_VAR("BlockBoundaryIter.dart_gptr", _viewspec);
      auto glob_coords = _position_to_coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE("BlockBoundaryIter.dart_gptr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    // Global pointer to element at given position:
    dash::GlobPtr<ElementType, PatternType> gptr(
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
  reference operator[](
    /// The global position of the element
    index_type g_index) const
  {
    DASH_LOG_TRACE("GlobStencilIter.[]", g_index, _view_idx_offset);
    index_type idx = g_index;
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = _position_to_coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.index);
    // Global reference to element at given position:
    return reference(
             _globmem->at(local_pos.unit,
                          local_pos.index));
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  inline bool is_local() const
  {
    return (_myid == lpos().unit);
  }

  /**
   * Convert global iterator to native pointer.
   */
  ElementType * local() const
  {
    DASH_LOG_TRACE_VAR("GlobViewIter.local=()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    DASH_LOG_TRACE_VAR("GlobViewIter.local=", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("GlobViewIter.local=", idx);
    DASH_LOG_TRACE_VAR("GlobViewIter.local=", offset);

    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      DASH_LOG_TRACE_VAR("GlobViewIter.local=", *_viewspec);
      // Viewspec projection required:
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobViewIter.local= >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobViewIter.local= >", local_pos.index);
    if (_myid != local_pos.unit) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + local_pos.index + offset);
  }

  /**
   * Map iterator to global index domain by projecting the iterator's view.
   */
  inline GlobIter<ElementType, PatternType> global() const
  {
    auto g_idx = gpos();
    return dash::GlobIter<ElementType, PatternType>(
             _globmem,
             *_pattern,
             g_idx
           );
  }

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
      auto g_coords = _position_to_coords(idx);
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
      auto glob_coords = _position_to_coords(idx);
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
  inline const GlobMem_t & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  inline GlobMem_t & globmem()
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
  std::array<index_type, NumDimensions> coords(const std::vector<viewspec_type> * boundary_regions,
    index_type boundary_pos) const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.coords<OUTER>()", boundary_pos);
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
    DASH_LOG_TRACE_VAR("BlockBoundaryIter.coords >", glob_coords);
    return glob_coords;
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem_t                        * _globmem          = nullptr;
  /// View specifying the block region. Iteration space contains the view
  /// elements within the boundary defined by the halo spec.
  const viewspec_type              * _viewspec         = nullptr;
  /// Pattern that created the encapsulated block.
  const pattern_type               * _pattern          = nullptr;
  /// Halo to apply to the encapsulated block.
  const halospec_type              * _halospec         = nullptr;
  /// Views of the block's adjacent boundary regions.
  const std::vector<viewspec_type> * _boundary_regions = nullptr;
  /// Iterator's position relative to the block border's iteration space.
  index_type                         _idx              = 0;
  /// The iterator's view index start offset in memory storage order.
  index_type                         _view_idx_offset  = 0;
  /// Number of elements in the block border's iteration space.
  index_type                         _size             = 0;
  /// Maximum iterator position in the block border's iteration space.
  index_type                         _max_idx          = 0;
  /// Unit id of the active unit
  dart_unit_t                        _myid;
  /// Pointer to first element in local memory
  ElementType                      * _lbegin           = nullptr;
  /// Function implementing mapping of iterator position to global element
  /// coordinates.
  std::function<std::array<index_type, NumDimensions>(index_type)>
    _position_to_coords;

}; // class BlockBoundaryIter

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
  typename PatternType,
  /// Whether the iteration space includes corners (inner scope, default) or
  /// not (outer scope, face adjacent neighbors).
  typename BlockBoundaryIter<ElementType, PatternType>::boundary_scope Scope>
class BlockBoundaryView
{
private:
  typedef BlockBoundaryView< ElementType, PatternType, Scope > self_t;

private:
  static const dim_t NumDimensions = PatternType::ndim();

public:
  typedef       BlockBoundaryIter<ElementType, PatternType>         iterator;
  typedef const BlockBoundaryIter<ElementType, PatternType>   const_iterator;

  typedef PatternType                                           pattern_type;
  typedef typename PatternType::index_type                        index_type;
  typedef typename PatternType::size_type                          size_type;
  typedef typename PatternType::viewspec_type                  viewspec_type;
  typedef dash::HaloSpec<NumDimensions>                        halospec_type;
  typedef typename iterator::boundary_scope                   boundary_scope;
  typedef HaloBlock<ElementType, PatternType>                halo_block_type;

  typedef std::function<std::array<index_type, NumDimensions>(index_type)>
    position_mapping_function;

public:
  BlockBoundaryView(
    halo_block_type                  & halo_block,
    /// A block's inner or outer viewspec.
    const viewspec_type              & viewspec,
    /// Views of the block's adjacent boundary regions.
    const std::vector<viewspec_type> * boundary_regions,
    /// Offset of the view's first index in global memory storage space.
    index_type                         view_idx_offs = 0)
  : _size([&boundary_regions]()
      {
        index_type size = 0;
        for(auto view : *boundary_regions)
          size += view.size();

        return size;
      }()),
    _beg(halo_block, viewspec, boundary_regions, 0,     _size, view_idx_offs),
    _end(halo_block, viewspec, boundary_regions, _size, _size, view_idx_offs)
  {}

  BlockBoundaryView(
    halo_block_type                  & halo_block,
    /// A block's inner or outer viewspec.
    const viewspec_type              & viewspec,
    /// Views of the block's adjacent boundary regions.
    const viewspec_type              & boundary_region,
    /// Offset of the view's first index in global memory storage space.
    index_type                         view_idx_offs = 0)
  : _size(boundary_region.size()),
    _position_coords([&boundary_region,this](index_type i_type)
      {
        return boundary_coords(boundary_region, i_type);
      }),
    _beg(halo_block, viewspec, _position_coords,     0, _size, view_idx_offs),
    _end(halo_block, viewspec, _position_coords, _size, _size, view_idx_offs)
  {}

  BlockBoundaryView() = default;

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
  inline iterator begin() const
  {
    return _beg;
  }

  /**
   * Iterator pointing past the last element in the view.
   */
  inline const_iterator end() const
  {
    return _end;
  }

  /**
   * The number of elements in the view.
   */
  inline size_type size() const
  {
    return static_cast<size_type>(_size);
  }

private:
  /**
   * Convert the given iterator position in border iteration space to
   * coordinates in the block view.
   */
  std::array<index_type, NumDimensions> boundary_coords(
    const viewspec_type & boundary_region,
    index_type            boundary_pos) const
  {
    DASH_LOG_TRACE_VAR("BlockBoundaryView.boundary_coords()", boundary_pos);
    // resolve coordinate in halo region from iterator boundary position:
    auto region_coords = CartesianIndexSpace<NumDimensions>(
                           boundary_region.extents())
                         .coords(boundary_pos);
    // apply view offsets to resolve global cartesian coords of current
    // iterator position:
    std::array<index_type, NumDimensions> glob_coords = region_coords;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      glob_coords[d] += boundary_region.offsets()[d];
    }
    DASH_LOG_TRACE_VAR("BlockBoundaryView.boundary_coords >", glob_coords);
    return glob_coords;
  }

private:
  /// The number of elements in this view.
  index_type                _size  = 0;
  /// Function mapping iterator position to global coordinates.
  position_mapping_function _position_coords;
  /// Iterator pointing at first element in the view.
  iterator                  _beg;
  /// Iterator pointing past the last element in the view.
  iterator                  _end;
}; // class BlockBoundaryView

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
  static const dim_t NumDimensions = PatternType::ndim();

private:
  typedef HaloBlock<ElementType, PatternType>
    self_t;

  typedef GlobMem<
            ElementType,
            dash::allocator::CollectiveAllocator<ElementType> >
    GlobMem_t;

public:
  typedef PatternType                                           pattern_type;
  typedef typename PatternType::index_type                        index_type;
  typedef typename PatternType::size_type                          size_type;
  typedef typename PatternType::viewspec_type                  viewspec_type;
  typedef dash::HaloSpec<NumDimensions>                        halospec_type;
  typedef BlockBoundaryIter<ElementType, PatternType>               iterator;
  typedef typename iterator::boundary_scope                   boundary_scope;

  typedef BlockBoundaryView<ElementType, PatternType, boundary_scope::INNER>
    boundary_view_type;
  typedef BlockBoundaryView<ElementType, PatternType, boundary_scope::OUTER>
    halo_view_type;

public:
  /**
   * Creates a new instance of HaloBlock that extends a given pattern block
   * by halo semantics.
   */
  HaloBlock(
    /// Global memory used to dereference iterated values.
    GlobMem_t           * globmem,
    // Pattern that created the encapsulated block.
    const pattern_type  & pattern,
    // View specifying the inner block region.
    const viewspec_type & viewspec,
    // The halo to apply to the block.
    const halospec_type & halospec,
    /// Offset of the view's first index in global memory storage space.
    index_type            view_index_offset = 0)
  : _globmem(globmem),
    _pattern(&pattern),
    _viewspec_inner(&viewspec),
    _halospec(&halospec)
  {
    _viewspec_outer = viewspec;
    _boundary_regions.reserve(NumDimensions * 2);
    _halo_regions.reserve(NumDimensions * 2);
    _boundary_views.reserve(NumDimensions * 2);

    for (dim_t d = 0; d < NumDimensions; ++d)
    {
      auto halo_offs_neg  = std::abs(halospec.offset_range(d).min);
      auto halo_offs_pos  = std::abs(halospec.offset_range(d).max);

      /* initializes outer viewspec (i.e. including halos) from original
         inner block viewspec. */
      auto view_outer_offset_d = _viewspec_outer.offset(d) - halo_offs_neg;
      auto view_outer_extent_d = _viewspec_outer.extent(d) + halo_offs_neg + halo_offs_pos;
      _viewspec_outer.resize_dim(d, view_outer_offset_d, view_outer_extent_d);

      /* initializes boundary regions (including empty once) */
      auto bnd_region_offsets = viewspec.offsets();
      auto bnd_region_extents = viewspec.extents();
      // boundary extends to negative direction, e.g. west or north:
      bnd_region_extents[d]   = halo_offs_neg;
      DASH_LOG_TRACE("HaloBlock() init boundary regions >", "d:", d,
                     "offsets:", bnd_region_offsets,
                     "extents:", bnd_region_extents);
      _boundary_regions.push_back(viewspec_type(bnd_region_offsets, bnd_region_extents));
      // creates boundary views with unique boundary elements for all pre (e.g west, north)
      // Halo boundary elements
      if(d == 0)
        _boundary_views.push_back(viewspec_type(bnd_region_offsets, bnd_region_extents));
      else
      {
        for(dim_t tmp_d = 0; tmp_d < d; ++tmp_d )
        {
          auto off_min = std::abs(halospec.offset_range(tmp_d).min);
          bnd_region_offsets[tmp_d] += off_min;
          bnd_region_extents[tmp_d] -= std::abs(halospec.offset_range(tmp_d).max) + off_min;
        }
        _boundary_views.push_back(viewspec_type(bnd_region_offsets, bnd_region_extents));
      }
      // boundary extends to positive direction, e.g. east or south:
      bnd_region_offsets = viewspec.offsets();
      bnd_region_extents = viewspec.extents();
      bnd_region_offsets[d]  += bnd_region_extents[d] - halo_offs_pos;
      bnd_region_extents[d]   = halo_offs_pos;
      DASH_LOG_TRACE("HaloBlock() init boundary regions >", "d:", d,
                     "offsets:", bnd_region_offsets,
                     "extents:", bnd_region_extents);
      _boundary_regions.push_back(viewspec_type(bnd_region_offsets, bnd_region_extents));
      // creates boundary views with unique boundary elements for all post (e.g east, south)
      // Halo boundary elements
      if(d == 0)
        _boundary_views.push_back(viewspec_type(bnd_region_offsets, bnd_region_extents));
      else
      {
        for(dim_t tmp_d = 0; tmp_d < d; ++tmp_d )
        {
          auto off_min = std::abs(halospec.offset_range(tmp_d).min);
          bnd_region_offsets[tmp_d] += off_min;
          bnd_region_extents[tmp_d] -= std::abs(halospec.offset_range(tmp_d).max) + off_min;
        }
        _boundary_views.push_back(viewspec_type(bnd_region_offsets, bnd_region_extents));
      }

      /* initializes halo regions (including empty once) */
      auto halo_region_offsets = viewspec.offsets();
      auto halo_region_extents = viewspec.extents();
      // halo extends to negative direction, e.g. west or north:
      halo_region_offsets[d]  -= halo_offs_neg;
      halo_region_extents[d]   = halo_offs_neg;
      DASH_LOG_TRACE("HaloBlock() init halo regions >", "d:", d,
                     "offsets:", halo_region_offsets,
                     "extents:", halo_region_extents);
      _halo_regions.push_back(viewspec_type(halo_region_offsets, halo_region_extents));
      // halo extends to positive direction, e.g. east or south:
      halo_region_offsets = viewspec.offsets();
      halo_region_offsets[d]  += viewspec.extent(d);
      halo_region_extents = viewspec.extents();
      halo_region_extents[d]   = halo_offs_pos;
      DASH_LOG_TRACE("HaloBlock() init halo regions >", "d:", d,
                     "offsets:", halo_region_offsets,
                     "extents:", halo_region_extents);
      _halo_regions.push_back(viewspec_type(halo_region_offsets, halo_region_extents));
    }

    _boundary_view = boundary_view_type(*this, *_viewspec_inner, &_boundary_views, view_index_offset);
    _halo_view = halo_view_type(*this, _viewspec_outer, &_halo_regions, view_index_offset);
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
   * Global memory accessor used to dereference iterated values.
   */
  inline GlobMem_t & globmem()
  {
    return *_globmem;
  }

  /**
   * The pattern instance that created the encapsulated block.
   */
  inline const pattern_type & pattern() const
  {
    return *_pattern;
  }

  inline const halospec_type & halospec() const
  {
    return *_halospec;
  }

  /**
   * Creates view on halo region for a given dimension and halo region.
   * For example, the east halo region in a two-dimensional block
   * has (1, dash::HaloRegion::post).
   */
  //TODO Which value the first dimension has? 0 or 1?
  boundary_view_type halo_region(dim_t dimension, HaloRegion region)
  {
    auto region_index = 2 * dimension + static_cast<char>(region);

    return boundary_view_type(*this, *_viewspec_inner, _halo_regions[region_index]);
  }

  /**
   * Creates view on boundary region for a given dimension and boundary region.
   * For example, the east boundary region in a two-dimensional block
   * has (1, dash::HaloRegion::post).
   */
  //TODO Which value the first dimension has? 0 or 1?
  boundary_view_type boundary_region(dim_t dimension, HaloRegion region)
  {
    auto region_index = 2 * dimension + static_cast<char>(region);

    return boundary_view_type(*this, *_viewspec_inner, _boundary_regions[region_index]);
  }

  /**
   * View specifying the inner block region.
   */
  inline const viewspec_type & inner() const {
    return *_viewspec_inner;
  }

  /**
   * View specifying the outer block region including halo.
   */
  inline const viewspec_type & outer() const {
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
  inline const halo_view_type & halo() const {
    return _halo_view;
  }

private:
  /// Global memory accessor used to dereference iterated values.
  GlobMem_t               * _globmem        = nullptr;

  /// The pattern that created the encapsulated block.
  const pattern_type      * _pattern        = nullptr;

  /// View specifying the original internal block region and its iteration
  /// space.
  const viewspec_type     * _viewspec_inner = nullptr;

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
  viewspec_type              _viewspec_outer;

  /// The halo to apply to the encapsulated block.
  const halospec_type      * _halospec       = nullptr;

  /// Viewspecs for all contiguous boundaries in the halo block.
  std::vector<viewspec_type> _boundary_regions;

  /// Viewspecs for all contiguous halo regions in the halo block.
  std::vector<viewspec_type> _halo_regions;

  /// Viewspecs for all contiguous boudaries in the halo block, excluding elements viewed in lower
  /// dimensions.
  std::vector<viewspec_type> _boundary_views;
  /// View proxy accessor providing iteration space of elements in inner
  /// block boundaries.
  boundary_view_type         _boundary_view;

  /// View proxy accessor providing iteration space of elements in outer
  /// (halo) block boundaries.
  halo_view_type             _halo_view;

}; // class HaloBlock

} // namespace dash

#endif // DASH__HALO_H__
