#ifndef DASH__HALO_HALOSTENCILOPERATOR_H
#define DASH__HALO_HALOSTENCILOPERATOR_H

#include <dash/halo/iterator/StencilIterator.h>

namespace dash {

namespace halo {

namespace internal {

// binary operation to replace values
template <typename T>
struct replace {
  constexpr T operator()(const T& lhs, const T& rhs) const { return rhs; }
};

}  // namespace internal

using namespace internal;

// Forward declaration
template <typename HaloBlockT, typename StencilSpecT>
class StencilOperator;

/**
 * Proxy StencilOperator for inner elements only
 */
template <typename StencilOperatorT>
class StencilOperatorInner {
private:

  static constexpr auto NumStencilPoints = StencilOperatorT::num_stencil_points();
  static constexpr auto NumDimensions    = StencilOperatorT::ndim();

  using CoordsIdxManagerInner_t = typename StencilOperatorT::CoordsIdxManagerInner_t;

public:
  using Element_t = typename StencilOperatorT::Element_t;
  using ViewSpec_t      = typename StencilOperatorT::ViewSpec_t;
  using Coords_t = typename StencilOperatorT::Coords_t;
  using iterator        = typename StencilOperatorT::iterator_inner;
  using const_iterator  = const iterator;

  using StencilOffsets_t = typename StencilOperatorT::StencilOffsets_t;

public:
  StencilOperatorInner(StencilOperatorT* stencil_op)
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

  std::pair<iterator,iterator> sub_iterator(const ViewSpec_t* sub_view) {
    auto& inner_view = _stencil_op->_spec_views.inner();
    auto& inner_offsets = inner_view.offsets();
    auto& inner_extents = inner_view.extents();
    auto& sub_offsets = sub_view->offsets();
    auto& sub_extents = sub_view->extents();
    for(dim_t d = 0; d < NumDimensions; ++d) {
      auto inner_last_elem = inner_offsets[d] + inner_extents[d];
      auto sub_last_elem = sub_offsets[d] + sub_extents[d];
      if(sub_offsets[d] < inner_offsets[d] || sub_last_elem > inner_last_elem) {
        DASH_LOG_ERROR("Sub view doesn't fit into inner view.");

        return std::make_pair(end(), end());
      }
    }

    return std::make_pair(iterator(CoordsIdxManagerInner_t(*_stencil_op, 0, sub_view)),
                          iterator(CoordsIdxManagerInner_t(*_stencil_op, sub_view->size(), sub_view)));
  }

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
  template <typename BinaryFunc = internal::replace<Element_t>>
  void set_values_at(const Coords_t& coords, Element_t value,
                     Element_t   coefficient_center,
                     BinaryFunc op = BinaryFunc()) {
    auto* center = _stencil_op->_local_memory + _stencil_op->get_offset(coords);

    *center = op(*center, coefficient_center * value);
    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto& stencil_point_value = center[_stencil_op->_stencil_offsets[i]];
      stencil_point_value =
        op(stencil_point_value,
           _stencil_op->_stencil_spec[i].coefficient() * value);
    }
  }

  /**
   * Returns the result of the given operation done on all stencil point
   * elements and the center.
   * The stencil points are multiplied with their coefficent (\ref StencilPoint)
   * and the center is  multiplied with the given center coefficient.
   * No test for halo elements. -> useful only for elements with stencil
   * points pointing to inner elements. (no halo elements)
   *
   * \param coords center coordinate
   * \param coefficient for center
   * \param op operation to use (e.g. std::plus). default: std::plus
   */
  template <typename BinaryFunc = std::plus<Element_t>>
  Element_t get_value_at(const Coords_t& coords,
                        Element_t               coefficient_center,
                        BinaryFunc             op = BinaryFunc()) const {
    auto* center = _stencil_op->_local_memory + _stencil_op->get_offset(coords);
    Element_t value = *center * coefficient_center;

    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto& stencil_point_value = center[_stencil_op->_stencil_offsets[i]];
      value = op(value, _stencil_op->_stencil_spec[i].coefficient()
                          * stencil_point_value);
    }

    return value;
  }

