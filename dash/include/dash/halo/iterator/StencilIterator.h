#ifndef DASH__HALO__ITERATOR__STENCILITERATOR_H
#define DASH__HALO__ITERATOR__STENCILITERATOR_H

#include <dash/Types.h>

#include <dash/halo/Halo.h>

#include <vector>

namespace dash {

namespace halo {

/**
 * View property of the StencilIterator
 */
enum class StencilViewScope : std::uint8_t {
  /// inner elements only
  INNER,
  /// Boundary elements only
  BOUNDARY,
  /// Inner and boundary elements
  ALL
};

inline std::ostream& operator<<(std::ostream&           os,
                                const StencilViewScope& scope) {
  if(scope == StencilViewScope::INNER)
    os << "INNER";
  else if(scope == StencilViewScope::BOUNDARY)
    os << "BOUNDARY";
  else
    os << "ALL";

  return os;
}

/**
 * Adapts all views \ref HaloBlock provides to the given \ref StencilSpec.
 */
template <typename HaloBlockT, typename StencilSpecT>
class StencilSpecificViews {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();

  using Pattern_t = typename HaloBlockT::Pattern_t;

public:
  using ViewSpec_t      = typename HaloBlockT::ViewSpec_t;
  using BoundaryViews_t = typename HaloBlockT::BoundaryViews_t;
  using pattern_size_t  = typename Pattern_t::size_type;

public:
  StencilSpecificViews(const HaloBlockT&   haloblock,
                       const StencilSpecT& stencil_spec,
                       const ViewSpec_t*   view_local)
  : _view_local(view_local) {
    auto minmax_dist = stencil_spec.minmax_distances();
    for(auto& dist : minmax_dist)
      dist.first = std::abs(dist.first);

    auto inner_off       = haloblock.view_inner().offsets();
    auto inner_ext       = haloblock.view_inner().extents();
    auto inner_bound_off = haloblock.view_inner_with_boundaries().offsets();
    auto inner_bound_ext = haloblock.view_inner_with_boundaries().extents();
    for(auto d = 0; d < NumDimensions; ++d) {
      resize_offset(inner_off[d], inner_ext[d], minmax_dist[d].first);
      resize_extent(inner_off[d], inner_ext[d], _view_local->extent(d),
                    minmax_dist[d].second);
      resize_offset(inner_bound_off[d], inner_bound_ext[d],
                    minmax_dist[d].first);
      resize_extent(inner_bound_off[d], inner_bound_ext[d],
                    _view_local->extent(d), minmax_dist[d].second);
    }
    _view_inner                 = ViewSpec_t(inner_off, inner_ext);
    _view_inner_with_boundaries = ViewSpec_t(inner_bound_off, inner_bound_ext);

    using RegionCoords_t = RegionCoords<NumDimensions>;
    using region_index_t = typename RegionCoords_t::region_index_t;

    const auto& bnd_elems    = haloblock.boundary_views();
    const auto& halo_ext_max = haloblock.halo_extension_max();
    _boundary_views.reserve(NumDimensions * 2);
    auto it_views = std::begin(bnd_elems);

    for(dim_t d = 0; d < NumDimensions; ++d) {
      region_index_t index  = RegionCoords_t::index(d, RegionPos::PRE);
      auto*          region = haloblock.boundary_region(index);
      if(region == nullptr || (region != nullptr && region->size() == 0))
        _boundary_views.push_back(ViewSpec_t());
      else {
        push_boundary_views(*it_views, halo_ext_max, minmax_dist);
        ++it_views;
      }
      index  = RegionCoords_t::index(d, RegionPos::POST);
      region = haloblock.boundary_region(index);
      if(region == nullptr || (region != nullptr && region->size() == 0))
        _boundary_views.push_back(ViewSpec_t());
      else {
        push_boundary_views(*it_views, halo_ext_max, minmax_dist);
        ++it_views;
      }
    }
  }

  /**
   * Returns \ref ViewSpec including all elements (locally)
   */
  const ViewSpec_t& view() const { return *_view_local; }

  /**
   * Returns \ref ViewSpec including all inner elements
   */
  const ViewSpec_t& inner() const { return _view_inner; }

  /**
   * Returns \ref ViewSpec including all inner and boundary elements
   */
  const ViewSpec_t& inner_with_boundaries() const {
    return _view_inner_with_boundaries;
  }

