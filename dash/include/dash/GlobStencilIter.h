#ifndef DASH__GLOB_STENCIL_ITER_H_
#define DASH__GLOB_STENCIL_ITER_H_

#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>

#include <array>
#include <utility>
#include <functional>
#include <sstream>
#include <cstdlib>

namespace dash {

template<dim_t NumDimensions>
class StencilSpec
{
public:
  typedef struct {
    int min;
    int max;
  } offset_range_type;

private:
  /// The stencil's offset range (min, max) in every dimension.
  std::array<offset_range_type, NumDimensions> _offset_ranges;
  /// Number of points in the stencil.
  int                                          _points;

public:
  /**
   * Creates a new instance of class StencilSpec with the given offset ranges
   * (pair of minimum offset, maximum offset) in the stencil's dimensions.
   *
   * For example, a two-dimensional five-point stencil has offset ranges
   * { (-1, 1), (-1, 1) }
   * and a stencil with only north and east halo cells has offset ranges
   * { (-1, 0), ( 0, 1) }.
   */
  StencilSpec(
    const std::array<offset_range_type, NumDimensions> & offset_ranges)
  : _offset_ranges(offset_ranges),
  {
    // minimum stencil size when containing center element only:
    _points = 1;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      _points += std::abs(_offset_ranges[d].max - _offset_ranges[d].min);
    }
  }

  /**
   * Creates a new instance of class StencilSpec that only consists of the
   * center point.
   */
  StencilSpec()
  : _points(1)
  {
    // initialize offset ranges with (0,0) in all dimensions:
    for (dim_t d = 0; d < NumDimensions; ++d) {
      _offset_ranges = {{ 0, 0 }};
    }
  }

  /**
   * The stencil's number of dimensions.
   *
   * \see DashGlobalIteratorConcept
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

  const offset_range_type & offset_range(dim_t dimension) const
  {
    return _offset_ranges[dimension];
  }

  const std::array< offset_range_type, NumDimensions> & offset_ranges() const
  {
    return _offset_ranges;
  }
};

template<typename GlobIterType>
class IteratorHalo
{
private:
  typedef int offset_type;

private:
  static const dim_t NumDimensions = GlobIterType::ndim();

public:
  /**
   *
   */
  IteratorHalo(const GlobIterType & glob_iter)
  : _glob_iter(glob_iter)
  { }

  /**
   * The number of dimensions of the halo region.
   */
  static dim_t ndim() {
    return NumDimensions;
  }

  template<typename... Args>
  inline const ElementType & operator()(
    /// Halo offset for each dimension.
    Args... args) const
  {
    return at(args);
  }

  template<typename... Args>
  const ElementType & at(
    /// Halo offset for each dimension.
    offset_type arg, Args... args) const
  {
    static_assert(sizeof...(Args) == NumDimensions-1,
                  "Invalid number of offset arguments");
    std::array<offset_type, NumDimensions> offsets =
      { arg, static_cast<offset_type>(args)... };
    return at(offsets);
  }

  const ElementType & at(
    /// Halo offset for each dimension.
    const std::array<offset_type, NumDimensions> & offs) const
  {
    return _glob_iter.halo_cell(offs);
  }

/*
 * TODO: Implement begin() and end()
 */

  /**
   * Number of elements in the halo region, i.e. the number of points in the
   * halo region's associated stencil without the center element.
   */
  inline extent_t size() const noexcept
  {
    return static_cast<extent_t>(_glob_iter.stencilspec().npoints()-1);
  }

  /**
   * Number of points in the stencil associated with this halo region.
   */
  inline int npoints() const noexcept
  {
    return _glob_iter.stencilspec().npoints();
  }

  /**
   * Specifier of the halo region's associated stencil.
   */
  inline const StencilSpec<NumDimensions> & stencilspec() const noexcept
  {
    return _glob_iter.stencilspec();
  }

private:
  GlobIterType _glob_iter;
};

/**
 * An iterator in global memory space providing access to halo cells of the
 * iterator position.
 *
 * \concept{DashGlobalIteratorConcept}
 */
template<
  typename ElementType,
  dim_t    NumStencilDim,
  class    PatternType   = Pattern<1>,
  class    PointerType   = GlobPtr<ElementType, PatternType>,
  class    ReferenceType = GlobRef<ElementType> >
class GlobStencilIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           typename PatternType::index_type,
           PointerType,
           ReferenceType >
{
private:
  static const dim_t      NumDimensions = PatternType::ndim();
  static const MemArrange Arrangement   = PatternType::memory_order();

private:
  typedef GlobStencilIter<
            ElementType,
            PatternType,
            PointerType,
            ReferenceType >
    self_t;
  typedef typename PatternType::index_type
    IndexType;
  typedef std::array<NumStencilDim, extent_t>
    StencilExtentsType;
  typedef dash::StencilSpec<NumDimensions>
    StencilSpecType;

public:
  typedef       ReferenceType                      reference;
  typedef const ReferenceType                const_reference;
  typedef       PointerType                          pointer;
  typedef const PointerType                    const_pointer;

  typedef       PatternType                     pattern_type;
  typedef       IndexType                         index_type;

  typedef       IteratorHalo<self_t>                    halo;
  typedef const IteratorHalo<self_t>              const_halo;

  typedef       StencilSpecType                 stencil_spec;

public:
  typedef std::integral_constant<bool, false>       has_view;

public:
  // For ostream output
  template <
    typename T_,
    class P_,
    class Ptr_,
    class Ref_ >
  friend std::ostream & operator<<(
      std::ostream & os,
      const GlobStencilIter<T_, P_, Ptr_, Ref_> & it);

  template<typename GlobIterType_>
  friend class IteratorHalo;

protected:
  /// Global memory used to dereference iterated values.
  GlobMem<ElementType> * _globmem;
  /// Pattern that specifies the iteration order (access pattern).
  const PatternType    * _pattern;
  /// Current position of the iterator in global canonical index space.
  IndexType              _idx             = 0;
  /// Maximum position allowed for this iterator.
  IndexType              _max_idx         = 0;
  /// Unit id of the active unit
  dart_unit_t            _myid;
  /// Pointer to first element in local memory
  ElementType          * _lbegin          = nullptr;
  /// Specification of the iterator's stencil.
  StencilSpecType        _stencilspec;

public:
  /**
   * Default constructor.
   */
  GlobStencilIter()
  : _globmem(nullptr),
    _pattern(nullptr),
    _idx(0),
    _max_idx(0),
    _myid(dash::myid()),
    _lbegin(nullptr)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter()", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter()", _max_idx);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern.
   */
  GlobStencilIter(
    GlobMem<ElementType>  * gmem,
	  const PatternType     & pat,
    const StencilSpecType & stencilspec,
	  IndexType               position = 0)
  : _globmem(gmem),
    _pattern(&pat),
    _idx(position),
    _max_idx(pat.size() - 1),
    _myid(dash::myid()),
    _lbegin(_globmem->lbegin()),
    _stencilspec(stencilspec)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,pat,stspec,dx,abs)", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,pat,stspec,dx,abs)", _max_idx);
  }

  /**
   * Copy constructor.
   */
  GlobStencilIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * The number of dimensions of the iterator's underlying pattern.
   *
   * \see DashGlobalIteratorConcept
   */
  static dim_t ndim() {
    return NumDimensions;
  }

  inline const StencilSpecType & stencilspec() const noexcept
  {
    return _stencilspec;
  }

  /**
   * Halo region at current iterator position.
   *
   * Example:
   *
   * \code
   *    // five-point stencil has offset range (-1, +1) in both row- and
   *    // column dimension:
   *    std::array<int, 2>   stencil_offs_range_rows = { -1, 1 };
   *    std::array<int, 2>   stencil_offs_range_cols = { -1, 1 };
   *    dash::StencilSpec<2> five_point_stencil(stencil_offs_range_rows,
   *                                            stencil_offs_range_cols);
   *    dash::StencilMatrix<2, double> stencil_matrix(sizespec,
   *                                                  five_point_stencil,
   *                                                  distspec,
   *                                                  teamspec);
   *    auto   st_iter = stencil_matrix.block(1,2).begin();
   *    // stencil points can either be accessed using halo view specifiers
   *    // returned by \c GlobStencilIter::halo which implement the sequential
   *    // container concept and thus provice an iteration space for the halo
   *    // cells:
   *    auto   halo_vs = st_iter.halo();
   *    double center  = halo_vs( 0, 0); // = halo_vs[1]
   *    double north   = halo_vs(-1, 0); // = halo_vs[0] = halo_vs.begin()
   *    double east    = halo_vs( 0, 1); // = halo_vs[4]
   *    // If the halo cells are not used as sequential containers, using
   *    // method \c GlobStencilIter::halo_cell for direct element access is
   *    // more efficient as it does not instantiate a view proxy object:
   *    double south   = st_iter.halo_cell( 1, 0); // = halo_vs[2]
   *    double west    = st_iter.halo_cell( 0,-1); // = halo_vs[3]
   * \endcode
   *
   */
  inline halo halo()
  {
    return halo hr(*this);
  }

  /**
   * Halo cell at given offsets at current iterator position.
   *
   * Example:
   *
   * \code
   *    // five-point stencil has offset range (-1, +1) in both row- and
   *    // column dimension:
   *    std::array<int, 2>   stencil_offs_range_rows = { -1, 1 };
   *    std::array<int, 2>   stencil_offs_range_cols = { -1, 1 };
   *    dash::StencilSpec<2> five_point_stencil(stencil_offs_range_rows,
   *                                            stencil_offs_range_cols);
   *    dash::StencilMatrix<2, double> stencil_matrix(sizespec,
   *                                                  five_point_stencil,
   *                                                  distspec,
   *                                                  teamspec);
   *    auto   st_iter = stencil_matrix.block(1,2).begin();
   *    // stencil points can either be accessed using halo view specifiers
   *    // returned by \c GlobStencilIter::halo which implement the sequential
   *    // container concept and thus provice an iteration space for the halo
   *    // cells:
   *    auto   halo_vs = st_iter.halo();
   *    double center  = halo_vs( 0, 0); // = halo_vs[1]
   *    double north   = halo_vs(-1, 0); // = halo_vs[0] = halo_vs.begin()
   *    double east    = halo_vs( 0, 1); // = halo_vs[4]
   *    // If the halo cells are not used as sequential containers, using
   *    // method \c GlobStencilIter::halo_cell for direct element access is
   *    // more efficient as it does not instantiate a view proxy object:
   *    double south   = st_iter.halo_cell( 1, 0); // = halo_vs[2]
   *    double west    = st_iter.halo_cell( 0,-1); // = halo_vs[3]
   * \endcode
   *
   */
  ReferenceType halo_cell(
    /// Halo offset for each dimension.
    const std::array<offset_type, NumDimensions> & offsets) const
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell()", offsets);
    // Global iterator position to Cartesian coordinates:
    auto cell_g_coords = pattern().coords(_idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", cell_g_coords);
    // Apply stencil offsets to Cartesian coordinates:
    for (dim_t d = 0; d < NumDimensions; ++d) {
      cell_g_coords[d] += offsets[d];
    }
    // Convert cell coordinates back to global index:
    cell_g_index = pattern().at(cell_g_coords);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", cell_g_index);
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(cell_g_index);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", local_pos.index);
    // Global pointer to element at given position:
    dart_gptr_t gptr = _globmem->index_to_gptr(
                                   local_pos.unit,
                                   local_pos.index);
    // Global reference to element at given position:
    return ReferenceType(gptr);
  }

  /**
   * Type conversion operator to \c GlobPtr.
   *
   * \see DashGlobalIteratorConcept
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator PointerType() const
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx    = _idx;
    IndexType offset = 0;
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr()", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr", idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr", offset);
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr >", local_pos.index);
    // Create global pointer from unit and local offset:
    PointerType gptr(
      _globmem->index_to_gptr(local_pos.unit, local_pos.index)
    );
    return gptr + offset;
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
    DASH_LOG_TRACE_VAR("GlobStencilIter.dart_gptr()", _idx);
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
      DASH_LOG_TRACE_VAR("GlobStencilIter.dart_gptr", _max_idx);
      DASH_LOG_TRACE_VAR("GlobStencilIter.dart_gptr", idx);
      DASH_LOG_TRACE_VAR("GlobStencilIter.dart_gptr", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE("GlobStencilIter.dart_gptr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    // Global pointer to element at given position:
    dash::GlobPtr<ElementType, PatternType> gptr(
      _globmem->index_to_gptr(
        local_pos.unit,
        local_pos.index)
    );
    DASH_LOG_TRACE_VAR("GlobStencilIter.dart_gptr >", gptr);
    return (gptr + offset).dart_gptr();
  }

  /**
   * Dereference operator.
   *
   * \see DashGlobalIteratorConcept
   *
   * \return  A global reference to the element at the iterator's position.
   */
  ReferenceType operator*() const
  {
    DASH_LOG_TRACE("GlobStencilIter.*", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx = _idx;
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.*", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.*", local_pos.index);
    // Global pointer to element at given position:
    dart_gptr_t gptr = _globmem->index_to_gptr(
                                   local_pos.unit,
                                   local_pos.index);
    // Global reference to element at given position:
    return ReferenceType(gptr);
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  ReferenceType operator[](
    /// The global position of the element
    IndexType g_index) const
  {
    DASH_LOG_TRACE("GlobStencilIter.[]", g_index);
    IndexType idx = g_index;
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.index);
    // Global pointer to element at given position:
    dart_gptr_t gptr = _globmem->index_to_gptr(
                                   local_pos.unit,
                                   local_pos.index);
    // Global reference to element at given position:
    return ReferenceType(gptr);
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   *
   * \see DashGlobalIteratorConcept
   */
  inline bool is_local() const
  {
    return (_myid == lpos().unit);
  }

  /**
   * Convert global iterator to native pointer.
   *
   * \see DashGlobalIteratorConcept
   */
  ElementType * local() const
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter.local=()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx    = _idx;
    IndexType offset = 0;
    DASH_LOG_TRACE_VAR("GlobStencilIter.local=", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.local=", idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.local=", offset);
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter.local= >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.local= >", local_pos.index);
    if (_myid != local_pos.unit) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + local_pos.index + offset);
  }

  /**
   * Map iterator to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  inline self_t global() const
  {
    return *this;
  }

  /**
   * Position of the iterator in global index space.
   *
   * \see DashGlobalIteratorConcept
   */
  inline IndexType pos() const
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   *
   * \see DashGlobalIteratorConcept
   */
  inline IndexType gpos() const
  {
    return _idx;
  }

  /**
   * Unit and local offset at the iterator's position.
   *
   * \see DashGlobalIteratorConcept
   */
  inline typename pattern_type::local_index_t lpos() const
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter.lpos()", _idx);
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
      DASH_LOG_TRACE_VAR("GlobStencilIter.lpos", _max_idx);
      DASH_LOG_TRACE_VAR("GlobStencilIter.lpos", idx);
      DASH_LOG_TRACE_VAR("GlobStencilIter.lpos", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    local_pos.index += offset;
    DASH_LOG_TRACE("GlobStencilIter.lpos >",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    return local_pos;
  }

  /**
   * Whether the iterator's position is relative to a view.
   *
   * \see DashGlobalIteratorConcept
   */
  inline constexpr bool is_relative() const noexcept
  {
    return false;
  }

  /**
   * The iterator's stencil specifier.
   */
  inline const StencilSpecType & stencilspec() const noexcept
  {
    return _stencilspec;
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
   *
   * \see DashGlobalIteratorConcept
   */
  self_t & operator++()
  {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t operator++(int)
  {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t & operator--()
  {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t operator--(int)
  {
    self_t result = *this;
    --_idx;
    return result;
  }

  self_t & operator+=(IndexType n)
  {
    _idx += n;
    return *this;
  }

  self_t & operator-=(IndexType n)
  {
    _idx -= n;
    return *this;
  }

  self_t operator+(IndexType n) const
  {
    self_t res(
      _globmem,
      *_pattern,
      _idx + static_cast<IndexType>(n));
    return res;
  }

  self_t operator-(IndexType n) const
  {
    self_t res(
      _globmem,
      *_pattern,
      _idx - static_cast<IndexType>(n));
    return res;
  }

  IndexType operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  IndexType operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }

  inline bool operator<(const self_t & other) const
  {
    return (_idx < other._idx);
  }

  inline bool operator<=(const self_t & other) const
  {
    return (_idx <= other._idx);
  }

  inline bool operator>(const self_t & other) const
  {
    return (_idx > other._idx);
  }

  inline bool operator>=(const self_t & other) const
  {
    return (_idx >= other._idx);
  }

  inline bool operator==(const self_t & other) const
  {
    return _idx == other._idx;
  }

  inline bool operator!=(const self_t & other) const
  {
    return _idx != other._idx;
  }

  inline const PatternType & pattern() const
  {
    return *_pattern;
  }

}; // class GlobStencilIter

/**
 * Resolve the number of elements between two global stencil iterators.
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
template<typename ElementType, typename PatternType>
auto distance(
  /// Global iterator to the initial position in the global sequence
  const GlobStencilIter<ElementType, PatternType> & first,
  /// Global iterator to the final position in the global sequence
  const GlobStencilIter<ElementType, PatternType> & last
) -> typename PatternType::index_type
{
  return last - first;
}

template <
  typename ElementType,
  class    Pattern,
  class    Pointer,
  class    Reference >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobStencilIter<ElementType, Pattern, Pointer, Reference> & it)
{
  std::ostringstream ss;
  dash::GlobPtr<ElementType, Pattern> ptr(it);
  ss << "dash::GlobStencilIter<" << typeid(ElementType).name() << ">("
     << "idx:"  << it._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__GLOB_STENCIL_ITER_H_
