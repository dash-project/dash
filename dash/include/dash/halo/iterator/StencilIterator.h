#ifndef DASH__HALO__ITERATOR__STENCILITERATOR_H
#define DASH__HALO__ITERATOR__STENCILITERATOR_H

#include <dash/Types.h>

#include <dash/halo/Halo.h>
#include <dash/halo/HaloMemory.h>

#include <vector>

namespace dash {

namespace halo {

using namespace internal;

template<typename StencilOpT>
class CoordsIdxManagerInner {
private:
  using Self_t     = CoordsIdxManagerInner<StencilOpT>;
  using StencilSpec_t = typename StencilOpT::StencilSpec_t;

  static constexpr auto NumDimensions    = StencilOpT::ndim();
  static constexpr auto NumStencilPoints = StencilOpT::num_stencil_points();
  static constexpr auto MemoryArrange    = StencilOpT::memory_order();
  static constexpr auto FastestDimension =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;

public:
  using Element_t = typename StencilOpT::Element_t;
  using ViewSpec_t  = typename StencilOpT::ViewSpec_t;
  using index_t   = typename StencilOpT::index_t;
  using uindex_t   = typename StencilOpT::uindex_t;
  using StencilP_t = StencilPoint<NumDimensions>;
  using Coords_t       = typename StencilOpT::Coords_t;
  using stencil_index_t = typename StencilSpec_t::stencil_index_t;

private:
  using RangeDim_t = std::pair<uindex_t, uindex_t>;
  using Ranges_t   = std::array<RangeDim_t, NumDimensions>;
  using StencilOffsPtrs_t     = std::array<Element_t*, NumStencilPoints>;
  using OffsetsDim_t          = std::array<uindex_t, NumDimensions>;
  using viewspec_index_t = typename ViewSpec_t::index_type;
  using LocalLayout_t =
    CartesianIndexSpace<NumDimensions, MemoryArrange, viewspec_index_t>;

public:

  CoordsIdxManagerInner(StencilOpT& stencil_op,
                    uindex_t start_idx = 0, const ViewSpec_t* sub_view = nullptr)
  : _stencil_op(&stencil_op),
    _sub_view((sub_view != nullptr) ? sub_view : &(stencil_op.inner.view())),
    _size(_sub_view->size()),
    _local_layout(stencil_op.view_local().extents()) {
    // initializes ranges, coordinates depending on the index, offsets for all dimensions and all stencil pointer
    init_ranges();
    set(start_idx);
  }

  static constexpr decltype(auto) ndim() { return NumDimensions; }

  const ViewSpec_t& view() const { return _stencil_op->view_local(); }

  const ViewSpec_t& sub_view() const { return *_sub_view; }

  const Coords_t& coords() const { return _coords; }

  Coords_t coords(uindex_t idx) const { return _local_layout.coords(idx, *_sub_view); }

  const uindex_t& index() const { return _idx; }

  const uindex_t& offset() const { return _offset; }

  Element_t& value() const { return *_current_lmemory_addr; }

  Element_t& value_at(const stencil_index_t index_stencil ) const { return *_stencil_mem_ptr[index_stencil]; }

  Element_t& value_at(const StencilP_t& stencil) {
    const auto index_stencil = _stencil_op->stencil_spec().index(stencil);

    DASH_ASSERT_MSG(index_stencil.second,
                    "No valid region index for given stencil point found");

    return value_at(index_stencil.first);
  }

  void set(uindex_t idx) {
    if(idx >=_size) {
      _idx = _size;

      return;
    }

    _idx = idx;
    init_coords();
    init_offset();
    init_stencil_points();
  }

  Element_t operator[](index_t n) const {
    auto index = _idx + n;
    auto new_coords = coords(index);

    return _stencil_op->local_memory()[_local_layout.at(new_coords)];
  }

  Element_t& operator[](index_t n) {
    return operator[](n);
  }

  void next_element() {
    ++_idx;
    ++_coords[FastestDimension];
    if(static_cast<uindex_t>(_coords[FastestDimension]) < _ranges[FastestDimension].second) {
      for(auto i = 0u; i < NumStencilPoints; ++i)
        ++_stencil_mem_ptr[i];

      ++_current_lmemory_addr;
      ++_offset;

      return;
    }
    _coords[FastestDimension] = _sub_view->offset(FastestDimension);
    uindex_t add = 0;
    if(MemoryArrange == ROW_MAJOR) {
      for(dim_t d = NumDimensions-1; d > 0;) {
        --d;
        ++_coords[d];
        if(static_cast<uindex_t>(_coords[d]) < _ranges[d].second) {
          add = _offsets_dim[d];
          break;
        } else {
          _coords[d] = _ranges[d].first;

        }
      }
    } else {
      for(dim_t d = 1; d < NumDimensions; ++d) {
        ++_coords[d];
        if(static_cast<uindex_t>(_coords[d]) < _ranges[d].second) {
          add = _offsets_dim[d];
          break;
        } else {
          _coords[d] = _ranges[d].first;
        }
      }
    }
    _current_lmemory_addr += add;
    _offset += add;
    for(auto i = 0u; i < NumStencilPoints; ++i) {
      _stencil_mem_ptr[i] += add;
    }
  }

private:
  void init_ranges() {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      _ranges[d] = std::make_pair(_sub_view->offset(d), _sub_view->offset(d) + _sub_view->extent(d));
    }
  }

