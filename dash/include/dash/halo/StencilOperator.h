#ifndef DASH__HALO_HALOSTENCILOPERATOR_H
#define DASH__HALO_HALOSTENCILOPERATOR_H

#include <dash/halo/iterator/StencilIterator.h>

namespace dash {

namespace halo {

// Forward declaration
template <typename ElementT, typename PatternT, typename StencilSpecT>
class StencilOperator;

/**
 * Proxy StencilOperator for inner elements only
 */
template <typename ElementT, typename PatternT, typename StencilSpecT>
class StencilOperatorInner {
private:
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto NumDimensions    = PatternT::ndim();

  using StencilOperator_t = StencilOperator<ElementT, PatternT, StencilSpecT>;

public:
  using ViewSpec_t      = typename StencilOperator_t::ViewSpec_t;
  using ElementCoords_t = typename StencilOperator_t::ElementCoords_t;
  using iterator        = typename StencilOperator_t::iterator_inner;
  using const_iterator  = const iterator;

public:
  StencilOperatorInner(const StencilOperator_t* stencil_op)
  : _stencil_op(stencil_op) {}

  /**
   * Returns the begin iterator for all inner elements
   */
  iterator begin() noexcept { return _stencil_op->_ibegin; }

  /**
   * Returns the begin const iterator for all inner elements
   */
  const_iterator begin() const noexcept { return _stencil_op->_ibegin; }

  /**
   * Returns the end iterator for all inner elements
   */
  iterator end() noexcept { return _stencil_op->_iend; }

  /**
   * Returns the end const iterator for all inner elements
   */
  const_iterator end() const noexcept { return _stencil_op->_iend; }

  /**
   * Returns a view for all inner elements
   */
  const ViewSpec_t& view() const { return _stencil_op->_spec_views.inner(); }

  /**
   * Modifies all stencil point elements and the center within the inner view.
   * The stencil points are multiplied with their coefficent (\ref StencilPoint)
   * and the center is  multiplied with the given center coefficient. The
   * results then modifies the center/stencil point elements via the given
   * operation.
   *
   * \param coords center coordinate
   * \param value base value for all points
   * \param coefficient for center
   * \param op operation to use (e.g. std::plus). default: replace
   */
  void set_values_at(
    const ElementCoords_t& coords, ElementT value, ElementT coefficient_center,
    std::function<ElementT(const ElementT&, const ElementT&)> op =
      [](const ElementT& lhs, const ElementT& rhs) { return rhs; }) {
    auto* center = _stencil_op->_local_memory + _stencil_op->get_offset(coords);

    *center = op(*center, coefficient_center * value);
    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto& stencil_point_value = center[_stencil_op->_stencil_offsets[i]];
      stencil_point_value =
        op(stencil_point_value,
           _stencil_op->_stencil_spec[i].coefficient() * value);
    }
  }

private:
  const StencilOperator_t* _stencil_op;
};

/**
 * Proxy StencilOperator for boundary elements only
 */
template <typename ElementT, typename PatternT, typename StencilSpecT>
class StencilOperatorBoundary {
private:
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto NumDimensions    = PatternT::ndim();

  using StencilOperator_t  = StencilOperator<ElementT, PatternT, StencilSpecT>;
  using pattern_size_t     = typename StencilOperator_t::pattern_size_t;
  using StencilSpecViews_t = typename StencilOperator_t::StencilSpecViews_t;

public:
  using ViewSpec_t      = typename StencilOperator_t::ViewSpec_t;
  using ElementCoords_t = typename StencilOperator_t::ElementCoords_t;
  using iterator        = typename StencilOperator_t::iterator_bnd;
  using const_iterator  = const iterator;
  using BoundaryViews_t = typename StencilSpecViews_t::BoundaryViews_t;

public:
  StencilOperatorBoundary(const StencilOperator_t* stencil_op)
  : _stencil_op(stencil_op) {}

  /**
   * Returns the begin iterator for all boundary elements
   */
  iterator begin() noexcept { return _stencil_op->_bbegin; }

  /**
   * Returns the begin const iterator for all boundary elements
   */
  const_iterator begin() const noexcept { return _stencil_op->_bbegin; }

  /**
   * Returns the end iterator for all boundary elements
   */
  iterator end() noexcept { return _stencil_op->_bend; }

  /**
   * Returns the end const iterator for all boundary elements
   */
  const_iterator end() const noexcept { return _stencil_op->_bend; }

