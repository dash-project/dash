#ifndef DASH__EXPERIMENTAL__HALO_H__
#define DASH__EXPERIMENTAL__HALO_H__

#include <dash/memory/GlobStaticMem.h>
#include <dash/iterator/GlobIter.h>

#include <dash/internal/Logging.h>

#include <functional>

namespace dash {
namespace experimental {

//TODO move to util/FunctionalExpr.h
template<typename BaseT, typename ExpT>
constexpr BaseT pow(BaseT base, ExpT exp) {
  static_assert(std::is_integral<BaseT>::value, "Base must be an integer.");
  static_assert(std::is_integral<ExpT>::value && std::is_unsigned<ExpT>::value,
      "Exponent must be an unsigned integer.");

  return (exp == 0 ? 1 : base * pow(base, exp - 1));
}



template <dim_t NumDimensions>
class Stencil : public Dimensional<int16_t, NumDimensions> {

private:
  using BaseT = Dimensional<int16_t, NumDimensions>;

public:
  Stencil() {
    for (dim_t i(0); i < NumDimensions; ++i) {
      this->_values[i] = 0;
    }
  }

  template <typename... Values>
  Stencil(int16_t value, Values... values) : BaseT::Dimensional(value, values...) {
  }

  // TODO as constexpr
  int max() const {
    int16_t max = 0;
    for (dim_t i(0); i < NumDimensions; ++i)
      max = std::max((int) max, (int) std::abs(this->_values[i]));
    return max;
  }

private:
}; // Stencil



template <dim_t NumDimensions, std::size_t NumStencilPoints>
class StencilSpec {

private:
  using SelfT    = StencilSpec<NumDimensions, NumStencilPoints>;
  using StencilT = Stencil<NumDimensions>;
  using SpecsT   = std::array<StencilT, NumStencilPoints>;

public:
  constexpr StencilSpec(const SpecsT& specs) : _specs(specs) {}

  StencilSpec(const SelfT& other) { _specs = other._specs; }

  constexpr const SpecsT& stencilSpecs() const { return _specs; }

  static constexpr std::size_t numStencilPoints() { return NumStencilPoints; }

  constexpr const StencilT& operator[](std::size_t index) const { return _specs[index]; }

private:
  SpecsT _specs{};
}; // StencilSpec



/**
 * Specifies the values global boundary Halos
 */
enum class Cycle : uint8_t {
  /// No global boundary Halos
  NONE,
  /// Global boundary Halos with values from the opposite boundary
  CYCLIC,
  /// Global boundary Halos with predefined fixed values
  FIXED
};



/**
 * Cycle specification for every dimension
 */
template <dim_t NumDimensions>
class CycleSpec : public Dimensional<Cycle, NumDimensions> {

private:
  using BaseT = Dimensional<Cycle, NumDimensions>;

public:
  CycleSpec() {
    for (dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = Cycle::NONE;
    }
  }

  template <typename... Values>
  CycleSpec(Cycle value, Values... values) : BaseT::Dimensional(value, values...) {}
}; // CycleSpec



template <dim_t NumDimensions>
class RegionCoords : public Dimensional<uint8_t, NumDimensions> {

public:
  using SelfT   = RegionCoords<NumDimensions>;
  using coord_t = uint8_t;
  using region_index_t = uint32_t;
  using extent_t       = uint16_t;

private:
  using BaseT   = Dimensional<coord_t, NumDimensions>;
  using CoordsT = std::array<coord_t, NumDimensions>;

public:
  RegionCoords() {
    for (dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = 1;
    }
    _index = index(this->_values);
  }

  template <typename... Values>
  RegionCoords(coord_t value, Values... values) : BaseT::Dimensional(value, values...) {
    _index = index(this->_values);
  }

  RegionCoords(region_index_t index) : _index(index) {
    this->_values = coords(index);
  }

  constexpr region_index_t index() const { return _index; }

  static region_index_t index(const CoordsT& coords) {
    region_index_t index = coords[0];
    for (auto i = 1; i < NumDimensions; ++i)
      index = coords[i] + index * 3;

    return index;
  }

