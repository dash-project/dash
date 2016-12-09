#ifndef DASH__GLOB_STENCIL_ITER_H_
#define DASH__GLOB_STENCIL_ITER_H_

#include <dash/Pattern.h>
#include <dash/Allocator.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>
#include <dash/GlobMem.h>

#include <dash/iterator/GlobIter.h>

#include <iterator>
#include <array>
#include <utility>
#include <functional>
#include <sstream>
#include <cstdlib>

namespace dash {

// Forward-declaration
template<dim_t NumDimensions>
class HaloSpec;

template<
  typename GlobIterType,
  dim_t    NumDimensions = GlobIterType::ndim() >
class IteratorHalo
{
private:
  typedef int
    offset_type;
  typedef typename GlobIterType::pattern_type::size_type
    extent_t;
  typedef typename std::iterator_traits<GlobIterType>::value_type
    ElementType;

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
    offset_type arg, Args... args) const
  {
    std::array<offset_type, NumDimensions> offsets =
      { arg, static_cast<offset_type>(args)... };
    return at(offsets);
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
    return static_cast<extent_t>(_glob_iter.halospec().npoints()-1);
  }

  /**
   * Number of points in the stencil associated with this halo region.
   */
  inline int npoints() const noexcept
  {
    return _glob_iter.halospec().npoints();
  }

  /**
   * Specifier of the halo region's associated stencil.
   */
  inline const HaloSpec<NumDimensions> & halospec() const noexcept
  {
    return _glob_iter.halospec();
  }

private:
  GlobIterType _glob_iter;
};

/**
 * An iterator in global memory space providing access to halo cells of its
 * referenced element.
 *
 * \concept{DashGlobalIteratorConcept}
 */
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType   = GlobMem<ElementType>,
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
            GlobMemType,
            PointerType,
            ReferenceType>
    self_t;

  typedef typename PatternType::viewspec_type
    ViewSpecType;
  typedef typename PatternType::index_type
    IndexType;
  typedef dash::HaloSpec<NumDimensions>
    HaloSpecType;
  typedef IteratorHalo<self_t, NumDimensions>
    IteratorHaloType;

public:
  typedef       ReferenceType                      reference;
  typedef const ReferenceType                const_reference;
  typedef       PointerType                          pointer;
  typedef const PointerType                    const_pointer;

  typedef       PatternType                     pattern_type;
  typedef       IndexType                         index_type;

  typedef       IteratorHaloType                   halo_type;
  typedef const IteratorHaloType             const_halo_type;

  typedef       HaloSpecType                    stencil_spec;

  typedef typename HaloSpecType::offset_type     offset_type;

public:
  typedef std::integral_constant<bool, true>        has_view;

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
      const GlobStencilIter<T_, P_, GM_, Ptr_, Ref_> & it);

protected:
  /// Global memory used to dereference iterated values.
  GlobMemType         * _globmem         = nullptr;
  /// Pattern that specifies the iteration order (access pattern).
  const PatternType   * _pattern         = nullptr;
  /// View that specifies the iterator's index range relative to the global
  /// index range of the iterator's pattern.
  const ViewSpecType  * _viewspec        = nullptr;
  /// Current position of the iterator relative to the iterator's view.
  IndexType             _idx             = 0;
  /// The iterator's view index start offset.
  IndexType             _view_idx_offset = 0;
  /// Maximum position relative to the viewspec allowed for this iterator.
  IndexType             _max_idx         = 0;
  /// Unit id of the active unit
  dart_unit_t           _myid;
  /// Pointer to first element in local memory
  ElementType         * _lbegin          = nullptr;
  /// Specification of the iterator's stencil.
  HaloSpecType          _halospec;