  void init_coords() {
    _coords = _local_layout.coords(_idx, *_sub_view);
  }

  void init_offset() {
    if(MemoryArrange == ROW_MAJOR) {
      _offset = _coords[0];
      for(dim_t d = 1; d < NumDimensions; ++d)
        _offset = _offset * _local_layout.extent(d) + _coords[d];
    } else {
      _offset = _coords[NumDimensions - 1];
      for(dim_t d = NumDimensions - 1; d > 0;) {
        --d;
        _offset = _offset * _local_layout.extent(d) + _coords[d];
      }
    }

    const auto& view = _stencil_op->view_local();
    _offsets_dim[FastestDimension] = 1;
    if(MemoryArrange == ROW_MAJOR) {
      if(FastestDimension > 0) {
        _offsets_dim[FastestDimension - 1] = (view.extent(FastestDimension) - _sub_view->extent(FastestDimension)) + 1;
      }
      for(dim_t d = FastestDimension - 1; d > 0;) {
        --d;
        _offsets_dim[d] = (view.extent(d+1) - _sub_view->extent(d+1)) * view.extent(d+2) + _offsets_dim[d+1];
      }
    } else {
      if(NumDimensions > 1) {
      _offsets_dim[FastestDimension + 1] = (view.extent(FastestDimension) - _sub_view->extent(FastestDimension)) + 1;
      }
      for(dim_t d = 2; d < NumDimensions; ++d) {
        _offsets_dim[d] = (view.extent(d-1) - _sub_view->extent(d-1)) * view.extent(d-2) + _offsets_dim[d-1];
      }
    }
  }

  void init_stencil_points() {
    _current_lmemory_addr = _stencil_op->local_memory() + _offset;
    for(auto i = 0u; i < NumStencilPoints; ++i) {
      _stencil_mem_ptr[i] = _current_lmemory_addr + _stencil_op->stencil_offsets()[i];
    }
  }

private:
  StencilOpT*       _stencil_op;
  const ViewSpec_t* _sub_view;
  uindex_t          _size;
  LocalLayout_t     _local_layout;
  uindex_t          _idx;
  Element_t*        _current_lmemory_addr;
  StencilOffsPtrs_t _stencil_mem_ptr;
  Ranges_t          _ranges;
  Coords_t          _coords;
  uindex_t          _offset;
  OffsetsDim_t      _offsets_dim;
};

template <typename StencilOpT>
std::ostream& operator<<(
  std::ostream&                                   os,
  const CoordsIdxManagerInner<StencilOpT>& helper) {
  os << "dash::halo::CoordsHelper"
     << "(view: " << helper.view()
     << "; sub_view: " << helper.sub_view()
     << "; index: " << helper.index()
     << "; offset: " << helper.offset()
     << "; coords: { ";
     for(const auto& elem : helper.coords()) {
       os << elem << " ";
     }
     os << "})";

  return os;
}

template<typename StencilOpT>
class CoordsIdxManagerBoundary {

private:
  using Self_t     = CoordsIdxManagerBoundary<StencilOpT>;
  using StencilSpec_t = typename StencilOpT::StencilSpec_t;
  using StencilSpecViews_t   = typename StencilOpT::StencilSpecViews_t;

  static constexpr auto NumDimensions    = StencilOpT::ndim();
  static constexpr auto NumStencilPoints = StencilOpT::num_stencil_points();
  static constexpr auto MemoryArrange    = StencilOpT::memory_order();
  static constexpr auto FastestDimension =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;

public:
  using Element_t = typename StencilOpT::Element_t;
  using ViewSpec_t  = typename StencilOpT::ViewSpec_t;
  using index_t   = typename StencilOpT::index_t;
  using uindex_t  = typename StencilOpT::uindex_t;
  using StencilP_t = StencilPoint<NumDimensions>;
  using Coords_t       = typename StencilOpT::Coords_t;
  using stencil_index_t = typename StencilSpec_t::stencil_index_t;

  using RegionCoords_t        = RegionCoords<NumDimensions>;
private:
  using BoundaryViews_t       = typename StencilSpecViews_t::BoundaryViews_t;
  using viewspec_index_t = typename ViewSpec_t::index_type;
  using LocalLayout_t =
    CartesianIndexSpace<NumDimensions, MemoryArrange, viewspec_index_t>;
  using ViewIndexPair_t       = std::pair<const ViewSpec_t*, uindex_t>;
  using StencilOffsPtrs_t     = std::array<Element_t*, NumStencilPoints>;
  using OffsetsDim_t          = std::array<uindex_t, NumDimensions>;

  struct RangeDim_t {
    uindex_t begin = 0;
    uindex_t end = 0;
  };
  using Ranges_t   = std::array<RangeDim_t, NumDimensions>;

  struct HaloPointProp_t {
    bool           possible;
    bool           always;
    region_index_t index;
  };
  using HaloPoints_t = std::array<HaloPointProp_t, NumStencilPoints>;


public:

  CoordsIdxManagerBoundary(StencilOpT& stencil_op, uindex_t start_idx = 0)
  : _stencil_op(&stencil_op),
    _size(stencil_op.spec_views().boundary_size()),
    _region_number(0),
    _local_layout(stencil_op.view_local().extents()),
    _idx(0) {
    const auto& ext_max = stencil_op.stencil_spec().minmax_distances(FastestDimension);

    _ext_dim_reduced = {static_cast<uindex_t>(std::abs(ext_max.first)),
                        _local_layout.extent(FastestDimension) - static_cast<uindex_t>(ext_max.second)};
    set(start_idx);
  }