  /**
   * Returns all boundary views including all boundary elements (no dublicates)
   */
  const BoundaryViews_t& boundary_views() const { return _boundary_views; }

  /**
   * Returns the number of all boundary elements (no dublicates)
   */
  pattern_size_t boundary_size() const { return _size_bnd_elems; }

private:
  template <typename MaxExtT, typename MaxDistT>
  void push_boundary_views(const ViewSpec_t& view, const MaxExtT& max_ext,
                           const MaxDistT& max_dist) {
    auto view_off = view.offsets();
    auto view_ext = view.extents();
    for(auto d = 0; d < NumDimensions; ++d) {
      if(view_off[d] < max_ext[d].first && view_ext[d] == max_ext[d].first) {
        view_ext[d] = max_dist[d].first;
      } else if(view_ext[d] == max_ext[d].second) {
        view_ext[d] = max_dist[d].second;
        view_off[d] += max_ext[d].second - max_dist[d].second;
      } else {
        resize_offset(view_off[d], view_ext[d], max_dist[d].first);
        resize_extent(view_off[d], view_ext[d], _view_local->extent(d),
                      max_dist[d].second);
      }
    }
    ViewSpec_t tmp(view_off, view_ext);
    _size_bnd_elems += tmp.size();
    _boundary_views.push_back(std::move(tmp));
  }

  template <typename OffT, typename ExtT, typename MaxT>
  void resize_offset(OffT& offset, ExtT& extent, MaxT max) {
    if(offset > max) {
      extent += offset - max;
      offset = max;
    }
  }

  template <typename OffT, typename ExtT, typename MinT>
  void resize_extent(OffT& offset, ExtT& extent, ExtT extent_local, MinT max) {
    auto diff_ext = extent_local - offset - extent;
    if(diff_ext > max)
      extent += diff_ext - max;
  }

private:
  const ViewSpec_t* _view_local;
  ViewSpec_t        _view_inner;
  ViewSpec_t        _view_inner_with_boundaries;
  BoundaryViews_t   _boundary_views;
  pattern_size_t    _size_bnd_elems = 0;
};

template <typename HaloBlockT, typename StencilSpecT>
std::ostream& operator<<(
  std::ostream&                                         os,
  const StencilSpecificViews<HaloBlockT, StencilSpecT>& stencil_views) {
  std::ostringstream ss;
  ss << "dash::halo::StencilSpecificViews"
     << "(local: " << stencil_views.local()
     << "; inner: " << stencil_views.inner()
     << "; inner_bound: " << stencil_views.inner_with_boundaries()
     << "; boundary_views: " << stencil_views.boundary_views()
     << "; boundary elems: " << stencil_views.boundary_size() << ")";

  return operator<<(os, ss.str());
}

/*
 * Stencil specific iterator to iterate over a given scope of elements.
 * The iterator provides element access via stencil points and for boundary
 * elements halo element access.
 */
template <typename ElementT, typename PatternT, typename StencilSpecT,
          StencilViewScope Scope>
class StencilIterator {
private:
  static constexpr auto NumDimensions    = PatternT::ndim();
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto MemoryArrange    = PatternT::memory_order();
  static constexpr auto FastestDimension =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;

  using Self_t     = StencilIterator<ElementT, PatternT, StencilSpecT, Scope>;
  using ViewSpec_t = typename PatternT::viewspec_type;
  using pattern_size_t        = typename PatternT::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using RegionCoords_t        = RegionCoords<NumDimensions>;
  using HaloBlock_t           = HaloBlock<ElementT, PatternT>;

public:
  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = ElementT;
  using difference_type   = typename PatternT::index_type;
  using pointer           = ElementT*;
  using reference         = ElementT&;

  using HaloMemory_t    = HaloMemory<HaloBlock_t>;
  using pattern_index_t = typename PatternT::index_type;
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using LocalLayout_t =
    CartesianIndexSpace<NumDimensions, MemoryArrange, pattern_index_t>;
  using StencilP_t       = StencilPoint<NumDimensions>;
  using ElementCoords_t  = std::array<pattern_index_t, NumDimensions>;
  using StencilOffsets_t = std::array<signed_pattern_size_t, NumStencilPoints>;
  using StencilSpecViews_t = StencilSpecificViews<HaloBlock_t, StencilSpecT>;
  using BoundaryViews_t    = typename StencilSpecViews_t::BoundaryViews_t;

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
    if(_idx < _view.size())
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
    pattern_index_t size = 0;
    for(const auto& view : boundary_views)
      size += view.size();
    if(_idx < size)
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

