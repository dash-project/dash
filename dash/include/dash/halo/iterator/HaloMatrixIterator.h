#ifndef DASH__HALO__ITERATOR__HALOMATRIXITERATOR_H
#define DASH__HALO__ITERATOR__HALOMATRIXITERATOR_H

#include <dash/Types.h>

#include <dash/halo/Halo.h>

#include <vector>

namespace dash {

enum class StencilViewScope : std::uint8_t { INNER, BOUNDARY, ALL };

template <typename ElementT, typename PatternT, typename StencilSpecT,
          StencilViewScope Scope>
class HaloMatrixIterator {
private:
  static constexpr auto NumDimensions    = PatternT::ndim();
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();
  static constexpr auto MemoryArrange    = PatternT::memory_order();

  using Self_t = HaloMatrixIterator<ElementT, PatternT, StencilSpecT, Scope>;
  using ViewSpec_t            = typename PatternT::viewspec_type;
  using pattern_size_t        = typename PatternT::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;

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
  using region_index_t  = typename RegionCoords<NumDimensions>::region_index_t;
  using LocalLayout_t =
    CartesianIndexSpace<NumDimensions, MemoryArrange, pattern_index_t>;
  using Stencil_t       = Stencil<NumDimensions>;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;

public:
  HaloMatrixIterator(const HaloBlock_t& haloblock, HaloMemory_t& halomemory,
                     const StencilSpecT& stencil_spec, pattern_index_t idx)
  : _haloblock(haloblock), _halomemory(halomemory), _stencil_spec(stencil_spec),
    _local_memory((ElementT*) _haloblock.globmem().lbegin()),
    _local_layout(_haloblock.pattern().local_memory_layout()), _idx(idx)

  {
    if(Scope == StencilViewScope::INNER)
      set_view_local(_haloblock.view_inner());

    if(Scope == StencilViewScope::ALL)
      set_view_local(_haloblock.view_guaranteed());

    if(Scope == StencilViewScope::BOUNDARY)
      set_view_local(_haloblock.view());

    if(Scope == StencilViewScope::BOUNDARY)
      _size = _haloblock.boundary_size();
    else
      _size = _view_local.size();

    set_coords();
    set_stencil_offsets(stencil_spec);
  }

  /**
   * Copy constructor.
   */
  HaloMatrixIterator(const Self_t& other) = default;

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

  pattern_index_t lpos() const { return _local_layout.at(_coords); }

  const ElementCoords_t& coords() const { return _coords; };