  static constexpr decltype(auto) ndim() { return NumDimensions; }

  const ViewSpec_t& view() const { return _stencil_op->view_local(); }

  const ViewSpec_t& sub_view() const { return *(_current_view.first); }

  const Coords_t& coords() const { return _coords; }

  const uindex_t& index() const { return _idx; }

  const uindex_t& offset() const { return _offset; }

  Element_t& value() const { return *_current_lmemory_addr; }

  Element_t& value_at(const stencil_index_t index_stencil ) const { return *_stencil_mem_ptr[index_stencil]; }

  Element_t& value_at(const StencilP_t& stencil) {
    const auto index_stencil = _stencil_op->stencil_spec().index(stencil);

    DASH_ASSERT_MSG(index_stencil.second,
                    "No valid region index for given stencil point found");

    return value_at(index_stencil.first);
  }

  void set(uindex_t idx) {
    if(idx >=_size) {
      _idx = _size;

      return;
    }

    _idx = idx;
    _current_view = get_current_view(_idx);
    init_ranges();
    init_coords();
    init_offset();
    init_stencil_points();

  }

  Element_t operator[](index_t n) const {
    auto index = _idx + n;
    auto new_coords = coords(index);

    return _stencil_op->local_memory()[_local_layout.at(new_coords)];
  }

  Element_t& operator[](index_t n) {
    return operator[](n);
  }

  const region_index_t region_id() const { return _region_number; }

  const uindex_t& size() const { return _size; }

  void next_element() {
    ++_idx;
    ++_current_view.second;
    ++_coords[FastestDimension];
    uindex_t add = 1;
    if(static_cast<uindex_t>(_coords[FastestDimension]) < _ranges[FastestDimension].end) {
      if(static_cast<uindex_t>(_coords[FastestDimension]) >= _ext_dim_reduced.begin
        && static_cast<uindex_t>(_coords[FastestDimension]) < _ext_dim_reduced.end) {
        ++_current_lmemory_addr;
        ++_offset;
        for(auto i = 0u; i < NumStencilPoints; ++i) {
          ++_stencil_mem_ptr[i];
        }

        return;
      }
    } else {

      if(_current_view.second == (*_current_view.first).size()) {

        auto& bnd_views = _stencil_op->boundary.view();

        do {
          ++_region_number;
          if(_region_number >= bnd_views.size()) {
            _region_number = bnd_views.size();

            return;
          }

        } while(bnd_views[_region_number].size() == 0);

        if(_idx < _size) {
          _current_view = {&bnd_views[_region_number],0};
          init_ranges();
          init_coords();
          init_offset();
          init_stencil_points();
        }

        return;
      }

      if(static_cast<uindex_t>(_coords[FastestDimension]) >= _ranges[FastestDimension].end) {
        _coords[FastestDimension] = _ranges[FastestDimension].begin;
        if(MemoryArrange == ROW_MAJOR) {
          for(dim_t d = NumDimensions-1; d > 0;) {
            --d;
            ++_coords[d];
            if(static_cast<uindex_t>(_coords[d]) < _ranges[d].end) {
              add = _offsets_dim[d];
              break;
            } else {
              _coords[d] = _ranges[d].begin;
            }
          }
        }

        if(MemoryArrange == COL_MAJOR) {
          for(dim_t d = 1; d < NumDimensions; ++d) {
            ++_coords[d];
            if(static_cast<uindex_t>(_coords[d]) < _ranges[d].end) {
              add = _offsets_dim[d];
              break;
            } else {
              _coords[d] = _ranges[d].begin;
            }
          }
        }
      }
    }

    _current_lmemory_addr += add;
    _offset += add;
    const auto& extents = _local_layout.extents();
    const auto& specs = _stencil_op->stencil_spec();
    const auto& stencil_offs = _stencil_op->stencil_offsets();
    for(auto i = 0u; i < NumStencilPoints; ++i) {
      if(_spoint_is_halo[i].possible) {
        auto& stencil = specs[i];
        auto coords = _coords;

        if(_spoint_is_halo[i].always) {
          for(dim_t d = 0; d < NumDimensions; ++d)
            coords[d] += stencil[d];
          _stencil_mem_ptr[i] = value_halo_at(_spoint_is_halo[i].index, coords);
          continue;
        }

        bool is_halo = false;
        region_index_t index = 0;
        for(dim_t d = 0; d < NumDimensions; ++d) {
          auto stencil_off = stencil[d];
          if(stencil_off == 0) {
            index = 1 + index * REGION_INDEX_BASE;
            continue;
          }
          coords[d] += stencil_off;
          if(coords[d] < 0) {
            index *= REGION_INDEX_BASE;
            is_halo = true;
            continue;
          }

          if(static_cast<uindex_t>(coords[d]) < extents[d]) {
            index = 1 + index * REGION_INDEX_BASE;
            continue;
          }

          index = 2 + index * REGION_INDEX_BASE;
          is_halo = true;
        }
        if(is_halo) {
           _stencil_mem_ptr[i] = value_halo_at(index, coords);
           continue;
        }

        _stencil_mem_ptr[i] = _current_lmemory_addr + stencil_offs[i];
      } else {
        _stencil_mem_ptr[i] += add;
      }
    }
  }


private:

  void init_ranges() {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      auto sub_view = _current_view.first;
      _ranges[d] = {static_cast<uindex_t>(sub_view->offset(d)),
                    static_cast<uindex_t>(sub_view->offset(d)) + sub_view->extent(d)};
    }
  }

  void init_coords() {
    _coords = _local_layout.coords(_current_view.second, *_current_view.first);
  }

  void init_offset() {
    if(MemoryArrange == ROW_MAJOR) {
      _offset = _coords[0];
      for(dim_t d = 1; d < NumDimensions; ++d)
        _offset = _offset * _local_layout.extent(d) + _coords[d];
    } else {
      _offset = _coords[NumDimensions - 1];
      for(dim_t d = NumDimensions - 1; d > 0;) {
        --d;
        _offset = _offset * _local_layout.extent(d) + _coords[d];
      }
    }

    auto& sub_view = *(_current_view.first);
     _offsets_dim[FastestDimension] = 1;
     const auto& view = _stencil_op->view_local();
    if(MemoryArrange == ROW_MAJOR) {
      if(FastestDimension > 0) {
        _offsets_dim[FastestDimension - 1] = (view.extent(FastestDimension) - sub_view.extent(FastestDimension)) + 1;
      }
      for(dim_t d = FastestDimension - 1; d > 0;) {
        --d;
        _offsets_dim[d] = (view.extent(d+1) - sub_view.extent(d+1)) * view.extent(d+2) + _offsets_dim[d+1];
      }
    } else {
      if(NumDimensions > 1) {
      _offsets_dim[FastestDimension + 1] = (view.extent(FastestDimension) - sub_view.extent(FastestDimension)) + 1;
      }
      for(dim_t d = 2; d < NumDimensions; ++d) {
        _offsets_dim[d] = (view.extent(d-1) - sub_view.extent(d-1)) * view.extent(d-2) + _offsets_dim[d-1];
      }
    }
  }

  ViewIndexPair_t get_current_view(uindex_t idx) {
    _region_number = 0;
    const auto& bnd_views = _stencil_op->boundary.view();
    for(const auto& region : bnd_views) {
      if(idx < region.size()) {
        return std::make_pair(&region, idx);
      }
      ++_region_number;
      idx -= region.size();
    }

    auto& last_region = bnd_views.back();
    return std::make_pair(&last_region, last_region.size());
  }

  void init_stencil_points() {
    _current_lmemory_addr = _stencil_op->local_memory() + _offset;
    const auto& specs = _stencil_op->stencil_spec();
    const auto& stencil_offs = _stencil_op->stencil_offsets();
    auto minmax = specs.minmax_distances();
    const auto& extents = _local_layout.extents();

    for(auto i = 0u; i < NumStencilPoints; ++i) {
      auto& spoint_halo =_spoint_is_halo[i];
      spoint_halo = {false, true, 0};

      auto halo_coord = _coords;
      bool is_halo = false;
      for(dim_t d = 0; d < NumDimensions; ++d) {
        auto stencil_off = specs[i][d];
        if(stencil_off == 0) {
          spoint_halo.index = 1 + spoint_halo.index * REGION_INDEX_BASE;
          continue;
        }

        halo_coord[d] += stencil_off;
        if(halo_coord[d] < 0) {
          spoint_halo.index *= REGION_INDEX_BASE;
          spoint_halo.possible = true;
          is_halo = true;
          if( halo_coord[d] > minmax[d].first) {
            spoint_halo.always = false;
          }
          continue;
        }

        if(static_cast<uindex_t>(halo_coord[d]) < extents[d]) {
          spoint_halo.index = 1 + spoint_halo.index * REGION_INDEX_BASE;
          if(_coords[d] < std::abs(minmax[d].first) ||
            (extents[d] - static_cast<uindex_t>(_coords[d])) <=  static_cast<uindex_t>(minmax[d].second)) {
            spoint_halo.always = false;
            spoint_halo.possible = true;
          }

          continue;
        }

        spoint_halo.index = 2 + spoint_halo.index * REGION_INDEX_BASE;
        spoint_halo.possible = true;
        is_halo = true;
        if(minmax[d].second != stencil_off) {
          spoint_halo.always = false;
         }
      }

      if(is_halo)
        _stencil_mem_ptr[i] = value_halo_at(spoint_halo.index, halo_coord);
      else
        _stencil_mem_ptr[i] = _current_lmemory_addr + stencil_offs[i];
    }
  }

  Element_t* value_halo_at(region_index_t   region_index,
                          Coords_t& halo_coords) {
    auto& halo_memory = _stencil_op->halo_memory();
    halo_memory.to_halo_mem_coords(region_index, halo_coords);

    return &*(halo_memory.first_element_at(region_index)
           + halo_memory.offset(region_index, halo_coords));
  }

private:
  StencilOpT*       _stencil_op;
  uindex_t          _size;
  region_index_t    _region_number{ 0 };
  LocalLayout_t     _local_layout;
  uindex_t          _idx;
  Coords_t          _coords;
  ViewIndexPair_t   _current_view;
  Element_t*        _current_lmemory_addr;
  StencilOffsPtrs_t _stencil_mem_ptr;
  HaloPoints_t      _spoint_is_halo;