  /**
   * Updates all inner elements using a user-defined stencil operation.
   *
   * \param begin_dst Pointer to the beginning of the destination memory
   * \param operation User-definied operation for updating all inner elements
  */
  template <typename Op>
  void update(Element_t* begin_dst, Op operation) {
    update(begin(), end(), begin_dst, operation);
  }

  /**
   * Updates all inner elements within a user defined range using a user-defined
   * stencil operation.
   *
   * \param begin Iterator of the beginnning inner data element
   * \param end Iterator of the last inner data element
   * \param begin_dst Pointer to the beginning of the destination memory
   * \param operation User-definied operation for updating all inner elements
  */
  template <typename Op>
  void update(iterator begin, iterator end, Element_t* begin_dst, Op operation) {
    if(begin < this->begin() || end < this->begin() ||
       begin > this->end() || end > this->end()) {

      DASH_LOG_ERROR("Begin or End iterator located outside of inner view.");

      return;
    }

    end -= 1;

    const auto& view = this->view();
    const auto& offsets_view = view.offsets();
    const auto& extents_view = view.extents();
    auto end_coords_view = offsets_view;
    for(auto d = 0; d < NumDimensions; ++d) {
      end_coords_view[d] += extents_view[d];
    }

    auto begin_coords = begin.coords();
    auto end_coords   = end.coords();

    auto center  = _stencil_op->_local_memory;
    auto offsets = _stencil_op->set_dimension_offsets();
    auto offset  = 0;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      offset += offsets[d] * begin_coords[d];
    }
    center += offset;
    auto center_dst = begin_dst + offset;

    // specialization for 2-D
    if(NumDimensions == 2) {
      if(begin_coords[0] == end_coords[0]) {
        auto center_i     = center;
        auto center_dst_i = center_dst;
        auto offset_i     = offset;
        for(int j = begin_coords[1]; j <= end_coords[1];
            ++j, ++center_i, ++center_dst_i, ++offset_i) {
          operation(center_i, center_dst_i, offset_i,
                    _stencil_op->_stencil_offsets);
        }

        return;
      }

      auto center_i     = center;
      auto center_dst_i = center_dst;
      auto offset_i     = offset;
      auto align_offset = begin_coords[1] - offsets_view[1];
      for(int j = begin_coords[1]; j < end_coords_view[1];
          ++j, ++center_i, ++center_dst_i, ++offset_i) {
        operation(center_i, center_dst_i, offset_i,
                  _stencil_op->_stencil_offsets);
      }

      center += offsets[0] - align_offset;
      center_dst += offsets[0] - align_offset;
      offset += offsets[0] - align_offset;
      for(int i = begin_coords[0] + 1; i < end_coords[0]; ++i,
              center += offsets[0], center_dst += offsets[0],
              offset += offsets[0]) {
        center_i     = center;
        center_dst_i = center_dst;
        offset_i     = offset;
        for(int j = offsets_view[1]; j < end_coords_view[1];
            ++j, ++center_i, ++center_dst_i, ++offset_i) {
          operation(center_i, center_dst_i, offset_i,
                    _stencil_op->_stencil_offsets);
        }
      }

      center_i     = center;
      center_dst_i = center_dst;
      offset_i     = offset;
      for(int j = offsets_view[1]; j <= end_coords[1];
          ++j, ++center_i, ++center_dst_i, ++offset_i) {
        operation(center_i, center_dst_i, offset_i,
                  _stencil_op->_stencil_offsets);
      }
      return;
    }

    // specialization for 3-D
    if(NumDimensions == 3) {
      for(int i = begin_coords[0]; i <= end_coords[0]; ++i,
              center += offsets[0], center_dst += offsets[0],
              offset += offsets[0]) {
        auto center_i     = center;
        auto center_dst_i = center_dst;
        auto offset_i     = offset;
        for(int j = begin_coords[1]; j <= end_coords[1]; ++j,
                center_i += offsets[1], center_dst_i += offsets[1],
                offset_i += offsets[1]) {
          auto center_j     = center_i;
          auto center_dst_j = center_dst_i;
          auto offset_j     = offset_i;
          for(int k = begin_coords[2]; k <= end_coords[2];
              ++k, ++center_j, ++center_dst_j, ++offset_j) {
            operation(center_j, center_dst_j, offset_j,
                      _stencil_op->_stencil_offsets);
          }
        }
      }

      return;
    }