  /**
   * Returns all boundary views including all boundary elements (no dublicates)
   * Unlike the view methods in \ref StencilOperator and
   * \ref StencilOperatorInner this method returns a container with
   * views.
   */
  const BoundaryViews_t& view() const {
    return _stencil_op->spec_views.boundary_views();
  }

  /**
   * Returns the number of all boundary elements (no dublicates)
   */
  pattern_size_t boundary_size() const {
    return _stencil_op->spec_views.boundary_size();
  }

  /**
   * Modifies all stencil point elements and the center wth halo check.
   * If a stencil point points to a halo element or a non existing element
   * no operation is performed for this one.
   * The stencil points are multiplied with their coefficent (\ref StencilPoint)
   * and the center is  multiplied with the given center coefficient. The
   * results then modifies the center/stencil point elements via the given
   * operation.
   *
   * \param coords center coordinate
   * \param value base value for all points
   * \param coefficient for center
   * \param op operation to use (e.g. std::plus). default: replace
   */
  void set_values_at(
    const ElementCoords_t& coords, ElementT value, ElementT coefficient_center,
    std::function<ElementT(const ElementT&, const ElementT&)> op =
      [](const ElementT& lhs, const ElementT& rhs) { return rhs; }) {
    auto* center = _stencil_op->_local_memory + _stencil_op->get_offset(coords);

    *center = op(*center, coefficient_center * value);
    for(auto i = 0; i < NumStencilPoints; ++i) {
      bool halo = false;
      for(auto d = 0; d < NumDimensions; ++d) {
        auto coord_value = coords[d] + _stencil_op->_stencil_spec[i][d];
        if(coord_value < 0
           || coord_value >= _stencil_op->_view_local->extent(d)) {
          halo = true;
          break;
        }
      }

      if(halo)
        continue;

      auto& stencil_point_value = center[_stencil_op->_stencil_offsets[i]];
      stencil_point_value =
        op(stencil_point_value,
           _stencil_op->_stencil_spec[i].coefficient() * value);
    }
  }

  /**
   * Returns a pair of boundary iterators (first: begin, second: end) for a
   * given dimension and \ref RegionPos. Iterators of higher dimensions will
   * not iterate over elements which were already covered in lower dimensions.
   * Using all iterators for all dimensions and \ref RegionPos has the same
   * effect as using bbegin and bend.
   */
  std::pair<iterator, iterator> iterator_at(dim_t dim, RegionPos pos) {
    DASH_ASSERT_LT(dim, NumDimensions, "Given dimension to great");
    const auto&    bnd_views = _stencil_op->_spec_views.boundary_views();
    pattern_size_t offset    = 0;
    auto           it_views  = std::begin(bnd_views);
    for(dim_t d = 0; d < dim; ++d, ++it_views)
      offset += it_views->size() + (++it_views)->size();

    if(pos == RegionPos::POST) {
      offset += it_views->size();
      ++it_views;
    }

    auto it_begin = _stencil_op->_bbegin + offset;

    return std::make_pair(it_begin, it_begin + it_views->size());
  }

private:
  const StencilOperator_t* _stencil_op;
};

/**
 * The StencilOperator provides stencil specific iterator and functions for
 * a given \ref HaloBlock and HaloMemory.
 *
 * Provided \ref StencilIterator are for the inner block, the boundary
 * elements and both. The inner block iterator ensures that no stencil point
 * accesses halo elements or not existing elements. The stencil points of the
 * boundary iterator point at least to one halo element.
 *
 * Example for an inner block boundary iteration space:
 *
 *                          boundary region 1
 *                                 :
 *                   .-------------'------------.
 *                   |                          |
 *           .-------.-------------------------.-------.
 *           |  0  1 |  2  3  4  5  6  7  8  9 | 10 11 |
 *           | 12 13 | 14 15 16 17 18 19 20 21 | 22 23 |
 *        .--:-------+-------------------------+-------:--.
 *        |  | 24 23 |                         | 34 35 |  |
 *      .-:  :  ...  :   inner block region    :  ...  :  :- boundary
 *      | |  | 60 62 |                         | 70 71 |  |  region 3
 *      | '--:-------+-------------------------+-------:--:
 *      |    | 72 73 | 74 75 76 77 78 79 80 81 | 82 83 |  :- boundary
 *      |    | 84 85 | 86 87 88 89 90 91 92 93 | 94 95 |  |  region 8
 *      |    `-------'-------------------------'-------'--'
 *      |            |                         |
 *      |            `------------.------------+
 *      :                         :
 *      boundary region 3   boundary region 8
 *
 */
template <typename ElementT, typename PatternT, typename StencilSpecT>
class StencilOperator {
private:
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto NumDimensions    = PatternT::ndim();
  static constexpr auto MemoryArrange    = PatternT::memory_order();

