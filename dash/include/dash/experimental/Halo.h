#ifndef DASH__EXPERIMENTAL__HALO_H__
#define DASH__EXPERIMENTAL__HALO_H__

#include <dash/memory/GlobStaticMem.h>
#include <dash/iterator/GlobIter.h>

#include <dash/internal/Logging.h>

#include <functional>

namespace dash {
namespace experimental {

//TODO move to util/FunctionalExpr.h
template<typename T>
constexpr T pow(T base, T exp) {
  return (exp == 0 ? 1 : base * pow(base, exp - 1));
}

template<dim_t NumDimensions>
class Stencil  : public Dimensional<int, NumDimensions> {
private:
  using base_t = Dimensional<int, NumDimensions>;

public:
  Stencil() {
    for (dim_t i(0); i < NumDimensions; ++i) {
      this->_values[i] = 0;
    }
  }

  template <typename... Values>
  Stencil(int value, Values... values)
  : base_t::Dimensional(value, values...)
  {
    for (dim_t i(0); i < NumDimensions; ++i)
      _max = std::max(_max, std::abs(this->_values[i]));
  }

  //TODO as constexpr
  constexpr int max() const {
    return _max;
  }

private:
  int _max{0};
};

template<dim_t NumDimensions, std::size_t NumStencilPoints>
class StencilSpec {
public:
  using self = StencilSpec<NumDimensions, NumStencilPoints>;
  using StencilT = Stencil<NumDimensions>;

  using SpecsT  = std::array<StencilT, NumStencilPoints>;

  static constexpr std::size_t numStencilPoints() { return NumStencilPoints; }
public:
  constexpr StencilSpec(const SpecsT& specs) : _specs(specs) {}

  StencilSpec(const self& other){
    _specs = other._specs;
  }

  constexpr const SpecsT& stencilSpecs() const {
    return _specs;
  }

  constexpr const StencilT& operator[](std::size_t index) const {
    return _specs[index];
  }

private:
  SpecsT _specs{};
};


enum class Cycle : bool{
  NONCYCLIC,
  CYCLIC
};

template <dim_t NumDimensions>
class CycleSpec : public Dimensional<Cycle, NumDimensions> {
private:
  using base_t = Dimensional<Cycle, NumDimensions>;

public:
  CycleSpec() {
    for (dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = Cycle::NONCYCLIC;
    }
    coords[0] = index_tmp;

    return coords;
  }

  template <typename... Values>
  CycleSpec(Cycle value, Values... values)
  : base_t::Dimensional(value, values...)
  {
  }
};

template<dim_t NumDimensions>
class HaloRegionSpec{
private:
  using self = HaloRegionSpec<NumDimensions>;

public:
  using index_t  = uint32_t;
  using extent_t = uint16_t;
  using coord_t  = uint8_t;
  using coords_t = std::array<coord_t, NumDimensions>;

  static constexpr index_t MaxIndex = pow(3, NumDimensions);

  constexpr HaloRegionSpec(const coords_t& coords, const extent_t extent)
      : _coords(coords), _index(index(coords)), _extent(extent) {}

  constexpr HaloRegionSpec(index_t index, const extent_t extent)
      : _index(index), _coords(coords(index)), _extent(extent) {}

  constexpr HaloRegionSpec() {}

  static index_t index(const coords_t& coords){
    index_t index = coords[0];
    for(auto i(1); i < NumDimensions; ++i)
      index = coords[i] + index * 3;

    return index;
  }

  static coords_t coords(const index_t index){
    coords_t coords{};
    index_t index_tmp = static_cast<long>(index);
    for(auto i(NumDimensions - 1); i >= 1; --i)
    {
      auto res = std::div(index_tmp, 3);
      coords[i] = res.rem;
      index_tmp = res.quot;
    }
    coords[0] = index_tmp;

    return coords;
  }

  template<typename StencilT>
  static index_t index(const StencilT& stencil) {
    index_t index = 0;
    if (stencil[0] == 0)
      index = 1;
    else if (stencil[0] > 0)
      index = 2;
    for (auto d(1); d < NumDimensions; ++d) {
      if (stencil[d] < 0)
        index *= 3;
      else if (stencil[d] == 0)
        index = 1 + index * 3;
      else
        index = 2 + index * 3;
    }

    return index;
  }