public:
  /**
   * Default constructor.
   */
  GlobStencilIter()
  : _globmem(nullptr),
    _pattern(nullptr),
    _viewspec(nullptr),
    _idx(0),
    _view_idx_offset(0),
    _max_idx(0),
    _myid(dash::Team::GlobalUnitID()),
    _lbegin(nullptr)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter()", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter()", _max_idx);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern and view spec.
   */
  GlobStencilIter(
    GlobMemType        * gmem,
	  const PatternType  & pat,
    const ViewSpecType & viewspec,
    const HaloSpecType & halospec,
	  IndexType            position          = 0,
    IndexType            view_index_offset = 0)
  : _globmem(gmem),
    _pattern(&pat),
    _viewspec(&viewspec),
    _idx(position),
    _view_idx_offset(view_index_offset),
    _max_idx(viewspec.size() - 1),
    _myid(dash::Team::GlobalUnitID()),
    _lbegin(_globmem->lbegin()),
    _halospec(halospec)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,vs,hs,idx,abs)", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,vs,hs,idx,abs)", _max_idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,vs,hs,idx,abs)", *_viewspec);
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,vs,hs,idx,abs)",
                       _view_idx_offset);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern and view spec.
   */
  GlobStencilIter(
    GlobMemType        * gmem,
	  const PatternType  & pat,
    const HaloSpecType & halospec,
	  IndexType            position          = 0,
    IndexType            view_index_offset = 0)
  : _globmem(gmem),
    _pattern(&pat),
    _viewspec(nullptr),
    _idx(position),
    _view_idx_offset(view_index_offset),
    _max_idx(pat.size() - 1),
    _myid(dash::Team::GlobalUnitID()),
    _lbegin(_globmem->lbegin()),
    _halospec(halospec)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,hs,idx,abs)", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,hs,idx,abs)", _max_idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(gmem,p,hs,idx,abs)",
                       _view_idx_offset);
  }

  /**
   * Constructor, creates a global stencil iterator from a global iterator.
   */
  template<class PtrT, class RefT>
  GlobStencilIter(
    const GlobIter<ElementType, PatternType, PtrT, RefT> & other,
    const ViewSpecType                                   & viewspec,
    const HaloSpecType                                   & halospec,
    IndexType                                              view_idx_offs = 0)
  : _globmem(&other.globmem()),
    _pattern(other._pattern),
    _viewspec(&viewspec),
    _idx(other._idx),
    _view_idx_offset(view_idx_offs),
    _max_idx(other._max_idx),
    _myid(other._myid),
    _lbegin(other._lbegin),
    _halospec(halospec)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobIter)", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobIter)", _max_idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobIter)", *_viewspec);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobIter)", _view_idx_offset);
  }

  /**
   * Constructor, creates a global stencil iterator from a global view
   * iterator.
   */
  template<class PtrT, class RefT>
  GlobStencilIter(
    const GlobViewIter<ElementType,
                       PatternType,
                       GlobMemType,
                       PtrT,
                       RefT> & other,
    const HaloSpecType       & halospec)
  : _globmem(other._globmem),
    _pattern(other._pattern),
    _viewspec(other._viewspec),
    _idx(other._idx),
    _view_idx_offset(other._view_idx_offset),
    _max_idx(other._max_idx),
    _myid(other._myid),
    _lbegin(other._lbegin),
    _halospec(halospec)
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobViewIter)", _idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobViewIter)", _max_idx);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobViewIter)", *_viewspec);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobViewIter)", _halospec);
    DASH_LOG_TRACE_VAR("GlobStencilIter(GlobViewIter)", _view_idx_offset);
  }

  /**
   * Copy constructor.
   */
  GlobStencilIter(
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
  static dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * Halo region at current iterator position.
   *
   * Example:
   *
   * \code
   *    // five-point stencil has offset range (-1, +1) in both row- and
   *    // column dimension:
   *    std::array<int, 2> stencil_offs_range_rows = { -1, 1 };
   *    std::array<int, 2> stencil_offs_range_cols = { -1, 1 };
   *    dash::HaloSpec<2>  five_point_stencil(stencil_offs_range_rows,
   *                                          stencil_offs_range_cols);
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
  inline halo_type halo()
  {
    return halo_type(*this);
  }

  /**
   * Halo cell at given offsets at current iterator position.
   *
   * Example:
   *
   * \code
   *    // five-point stencil has offset range (-1, +1) in both row- and
   *    // column dimension:
   *    std::array<int, 2> stencil_offs_range_rows = { -1, 1 };
   *    std::array<int, 2> stencil_offs_range_cols = { -1, 1 };
   *    dash::HaloSpec<2>  five_point_stencil(stencil_offs_range_rows,
   *                                          stencil_offs_range_cols);
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
  ElementType halo_cell(
    /// Halo offset for each dimension.
    const std::array<offset_type, NumDimensions> & offsets) const
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell()", offsets);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx    = _idx;
    IndexType offset = 0;
    DASH_LOG_TRACE("GlobStencilIter.halo_cell",
                   "idx:", _idx, "max_idx:", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    // Global iterator position to Cartesian coordinates relative to view
    // spec:
    auto cell_g_coords = coords(_idx);
    DASH_LOG_TRACE("GlobStencilIter.halo_cell",
                   "stencil center coords:", cell_g_coords);
    // Apply stencil offsets to Cartesian coordinates:
    for (dim_t d = 0; d < NumDimensions; ++d) {
      // TODO: negative coordinates can be used to detect access to ghost cell
      cell_g_coords[d] = std::max<IndexType>(
                           cell_g_coords[d] + offsets[d],
                           0);
    }
    DASH_LOG_TRACE("GlobStencilIter.halo_cell",
                   "halo cell coords:", cell_g_coords);
    // Convert cell coordinates back to global index:
    auto cell_g_index = _pattern->memory_layout().at(cell_g_coords);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", cell_g_index);
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(cell_g_index);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell", local_pos.index);

    if (_myid == local_pos.unit) {
      // Referenced cell is local to active unit, access element using offset
      // of halo cell in local memory space:
      auto local_cell_offset = local_pos.index + offset;
      DASH_LOG_TRACE_VAR("GlobStencilIter.halo_cell >", local_cell_offset);
      // Global reference to element at given position:
      return *(_lbegin + local_cell_offset);
    } else {
      DASH_LOG_TRACE("GlobStencilIter.halo_cell",
                     "requesting cell element from remote unit",
                     local_pos.unit);
      // Referenced cell is located at remote unit, access element using
      // blocking get request.
      // Global reference to element at given position:
      return ReferenceType(
               _globmem->at(local_pos.unit,
                            local_pos.index));
    }
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
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr", _max_idx);
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
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.GlobPtr >",
                       local_pos.index + offset);
    // Create global pointer from unit and local offset:
    PointerType gptr(
      _globmem->at(local_pos.unit, local_pos.index)
    );
    gptr += offset;
    return gptr;
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
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      DASH_LOG_TRACE_VAR("GlobStencilIter.dart_gptr", _viewspec);
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE("GlobStencilIter.dart_gptr",
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
    typedef typename pattern_type::local_index_t
      local_pos_t;
    IndexType idx = _idx;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.*", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.*", local_pos.index);
    // Global reference to element at given position:
    return ReferenceType(
             _globmem->at(local_pos.unit,
                          local_pos.index));
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
    IndexType idx = g_index;
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.[]", local_pos.index);
    // Global pointer to element at given position:
    PointerType gptr(
      _globmem->at(local_pos.unit, local_pos.index)
    );
    // Global reference to element at given position:
    return ReferenceType(gptr);
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
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      DASH_LOG_TRACE_VAR("GlobStencilIter.local=", *_viewspec);
      // Viewspec projection required:
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobStencilIter.local= >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobStencilIter.local= >", local_pos.index);
    if (_myid != local_pos.unit) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + local_pos.index + offset);
  }

  /**
   * Map iterator to global index domain by projecting the iterator's view.
   *
   * \see DashGlobalIteratorConcept
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
    DASH_LOG_TRACE("GlobStencilIter.pos()",
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
    DASH_LOG_TRACE_VAR("GlobStencilIter.gpos()", _idx);
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos >", _idx);
      return _idx;
    } else {
      IndexType idx    = _idx;
      IndexType offset = 0;
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos", *_viewspec);
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos", _max_idx);
      // Convert iterator position (_idx) to local index and unit.
      if (_idx > _max_idx) {
        // Global iterator pointing past the range indexed by the pattern
        // which is the case for .end() iterators.
        idx    = _max_idx;
        offset = _idx - _max_idx;
      }
      // Viewspec projection required:
      auto g_coords = coords(idx);
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos", _idx);
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos", g_coords);
      auto g_idx    = _pattern->memory_layout().at(g_coords);
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos", g_idx);
      g_idx += offset;
      DASH_LOG_TRACE_VAR("GlobStencilIter.gpos >", g_idx);
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
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      DASH_LOG_TRACE_VAR("GlobStencilIter.lpos", *_viewspec);
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
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
  inline bool is_relative() const noexcept
  {
    return _viewspec != nullptr;
  }

  /**
   * The view that specifies this iterator's index range.
   *
   * \see DashViewIteratorConcept
   */
  inline ViewSpecType viewspec() const
  {
    if (_viewspec != nullptr) {
      return *_viewspec;
    }
    return ViewSpecType(_pattern->memory_layout().extents());
  }

  inline const HaloSpecType & halospec() const noexcept
  {
    return _halospec;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  inline const GlobMemType & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  inline GlobMemType & globmem()
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

  self_t operator-(index_type n) const
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

  inline dash::Team & team() const
  {
    return _pattern->team();
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
   * Convert a global offset within the global iterator's range to
   * corresponding global coordinates with respect to viewspec projection.
   *
   * NOTE:
   * This method could be specialized for NumDimensions = 1 for performance
   * tuning.
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType glob_index) const
  {
    DASH_LOG_TRACE_VAR("GlobStencilIter.coords()", glob_index);
    // Global cartesian coords of current iterator position:
    std::array<IndexType, NumDimensions> glob_coords;
    if (_viewspec != nullptr) {
      DASH_LOG_TRACE_VAR("GlobStencilIter.coords", *_viewspec);
      // Create cartesian index space from extents of view projection:
      CartesianIndexSpace<NumDimensions, Arrangement, IndexType> index_space(
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
    DASH_LOG_TRACE_VAR("GlobStencilIter.coords >", glob_coords);
    return glob_coords;
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
template <
  typename ElementType,
  class    Pattern,
  class    GlobMem,
  class    Pointer,
  class    Reference >
auto distance(
  /// Global iterator to the initial position in the global sequence
  const GlobStencilIter<ElementType, Pattern, GlobMem, Pointer, Reference> &
    first,
  /// Global iterator to the final position in the global sequence
  const GlobStencilIter<ElementType, Pattern, GlobMem, Pointer, Reference> &
    last
) -> typename Pattern::index_type
{
  return last - first;
}

template <
  typename ElementType,
  class    Pattern,
  class    GlobMem,
  class    Pointer,
  class    Reference >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobStencilIter<
          ElementType, Pattern, GlobMem, Pointer, Reference> & it)
{
  std::ostringstream ss;
  dash::GlobPtr<ElementType, Pattern> ptr(it);
  ss << "dash::GlobStencilIter<" << typeid(ElementType).name() << ">("
     << "idx:" << it._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__GLOB_STENCIL_ITER_H_