  static CoordsT coords(const region_index_t index) {
    CoordsT coords{};
    region_index_t  index_tmp = static_cast<long>(index);
    for (auto i = (NumDimensions - 1); i >= 1; --i) {
      auto res  = std::div(index_tmp, 3);
      coords[i] = res.rem;
      index_tmp = res.quot;
    }
    coords[0] = index_tmp;

    return coords;
  }

  constexpr bool operator==(const SelfT& other) const {
    return _index == other._index && this->_values == other._values;
  }

  constexpr bool operator!=(const SelfT& other) const {
    return !(*this == other);
  }

private:
  region_index_t _index;
}; // RegionCoords



template <dim_t NumDimensions>
class HaloRegionSpec : public Dimensional<uint8_t, NumDimensions> {

private:
  using self          = HaloRegionSpec<NumDimensions>;
  using udim_t        = std::make_unsigned<dim_t>::type;
  using RegionCoordsT = RegionCoords<NumDimensions>;
  using coord_t        = typename RegionCoordsT::coord_t;

public:
  using region_index_t = typename RegionCoordsT::region_index_t;
  using extent_t       = typename RegionCoordsT::extent_t;
  static constexpr region_index_t MaxIndex = pow(3, static_cast<udim_t>(NumDimensions));

public:

  HaloRegionSpec(const RegionCoordsT& coords, const extent_t extent)
      : _coords(coords), _extent(extent), _rel_dim(init_rel_dim()) {}

  HaloRegionSpec(region_index_t index, const extent_t extent)
      : _coords(RegionCoordsT(index)), _extent(extent), _rel_dim(init_rel_dim()) {}

  constexpr HaloRegionSpec() = default;


  template <typename StencilT>
  static region_index_t index(const StencilT& stencil) {
    region_index_t index = 0;
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

  constexpr region_index_t index() const { return _coords.index(); }

  constexpr const RegionCoordsT& coords() const { return _coords; }

  constexpr extent_t extent() const { return _extent; }

  constexpr coord_t operator[](const region_index_t index) const { return _coords[index]; }

  constexpr bool operator==(const self& other) const {
    return _coords.index() == other._coords.index() && _extent == other._extent;
  }

  constexpr bool operator!=(const self& other) const {
    return !(*this == other);
  }

  // returns the highest dimension with changed coordinates
  dim_t relevantDim() const { return _rel_dim; }

private:
  dim_t init_rel_dim() {
    dim_t dim = 1;
    for (auto d = 0; d < NumDimensions; ++d) {
      if (_coords[d] != 1)
        dim = d + 1;
    }

    return dim;
  }

private:
  RegionCoordsT _coords{};
  extent_t      _extent{0};
  dim_t         _rel_dim = 1;
}; // HaloRegionSpec

template <dim_t NumDimensions>
std::ostream& operator<<(std::ostream& os, const HaloRegionSpec<NumDimensions>& hrs) {
  os << "dash::HaloRegionSpec<" << NumDimensions << ">(" << (uint32_t)hrs[0];
  for (auto i = 1; i < NumDimensions; ++i)
    os << "," << (uint32_t)hrs[i];
  os << "), Extent:" << hrs.extent();

  return os;
}



template<dim_t NumDimensions>
class HaloSpec {
public:
  using self            = HaloSpec<NumDimensions>;
  using HaloRegionSpecT = HaloRegionSpec<NumDimensions>;
  using region_index_t  = typename HaloRegionSpecT::region_index_t;
  using extent_t        = typename HaloRegionSpecT::extent_t;
  using SpecsT          = std::array<HaloRegionSpecT, HaloRegionSpecT::MaxIndex>;
  using StencilT        = Stencil<NumDimensions>;

public:

  constexpr HaloSpec(const SpecsT& specs) : _specs(specs) {}