  constexpr index_t index() const { return _index; }

  constexpr const coords_t& coords() const {
    return _coords;
  }

  constexpr extent_t extent() const { return _extent; }

  constexpr coord_t operator[](const index_t index) const {
    return _coords[index];
  }

  constexpr bool operator==(const self& other) const {
    return _coords == other._coords;
  }

  constexpr bool operator!=(const self& other) const {
    return _coords != other._coords;
  }

private:
  coords_t _coords{};
  index_t  _index{0};
  extent_t _extent{0};
};

template <dim_t NumDimensions>
std::ostream &operator<<(std::ostream &os,
                         const HaloRegionSpec<NumDimensions>& coords) {
  os << "dash::HaloRegionSpec<" << NumDimensions << ">(" << (uint32_t)coords[0];
  for(auto i(1); i < NumDimensions; ++i)
     os << "," << (uint32_t)coords[i];
  os << "), Extent:" << coords.extent();

  return os;
}


template<dim_t NumDimensions>
class HaloSpec {
public:
  using self = HaloSpec<NumDimensions>;
  using HaloRegionSpecT = HaloRegionSpec<NumDimensions>;
  using index_t = typename HaloRegionSpecT::index_t;
  using extent_t = typename HaloRegionSpecT::extent_t;
  using specs_t  = std::array<HaloRegionSpecT, HaloRegionSpecT::MaxIndex>;

public:

  constexpr HaloSpec(const specs_t& specs) : _specs(specs) {}

  template<typename StencilSpecT>
  constexpr HaloSpec(const StencilSpecT& stencil_specs) {
    for(const auto& stencil : stencil_specs.stencilSpecs()) {
      auto index = HaloRegionSpecT::index(stencil);
      auto max = stencil.max();
      if(_specs[index].extent() == 0)
        ++_num_regions;
      if(max >_specs[index].extent())
        _specs[index] = HaloRegionSpecT(index,max);
    }
  }


  template<typename... ARGS>
  HaloSpec(const ARGS&... args) {
    std::vector<HaloRegionSpecT> tmp{args...};
    for(auto& spec : tmp) {
      _specs[spec.index()] = spec;
      ++_num_regions;
    }
  }

  HaloSpec(const self& other){
    _specs = other._specs;
  }

  constexpr HaloRegionSpecT spec(const index_t index) const {
    return _specs[index];
  }

  constexpr extent_t extent(const index_t index) const {
    return _specs[index].extent();
  }

  constexpr std::size_t numRegions() const{
    return _num_regions;
  }