  bool is_halo_value(const region_index_t index_stencil) {
    if(Scope == StencilViewScope::INNER)
      return false;

    auto        halo_coords{ _coords };
    const auto& stencil = _stencil_spec[index_stencil];
    for(auto d = 0; d < NumDimensions; ++d) {
      halo_coords[d] += stencil[d];
      if(halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
        return true;
    }

    return false;
  }

  std::vector<ElementT> halo_values() {
    // TODO: is the given offset in halospec range?
    std::vector<ElementT> halos;
    if(Scope == StencilViewScope::INNER)
      return halos;

    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto        halo_coords{ _coords };
      const auto& stencil = _stencil_spec[i];
      bool        halo    = false;
      for(auto d = 0; d < NumDimensions; ++d) {
        halo_coords[d] += stencil[d];
        if(halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
          halo = true;
      }
      // TODO check wether region is nullptr or not
      // TODO implement as method in RegionSpec
      if(halo)
        halos.push_back(value_halo_at(halo_coords));
    }
    return halos;
  }

  ElementT value_at(const region_index_t index_stencil) {

    if(Scope == StencilViewScope::INNER)
      return *(_current_lmemory_addr + _stencil_offsets[index_stencil]);

    auto        halo_coords{ _coords };
    const auto& stencil = _stencil_spec[index_stencil];
    bool        halo    = false;
    for(auto d = 0; d < NumDimensions; ++d) {
      halo_coords[d] += stencil[d];
      if(halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
        halo = true;
    }
    // TODO check wether region is nullptr or not
    // TODO implement as method in RegionSpec
    if(halo)
      return value_halo_at(halo_coords);

    return *(_current_lmemory_addr + _stencil_offsets[index_stencil]);
  }

  ElementT value_at(const Stencil_t& stencil) {

    if(Scope == StencilViewScope::INNER) {
      return *halo_pos(stencil);
    } else {
      auto halo_coords{ _coords };

      bool halo = false;
      for(auto d = 0; d < NumDimensions; ++d) {
        halo_coords[d] += stencil[d];
        if(halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
          halo = true;
      }
      // TODO check wether region is nullptr or not
      // TODO implement as method in RegionSpec
      if(halo)
        return value_halo_at(halo_coords);

      return *halo_pos(stencil);
    }
  }

  /**
   * Prefix increment operator.
   */
  Self_t& operator++() {
    ++_idx;
    set_coords();

    return *this;
  }

  /**
   * Postfix increment operator.
   */
  Self_t operator++(int) {
    Self_t result = *this;
    ++_idx;
    set_coords();

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

  /*pattern_index_t operator+(
    const Self_t & other) const
  {
    return _idx + other._idx;
  }

  pattern_index_t operator-(
    const Self_t & other) const
  {
    return _idx - other._idx;
  }*/

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

  void set_coords() {
    _coords            = set_coords(_idx);
    pattern_size_t off = 0;
    if(MemoryArrange == ROW_MAJOR) {
      off = _coords[0];
      for(auto d = 1; d < NumDimensions; ++d)
        off = off * _local_layout.extent(d) + _coords[d];
    } else {
      off = _coords[NumDimensions - 1];
      for(auto d = NumDimensions - 2; d >= 0; --d)
        off = off * _local_layout.extent(d) + _coords[d];
    }
    _current_lmemory_addr = _local_memory + off;
  }

  std::array<pattern_index_t, NumDimensions> set_coords(
    pattern_index_t idx) const {
    if(Scope == StencilViewScope::BOUNDARY) {
      auto local_idx = idx;
      for(const auto& region : _bnd_elements) {
        if(local_idx < region.size()) {
          return _local_layout.coords(local_idx, region);
        }
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

  ElementT value_halo_at(ElementCoords_t halo_coords) {
    auto index =
      _haloblock.index_at(ViewSpec_t(_local_layout.extents()), halo_coords);
    _halomemory.to_halo_mem_coords(index, halo_coords);

    return *(_halomemory.pos_at(index)
             + _halomemory.value_at(index, halo_coords));
  }

  ElementT* halo_pos(const Stencil_t& stencil) {
    ElementT* halo_pos = _current_lmemory_addr;
    if(MemoryArrange == ROW_MAJOR) {
      halo_pos += stencil[NumDimensions - 1];
      for(auto d = NumDimensions - 2; d >= 0; --d)
        halo_pos += stencil[d] * _local_layout.extent(d);
    } else {
      halo_pos += stencil[0];
      for(auto d = 1; d < NumDimensions; ++d)
        halo_pos += stencil[d] * _local_layout.extent(d);
    }

    return halo_pos;
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
  const HaloBlock_t&                                  _haloblock;
  HaloMemory_t&                                       _halomemory;
  const StencilSpecT&                                 _stencil_spec;
  ElementT*                                           _local_memory;
  ViewSpec_t                                          _view_local;
  std::vector<ViewSpec_t>                             _bnd_elements;
  std::array<signed_pattern_size_t, NumStencilPoints> _stencil_offsets;
  const LocalLayout_t&                                _local_layout;
  pattern_index_t                                     _idx{ 0 };
  pattern_index_t                                     _size{ 0 };
  dart_unit_t                                         _myid;

  ElementCoords_t _coords;
  ElementT*       _current_lmemory_addr;
};  // class HaloMatrixIterator

}  // namespace dash

#endif  // DASH__HALO__ITERATOR__HALOMATRIXITERATOR_H

