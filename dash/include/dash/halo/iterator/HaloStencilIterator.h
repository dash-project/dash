#ifndef DASH__HALO__ITERATOR__HALOSTENCILITERATOR_H
#define DASH__HALO__ITERATOR__HALOSTENCILITERATOR_H

#include <dash/Types.h>

#include <dash/halo/Halo.h>

#include <vector>

namespace dash {

enum class StencilViewScope : std::uint8_t { INNER, BOUNDARY, ALL };
/*
 * Iterator with stencil points and halo access \see HaloStencilOperator.
 */
template <typename ElementT, typename PatternT, typename StencilSpecT,
          StencilViewScope Scope>
class HaloStencilIterator {
private:
  static constexpr auto NumDimensions    = PatternT::ndim();
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto MemoryArrange    = PatternT::memory_order();
  static constexpr auto FastestDimension =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;

  using Self_t = HaloStencilIterator<ElementT, PatternT, StencilSpecT, Scope>;
  using ViewSpec_t            = typename PatternT::viewspec_type;
  using pattern_size_t        = typename PatternT::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using RegionCoords_t        = RegionCoords<NumDimensions>;

public:
  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = ElementT;
  using difference_type   = typename PatternT::index_type;
  using pointer           = ElementT*;
  using reference         = ElementT&;

  using HaloBlock_t     = HaloBlock<ElementT, PatternT>;
  using HaloMemory_t    = HaloMemory<HaloBlock_t>;
  using pattern_index_t = typename PatternT::index_type;
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using LocalLayout_t =
    CartesianIndexSpace<NumDimensions, MemoryArrange, pattern_index_t>;
  using StencilP_t       = StencilPoint<NumDimensions>;
  using ElementCoords_t  = std::array<pattern_index_t, NumDimensions>;
  using StencilOffsets_t = std::array<signed_pattern_size_t, NumStencilPoints>;

public:
  /**
   * Constructor
   *
   * \param haloblock \ref HaloBlock instance to use
   * \param stencil_spec \ref StencilSpec to use
   * \param stencil_offsets stencil offsets for every stencil point
   * \param idx position of the iterator
   */
  HaloStencilIterator(const HaloBlock_t& haloblock, HaloMemory_t& halomemory,
                      const StencilSpecT&     stencil_spec,
                      const StencilOffsets_t& stencil_offsets,
                      pattern_index_t         idx)
  : _haloblock(haloblock), _halomemory(halomemory), _stencil_spec(stencil_spec),
    _stencil_offsets(stencil_offsets),
    _local_memory((ElementT*) _haloblock.globmem().lbegin()),
    _local_layout(_haloblock.pattern().local_memory_layout()), _idx(idx) {
    if(Scope == StencilViewScope::INNER)
      set_view_local(_haloblock.view_inner());

    if(Scope == StencilViewScope::ALL)
      set_view_local(_haloblock.view_inner_with_boundaries());

    if(Scope == StencilViewScope::BOUNDARY)
      set_view_local(_haloblock.view());

    pattern_index_t _size = 0;
    if(Scope == StencilViewScope::BOUNDARY)
      _size = _haloblock.boundary_size();
    else
      _size = _view_local.size();

    if(_idx < _size)
      set_coords();

    const auto& ext_max = haloblock.halo_extension_max(FastestDimension);
    if(Scope == StencilViewScope::INNER) {
      _ext_dim_reduced = std::make_pair(
        _view_local.offset(FastestDimension),
        _local_layout.extent(FastestDimension) - ext_max.second - 1);
    } else {
      _ext_dim_reduced =
        std::make_pair(ext_max.first, _view_local.extent(FastestDimension)
                                        - ext_max.second - 1);
    }
  }