    // dimensions above 3-D
    for(int i = begin_coords[0]; i <= end_coords[0]; ++i, center += offsets[0],
            center_dst += offsets[0], offset += offsets[0]) {
      Loop<1, Op>()(_stencil_op->_stencil_offsets, offsets, begin_coords,
                    end_coords, center, center_dst, offset, operation);
    }
  }

  /**
   * Updates all inner elements within a user defined range using a user-defined
   * stencil operation.
   *
   * \param begin Iterator of the beginnning inner data element
   * \param end Iterator of the last inner data element
   * \param begin_dst Pointer to the beginning of the destination memory
   * \param operation User-definied operation for updating all inner elements
  */
  template <typename Op>
  void update_blocked(const Coords_t& begin_coords, const Coords_t& end_coords, Element_t* begin_dst, Op operation) {
    const auto& view = this->view();
    const auto& offsets_view = view.offsets();
    const auto& extents_view = view.extents();

    for(auto d = 0; d < NumDimensions; ++d) {
      auto end_coord = offsets_view[d] + extents_view[d];
      if(begin_coords[d] < offsets_view[d] ||
         end_coords[d] < offsets_view[d] ||
         begin_coords[d] >= end_coord ||
         end_coords[d] >= end_coord) {

        DASH_LOG_ERROR("Begin or End coordinates located outside of inner view.");

        return;
      }
    }

    auto center  = _stencil_op->_local_memory;
    auto offsets = _stencil_op->set_dimension_offsets();
    auto offset  = 0;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      offset += offsets[d] * begin_coords[d];
    }
    center += offset;
    auto center_dst = begin_dst + offset;

    // specialization for 2-D
    if(NumDimensions == 2) {
      for(int i = begin_coords[0]; i <= end_coords[0]; ++i,
              center += offsets[0], center_dst += offsets[0],
              offset += offsets[0]) {
        auto center_i     = center;
        auto center_dst_i = center_dst;
        auto offset_i     = offset;
        for(int j = begin_coords[1]; j <= end_coords[1];
            ++j, ++center_i, ++center_dst_i, ++offset_i) {
          operation(center_i, center_dst_i, offset_i,
                    _stencil_op->_stencil_offsets);
        }
      }
      return;
    }

    // specialization for 3-D
    if(NumDimensions == 3) {
      for(int i = begin_coords[0]; i <= end_coords[0]; ++i,
              center += offsets[0], center_dst += offsets[0],
              offset += offsets[0]) {
        auto center_i     = center;
        auto center_dst_i = center_dst;
        auto offset_i     = offset;
        for(int j = begin_coords[1]; j <= end_coords[1]; ++j,
                center_i += offsets[1], center_dst_i += offsets[1],
                offset_i += offsets[1]) {
          auto center_j     = center_i;
          auto center_dst_j = center_dst_i;
          auto offset_j     = offset_i;
          for(int k = begin_coords[2]; k <= end_coords[2];
              ++k, ++center_j, ++center_dst_j, ++offset_j) {
            operation(center_j, center_dst_j, offset_j,
                      _stencil_op->_stencil_offsets);
          }
        }
      }

      return;
    }

    // dimensions above 3-D
    for(int i = begin_coords[0]; i <= end_coords[0]; ++i, center += offsets[0],
            center_dst += offsets[0], offset += offsets[0]) {
      Loop<1, Op>()(_stencil_op->_stencil_offsets, offsets, begin_coords,
                    end_coords, center, center_dst, offset, operation);
    }
  }