  using pattern_size_t        = typename PatternT::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using pattern_index_t       = typename PatternT::index_type;

  template <typename _T, typename _P, typename _S>
  friend class StencilOperatorInner;

  template <typename _T, typename _P, typename _S>
  friend class StencilOperatorBoundary;

public:
  using Inner_t = StencilOperatorInner<ElementT, PatternT, StencilSpecT>;
  using Bnd_t   = StencilOperatorBoundary<ElementT, PatternT, StencilSpecT>;

  using iterator =
    StencilIterator<ElementT, PatternT, StencilSpecT, StencilViewScope::ALL>;
  using const_iterator = const iterator;
  using iterator_inner =
    StencilIterator<ElementT, PatternT, StencilSpecT, StencilViewScope::INNER>;
  using const_iterator_inner = const iterator;
  using iterator_bnd         = StencilIterator<ElementT, PatternT, StencilSpecT,
                                       StencilViewScope::BOUNDARY>;
  using const_iterator_bnd   = const iterator;

  using StencilOffsets_t = typename iterator::StencilOffsets_t;
  using HaloBlock_t      = HaloBlock<ElementT, PatternT>;
  using HaloMemory_t     = HaloMemory<HaloBlock_t>;
  using ViewSpec_t       = ViewSpec<NumDimensions, pattern_index_t>;
  using ElementCoords_t  = std::array<pattern_index_t, NumDimensions>;

  using StencilSpecViews_t = StencilSpecificViews<HaloBlock_t, StencilSpecT>;

  using region_index_t = typename RegionSpec<NumDimensions>::region_index_t;

public:
  /**
   * Constructor that takes a \ref HaloBlock, a \ref HaloMemory,
   * a \ref StencilSpec and a local \ref ViewSpec
   */
  StencilOperator(const HaloBlock_t* haloblock, HaloMemory_t* halomemory,
                  const StencilSpecT& stencil_spec,
                  const ViewSpec_t*   view_local)
  : inner(this), boundary(this), _halo_block(haloblock),
    _halo_memory(halomemory), _stencil_spec(stencil_spec),
    _view_local(view_local), _stencil_offsets(set_stencil_offsets()),
    _local_memory(const_cast<ElementT*>(_halo_block->globmem().lbegin())),
    _spec_views(*_halo_block, _stencil_spec, _view_local),
    _begin(_local_memory, _halo_memory, &_stencil_spec, &_stencil_offsets,
           *_view_local, _spec_views.inner_with_boundaries(), 0),
    _end(_local_memory, _halo_memory, &_stencil_spec, &_stencil_offsets,
         *_view_local, _spec_views.inner_with_boundaries(),
         _spec_views.inner_with_boundaries().size()),
    _ibegin(_local_memory, _halo_memory, &_stencil_spec, &_stencil_offsets,
            *_view_local, _spec_views.inner(), 0),
    _iend(_local_memory, _halo_memory, &_stencil_spec, &_stencil_offsets,
          *_view_local, _spec_views.inner(), _spec_views.inner().size()),
    _bbegin(_local_memory, _halo_memory, &_stencil_spec, &_stencil_offsets,
            *_view_local, _spec_views.boundary_views(), 0),
    _bend(_local_memory, _halo_memory, &_stencil_spec, &_stencil_offsets,
          *_view_local, _spec_views.boundary_views(),
          _spec_views.boundary_size()) {}

  /**
   * Returns the begin iterator for all relevant elements (inner + boundary)
   */
  iterator begin() noexcept { return _begin; }

  /**
   * Returns the begin const iterator for all relevant elements
   * (inner + boundary)
   */
  const_iterator begin() const noexcept { return _begin; }

  /**
   * Returns the end iterator for all relevant elements (inner + boundary)
   */
  iterator end() noexcept { return _end; }

  /**
   * Returns the end const iterator for all relevant elements (inner +
   * boundary)
   */
  const_iterator end() const noexcept { return _end; }

  /**
   * Returns the \ref HaloBlock
   */
  const HaloBlock_t& halo_block() { return *_halo_block; }