  uindex_t          _offset;
  OffsetsDim_t      _offsets_dim;
  Ranges_t          _ranges;
  RangeDim_t        _ext_dim_reduced;
};

template <typename StencilOpT>
std::ostream& operator<<(
  std::ostream&                                         os,
  const CoordsIdxManagerBoundary<StencilOpT>& helper) {
  os << "dash::halo::CoordsHelper"
     << "(view: " << helper.view()
     << "; region id: " << helper.region_id()
     << "; sub_view: " << helper.sub_view()
     << "; index: " << helper.index()
     << "; offset: " << helper.offset()
     << "; coords: { ";
     for(const auto& elem : helper.coords()) {
       os << elem << " ";
     }
     os << "})";

  return os;
}


/*
 * Stencil specific iterator to iterate over a given scope of elements.
 * The iterator provides element access via stencil points and for boundary
 * elements halo element access.
 */
template <typename CoordsIdxManagerT>
class StencilIteratorTest {
private:
  using Element_t = typename CoordsIdxManagerT::Element_t;

  using Self_t     = StencilIteratorTest<CoordsIdxManagerT>;
  using ViewSpec_t = typename CoordsIdxManagerT::ViewSpec_t;

  static constexpr auto NumDimensions    = CoordsIdxManagerT::ndim();

public:
  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = Element_t;
  using difference_type   = typename CoordsIdxManagerT::uindex_t;
  using pointer           = Element_t*;
  using reference         = Element_t&;

  using index_t = typename CoordsIdxManagerT::index_t;
  using uindex_t = typename CoordsIdxManagerT::uindex_t;
  using StencilP_t      = StencilPoint<NumDimensions>;
  using Coords_t       = typename CoordsIdxManagerT::Coords_t;
  using stencil_index_t  = typename CoordsIdxManagerT::stencil_index_t;

public:
  //TODO anpassen
  /**
   * Constructor
   *
   * \param
   */
  StencilIteratorTest(CoordsIdxManagerT coords_mng)
  : _coords_mng(coords_mng) {
  }