private:
  template <dim_t dim, typename Op>
  struct Loop {
    template <typename OffsetT>
    constexpr void operator()(const StencilOffsets_t& stencil_offs,
                              const StencilOffsets_t& dim_offs,
                              const Coords_t&  begin,
                              const Coords_t& end, Element_t* center,
                              Element_t* center_dst, OffsetT offset, Op op) {
      for(int i = begin[dim]; i <= end[dim]; ++i, center += dim_offs[dim],
              center_dst += dim_offs[dim], offset += dim_offs[dim]) {
        Loop<dim + 1, Op>()(stencil_offs, dim_offs, begin, end, center,
                            center_dst, offset, op);
      }
    }
  };

  template <typename Op>
  struct Loop<NumDimensions - 1, Op> {
    template <typename OffsetT>
    constexpr void operator()(const StencilOffsets_t& stencil_offs,
                              const StencilOffsets_t& dim_offs,
                              const Coords_t&  begin,
                              const Coords_t& end, Element_t* center,
                              Element_t* center_dst, OffsetT offset, Op op) {
      for(int i = begin[NumDimensions - 1]; i <= end[NumDimensions - 1];
          ++i, ++center, ++center_dst, ++offset) {
        op(center, center_dst, offset, stencil_offs);
      }
    }
  };

private:
  StencilOperatorT* _stencil_op;
};

/**
 * Proxy StencilOperator for boundary elements only
 */
template <typename StencilOperatorT>
class StencilOperatorBoundary {
private:

  static constexpr auto NumStencilPoints = StencilOperatorT::num_stencil_points();
  static constexpr auto NumDimensions    = StencilOperatorT::ndim();

  using StencilSpecViews_t = typename StencilOperatorT::StencilSpecViews_t;

public:
  using Element_t = typename StencilOperatorT::Element_t;
  using uindex_t  = typename StencilOperatorT::uindex_t;
  using ViewSpec_t      = typename StencilOperatorT::ViewSpec_t;
  using Coords_t        = typename StencilOperatorT::Coords_t;
  using iterator        = typename StencilOperatorT::iterator_bnd;
  using const_iterator  = const iterator;
  using BoundaryViews_t = typename StencilSpecViews_t::BoundaryViews_t;
  using RegionCoords_t  = RegionCoords<NumDimensions>;

public:
  StencilOperatorBoundary(const StencilOperatorT* stencil_op)
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
    return _stencil_op->_spec_views.boundary_views();
  }

  /**
   * Returns the number of all boundary elements (no dublicates)
   */
  uindex_t boundary_size() const {
    return _stencil_op->_spec_views.boundary_size();
  }

  /**
   * Modifies all stencil point elements and the center with halo check.
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
  template <typename BinaryFunc = internal::replace<Element_t>>
  void set_values_at(const Coords_t& coords, Element_t value,
                     Element_t   coefficient_center,
                     BinaryFunc op = BinaryFunc()) {
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
    uindex_t offset = 0;
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

  std::pair<iterator, iterator> iterator_at(region_index_t index) {
    DASH_ASSERT_LT(index, NumRegionsMax<NumDimensions>, "Given index out of range");
    const auto&    bnd_views = _stencil_op->_spec_views.boundary_views();
    uindex_t offset = 0;
    for(region_index_t r = 0; r < index; ++r) {
      offset += bnd_views[r].size();
    }

    auto it_begin = _stencil_op->_bbegin + offset;

    return std::make_pair(it_begin, it_begin + bnd_views[index].size());
  }



  /**
   * Returns the result of the given operation done on all stencil point
   * elements and the center.
   * The stencil points are multiplied with their coefficent (\ref StencilPoint)
   * and the center is  multiplied with the given center coefficient.
   * Test and use of halo elements. -> useful only for elements with stencil
   * points pointing to halo elements. (boundary elements)
   *
   * \param coords center coordinate
   * \param coefficient for center
   * \param op operation to use (e.g. std::plus). default: std::plus
   */
  template <typename BinaryFunc = std::plus<Element_t>>
  Element_t get_value_at(const Coords_t& coords,
                        Element_t               coefficient_center,
                        BinaryFunc             op = BinaryFunc()) const {
    auto* center = _stencil_op->_local_memory + _stencil_op->get_offset(coords);
    Element_t value        = *center * coefficient_center;
    auto&    stencil_spec = _stencil_op->_stencil_spec;
    for(auto i = 0; i < NumStencilPoints; ++i) {
      bool halo           = false;
      auto coords_stencil = coords;

      for(auto d = 0; d < NumDimensions; ++d) {
        coords_stencil[d] += stencil_spec[i][d];
        if(coords_stencil[d] < 0
           || coords_stencil[d] >= _stencil_op->_view_local->extent(d))
          halo = true;
      }

      if(halo) {
        auto& halo_memory = *_stencil_op->_halo_memory;
        auto  index       = _stencil_op->_halo_block->index_at(
          *(_stencil_op->_view_local), coords_stencil);
        auto halomem_pos = halo_memory.first_element_at(index);

        halo_memory.to_halo_mem_coords(index, coords_stencil);
        auto offset = halo_memory.offset(index, coords_stencil);
        value = op(value, stencil_spec[i].coefficient() * halomem_pos[offset]);
      } else {
        auto stencil_point_value = center[_stencil_op->_stencil_offsets[i]];
        value = op(value, stencil_spec[i].coefficient() * stencil_point_value);
      }
    }

    return value;
  }

  /**
   * Updates all boundary elements using a user-defined stencil operation.
   *
   * \param begin_dst Pointer to the beginning of the destination memory
   * \param operation User-definied operation for updating all boundary elements
  */
  template <typename Op>
  void update(Element_t* begin_dst, Op operation) {
    update(begin(), end(), begin_dst, operation);
  }


  /**
   * Updates all boundary elements within a user defined range using a userdefined
   * stencil operation.
   *
   * \param begin Iterator of the beginnning boundary data element
   * \param end Iterator of the last boundary data element
   * \param begin_dst Pointer to the beginning of the destination memory
   * \param operation User-definied operation for updating all inner elements
  */
  template <typename Op>
  void update(const iterator& begin, const iterator& end, Element_t* begin_out,
              Op operation) {
    for(auto it = begin; it != end; ++it) {
      begin_out[it.lpos()] = operation(it);
    }
  }