  const ElementCoords_t& coords() const { return _coords; };

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
    set_coords();

    return result;
  }

  Self_t& operator+=(pattern_index_t n) {
    _idx += n;
    set_coords();

    return *this;
  }

  Self_t& operator-=(pattern_index_t n) {
    _idx -= n;
    set_coords();

    return *this;
  }

  Self_t operator+(pattern_index_t n) const {
    Self_t res{ *this };
    res += n;

    return res;
  }

  Self_t operator-(pattern_index_t n) const {
    Self_t res{ *this };
    res -= n;

    return res;
  }

  difference_type operator-(Self_t& other) const { return _idx - other._idx; }

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
    if(&_view == &(other._view) || _view == other._view) {
      return gidx_cmp(_idx, other._idx);
    }
    // TODO not the best solution
    return false;
  }

  /*void set_view(const ViewSpec_t& view_tmp) {
    if(Scope == StencilViewScope::BOUNDARY) {
      const auto& bnd_elems = _haloblock.boundary_elements();
      _bnd_elements.reserve(bnd_elems.size());
      const auto& view_offs = view_tmp.offsets();
      for(const auto& region : bnd_elems) {
        auto off = region.offsets();
        for(int d = 0; d < NumDimensions; ++d)
          off[d] -= view_offs[d];

        _bnd_elements.push_back(ViewSpec_t(off, region.extents()));
      }

      _view = ViewSpec_t(view_tmp.extents());
    } else {
      const auto& view_offsets = _haloblock.view().offsets();
      auto        off          = view_tmp.offsets();
      for(int d = 0; d < NumDimensions; ++d)
        off[d] -= view_offsets[d];

      _view = ViewSpec_t(off, view_tmp.extents());
    }
  }*/

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
      _coords = set_coords(_idx);
    }

    // setup center point offset
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
            indexes[i] *= RegionCoords_t::REGION_INDEX_BASE;
            is_halo[i] = true;
            continue;
          }

          if(halo_coord < static_cast<signed_extent_t>(extent)) {
            indexes[i] = 1 + indexes[i] * RegionCoords_t::REGION_INDEX_BASE;
            continue;
          }

          indexes[i] = 2 + indexes[i] * RegionCoords_t::REGION_INDEX_BASE;
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

  void set_stencil_offsets(const StencilSpecT& stencil_spec) {
    for(auto i = 0; i < NumStencilPoints; ++i) {
      signed_pattern_size_t offset = 0;
      if(MemoryArrange == ROW_MAJOR) {
        offset = stencil_spec[i][0];
        for(dim_t d = 1; d < NumDimensions; ++d)
          offset = stencil_spec[i][d] + offset * _local_layout.extent(d);
      } else {
        offset = stencil_spec[i][NumDimensions - 1];
        for(dim_t d = NumDimensions - 1; d > 0;) {
          --d;
          offset = stencil_spec[i][d] + offset * _local_layout.extent(d);
        }
      }
      (*_stencil_offsets)[i] = offset;
    }
  }

private:
  HaloMemory_t*                           _halomemory;
  const StencilSpecT*                     _stencil_spec;
  const StencilOffsets_t*                 _stencil_offsets;
  const ViewSpec_t                        _view;
  const BoundaryViews_t                   _boundary_views{};
  ElementT*                               _local_memory;
  std::array<ElementT*, NumStencilPoints> _stencil_mem_ptr;
  const LocalLayout_t                     _local_layout;
  pattern_index_t                         _idx{ 0 };
  // extension of the fastest index dimension minus the halo extension
  std::pair<pattern_index_t, pattern_index_t> _ext_dim_reduced;
  signed_pattern_size_t                       _offset;
  pattern_index_t                             _region_bound{ 0 };
  size_t                                      _region_number{ 0 };
  ElementCoords_t                             _coords;
  ElementT*                                   _current_lmemory_addr;
};  // class StencilIterator

}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO__ITERATOR__STENCILITERATOR_H