  /**
   * Copy constructor.
   */
  HaloStencilIterator(const Self_t& other) = default;

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
    const auto& stencil     = _stencil_spec[index_stencil];
    for(auto d = 0; d < NumDimensions; ++d) {
      halo_coords[d] += stencil[d];
      if(halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
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
    auto index_stencil = _stencil_spec.index(stencil);

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
    if(&_view_local == &(other._view_local)
       || _view_local == other._view_local) {
      return gidx_cmp(_idx, other._idx);
    }
    // TODO not the best solution
    return false;
  }

  void set_view_local(const ViewSpec_t& view_tmp) {
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

      _view_local = ViewSpec_t(view_tmp.extents());
    } else {
      const auto& view_offsets = _haloblock.view().offsets();
      auto        off          = view_tmp.offsets();
      for(int d = 0; d < NumDimensions; ++d)
        off[d] -= view_offsets[d];

      _view_local = ViewSpec_t(off, view_tmp.extents());
    }
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
        for(auto i = NumDimensions - 1; i >= 0; --i) {
          if(_coords[i] < _view_local.extent(i) + _view_local.offset(i) - 1) {
            ++_coords[i];
            break;
          } else
            _coords[i] = _view_local.offset(i);
        }
      } else {
        for(auto i = 0; i < NumDimensions; ++i) {
          if(_coords[i] < _view_local.extent(i) + _view_local.offset(i) - 1) {
            ++_coords[i];
            break;
          } else
            _coords[i] = _view_local.offset(i);
        }
      }
      if(MemoryArrange == ROW_MAJOR) {
        _offset = _coords[0];
        for(auto d = 1; d < NumDimensions; ++d)
          _offset = _offset * _local_layout.extent(d) + _coords[d];
      } else {
        _offset = _coords[NumDimensions - 1];
        for(auto d = NumDimensions - 2; d >= 0; --d)
          _offset = _offset * _local_layout.extent(d) + _coords[d];
      }
      _current_lmemory_addr = _local_memory + _offset;
      for(auto i = 0; i < NumStencilPoints; ++i)
        _stencil_mem_ptr[i] = _current_lmemory_addr + _stencil_offsets[i];
    } else
      set_coords();
  }

  void set_coords() {
    if(Scope == StencilViewScope::BOUNDARY) {
      if(_region_bound == 0) {
        _coords = set_coords(_idx);
      } else {
        if(_idx < _region_bound) {
          const auto& region = _bnd_elements[_region_number];
          if(MemoryArrange == ROW_MAJOR) {
            for(auto i = NumDimensions - 1; i >= 0; --i) {
              if(_coords[i] < region.extent(i) + region.offset(i) - 1) {
                ++_coords[i];
                break;
              } else
                _coords[i] = region.offset(i);
            }
          } else {
            for(auto i = 0; i < NumDimensions; ++i) {
              if(_coords[i] < region.extent(i) + region.offset(i) - 1) {
                ++_coords[i];
                break;
              } else
                _coords[i] = region.offset(i);
            }
          }
        } else {
          ++_region_number;
          if(_region_number < _bnd_elements.size()) {
            _region_bound += _bnd_elements[_region_number].size();
            _coords = _local_layout.coords(0, _bnd_elements[_region_number]);
          }
        }
      }
    } else {
      _coords = set_coords(_idx);
    }

    if(MemoryArrange == ROW_MAJOR) {
      _offset = _coords[0];
      for(auto d = 1; d < NumDimensions; ++d)
        _offset = _offset * _local_layout.extent(d) + _coords[d];
    } else {
      _offset = _coords[NumDimensions - 1];
      for(auto d = NumDimensions - 2; d >= 0; --d)
        _offset = _offset * _local_layout.extent(d) + _coords[d];
    }
    _current_lmemory_addr = _local_memory + _offset;
    if(Scope == StencilViewScope::INNER) {
      for(auto i = 0; i < NumStencilPoints; ++i)
        _stencil_mem_ptr[i] = _current_lmemory_addr + _stencil_offsets[i];
    } else {
      using signed_extent_t = typename std::make_signed<pattern_size_t>::type;
      std::array<ElementCoords_t, NumStencilPoints> halo_coords{};
      std::array<bool, NumStencilPoints>            is_halo{};
      std::array<region_index_t, NumStencilPoints>  indexes{};
      for(auto d = 0; d < NumDimensions; ++d) {
        auto extent = _haloblock.view().extent(d);

        for(auto i = 0; i < NumStencilPoints; ++i) {
          auto& halo_coord = halo_coords[i][d];
          halo_coord       = _coords[d] + _stencil_spec[i][d];
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
          _stencil_mem_ptr[i] = _current_lmemory_addr + _stencil_offsets[i];
      }
    }
  }

  std::array<pattern_index_t, NumDimensions> set_coords(pattern_index_t idx) {
    if(Scope == StencilViewScope::BOUNDARY) {
      auto local_idx = idx;
      for(const auto& region : _bnd_elements) {
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
      if(_view_local.size() == 0)
        return std::array<pattern_index_t, NumDimensions>{};
      else
        return _local_layout.coords(idx, _view_local);
    }
  }

  ElementT* value_halo_at(region_index_t   region_index,
                          ElementCoords_t& halo_coords) {
    _halomemory.to_halo_mem_coords(region_index, halo_coords);

    return _halomemory.pos_at(region_index)
           + _halomemory.offset(region_index, halo_coords);
  }

  void set_stencil_offsets(const StencilSpecT& stencil_spec) {
    for(auto i = 0; i < NumStencilPoints; ++i) {
      signed_pattern_size_t offset = 0;
      if(MemoryArrange == ROW_MAJOR) {
        offset = stencil_spec[i][0];
        for(auto d = 1; d < NumDimensions; ++d)
          offset = stencil_spec[i][d] + offset * _local_layout.extent(d);
      } else {
        offset = stencil_spec[i][NumDimensions - 1];
        for(auto d = NumDimensions - 2; d >= 0; --d)
          offset = stencil_spec[i][d] + offset * _local_layout.extent(d);
      }
      _stencil_offsets[i] = offset;
    }
  }

private:
  const HaloBlock_t&                      _haloblock;
  HaloMemory_t&                           _halomemory;
  const StencilSpecT&                     _stencil_spec;
  const StencilOffsets_t&                 _stencil_offsets;
  ElementT*                               _local_memory;
  ViewSpec_t                              _view_local;
  std::vector<ViewSpec_t>                 _bnd_elements;
  std::array<ElementT*, NumStencilPoints> _stencil_mem_ptr;
  const LocalLayout_t&                    _local_layout;
  pattern_index_t                         _idx{ 0 };
  // extension of the fastest index dimension minus the halo extension
  std::pair<pattern_index_t, pattern_index_t> _ext_dim_reduced;
  signed_pattern_size_t                       _offset;
  pattern_index_t                             _region_bound{ 0 };
  size_t                                      _region_number{ 0 };
  ElementCoords_t                             _coords;
  ElementT*                                   _current_lmemory_addr;
};  // class HaloStencilIterator

}  // namespace dash

#endif  // DASH__HALO__ITERATOR__HALOSTENCILITERATOR_H