private:
  const StencilOperatorT* _stencil_op;
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
template <typename HaloBlockT, typename StencilSpecT>
class StencilOperator {
private:
  using Self_t = StencilOperator<HaloBlockT, StencilSpecT>;
  using Pattern_t = typename HaloBlockT::Pattern_t;

  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto NumDimensions    = Pattern_t::ndim();
  static constexpr auto MemoryArrange    = Pattern_t::memory_order();



  template <typename _SO>
  friend class StencilOperatorInner;

  template <typename _SO>
  friend class StencilOperatorBoundary;

public:
  using Inner_t = StencilOperatorInner<Self_t>;
  using Bnd_t   = StencilOperatorBoundary<Self_t>;


  using Element_t         = typename HaloBlockT::Element_t;
  using index_t           = typename std::make_signed<typename Pattern_t::index_type>::type;
  using uindex_t          = typename std::make_unsigned<index_t>::type;
  using StencilOffsets_t      = std::array<index_t, NumStencilPoints>;
  using HaloBlock_t           = HaloBlockT;
  using HaloMemory_t          = HaloMemory<HaloBlock_t>;
  using ViewSpec_t            = typename HaloBlockT::ViewSpec_t;
  using Coords_t              = std::array<index_t, NumDimensions>;
  using StencilSpec_t         = StencilSpecT;

  using StencilSpecViews_t = StencilSpecificViews<HaloBlock_t, StencilSpecT>;

  using stencil_index_t = typename StencilSpecT::stencil_index_t;