  /**
   * Returns the stencil specification \ref StencilSpec
   */
  const StencilSpecT& stencil_spec() const { return _stencil_spec; }

  /**
   * Returns the halo memory management object \ref HaloMemory
   */
  HaloMemory_t& halo_memory() { return *_halo_memory; }

  /**
   * Returns the \ref StencilSpecificView
   */
  const StencilSpecViews_t& spec_views() const { return _spec_views; }

  /**
   * Returns a \ref SpecView including inner and boundary elements
   */
  const ViewSpec_t& view() const { return _spec_views.inner_with_boundaries(); }

  /*
  ElementT get_value_at_inner_local(
    const ElementCoords_t& coords, ElementT coefficient_center,
    std::function<ElementT(const ElementT&, const ElementT&)> op =
    std::plus<ElementT>()) {
    auto* center = _local_memory + get_offset(coords);
    ElementT value = op(0, *center * coefficient_center);

    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto& stencil_point_value = center[_stencil_offsets[i]];
      value = op(value, _stencil_spec[i].coefficient() * stencil_point_value);
    }

    return value;
  }

  ElementT get_value_at_boundary_local(
    const ElementCoords_t& coords, ElementT coefficient_center,
    std::function<ElementT(const ElementT&, const ElementT&)> op =
    std::plus<ElementT>()) {
    auto* center = _local_memory + get_offset(coords);
    ElementT value = op(0, *center * coefficient_center);

    for(auto i = 0; i < NumStencilPoints; ++i) {
      bool halo = false;
      auto coords_stencil = coords;
      for(auto d = 0; d < NumDimensions; ++d) {
        coords_stencil[d] += _stencil_spec[i][d];
        if(coords_stencil[d] < 0 || coords_stencil[d] >= _view_local->extent(d))
          halo = true;
      }

      if(halo) {
        auto index = _halo_block->index_at(*_view_local, coords_stencil);
        auto* halomem_pos = _halo_memory->first_element_at(index);

        _halo_memory->to_halo_mem_coords(index, coords_stencil);

        value = op(value, *( halomem_pos + _halo_memory->offset(index,
  coords_stencil))); } else { auto& stencil_point_value =
  center[_stencil_offsets[i]]; value = op(value, _stencil_spec[i].coefficient()
  * stencil_point_value);
      }
    }

    return value;
  }
  */

private:
  StencilOffsets_t set_stencil_offsets() {
    StencilOffsets_t stencil_offs;
    for(auto i = 0; i < NumStencilPoints; ++i) {
      signed_pattern_size_t offset = 0;
      if(MemoryArrange == ROW_MAJOR) {
        offset = _stencil_spec[i][0];
        for(auto d = 1; d < NumDimensions; ++d)
          offset = _stencil_spec[i][d] + offset * _view_local->extent(d);
      } else {
        offset = _stencil_spec[i][NumDimensions - 1];
        for(auto d = NumDimensions - 1; d > 0;) {
          --d;
          offset = _stencil_spec[i][d] + offset * _view_local->extent(d);
        }
      }
      stencil_offs[i] = offset;
    }

    return stencil_offs;
  }

  pattern_index_t get_offset(const ElementCoords_t& coords) {
    pattern_index_t offset = 0;

    if(MemoryArrange == ROW_MAJOR) {
      offset = coords[0];
      for(auto d = 1; d < NumDimensions; ++d)
        offset = offset * _view_local->extent(d) + coords[d];
    } else {
      offset = coords[NumDimensions - 1];
      for(auto d = NumDimensions - 1; d > 0;) {
        --d;
        offset = offset * _view_local->extent(d) + coords[d];
      }
    }

    return offset;
  }

public:
  Inner_t inner;
  Bnd_t   boundary;

private:
  const HaloBlock_t* _halo_block;
  HaloMemory_t*      _halo_memory;
  const StencilSpecT _stencil_spec;
  const ViewSpec_t*  _view_local;
  StencilOffsets_t   _stencil_offsets;
  ElementT*          _local_memory;
  StencilSpecViews_t _spec_views;

  iterator       _begin;
  iterator       _end;
  iterator_inner _ibegin;
  iterator_inner _iend;
  iterator_bnd   _bbegin;
  iterator_bnd   _bend;
};

}  // namespace halo

}  // namespace dash
#endif  // DASH__HALO_HALOSTENCILOPERATOR_H
