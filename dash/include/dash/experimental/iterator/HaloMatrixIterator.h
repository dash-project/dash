#ifndef DASH__EXPERIMENTAL__HALOMATRIXITERATOR_H__INCLUDED
#define DASH__EXPERIMENTAL__HALOMATRIXITERATOR_H__INCLUDED

#include <dash/Types.h>

#include <dash/experimental/Halo.h>

#include <vector>

namespace dash {
namespace experimental {

enum class StencilViewScope : std::uint8_t { INNER, BOUNDARY, ALL };

template <typename ElementT, typename PatternT, typename StencilSpecT, StencilViewScope Scope>
class HaloMatrixIterator {
public:
  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = ElementT;
  using difference_type   = typename PatternT::index_type;
  using pointer           = ElementT*;
  using reference         = ElementT&;

  using index_type = typename PatternT::index_type;
  using size_type  = typename PatternT::size_type;

private:
  static constexpr dim_t       NumDimensions    = PatternT::ndim();
  static constexpr std::size_t NumStencilPoints = StencilSpecT::numStencilPoints();
  static constexpr MemArrange  MemoryArrange    = PatternT::memory_order();

  using SelfT            = HaloMatrixIterator<ElementT, PatternT, StencilSpecT, Scope>;
  using HaloBlock_t      = HaloBlock<ElementT, PatternT>;
  using HaloMemory_t     = HaloMemory<HaloBlock_t>;
  using viewspec_t       = typename PatternT::viewspec_type;
  using position_t       = int16_t;
  using region_index_t   = typename HaloBlock_t::region_index_t;
  using local_layout_t   = CartesianIndexSpace<NumDimensions, MemoryArrange, index_type>;
  using StencilT         = Stencil<NumDimensions>;
  using CoordsT          = std::array<index_type, NumDimensions>;
  using signed_size_type = typename std::make_signed<size_type>::type;

public:
  HaloMatrixIterator(const HaloBlock_t& haloblock, HaloMemory_t& halomemory,
                     const StencilSpecT& stencil_spec, index_type idx)
      : _haloblock(haloblock), _halomemory(halomemory), _stencil_spec(stencil_spec),
        _local_memory((ElementT*)_haloblock.globmem().lbegin()),
        _local_layout(_haloblock.pattern().local_memory_layout()), _idx(idx)

  {
    if (Scope == StencilViewScope::INNER)
      setViewLocal(_haloblock.view_inner());

    if (Scope == StencilViewScope::ALL)
      setViewLocal(_haloblock.view_safe());

    if (Scope == StencilViewScope::BOUNDARY)
      setViewLocal(_haloblock.view());

    if (Scope == StencilViewScope::BOUNDARY)
      _size = _haloblock.boundary_size();
    else
      _size = _view_local.size();

    setCoords();
    setStencilOffsets(stencil_spec);

  }