   /**
   * Copy constructor.
   */
  StencilIteratorTest(const Self_t& other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  Self_t& operator=(const Self_t& other) = default;

  /**
   * The number of dimensions of the iterator's underlying pattern.
   *
   * \see DashGlobalIteratorConcept
   */
  static constexpr dim_t ndim() { return NumDimensions; }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const { return _coords_mng.value(); }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  Element_t operator[](index_t n) const {
    return _coords_mng[n];
  }

  reference operator[](index_t n) {
    return _coords_mng[n];
  }

  uindex_t rpos() const { return _coords_mng.index(); }

  uindex_t lpos() const { return _coords_mng.offset(); }

  Coords_t coords() const { return _coords_mng.coords(); }

  CoordsIdxManagerT& helper() {return _coords_mng;}

  /**
   * Returns the value for a given stencil point index (index postion in
   * \ref StencilSpec)
   */
  Element_t value_at(const stencil_index_t index_stencil) {
    return _coords_mng.value_at(index_stencil);
  }

  /* returns the value of a given stencil point (not as efficient as
   * stencil point index )
   */
  Element_t value_at(const StencilP_t& stencil) {
    return _coords_mng.value_at(stencil);
  }

  /**
   * Prefix increment operator.
   */
  Self_t& operator++() {
    _coords_mng.next_element();

    return *this;
  }

  /**
   * Postfix increment operator.
   */
  Self_t operator++(int) {
    Self_t result = *this;

    _coords_mng.next_element();

    return result;
  }

  /**
   * Prefix decrement operator.
   */
  Self_t& operator--() {
    _coords_mng.set(_coords_mng.index()-1);

    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  Self_t operator--(int) {
     Self_t result = *this;

    _coords_mng.set(_coords_mng.index()-1);

    return result;
  }

  Self_t& operator+=(index_t n) {
    _coords_mng.set(_coords_mng.index() + n);

    return *this;
  }

  Self_t& operator-=(index_t n) {
    auto index = _coords_mng.index();
    if(index >= n)
      _coords_mng.set(index - n);

    return *this;
  }

  Self_t operator+(index_t n) const {
    auto res( *this );
    res += n;

    return res;
  }

  Self_t operator-(index_t n) const {
    auto res( *this );
    res -= n;

    return res;
  }

  difference_type operator-(const Self_t& other) const { return _coords_mng.index() - other._coords_mng.index(); }

  bool operator<(const Self_t& other) const {
    return compare(other, std::less<index_t>());
  }

  bool operator<=(const Self_t& other) const {
    return compare(other, std::less_equal<index_t>());
  }

  bool operator>(const Self_t& other) const {
    return compare(other, std::greater<index_t>());
  }

  bool operator>=(const Self_t& other) const {
    return compare(other, std::greater_equal<index_t>());
  }

  bool operator==(const Self_t& other) const {
    return compare(other, std::equal_to<index_t>());
  }

  bool operator!=(const Self_t& other) const {
    return compare(other, std::not_equal_to<index_t>());
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template <typename GlobIndexCmpFunc>
  bool compare(const Self_t& other, const GlobIndexCmpFunc& gidx_cmp) const {

    return gidx_cmp(_coords_mng.index(), other._coords_mng.index());
  }

private:
  CoordsIdxManagerT _coords_mng;
};  // class StencilIterator


/*
 * Stencil specific iterator to iterate over a given scope of elements.
 * The iterator provides element access via stencil points and for boundary
 * elements halo element access.
 */
template <typename ElementT, typename PatternT, typename GlobMemT, typename StencilSpecT,
          StencilViewScope Scope>
class StencilIterator {
private:
  static constexpr auto NumDimensions    = PatternT::ndim();
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto MemoryArrange    = PatternT::memory_order();
  static constexpr auto FastestDimension =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;

  using Self_t     = StencilIterator<ElementT, PatternT, GlobMemT, StencilSpecT, Scope>;
  using ViewSpec_t = typename PatternT::viewspec_type;
  using pattern_size_t        = typename PatternT::size_type;
  using RegionCoords_t        = RegionCoords<NumDimensions>;
  using HaloBlock_t           = HaloBlock<ElementT, PatternT, GlobMemT>;

public:
  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = ElementT;
  using difference_type   = typename PatternT::index_type;
  using pointer           = ElementT*;
  using reference         = ElementT&;

  using HaloMemory_t    = HaloMemory<HaloBlock_t>;
  using pattern_index_t = typename PatternT::index_type;
  using LocalLayout_t =
    CartesianIndexSpace<NumDimensions, MemoryArrange, pattern_index_t>;
  using StencilP_t            = StencilPoint<NumDimensions>;
  using ElementCoords_t       = std::array<pattern_index_t, NumDimensions>;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using StencilOffsets_t      =
    std::array<signed_pattern_size_t, NumStencilPoints>;
  using StencilSpecViews_t    = StencilSpecificViews<HaloBlock_t, StencilSpecT>;
  using BoundaryViews_t       = typename StencilSpecViews_t::BoundaryViews_t;

public:
  /**
   * Constructor
   *
   * \param local_memory Pointer to the begining of the local NArray memory
   * \param halomemory \ref HaloMemory instance for loacl halo memory
   * \param stencil_spec \ref StencilSpec to use
   * \param stencil_offsets stencil offsets for every stencil point
   * \param view_local local \ref SpecView including all local elements
   * \param view_scope \ref ViewSpec to use
   * \param idx position of the iterator
   */
  StencilIterator(ElementT* local_memory, HaloMemory_t* halomemory,
                  const StencilSpecT*     stencil_spec,
                  const StencilOffsets_t* stencil_offsets,
                  const ViewSpec_t& view_local, const ViewSpec_t& view_scope,
                  pattern_index_t idx)
  : _halomemory(halomemory), _stencil_spec(stencil_spec),
    _stencil_offsets(stencil_offsets), _view(view_scope),
    _local_memory(local_memory),
    _local_layout(view_local.extents()), _idx(idx) {
    _size = _view.size();
    if(_idx < _size)
      set_coords();

    const auto ext_max = stencil_spec->minmax_distances(FastestDimension);
    if(Scope == StencilViewScope::INNER) {
      _ext_dim_reduced = std::make_pair(
        _view.offset(FastestDimension),
        _local_layout.extent(FastestDimension) - ext_max.second - 1);
    } else {
      _ext_dim_reduced =
        std::make_pair(std::abs(ext_max.first),
                       _view.extent(FastestDimension) - ext_max.second - 1);
    }
  }

  /**
   * Constructor
   *
   * \param local_memory Pointer to the begining of the local NArray memory
   * \param halomemory \ref HaloMemory instance for halo memory elements
   * \param stencil_spec \ref StencilSpec to use
   * \param stencil_offsets stencil offsets for every stencil point
   * \param view_local local \ref SpecView including all local elements
   * \param boundary_views all relevant boundary views
   * \param idx position of the iterator
   */
  StencilIterator(ElementT* local_memory, HaloMemory_t* halomemory,
                  const StencilSpecT*     stencil_spec,
                  const StencilOffsets_t* stencil_offsets,
                  const ViewSpec_t&       view_local,
                  const BoundaryViews_t& boundary_views, pattern_index_t idx)
  : _halomemory(halomemory), _stencil_spec(stencil_spec),
    _stencil_offsets(stencil_offsets),
    _view(view_local.extents()),
    _boundary_views(boundary_views),
    _local_memory(local_memory),
    _local_layout(view_local.extents()), _idx(idx) {
    _size = 0;
    for(const auto& view : boundary_views)
      _size += view.size();

    if(_idx < _size)
      set_coords();

    const auto ext_max = stencil_spec->minmax_distances(FastestDimension);

    _ext_dim_reduced =
      std::make_pair(std::abs(ext_max.first),
                     _view.extent(FastestDimension) - ext_max.second - 1);
  }

  /**
   * Copy constructor.
   */
  StencilIterator(const Self_t& other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  Self_t& operator=(const Self_t& other) = default;

  /**
   * The number of dimensions of the iterator's underlying pattern.
   *
   * \see DashGlobalIteratorConcept
   */
  static constexpr dim_t ndim() { return NumDimensions; }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const { return *_current_lmemory_addr; }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  reference operator[](pattern_index_t n) const {
    auto coords = set_coords(_idx + n);
    return _local_memory[_local_layout.at(coords)];
  }

  pattern_index_t rpos() const { return _idx; }

  pattern_index_t lpos() const { return _offset; }

  ElementCoords_t coords() const { return _coords; };

  bool is_halo_value(const region_index_t index_stencil) {
    if(Scope == StencilViewScope::INNER)
      return false;

    auto        halo_coords = _coords;
    const auto& stencil     = (*_stencil_spec)[index_stencil];
    for(auto d = 0; d < NumDimensions; ++d) {
      halo_coords[d] += stencil[d];
      if(halo_coords[d] < 0 || halo_coords[d] >= _local_layout.extent(d))
        return true;
    }

    return false;
  }

  /**
   * Returns the value for a given stencil point index (index postion in
   * \ref StencilSpec)
   */
  ElementT value_at(const region_index_t index_stencil) {
    return *(_stencil_mem_ptr[index_stencil]);
  }

  /* returns the value of a given stencil point (not as efficient as
   * stencil point index )
   */
  ElementT value_at(const StencilP_t& stencil) {
    auto index_stencil = _stencil_spec->index(stencil);

    DASH_ASSERT_MSG(index_stencil.second,
                    "No valid region index for given stencil point found");

    return value_at(index_stencil.first);
  }

  /**
   * Prefix increment operator.
   */
  Self_t& operator++() {
    ++_idx;
    next_element();

    return *this;
  }

  /**
   * Postfix increment operator.
   */
  Self_t operator++(int) {
    Self_t result = *this;
    ++_idx;
    next_element();

    return result;
  }

  /**
   * Prefix decrement operator.
   */
  Self_t& operator--() {
    --_idx;
    set_coords();

    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  Self_t operator--(int) {
    Self_t result = *this;
    --_idx;
    if(_idx < _size) {
      _coords = set_coords(_idx);
      set_offsets();
    }

    return result;
  }

  Self_t& operator+=(pattern_index_t n) {
    _idx += n;
    if(_idx < _size) {
      _coords = set_coords(_idx);
      set_offsets();
    }

    return *this;
  }

  Self_t& operator-=(pattern_index_t n) {
    _idx -= n;
    if(_idx < _size) {
      _coords = set_coords(_idx);
      set_offsets();
    }

    return *this;
  }

  Self_t operator+(pattern_index_t n) const {
    auto res( *this );
    res += n;

    return res;
  }

  Self_t operator-(pattern_index_t n) const {
    auto res( *this );
    res -= n;

    return res;
  }

  difference_type operator-(const Self_t& other) const { return _idx - other._idx; }

  bool operator<(const Self_t& other) const {
    return compare(other, std::less<pattern_index_t>());
  }

  bool operator<=(const Self_t& other) const {
    return compare(other, std::less_equal<pattern_index_t>());
  }

  bool operator>(const Self_t& other) const {
    return compare(other, std::greater<pattern_index_t>());
  }

  bool operator>=(const Self_t& other) const {
    return compare(other, std::greater_equal<pattern_index_t>());
  }

  bool operator==(const Self_t& other) const {
    return compare(other, std::equal_to<pattern_index_t>());
  }

  bool operator!=(const Self_t& other) const {
    return compare(other, std::not_equal_to<pattern_index_t>());
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template <typename GlobIndexCmpFunc>
  bool compare(const Self_t& other, const GlobIndexCmpFunc& gidx_cmp) const {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if(this == &other) {
      return true;
    }
#endif
    return gidx_cmp(_idx, other._idx);

    // TODO not the best solution
    return false;
  }

  void next_element() {
    const auto& coord_fastest_dim = _coords[FastestDimension];

    if(coord_fastest_dim >= _ext_dim_reduced.first
       && coord_fastest_dim < _ext_dim_reduced.second) {
      for(auto it = _stencil_mem_ptr.begin(); it != _stencil_mem_ptr.end();
          ++it)
        *it += 1;

      ++_coords[FastestDimension];
      ++_current_lmemory_addr;
      ++_offset;

      return;
    }

    if(Scope == StencilViewScope::INNER) {
      if(MemoryArrange == ROW_MAJOR) {
        for(dim_t d = NumDimensions; d > 0;) {
          --d;
          if(_coords[d] < _view.extent(d) + _view.offset(d) - 1) {
            ++_coords[d];
            break;
          } else
            _coords[d] = _view.offset(d);
        }
      } else {
        for(dim_t d = 0; d < NumDimensions; ++d) {
          if(_coords[d] < _view.extent(d) + _view.offset(d) - 1) {
            ++_coords[d];
            break;
          } else
            _coords[d] = _view.offset(d);
        }
      }
      if(MemoryArrange == ROW_MAJOR) {
        _offset = _coords[0];
        for(dim_t d = 1; d < NumDimensions; ++d)
          _offset = _offset * _local_layout.extent(d) + _coords[d];
      } else {
        _offset = _coords[NumDimensions - 1];
        for(dim_t d = NumDimensions - 1; d > 0;) {
          --d;
          _offset = _offset * _local_layout.extent(d) + _coords[d];
        }
      }
      _current_lmemory_addr = _local_memory + _offset;
      for(auto i = 0; i < NumStencilPoints; ++i)
        _stencil_mem_ptr[i] = _current_lmemory_addr + (*_stencil_offsets)[i];
    } else
      set_coords();
  }

  void set_coords() {
    if(Scope == StencilViewScope::BOUNDARY) {
      if(_region_bound == 0) {
        _coords = set_coords(_idx);
      } else {
        if(_idx < _region_bound) {
          const auto& region = _boundary_views[_region_number];

          if(MemoryArrange == ROW_MAJOR) {
            for(dim_t d = NumDimensions; d > 0;) {
              --d;
              if(_coords[d] < region.extent(d) + region.offset(d) - 1) {
                ++_coords[d];
                break;
              } else {
                _coords[d] = region.offset(d);
              }
            }
          } else {
            for(dim_t d = 0; d < NumDimensions; ++d) {
              if(_coords[d] < region.extent(d) + region.offset(d) - 1) {
                ++_coords[d];
                break;
              } else {
                _coords[d] = region.offset(d);
              }
            }
          }
        } else {
          do {
            ++_region_number;
            if(_region_number >= _boundary_views.size())
              return;

            _region_bound += _boundary_views[_region_number].size();
          } while(_idx >= _region_bound);
          _coords = _local_layout.coords(0, _boundary_views[_region_number]);
        }
      }
    } else {
      if(_idx < _size)
        _coords = set_coords(_idx);
    }

    set_offsets();
  }

  void set_offsets() {// setup center point offset
    if(MemoryArrange == ROW_MAJOR) {
      _offset = _coords[0];
      for(dim_t d = 1; d < NumDimensions; ++d)
        _offset = _offset * _local_layout.extent(d) + _coords[d];
    } else {
      _offset = _coords[NumDimensions - 1];
      for(dim_t d = NumDimensions - 1; d > 0;) {
        --d;
        _offset = _offset * _local_layout.extent(d) + _coords[d];
      }
    }
    _current_lmemory_addr = _local_memory + _offset;

    // setup stencil point offsets
    if(Scope == StencilViewScope::INNER) {
      for(auto i = 0; i < NumStencilPoints; ++i)
        _stencil_mem_ptr[i] = _current_lmemory_addr + (*_stencil_offsets)[i];
    } else {
      using signed_extent_t = typename std::make_signed<pattern_size_t>::type;
      std::array<ElementCoords_t, NumStencilPoints> halo_coords{};
      std::array<bool, NumStencilPoints>            is_halo{};
      std::array<region_index_t, NumStencilPoints>  indexes{};
      for(auto d = 0; d < NumDimensions; ++d) {
        auto extent = _local_layout.extent(d);

        for(auto i = 0; i < NumStencilPoints; ++i) {
          auto& halo_coord = halo_coords[i][d];
          halo_coord       = _coords[d] + (*_stencil_spec)[i][d];
          if(halo_coord < 0) {
            indexes[i] *= REGION_INDEX_BASE;
            is_halo[i] = true;
            continue;
          }

          if(halo_coord < static_cast<signed_extent_t>(extent)) {
            indexes[i] = 1 + indexes[i] * REGION_INDEX_BASE;
            continue;
          }

          indexes[i] = 2 + indexes[i] * REGION_INDEX_BASE;
          is_halo[i] = true;
        }
      }

      for(auto i = 0; i < NumStencilPoints; ++i) {
        if(is_halo[i])
          _stencil_mem_ptr[i] = value_halo_at(indexes[i], halo_coords[i]);
        else
          _stencil_mem_ptr[i] = _current_lmemory_addr + (*_stencil_offsets)[i];
      }
    }
  }

  ElementCoords_t set_coords(pattern_index_t idx) {
    if(Scope == StencilViewScope::BOUNDARY) {
      _region_bound = 0;
      _region_number = 0;
      auto local_idx = idx;
      for(const auto& region : _boundary_views) {
        _region_bound += region.size();
        if(local_idx < region.size()) {
          return _local_layout.coords(local_idx, region);
        }
        ++_region_number;
        local_idx -= region.size();
      }
      DASH_ASSERT("idx >= size not implemented yet");
      return std::array<pattern_index_t, NumDimensions>{};
    } else {
      if(_view.size() == 0)
        return std::array<pattern_index_t, NumDimensions>{};
      else
        return _local_layout.coords(idx, _view);
    }
  }

  ElementT* value_halo_at(region_index_t   region_index,
                          ElementCoords_t& halo_coords) {
    _halomemory->to_halo_mem_coords(region_index, halo_coords);

    return &*(_halomemory->first_element_at(region_index)
           + _halomemory->offset(region_index, halo_coords));
  }

private:
  HaloMemory_t*                           _halomemory;
  const StencilSpecT*                     _stencil_spec;
  const StencilOffsets_t*                 _stencil_offsets;
  ViewSpec_t                              _view;
  BoundaryViews_t                         _boundary_views{};
  ElementT*                               _local_memory;
  std::array<ElementT*, NumStencilPoints> _stencil_mem_ptr;
  LocalLayout_t                           _local_layout;
  pattern_index_t                         _idx{ 0 };
  // extension of the fastest index dimension minus the halo extension
  std::pair<pattern_index_t, pattern_index_t> _ext_dim_reduced;
  signed_pattern_size_t                       _offset;
  pattern_index_t                             _region_bound{ 0 };
  size_t                                      _region_number{ 0 };
  ElementCoords_t                             _coords;
  ElementT*                                   _current_lmemory_addr;
  pattern_index_t                             _size;
};  // class StencilIterator


}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO__ITERATOR__STENCILITERATOR_H