  const specs_t& haloSpecs() const{
    return _specs;
  }

private:
  specs_t _specs{};
  std::size_t _num_regions{0};
};

template <typename ElementT, typename PatternT, typename PointerT = GlobPtr<ElementT, PatternT>,
          typename ReferenceT = GlobRef<ElementT>>
class RegionIter : public std::iterator<std::random_access_iterator_tag, ElementT,
                                           typename PatternT::index_type, PointerT, ReferenceT> {
private:
  using self_t    = RegionIter<ElementT, PatternT, PointerT, ReferenceT>;
  using GlobMem_t = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;

  static const dim_t NumDimensions = PatternT::ndim();

public:
  using index_type = typename PatternT::index_type;
  using size_type  = typename PatternT::size_type;
  using viewspec_t = typename PatternT::viewspec_type;

  using reference       = ReferenceT;
  using const_reference = const reference;
  using pointer         = PointerT;
  using const_pointer   = const pointer;

public:
  /**
   * Constructor, creates a block boundary iterator on multiple boundary
   * regions.
   */
  RegionIter(GlobMem_t &globmem, const PatternT &pattern, const viewspec_t &halo_region,
                index_type pos, index_type size)
      : _globmem(globmem), _pattern(pattern), _halo_region(halo_region), _idx(pos), _size(size),
        _max_idx(_size - 1), _myid(dash::myid()), _lbegin(globmem.lbegin())
  {
  }

  /**
   * Copy constructor.
   */
  RegionIter(const self_t &other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t &operator=(const self_t &other) = default;

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
  reference operator*() const { return operator[](_idx); }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  reference operator[](index_type idx) const {
    auto coords = glob_coords(idx);
    auto local_pos = _pattern.local_index(coords);

    return reference(_globmem.at(local_pos.unit, local_pos.index));
  }

  dart_gptr_t dart_gptr() const { return operator[](_idx).dart_gptr(); }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  bool is_local() const { return (_myid == lpos().unit); }

  GlobIter<ElementT, PatternT> global() const {
    auto g_idx = gpos();
    return GlobIter<ElementT, PatternT>(&_globmem, _pattern, g_idx);
  }

  ElementT *local() const {
    auto local_pos = lpos();

    if (_myid != local_pos.unit)
      return nullptr;

    //
    return _lbegin + local_pos.index;
  }

  /**
   * Position of the iterator in global storage order.
   *
   * \see DashGlobalIteratorConcept
   */
  index_type pos() const { return gpos(); }

  /**
   * Position of the iterator in its view's iteration space, disregarding
   * the view's offset in global index space.
   *
   * \see DashViewIteratorConcept
   */
  index_type rpos() const { return _idx; }

  /**
   * Position of the iterator in global index range.
   * Projects iterator position from its view spec to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  index_type gpos() const { return _pattern.memory_layout().at(glob_coords(_idx)); }

  typename PatternT::local_index_t lpos() const { return _pattern.local_index(glob_coords(_idx)); }

  const viewspec_t viewspec() const { return _halo_region; }

  inline bool is_relative() const noexcept { return true; }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  const GlobMem_t &globmem() const { return _globmem; }

  /**
   * Prefix increment operator.
   */
  self_t &operator++() {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int) {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t &operator--() {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int) {
    self_t result = *this;
    --_idx;
    return result;
  }

  self_t &operator+=(index_type n) {
    _idx += n;
    return *this;
  }

  self_t &operator-=(index_type n) {
    _idx -= n;
    return *this;
  }

  self_t operator+(index_type n) const {
    self_t res{*this};
    res += n;

    return res;
  }

  index_type operator+(const self_t &other) const { return _idx + other._idx; }

  self_t operator-(index_type n) const {
    self_t res{*this};
    res -= n;

    return res;
  }

  index_type operator-(const self_t &other) const { return _idx - other._idx; }

  bool operator<(const self_t &other) const { return compare(other, std::less<index_type>()); }

  bool operator<=(const self_t &other) const {
    return compare(other, std::less_equal<index_type>());
  }

  bool operator>(const self_t &other) const { return compare(other, std::greater<index_type>()); }

  bool operator>=(const self_t &other) const {
    return compare(other, std::greater_equal<index_type>());
  }

  bool operator==(const self_t &other) const { return compare(other, std::equal_to<index_type>()); }

  bool operator!=(const self_t &other) const {
    return compare(other, std::not_equal_to<index_type>());
  }

  const PatternT &pattern() const { return _pattern; }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template <typename GlobIndexCmpFunc>
  bool compare(const self_t &other, const GlobIndexCmpFunc &gidx_cmp) const {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if (this == &other) {
      return true;
    }
#endif
    if (&_halo_region == &(other._halo_region) || _halo_region == other._halo_region) {
      return gidx_cmp(_idx, other._idx);
    }
    // TODO not the best solution
    return false;
  }

  std::array<index_type, NumDimensions> glob_coords(index_type idx) const {
    return _pattern.memory_layout().coords(idx, _halo_region);
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem_t &_globmem;
  /// Pattern that created the encapsulated block.
  const PatternT &_pattern;

  const viewspec_t _halo_region;
  /// Iterator's position relative to the block border's iteration space.
  index_type _idx{0};
  /// Number of elements in the block border's iteration space.
  index_type _size{0};
  /// Maximum iterator position in the block border's iteration space.
  index_type _max_idx{0};
  /// Unit id of the active unit
  dart_unit_t _myid;

  ElementT *_lbegin;
  /// Function implementing mapping of iterator position to global element
  /// coordinates.
  // position_mapping_function          _position_to_coords;

}; // class HaloBlockIter

template <typename ElementT, typename PatternT, typename PointerT, typename ReferenceT>
std::ostream &operator<<(std::ostream &os,
                         const RegionIter<ElementT, PatternT, PointerT, ReferenceT> &i) {
  std::ostringstream ss;
  dash::GlobPtr<ElementT, PatternT> ptr(i);
  ss << "dash::HaloBlockIter<" << typeid(ElementT).name() << ">("
     << "idx:" << i._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

template <typename ElementT, typename PatternT, typename PointerT, typename ReferenceT>
auto distance(
    /// Global iterator to the initial position in the global sequence
    const RegionIter<ElementT, PatternT, PointerT, ReferenceT> &first,
    /// Global iterator to the final position in the global sequence
    const RegionIter<ElementT, PatternT, PointerT, ReferenceT> &last) ->
    typename PatternT::index_type {
  return last - first;
}

template <typename ElementT, typename PatternT, dim_t NumDimensions>
class Region {

public:
  using GlobMem_t    = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;
  using HaloRegionSpecT       = HaloRegionSpec<NumDimensions>;
  using index_t      = typename HaloRegionSpecT::index_t;
  using viewspec_t   = typename PatternT::viewspec_type;
  using size_type = typename PatternT::size_type;
  using iterator       = RegionIter<ElementT, PatternT>;
  using const_iterator       = const iterator;

public:
  Region(const HaloRegionSpecT& region_spec, const viewspec_t& region, GlobMem_t& globmem, const PatternT& pattern)
      : _region_spec(region_spec), _region(region), _beg(globmem, pattern, _region, 0, _region.size()),
        _end(globmem, pattern, _region, _region.size(), _region.size()) {}

  const index_t index() const { return _region_spec.index(); }

  const HaloRegionSpecT& regionSpec() const { return _region_spec; }

  const viewspec_t& region() const { return _region; }

  constexpr size_type size() const { return _region.size(); }

  iterator begin() const {
    return _beg;
  }

  iterator end() const {
    return _end;
  }

private:
  const HaloRegionSpecT _region_spec;
  const viewspec_t      _region;
  iterator              _beg;
  iterator              _end;
};

template <typename ElementT, typename PatternT>
class HaloBlock {
public:
  static constexpr dim_t NumDimensions = PatternT::ndim();
  using index_t = typename HaloRegionSpec<NumDimensions>::index_t;
  using value_t      = ElementT;

private:
  using self_t    = HaloBlock<ElementT, PatternT>;
  using GlobMem_t = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;
  using CycleSpecT   = CycleSpec<NumDimensions>;
  using index_type   = typename PatternT::index_type;
  using size_type    = typename PatternT::size_type;
  using viewspec_t   = typename PatternT::viewspec_type;
  using halo_reg_spec_t   = HaloSpec<NumDimensions>;
  using HaloRegionSpecT = HaloRegionSpec<NumDimensions>;
  using region_t = Region<ElementT, PatternT, NumDimensions>;
  using regions_t = std::vector<region_t>;
  using halo_extent_t = typename halo_reg_spec_t::extent_t;
  using HaloExtsMaxT = std::array<std::pair<halo_extent_t,halo_extent_t>, NumDimensions>;

public:
  HaloBlock(GlobMem_t& globmem, const PatternT& pattern, const viewspec_t& view,
            const halo_reg_spec_t& halo_reg_spec, const CycleSpecT& cycle_spec = CycleSpecT{})
      : _globmem(globmem), _pattern(pattern), _view(view), _halo_reg_spec(halo_reg_spec) {

    _view_inner = view;
    _view_safe  = view;

    //TODO put functionallity to HaloSpec
    HaloExtsMaxT halo_extents_max{};
    _halo_regions.reserve(_halo_reg_spec.numRegions());
    _boundary_regions.reserve(_halo_reg_spec.numRegions());
    for (const auto& spec : _halo_reg_spec.haloSpecs()) {
      auto halo_extent = spec.extent();
      if (!halo_extent)
        continue;

      auto halo_region_offsets = view.offsets();
      auto halo_region_extents = view.extents();
      auto bnd_region_offsets  = view.offsets();
      auto bnd_region_extents  = view.extents();

      for (auto d(0); d < NumDimensions; ++d) {
        if (spec[d] == 1)
          continue;

        auto view_offset    = view.offset(d);
        auto view_extent    = view.extent(d);

        if (spec[d] < 1) {
          halo_extents_max[d].first = std::max(halo_extents_max[d].first, halo_extent);
          if (view_offset < halo_extents_max[d].first) {
            if (cycle_spec[d] == Cycle::NONCYCLIC) {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = 0;
              bnd_region_offsets[d] = 0;
              bnd_region_extents[d] = 0;
            } else {
              halo_region_offsets[d] = _pattern.extent(d) - halo_extent;
              halo_region_extents[d] = halo_extent;
              bnd_region_extents[d] = halo_extent;
            }

          } else {
            halo_region_offsets[d] -= halo_extent;
            halo_region_extents[d] = halo_extent;
            bnd_region_extents[d] = halo_extent;
          }
          continue;
        } else {
          halo_extents_max[d].second = std::max(halo_extents_max[d].second, halo_extent);
          auto check_extent = view_offset + view_extent + halo_extents_max[d].second;
          if (check_extent > _pattern.extent(d)) {
            if (cycle_spec[d] == Cycle::NONCYCLIC) {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = 0;
              bnd_region_offsets[d] = 0;
              bnd_region_extents[d] = 0;
            } else {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = halo_extent;
              bnd_region_offsets[d] += view_extent - halo_extent;
              bnd_region_extents[d] = halo_extent;
            }
          } else {
            halo_region_offsets[d] += halo_region_extents[d];
            halo_region_extents[d] = halo_extent;
            bnd_region_offsets[d] += view_extent - halo_extent;
            bnd_region_extents[d] = halo_extent;
          }
        }
      }
      auto index = spec.index();
      _halo_regions.push_back(
          region_t(spec, viewspec_t(halo_region_offsets, halo_region_extents), _globmem, _pattern));
      _halo_reg_mapping[index] = &_halo_regions.back();
      _boundary_regions.push_back(
          region_t(spec, viewspec_t(bnd_region_offsets, bnd_region_extents), _globmem, _pattern));
      _boundary_reg_mapping[index] = &_boundary_regions.back();
    }

    for (auto d(0); d < NumDimensions; ++d) {
      const auto view_offset = view.offset(d);
      const auto view_extent = view.extent(d);

      auto bnd_elem_offsets = _view.offsets();
      auto bnd_elem_extents = _view.extents();
      bnd_elem_extents[d] = halo_extents_max[d].first;
      for (auto d_tmp(0); d_tmp < d; ++d_tmp) {
        bnd_elem_offsets[d_tmp] += halo_extents_max[d_tmp].first;
        bnd_elem_extents[d_tmp] -=
            halo_extents_max[d_tmp].first + halo_extents_max[d_tmp].second;
      }

      _view_inner.resize_dim(d, view_offset + halo_extents_max[d].first,
                             view_extent - halo_extents_max[d].first - halo_extents_max[d].second);
      if (cycle_spec[d] == Cycle::NONCYCLIC) {
        auto safe_offset = view_offset;
        auto safe_extent = view_extent;
        if (view_offset < halo_extents_max[d].first) {
          safe_offset = halo_extents_max[d].first;
          safe_extent -= halo_extents_max[d].first - view_offset;
        }
        else {
          pushBndElems(d, bnd_elem_offsets, bnd_elem_extents, halo_extents_max, cycle_spec);
        }
        auto check_extent = view_offset + view_extent + halo_extents_max[d].second;
        if (check_extent > _pattern.extent(d)) {
          safe_extent -= check_extent - _pattern.extent(d);
        } else {
          bnd_elem_offsets[d] += view_extent - halo_extents_max[d].first;
          bnd_elem_extents[d] = halo_extents_max[d].second;
          pushBndElems(d, bnd_elem_offsets, bnd_elem_extents, halo_extents_max, cycle_spec);
        }
        _view_safe.resize_dim(d, safe_offset, safe_extent);
      } else {
        pushBndElems(d, bnd_elem_offsets, bnd_elem_extents, halo_extents_max, cycle_spec);
        bnd_elem_offsets[d] += view_extent - halo_extents_max[d].first;
        bnd_elem_extents[d] = halo_extents_max[d].second;
        pushBndElems(d, bnd_elem_offsets, bnd_elem_extents, halo_extents_max, cycle_spec);
      }
    }
  }

  HaloBlock() = delete;

  /**
   * Copy constructor.
   */
  HaloBlock(const self_t &other) = default;

  /**
   * Assignment operator.
   */
  self_t &operator=(const self_t &other) = default;

  static constexpr dim_t ndim() { return NumDimensions; }

  /**
   * The pattern instance that created the encapsulated block.
   */
  const PatternT &pattern() const { return _pattern; }

  const GlobMem_t &globmem() const { return _globmem; }

  const region_t* halo_region(const index_t index) const { return _halo_reg_mapping[index]; }

  const regions_t& halo_regions() const { return _halo_regions; }

  const region_t* boundary_region(const index_t index) const { return _boundary_reg_mapping[index]; }

  const regions_t& boundary_regions() const { return _boundary_regions; }

  //const halo_regs_t& halo_regions() const { return _halo_regions; }

  const viewspec_t& view() const { return _view; }

  const viewspec_t& view_safe() const { return _view_safe; }

  const viewspec_t& view_inner() const { return _view_inner; }

  const std::vector<viewspec_t>& boundary_elements() const { return _boundary_elements; }

  size_type halo_size() const {
    return std::accumulate(
        _halo_regions.begin(), _halo_regions.end(), 0,
        [](size_type sum, region_t region) { return sum + region.size(); });
  }

  size_type boundary_size() const { return _size_bnd_elems; }

  private:
  void pushBndElems(dim_t dim, std::array<index_type, NumDimensions>& offsets,
                    std::array<size_type, NumDimensions>& extents, const HaloExtsMaxT& halo_exts_max,
                    const CycleSpecT& cycle_spec) {
    for (auto d_tmp(dim + 1); d_tmp < NumDimensions; ++d_tmp) {
      if (cycle_spec[d_tmp] == Cycle::NONCYCLIC) {
        if (offsets[d_tmp] < halo_exts_max[d_tmp].first) {
          offsets[d_tmp] = halo_exts_max[d_tmp].first;
          extents[d_tmp] -= halo_exts_max[d_tmp].first;
          continue;
        }
        auto check_extent_tmp =
            offsets[d_tmp] + extents[d_tmp] + halo_exts_max[d_tmp].second;
        if (check_extent_tmp > _pattern.extent(d_tmp)) {
          extents[d_tmp] -= halo_exts_max[d_tmp].second;
        }
      }
    }
    viewspec_t boundary_next(offsets, extents);
    _size_bnd_elems += boundary_next.size();
    _boundary_elements.push_back(std::move(boundary_next));
  }

private:
  GlobMem_t &_globmem;

  const PatternT &_pattern;

  const viewspec_t &_view;

  const halo_reg_spec_t& _halo_reg_spec;

  viewspec_t _view_safe;

  viewspec_t _view_inner;

  regions_t _halo_regions;

  std::array<region_t*, HaloRegionSpecT::MaxIndex> _halo_reg_mapping{};

  regions_t _boundary_regions;

  std::array<region_t*, HaloRegionSpecT::MaxIndex> _boundary_reg_mapping{};

  std::vector<viewspec_t> _boundary_elements;

  size_type _size_bnd_elems = 0;

}; // class HaloBlock

template <typename HaloBlockT>
class HaloMemory {
public:
  static constexpr dim_t NumDimensions = HaloBlockT::ndim();
  using HaloRegionSpecT = HaloRegionSpec<NumDimensions>;
  using index_t = typename HaloRegionSpecT::index_t;
  static constexpr index_t MaxIndex = HaloRegionSpecT::MaxIndex;
  using value_t = typename HaloBlockT::value_t;

  HaloMemory(const HaloBlockT & haloblock) {
    _halobuffer.resize(haloblock.halo_size());
    value_t *offset = _halobuffer.data();

    for(const auto& region : haloblock.halo_regions()) {
      _halo_offsets[region.index()] = offset;
      offset += region.size();
    }
  }

  value_t* haloPos(index_t index) {
    return _halo_offsets[index];
  }

  value_t* startPos() { return _halobuffer.data(); }

  const std::vector<value_t>& haloBuffer() { return _halobuffer; }

private:
  std::vector<value_t>   _halobuffer;
  std::array<value_t*, MaxIndex> _halo_offsets{};
}; // class HaloMemory

} // namespace dash
} // namespace experimental

#endif // DASH__EXPERIMENTAL__HALO_H__