  /**
   * Copy constructor.
   */
  HaloMatrixIterator(const SelfT& other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  SelfT& operator=(const SelfT& other) = default;

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
  reference operator[](index_type idx) const {
    auto coords = setCoords(idx);
    return _local_memory[_local_layout.at(coords)];
  }

  index_type rpos() const { return _idx; }

  index_type lpos() const { return _local_layout.at(_coords); }

  const CoordsT&  coords() const { return _coords; };

  bool isHaloValue(const region_index_t index_stencil) {

    if (Scope == StencilViewScope::INNER)
      return false;

    auto        halo_coords{_coords};
    const auto& stencil = _stencil_spec[index_stencil];
    for (auto d = 0; d < NumDimensions; ++d) {
      halo_coords[d] += stencil[d];
      if (halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
        return true;
    }

    return false;
  }

  std::vector<ElementT> haloValues() {
    // TODO: is the given offset in halospec range?
    std::vector<ElementT> halos;
    if (Scope == StencilViewScope::INNER)
      return halos;

    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto        halo_coords{_coords};
      const auto& stencil = _stencil_spec[i];
      bool        halo    = false;
      for (auto d = 0; d < NumDimensions; ++d) {
        halo_coords[d] += stencil[d];
        if (halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
          halo = true;
      }
    // TODO check wether region is nullptr or not
    // TODO implement as method in HaloRegionSpec
      if (halo)
        halos.push_back(valueHaloAt(halo_coords));
    }
    return halos;
  }

  ElementT valueAt(const region_index_t index_stencil) {
    // TODO: is the given offset in halospec range?

    if (Scope == StencilViewScope::INNER)
      return *(_current_lmemory_addr + _stencil_offsets[index_stencil]);

    auto        halo_coords{_coords};
    const auto& stencil = _stencil_spec[index_stencil];
    bool        halo    = false;
    for (auto d = 0; d < NumDimensions; ++d) {
      halo_coords[d] += stencil[d];
      if (halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
        halo = true;
    }
    // TODO check wether region is nullptr or not
    // TODO implement as method in HaloRegionSpec
    if (halo)
      return valueHaloAt(halo_coords);

    return *(_current_lmemory_addr + _stencil_offsets[index_stencil]);
  }

  ElementT valueAt(const StencilT& stencil) {
    // TODO: is the given offset in halospec range?

    if (Scope == StencilViewScope::INNER) {
      return *haloPos(stencil);
    } else {
      auto halo_coords{_coords};

      bool halo = false;
      for (auto d = 0; d < NumDimensions; ++d) {
        halo_coords[d] += stencil[d];
        if (halo_coords[d] < 0 || halo_coords[d] >= _haloblock.view().extent(d))
          halo = true;
      }
      // TODO check wether region is nullptr or not
      // TODO implement as method in HaloRegionSpec
      if (halo)
        return valueHaloAt(halo_coords);

      return *haloPos(stencil);
    }
  }

  /**
   * Prefix increment operator.
   */
  SelfT& operator++() {
    ++_idx;
    setCoords();

    return *this;
  }

  /**
   * Postfix increment operator.
   */
  SelfT operator++(int) {
    SelfT result = *this;
    ++_idx;
    setCoords();

    return result;
  }

  /**
   * Prefix decrement operator.
   */
  SelfT& operator--() {
    --_idx;
    setCoords();

    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  SelfT operator--(int) {
    SelfT result = *this;
    --_idx;
    setCoords();

    return result;
  }

  SelfT& operator+=(index_type n) {
    _idx += n;
    setCoords();

    return *this;
  }

  SelfT& operator-=(index_type n) {
    _idx -= n;
    setCoords();

    return *this;
  }

  SelfT operator+(index_type n) const {
    SelfT res{*this};
    res += n;

    return res;
  }

  SelfT operator-(index_type n) const {
    SelfT res{*this};
    res -= n;

    return res;
  }

  /*index_type operator+(
    const SelfT & other) const
  {
    return _idx + other._idx;
  }

  index_type operator-(
    const SelfT & other) const
  {
    return _idx - other._idx;
  }*/

  bool operator<(const SelfT& other) const { return compare(other, std::less<index_type>()); }

  bool operator<=(const SelfT& other) const {
    return compare(other, std::less_equal<index_type>());
  }

  bool operator>(const SelfT& other) const { return compare(other, std::greater<index_type>()); }

  bool operator>=(const SelfT& other) const {
    return compare(other, std::greater_equal<index_type>());
  }

  bool operator==(const SelfT& other) const { return compare(other, std::equal_to<index_type>()); }

  bool operator!=(const SelfT& other) const {
    return compare(other, std::not_equal_to<index_type>());
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template <typename GlobIndexCmpFunc>
  bool compare(const SelfT& other, const GlobIndexCmpFunc& gidx_cmp) const {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if (this == &other) {
      return true;
    }
#endif
    if (&_view_local == &(other._view_local) || _view_local == other._view_local) {
      return gidx_cmp(_idx, other._idx);
    }
    // TODO not the best solution
    return false;
  }

  void setViewLocal(const viewspec_t& view_tmp) {
    if (Scope == StencilViewScope::BOUNDARY) {
      const auto& bnd_elems = _haloblock.boundary_elements();
      _bnd_elements.reserve(bnd_elems.size());
      const auto& view_offs = view_tmp.offsets();
      for (const auto& region : bnd_elems) {
        auto off = region.offsets();
        for (int d = 0; d < NumDimensions; ++d)
          off[d] -= view_offs[d];

        _bnd_elements.push_back(viewspec_t(off, region.extents()));
      }

      _view_local = viewspec_t(view_tmp.extents());
    } else {
      const auto& view_offsets = _haloblock.view().offsets();
      auto        off          = view_tmp.offsets();
      for (int d = 0; d < NumDimensions; ++d)
        off[d] -= view_offsets[d];

      _view_local = viewspec_t(off, view_tmp.extents());
    }
  }

  void setCoords() {
    _coords       = setCoords(_idx);
    size_type off = 0;
    if (MemoryArrange == ROW_MAJOR) {
      off = _coords[0];
      for (auto d = 1; d < NumDimensions; ++d)
        off = off * _local_layout.extent(d) + _coords[d];
    } else {
      off = _coords[NumDimensions - 1];
      for (auto d = NumDimensions - 2; d >= 0; --d)
        off = off * _local_layout.extent(d) + _coords[d];
    }
    _current_lmemory_addr = _local_memory + off;
  }

  std::array<index_type, NumDimensions> setCoords(index_type idx) const {
    if (Scope == StencilViewScope::BOUNDARY) {
      auto local_idx = idx;
      for (const auto& region : _bnd_elements) {
        if (local_idx < region.size()) {
          return _local_layout.coords(local_idx, region);
        }
        local_idx -= region.size();
      }
      // TODO return value for idx >= size
      DASH_ASSERT("idx >= size not implemented yet");
      return std::array<index_type, NumDimensions>{};
    } else {
      if (_view_local.size() == 0)
        return std::array<index_type, NumDimensions>{};
      else
        return _local_layout.coords(idx, _view_local);
    }
  }

  ElementT valueHaloAt(CoordsT halo_coords) {

    auto index = _haloblock.indexAt(viewspec_t(_local_layout.extents()), halo_coords);
    _halomemory.toHaloMemoryCoords(index, halo_coords);

    return *(_halomemory.haloPos(index) + _halomemory.haloValueAt(index, halo_coords));
  }

  ElementT* haloPos(const StencilT& stencil) {
    ElementT* halo_pos = _current_lmemory_addr;
    if (MemoryArrange == ROW_MAJOR) {
      halo_pos += stencil[NumDimensions - 1];
      for (auto d = NumDimensions - 2; d >= 0; --d)
        halo_pos += stencil[d] * _local_layout.extent(d);
    } else {
      halo_pos += stencil[0];
      for (auto d = 1; d < NumDimensions; ++d)
        halo_pos += stencil[d] * _local_layout.extent(d);
    }

    return halo_pos;
  }

  void setStencilOffsets(const StencilSpecT& stencil_spec) {

    for (auto i = 0; i < NumStencilPoints; ++i) {
      signed_size_type offset = 0;
      if (MemoryArrange == ROW_MAJOR) {
        offset = stencil_spec[i][0];
        for (auto d = 1; d < NumDimensions; ++d)
          offset = stencil_spec[i][d] + offset * _local_layout.extent(d);
      } else {
        offset = stencil_spec[i][NumDimensions - 1];
        for (auto d = NumDimensions - 2; d >= 0; --d)
          offset = stencil_spec[i][d] + offset * _local_layout.extent(d);
      }
      _stencil_offsets[i] = offset;
    }
  }

private:
  const HaloBlock_t&      _haloblock;
  HaloMemory_t&           _halomemory;
  const StencilSpecT&     _stencil_spec;
  ElementT*               _local_memory;
  viewspec_t              _view_local;
  std::vector<viewspec_t> _bnd_elements;
  std::array<signed_size_type, NumStencilPoints> _stencil_offsets;
  const local_layout_t& _local_layout;
  index_type            _idx{0};
  index_type            _size{0};
  dart_unit_t           _myid;

  CoordsT   _coords;
  ElementT* _current_lmemory_addr;
}; // class HaloMatrixIterator

} // namespace experimental
} // namespace dash

#endif // DASH__EXPERIMENTAL__HALOMATRIXITERATOR_H__INCLUDED