  template<typename StencilSpecT>
  HaloSpec(const StencilSpecT& stencil_specs) {
    for(const auto& stencil : stencil_specs.stencilSpecs()) {
      auto stencil_combination = stencil;

      setRegionSpec(stencil_combination);
      while (nextRegion(stencil, stencil_combination)){
        setRegionSpec(stencil_combination);
      }
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

  constexpr HaloRegionSpecT spec(const region_index_t index) const {
    return _specs[index];
  }

  constexpr extent_t extent(const region_index_t index) const {
    return _specs[index].extent();
  }

  constexpr std::size_t numRegions() const{
    return _num_regions;
  }

  const SpecsT& haloSpecs() const{
    return _specs;
  }

private:
  void setRegionSpec(const StencilT& stencil) {
        auto index = HaloRegionSpecT::index(stencil);
        auto max = stencil.max();
        if(_specs[index].extent() == 0)
          ++_num_regions;
        if(max >_specs[index].extent())
          _specs[index] = HaloRegionSpecT(index,max);
  }

  bool nextRegion(const StencilT& stencil, StencilT& stencil_combination) {
    for(auto d = 0; d <NumDimensions; ++d) {
      if(stencil[d] == 0)
        continue;
      stencil_combination[d] = (stencil_combination[d] == 0) ? stencil[d] : 0;
      if(stencil_combination[d] == 0) {
        return true;
      }
    }

    return false;
  }

private:
  SpecsT _specs{};
  std::size_t _num_regions{0};
}; // HaloSpec

template <typename ElementT, typename PatternT, typename PointerT = GlobPtr<ElementT, PatternT>,
          typename ReferenceT = GlobRef<ElementT>>
class RegionIter : public std::iterator<std::random_access_iterator_tag, ElementT,
                                        typename PatternT::index_type, PointerT, ReferenceT> {
private:
  using SelfT     = RegionIter<ElementT, PatternT, PointerT, ReferenceT>;
  using GlobMemT  = GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using ViewSpecT = typename PatternT::viewspec_type;

  static const dim_t NumDimensions = PatternT::ndim();

public:
  using index_type = typename PatternT::index_type;
  using size_type  = typename PatternT::size_type;

  using reference       = ReferenceT;
  using const_reference = const reference;
  using pointer         = PointerT;
  using const_pointer   = const pointer;

public:
  /**
   * Constructor, creates a block boundary iterator on multiple boundary
   * regions.
   */
  RegionIter(GlobMemT& globmem, const PatternT& pattern, const ViewSpecT& _region_view,
             index_type pos, index_type size)
      : _globmem(globmem), _pattern(pattern), _region_view(_region_view), _idx(pos), _size(size),
        _max_idx(_size - 1), _myid(dash::myid()), _lbegin(globmem.lbegin()) {}

  /**
   * Copy constructor.
   */
  RegionIter(const SelfT& other) = default;

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
  reference operator*() const { return operator[](_idx); }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  reference operator[](index_type idx) const {
    auto coords    = glob_coords(idx);
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

  ElementT* local() const {
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

  std::array<index_type, NumDimensions> gcoords() const { return glob_coords(_idx); }

  typename PatternT::local_index_t lpos() const { return _pattern.local_index(glob_coords(_idx)); }

  const ViewSpecT viewspec() const { return _region_view; }

  inline bool is_relative() const noexcept { return true; }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  const GlobMemT& globmem() const { return _globmem; }

  /**
   * Prefix increment operator.
   */
  SelfT& operator++() {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  SelfT operator++(int) {
    SelfT result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  SelfT& operator--() {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  SelfT operator--(int) {
    SelfT result = *this;
    --_idx;
    return result;
  }

  SelfT& operator+=(index_type n) {
    _idx += n;
    return *this;
  }

  SelfT& operator-=(index_type n) {
    _idx -= n;
    return *this;
  }

  SelfT operator+(index_type n) const {
    SelfT res{*this};
    res += n;

    return res;
  }

  index_type operator+(const SelfT& other) const { return _idx + other._idx; }

  SelfT operator-(index_type n) const {
    SelfT res{*this};
    res -= n;

    return res;
  }

  index_type operator-(const SelfT& other) const { return _idx - other._idx; }

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

  const PatternT& pattern() const { return _pattern; }

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
    if (&_region_view == &(other._region_view) || _region_view == other._region_view) {
      return gidx_cmp(_idx, other._idx);
    }
    // TODO not the best solution
    return false;
  }

  std::array<index_type, NumDimensions> glob_coords(index_type idx) const {
    return _pattern.memory_layout().coords(idx, _region_view);
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMemT& _globmem;
  /// Pattern that created the encapsulated block.
  const PatternT& _pattern;

  const ViewSpecT _region_view;
  /// Iterator's position relative to the block border's iteration space.
  index_type _idx{0};
  /// Number of elements in the block border's iteration space.
  index_type _size{0};
  /// Maximum iterator position in the block border's iteration space.
  index_type _max_idx{0};
  /// Unit id of the active unit
  dart_unit_t _myid;

  ElementT* _lbegin;

}; // class HaloBlockIter

template <typename ElementT, typename PatternT, typename PointerT, typename ReferenceT>
std::ostream& operator<<(std::ostream& os,
                         const RegionIter<ElementT, PatternT, PointerT, ReferenceT>& i) {
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
    const RegionIter<ElementT, PatternT, PointerT, ReferenceT>& first,
    /// Global iterator to the final position in the global sequence
    const RegionIter<ElementT, PatternT, PointerT, ReferenceT>& last) ->
    typename PatternT::index_type {
  return last - first;
}

template <typename ElementT, typename PatternT, dim_t NumDimensions>
class Region {

private:
  using GlobMemT        = GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using HaloRegionSpecT = HaloRegionSpec<NumDimensions>;
  using ViewSpecT       = typename PatternT::viewspec_type;
  using region_index_t  = typename HaloRegionSpecT::region_index_t;
  using size_type       = typename PatternT::size_type;
  using BorderT         = std::array<bool,NumDimensions>;

public:
  using iterator        = RegionIter<ElementT, PatternT>;
  using const_iterator  = const iterator;

public:
  Region(const HaloRegionSpecT& region_spec, const ViewSpecT& region, GlobMemT& globmem,
         const PatternT& pattern, const BorderT& border)
      : _region_spec(region_spec), _region(region), _border(border),
        _beg(globmem, pattern, _region, 0, _region.size()),
        _end(globmem, pattern, _region, _region.size(), _region.size()) {}

  const region_index_t index() const { return _region_spec.index(); }

  const HaloRegionSpecT& regionSpec() const { return _region_spec; }

  const ViewSpecT& region() const { return _region; }

  constexpr size_type size() const { return _region.size(); }

  constexpr BorderT border() const { return _border; }

  constexpr bool borderDim(dim_t dim) const { return _border[dim]; }

  iterator begin() const { return _beg; }

  iterator end() const { return _end; }

private:
  const HaloRegionSpecT _region_spec;
  const ViewSpecT       _region;
  BorderT               _border;
  iterator              _beg;
  iterator              _end;
}; // Region

template <typename ElementT, typename PatternT>
class HaloBlock {
private:
  static const dim_t NumDimensions = PatternT::ndim();

  using SelfT           = HaloBlock<ElementT, PatternT>;
  using GlobMemT        = GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using CycleSpecT      = CycleSpec<NumDimensions>;
  using index_type      = typename PatternT::index_type;
  using ViewSpecT       = typename PatternT::viewspec_type;
  using halo_reg_spec_t = HaloSpec<NumDimensions>;
  using HaloRegionSpecT = HaloRegionSpec<NumDimensions>;
  using region_t        = Region<ElementT, PatternT, NumDimensions>;
  using regions_t       = std::vector<region_t>;
  using halo_extent_t   = typename halo_reg_spec_t::extent_t;
  using HaloExtsMaxT    = std::array<std::pair<halo_extent_t, halo_extent_t>, NumDimensions>;
  using size_type       = typename PatternT::size_type;

public:
  using region_index_t  = typename HaloRegionSpecT::region_index_t;
  using value_t         = ElementT;

public:
  HaloBlock(GlobMemT& globmem, const PatternT& pattern, const ViewSpecT& view,
            const halo_reg_spec_t& halo_reg_spec, const CycleSpecT& cycle_spec = CycleSpecT{})
      : _globmem(globmem), _pattern(pattern), _view(view), _halo_reg_spec(halo_reg_spec) {

    _view_inner = view;
    _view_safe  = view;

    // TODO put functionallity to HaloSpec
    HaloExtsMaxT halo_extents_max{};
    _halo_regions.reserve(_halo_reg_spec.numRegions());
    _boundary_regions.reserve(_halo_reg_spec.numRegions());
    for (const auto& spec : _halo_reg_spec.haloSpecs()) {
      auto halo_extent = spec.extent();
      if (!halo_extent)
        continue;

      std::array<bool,NumDimensions> border{};

      auto halo_region_offsets = view.offsets();
      auto halo_region_extents = view.extents();
      auto bnd_region_offsets  = view.offsets();
      auto bnd_region_extents  = view.extents();

      for (auto d(0); d < NumDimensions; ++d) {
        if (spec[d] == 1)
          continue;

        auto view_offset = view.offset(d);
        auto view_extent = view.extent(d);

        if (spec[d] < 1) {
          halo_extents_max[d].first = std::max(halo_extents_max[d].first, halo_extent);
          if (view_offset < halo_extents_max[d].first) {
            border[d] = true;

            if (cycle_spec[d] == Cycle::NONE) {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = 0;
              bnd_region_offsets[d]  = 0;
              bnd_region_extents[d]  = 0;
            } else {
              halo_region_offsets[d] = _pattern.extent(d) - halo_extent;
              halo_region_extents[d] = halo_extent;
              bnd_region_extents[d]  = halo_extent;
            }

          } else {
            halo_region_offsets[d] -= halo_extent;
            halo_region_extents[d] = halo_extent;
            bnd_region_extents[d]  = halo_extent;
          }
          continue;
        } else {
          halo_extents_max[d].second = std::max(halo_extents_max[d].second, halo_extent);
          auto check_extent          = view_offset + view_extent + halo_extents_max[d].second;
          if (check_extent > _pattern.extent(d)) {
            border[d] = true;

            if (cycle_spec[d] == Cycle::NONE) {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = 0;
              bnd_region_offsets[d]  = 0;
              bnd_region_extents[d]  = 0;
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
      _halo_regions.push_back(region_t(spec, ViewSpecT(halo_region_offsets, halo_region_extents),
                                       _globmem, _pattern, border));
      _halo_reg_mapping[index] = &_halo_regions.back();
      _boundary_regions.push_back(region_t(spec, ViewSpecT(bnd_region_offsets, bnd_region_extents),
                                           _globmem, _pattern, border));
      _boundary_reg_mapping[index] = &_boundary_regions.back();
    }

    for (auto d = 0; d < NumDimensions; ++d) {
      const auto view_offset = view.offset(d);
      const auto view_extent = view.extent(d);

      auto bnd_elem_offsets = _view.offsets();
      auto bnd_elem_extents = _view.extents();
      bnd_elem_extents[d]   = halo_extents_max[d].first;
      for (auto d_tmp = 0; d_tmp < d; ++d_tmp) {
        bnd_elem_offsets[d_tmp] += halo_extents_max[d_tmp].first;
        bnd_elem_extents[d_tmp] -= halo_extents_max[d_tmp].first + halo_extents_max[d_tmp].second;
      }

      _view_inner.resize_dim(d, view_offset + halo_extents_max[d].first,
                             view_extent - halo_extents_max[d].first - halo_extents_max[d].second);
      if (cycle_spec[d] == Cycle::NONE) {
        auto safe_offset = view_offset;
        auto safe_extent = view_extent;
        if (view_offset < halo_extents_max[d].first) {
          safe_offset = halo_extents_max[d].first;
          safe_extent -= halo_extents_max[d].first - view_offset;
        } else {
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
  HaloBlock(const SelfT& other) = default;

  /**
   * Assignment operator.
   */
  SelfT& operator=(const SelfT& other) = default;

  static constexpr dim_t ndim() { return NumDimensions; }

  /**
   * The pattern instance that created the encapsulated block.
   */
  const PatternT& pattern() const { return _pattern; }

  const GlobMemT& globmem() const { return _globmem; }

  const region_t* halo_region(const region_index_t index) const { return _halo_reg_mapping[index]; }

  const regions_t& halo_regions() const { return _halo_regions; }

  const region_t* boundary_region(const region_index_t index) const {
    return _boundary_reg_mapping[index];
  }

  const regions_t& boundary_regions() const { return _boundary_regions; }

  // const halo_regs_t& halo_regions() const { return _halo_regions; }

  const ViewSpecT& view() const { return _view; }

  const ViewSpecT& view_safe() const { return _view_safe; }

  const ViewSpecT& view_inner() const { return _view_inner; }

  const std::vector<ViewSpecT>& boundary_elements() const { return _boundary_elements; }

  size_type halo_size() const {
    return std::accumulate(_halo_regions.begin(), _halo_regions.end(), 0,
                           [](size_type sum, region_t region) { return sum + region.size(); });
  }

  size_type boundary_size() const { return _size_bnd_elems; }

private:
  void pushBndElems(dim_t dim, std::array<index_type, NumDimensions>& offsets,
                    std::array<size_type, NumDimensions>& extents,
                    const HaloExtsMaxT& halo_exts_max, const CycleSpecT& cycle_spec) {
    for (auto d_tmp = dim + 1; d_tmp < NumDimensions; ++d_tmp) {
      if (cycle_spec[d_tmp] == Cycle::NONE) {
        if (offsets[d_tmp] < halo_exts_max[d_tmp].first) {
          offsets[d_tmp] = halo_exts_max[d_tmp].first;
          extents[d_tmp] -= halo_exts_max[d_tmp].first;
          if((offsets[d_tmp] + extents[d_tmp] + halo_exts_max[d_tmp].second) > _pattern.extent(d_tmp))
            extents[d_tmp] -= halo_exts_max[d_tmp].second;
          continue;
        }
        auto check_extent_tmp = offsets[d_tmp] + extents[d_tmp] + halo_exts_max[d_tmp].second;
        if (check_extent_tmp > _pattern.extent(d_tmp)) {
          extents[d_tmp] -= halo_exts_max[d_tmp].second;
        }
      }
    }
    ViewSpecT boundary_next(offsets, extents);
    _size_bnd_elems += boundary_next.size();
    _boundary_elements.push_back(std::move(boundary_next));
  }

private:
  GlobMemT& _globmem;

  const PatternT& _pattern;

  const ViewSpecT& _view;

  const halo_reg_spec_t& _halo_reg_spec;

  ViewSpecT _view_safe;

  ViewSpecT _view_inner;

  regions_t _halo_regions;

  std::array<region_t*, HaloRegionSpecT::MaxIndex> _halo_reg_mapping{};

  regions_t _boundary_regions;

  std::array<region_t*, HaloRegionSpecT::MaxIndex> _boundary_reg_mapping{};

  std::vector<ViewSpecT> _boundary_elements;

  size_type _size_bnd_elems = 0;

}; // class HaloBlock

template <typename HaloBlockT>
class HaloMemory {
private:
  static constexpr dim_t NumDimensions = HaloBlockT::ndim();
  using HaloRegionSpecT                = HaloRegionSpec<NumDimensions>;
  using region_index_t                 = typename HaloRegionSpecT::region_index_t;
  static constexpr region_index_t MaxIndex = HaloRegionSpecT::MaxIndex;

public:
  using ElementT                        = typename HaloBlockT::value_t;

  HaloMemory(const HaloBlockT& haloblock) {
    _halobuffer.resize(haloblock.halo_size());
    auto* offset = _halobuffer.data();

    for (const auto& region : haloblock.halo_regions()) {
      _halo_offsets[region.index()] = offset;
      offset += region.size();
    }
  }

  ElementT* haloPos(region_index_t index) { return _halo_offsets[index]; }

  ElementT* startPos() { return _halobuffer.data(); }

  const std::vector<ElementT>& haloBuffer() { return _halobuffer; }

private:
  std::vector<ElementT>           _halobuffer;
  std::array<ElementT*, MaxIndex> _halo_offsets{};
}; // class HaloMemory

} // namespace dash
} // namespace experimental

#endif // DASH__EXPERIMENTAL__HALO_H__