    using iterator =
    StencilIterator<Element_t, Pattern_t, typename HaloBlockT::GlobMem_t, StencilSpecT, StencilViewScope::ALL>;
  using const_iterator = const iterator;
  /*using iterator_inner =
    StencilIterator<ElementT, PatternT, GlobMemT, StencilSpecT, StencilViewScope::INNER>;
  using const_iterator_inner = const iterator;*/
  using CoordsIdxManagerInner_t = CoordsIdxManagerInner<Self_t>;
  using iterator_inner = StencilIteratorTest<CoordsIdxManagerInner_t>;
  using const_iterator_inner = const iterator;
  using CoordsIdxManagerBoundary_t = CoordsIdxManagerBoundary<Self_t>;
  using iterator_bnd         = StencilIteratorTest<CoordsIdxManagerBoundary_t>;
  using const_iterator_bnd   = const iterator;

public:
  /**
   * Constructor that takes a \ref HaloBlock, a \ref HaloMemory,
   * a \ref StencilSpec and a local \ref ViewSpec
   */
  StencilOperator(
      const HaloBlockT*   haloblock,
      Element_t*          local_memory,
      HaloMemory_t*       halomemory,
      const StencilSpecT& stencil_spec)
    : inner(this)
    , boundary(this)
    , _halo_block(haloblock)
    , _local_memory(local_memory)
    , _halo_memory(halomemory)
    , _stencil_spec(stencil_spec)
    , _view_local(&haloblock->view_local())
    , _stencil_offsets(set_stencil_offsets())
    , _spec_views(*_halo_block, _stencil_spec, _view_local)
    , _begin(
          _local_memory,
          _halo_memory,
          &_stencil_spec,
          &_stencil_offsets,
          *_view_local,
          _spec_views.inner_with_boundaries(),
          0)
    , _end(
          _local_memory,
          _halo_memory,
          &_stencil_spec,
          &_stencil_offsets,
          *_view_local,
          _spec_views.inner_with_boundaries(),
          _spec_views.inner_with_boundaries().size())
    , _ibegin(CoordsIdxManagerInner_t(*this))
    , _iend(CoordsIdxManagerInner_t(*this,
                _spec_views.inner().size()))
    , _bbegin(CoordsIdxManagerBoundary_t(*this))
    , _bend(CoordsIdxManagerBoundary_t(*this,
                _spec_views.boundary_size())) {
  }

  static constexpr decltype(auto) ndim() { return NumDimensions; }

  static constexpr decltype(auto) memory_order() { return MemoryArrange; }

  static constexpr decltype(auto) num_stencil_points() { return NumStencilPoints; }

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
   * Returns the halo memory management object \ref HaloMemory
   */
  Element_t* local_memory() { return _local_memory; }

  /**
   * Returns the \ref StencilSpecificView
   */
  const StencilSpecViews_t& spec_views() const { return _spec_views; }

  /**
   * Returns a \ref SpecView including inner and boundary elements
   */
  const ViewSpec_t& view() const { return _spec_views.inner_with_boundaries(); }

  /**
   * Returns the local \ref SpecView
   */
  const ViewSpec_t& view_local() const { return *_view_local; }

  /**
   * Returns the offsets for each stencil point.
   */
  const StencilOffsets_t& stencil_offsets() const { return _stencil_offsets; }

  /**
   * Returns the offset for specific stencil point.
   */
  const index_t stencil_offset_at(std::size_t pos) const {
    return _stencil_offsets[pos];
  }



  /**
   * Returns the local memory offset for a given coordinate
   */
  index_t get_offset(const Coords_t& coords) const {
    index_t offset = 0;

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

private:
  StencilOffsets_t set_stencil_offsets() {
    StencilOffsets_t stencil_offs;
    for(auto i = 0u; i < NumStencilPoints; ++i) {
      index_t offset = 0;
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

  StencilOffsets_t set_dimension_offsets() {
    StencilOffsets_t      dim_offs;
    index_t offset = 0;
    if(MemoryArrange == ROW_MAJOR) {
      dim_offs[NumDimensions - 1] = 1;
      for(auto d = NumDimensions - 1; d > 0;) {
        --d;
        dim_offs[d] = dim_offs[d + 1] * _view_local->extent(d + 1);
      }
    } else {
      dim_offs[0] = 1;
      for(auto d = 1; d < NumDimensions; ++d)
        dim_offs[d] = dim_offs[d - 1] * _view_local->extent(d - 1);
    }

    return dim_offs;
  }

public:
  Inner_t inner;
  Bnd_t   boundary;

private:
  const HaloBlock_t* _halo_block;
  Element_t*         _local_memory;
  HaloMemory_t*      _halo_memory;
  const StencilSpecT _stencil_spec;
  const ViewSpec_t*  _view_local;
  StencilOffsets_t   _stencil_offsets;
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
