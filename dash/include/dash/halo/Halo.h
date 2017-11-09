#ifndef DASH__HALO__HALO_H__
#define DASH__HALO__HALO_H__

#include <dash/iterator/GlobIter.h>
#include <dash/memory/GlobStaticMem.h>

#include <dash/internal/Logging.h>

#include <functional>

namespace dash {

template <dim_t NumDimensions>
class Stencil : public Dimensional<int16_t, NumDimensions> {
private:
  using Base_t = Dimensional<int16_t, NumDimensions>;

public:
  // TODO constexpr
  Stencil() {
    for(dim_t i(0); i < NumDimensions; ++i) {
      this->_values[i] = 0;
    }
  }

  template <typename... Values>
  constexpr Stencil(int16_t value, Values... values)
  : Base_t::Dimensional(value, values...) {}

  // TODO as constexpr
  int max() const {
    int16_t max = 0;
    for(dim_t i(0); i < NumDimensions; ++i)
      max = std::max((int) max, (int) std::abs(this->_values[i]));
    return max;
  }
};  // Stencil

template <dim_t NumDimensions, std::size_t NumStencilPoints>
class StencilSpec {
private:
  using Self_t    = StencilSpec<NumDimensions, NumStencilPoints>;
  using Stencil_t = Stencil<NumDimensions>;
  using Specs_t   = std::array<Stencil_t, NumStencilPoints>;

public:
  using stencil_size_t = std::size_t;

public:
  constexpr StencilSpec(const Specs_t& specs) : _specs(specs) {}

  // TODO constexpr
  StencilSpec(const Self_t& other) { _specs = other._specs; }

  constexpr const Specs_t& specs() const { return _specs; }

  static constexpr stencil_size_t num_stencil_points() {
    return NumStencilPoints;
  }

  constexpr const Stencil_t& operator[](std::size_t index) const {
    return _specs[index];
  }

private:
  Specs_t _specs{};
};  // StencilSpec

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
  using Base_t = Dimensional<Cycle, NumDimensions>;

public:
  // TODO constexpr
  CycleSpec() {
    for(dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = Cycle::NONE;
    }
  }

  template <typename... Values>
  constexpr CycleSpec(Cycle value, Values... values)
  : Base_t::Dimensional(value, values...) {}
};  // CycleSpec

template <dim_t NumDimensions>
class RegionCoords : public Dimensional<uint8_t, NumDimensions> {
private:
  using Self_t   = RegionCoords<NumDimensions>;
  using Base_t   = Dimensional<uint8_t, NumDimensions>;
  using Coords_t = std::array<uint8_t, NumDimensions>;
  using udim_t   = std::make_unsigned<dim_t>::type;

public:
  using region_coord_t = uint8_t;
  using region_index_t = uint32_t;
  using region_size_t  = uint32_t;

  // index calculation base 3^N regions for N-Dimensions
  static constexpr uint8_t REGION_INDEX_BASE = 3;

  // maximal number of regions
  static constexpr auto MaxIndex =
    ce::pow(REGION_INDEX_BASE, static_cast<udim_t>(NumDimensions));

public:
  RegionCoords() {
    for(dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = 1;
    }
    _index = index(this->_values);
  }

  template <typename... Values>
  RegionCoords(region_coord_t value, Values... values)
  : Base_t::Dimensional(value, values...) {
    _index = index(this->_values);
  }

  RegionCoords(region_index_t index) : _index(index) {
    this->_values = coords(index);
  }

  constexpr region_index_t index() const { return _index; }

  static region_index_t index(const Coords_t& coords) {
    region_index_t index = coords[0];
    for(auto i = 1; i < NumDimensions; ++i)
      index = coords[i] + index * REGION_INDEX_BASE;

    return index;
  }

  static Coords_t coords(const region_index_t index) {
    Coords_t       coords{};
    region_index_t index_tmp = static_cast<long>(index);
    for(auto i = (NumDimensions - 1); i >= 1; --i) {
      auto res  = std::div(index_tmp, REGION_INDEX_BASE);
      coords[i] = res.rem;
      index_tmp = res.quot;
    }
    coords[0] = index_tmp;

    return coords;
  }

  constexpr bool operator==(const Self_t& other) const {
    return _index == other._index && this->_values == other._values;
  }

  constexpr bool operator!=(const Self_t& other) const {
    return !(*this == other);
  }

private:
  region_index_t _index;
};  // RegionCoords

template <dim_t NumDimensions>
class RegionSpec : public Dimensional<uint8_t, NumDimensions> {
private:
  using Self_t         = RegionSpec<NumDimensions>;
  using RegionCoords_t = RegionCoords<NumDimensions>;
  using region_coord_t = typename RegionCoords_t::region_coord_t;

public:
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using region_extent_t = uint16_t;

public:
  RegionSpec(const RegionCoords_t& coords, const region_extent_t extent)
  : _coords(coords), _extent(extent), _rel_dim(init_rel_dim()) {
    init_level();
  }

  RegionSpec(region_index_t index, const region_extent_t extent)
  : _coords(RegionCoords_t(index)), _extent(extent), _rel_dim(init_rel_dim()) {
    init_level();
  }

  constexpr RegionSpec() = default;

  template <typename StencilT>
  static region_index_t index(const StencilT& stencil) {
    region_index_t index = 0;
    if(stencil[0] == 0)
      index = 1;
    else if(stencil[0] > 0)
      index = 2;
    for(auto d(1); d < NumDimensions; ++d) {
      if(stencil[d] < 0)
        index *= RegionCoords_t::REGION_INDEX_BASE;
      else if(stencil[d] == 0)
        index = 1 + index * RegionCoords_t::REGION_INDEX_BASE;
      else
        index = 2 + index * RegionCoords_t::REGION_INDEX_BASE;
    }

    return index;
  }

  constexpr region_index_t index() const { return _coords.index(); }

  constexpr const RegionCoords_t& coords() const { return _coords; }

  constexpr region_extent_t extent() const { return _extent; }

  constexpr region_coord_t operator[](const region_index_t index) const {
    return _coords[index];
  }

  constexpr bool operator==(const Self_t& other) const {
    return _coords.index() == other._coords.index() && _extent == other._extent;
  }

  constexpr bool operator!=(const Self_t& other) const {
    return !(*this == other);
  }

  // returns the highest dimension with changed coordinates
  dim_t relevant_dim() const { return _rel_dim; }

  dim_t level() const { return _level; }

private:
  // TODO put init_rel_dim and level together
  dim_t init_rel_dim() {
    dim_t dim = 1;
    for(auto d = 0; d < NumDimensions; ++d) {
      if(_coords[d] != 1)
        dim = d + 1;
    }

    return dim;
  }

  void init_level() {
    for(auto d = 0; d < NumDimensions; ++d) {
      if(_coords[d] != 1)
        ++_level;
    }
  }

private:
  RegionCoords_t  _coords{};
  region_extent_t _extent  = 0;
  dim_t           _rel_dim = 1;
  dim_t           _level   = 0;
};  // RegionSpec

template <dim_t NumDimensions>
std::ostream& operator<<(std::ostream&                    os,
                         const RegionSpec<NumDimensions>& hrs) {
  os << "dash::RegionSpec<" << NumDimensions << ">(" << (uint32_t) hrs[0];
  for(auto i = 1; i < NumDimensions; ++i)
    os << "," << (uint32_t) hrs[i];
  os << "), Extent:" << hrs.extent();

  return os;
}

template <dim_t NumDimensions>
class HaloSpec {
private:
  using Self_t          = HaloSpec<NumDimensions>;
  using RegionCoords_t  = RegionCoords<NumDimensions>;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Specs_t         = std::array<RegionSpec_t, RegionCoords_t::MaxIndex>;
  using Stencil_t       = Stencil<NumDimensions>;
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using region_size_t   = typename RegionCoords_t::region_index_t;
  using region_extent_t = typename RegionSpec_t::region_extent_t;

public:
  constexpr HaloSpec(const Specs_t& specs) : _specs(specs) {}

  template <typename StencilSpecT>
  HaloSpec(const StencilSpecT& stencil_specs) {
    for(const auto& stencil : stencil_specs.specs()) {
      auto stencil_combination = stencil;

      set_region_spec(stencil_combination);
      while(next_region(stencil, stencil_combination)) {
        set_region_spec(stencil_combination);
      }
    }
  }

  template <typename... ARGS>
  HaloSpec(const ARGS&... args) {
    std::array<RegionSpec_t, sizeof...(ARGS)> tmp{ args... };
    for(auto& spec : tmp) {
      _specs[spec.index()] = spec;
      ++_num_regions;
    }
  }

  HaloSpec(const Self_t& other) { _specs = other._specs; }

  constexpr RegionSpec_t spec(const region_index_t index) const {
    return _specs[index];
  }

  constexpr region_extent_t extent(const region_index_t index) const {
    return _specs[index].extent();
  }

  constexpr region_size_t num_regions() const { return _num_regions; }

  const Specs_t& specs() const { return _specs; }

private:
  void set_region_spec(const Stencil_t& stencil) {
    auto index = RegionSpec_t::index(stencil);
    auto max   = stencil.max();

    if(_specs[index].extent() == 0)
      ++_num_regions;

    if(max > _specs[index].extent())
      _specs[index] = RegionSpec_t(index, max);
  }

  bool next_region(const Stencil_t& stencil, Stencil_t& stencil_combination) {
    for(auto d = 0; d < NumDimensions; ++d) {
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
  Specs_t       _specs{};
  region_size_t _num_regions{ 0 };
};  // HaloSpec

template <typename ElementT, typename PatternT,
          typename PointerT   = GlobPtr<ElementT, PatternT>,
          typename ReferenceT = GlobRef<ElementT>>
class RegionIter {
private:
  using Self_t = RegionIter<ElementT, PatternT, PointerT, ReferenceT>;
  using GlobMem_t =
    GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using ViewSpec_t      = typename PatternT::viewspec_type;
  using pattern_index_t = typename PatternT::index_type;
  using pattern_size_t  = typename PatternT::size_type;

  static const auto NumDimensions = PatternT::ndim();

public:
  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = ElementT;
  using difference_type   = typename PatternT::index_type;
  using pointer           = PointerT;
  using reference         = ReferenceT;

  using const_reference = const reference;
  using const_pointer   = const pointer;

public:
  /**
   * Constructor, creates a region iterator.
   */
  RegionIter(GlobMem_t& globmem, const PatternT& pattern,
             const ViewSpec_t& _region_view, pattern_index_t pos,
             pattern_size_t size)
  : _globmem(globmem), _pattern(pattern), _region_view(_region_view), _idx(pos),
    _max_idx(size - 1), _myid(pattern.team().myid()),
    _lbegin(globmem.lbegin()) {}

  /**
   * Copy constructor.
   */
  RegionIter(const Self_t& other) = default;

  /**
   * Move constructor
   */
  RegionIter(Self_t&& other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  Self_t& operator=(const Self_t& other) = default;

  /**
   * Move assignment operator
   */
  Self_t& operator=(Self_t&& other) = default;

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
  reference operator[](pattern_index_t n) const {
    auto coords    = glob_coords(_idx + n);
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

    if(_myid != local_pos.unit)
      return nullptr;

    //
    return _lbegin + local_pos.index;
  }

  /**
   * Position of the iterator in global storage order.
   *
   * \see DashGlobalIteratorConcept
   */
  pattern_index_t pos() const { return gpos(); }

  /**
   * Position of the iterator in its view's iteration space, disregarding
   * the view's offset in global index space.
   *
   * \see DashViewIteratorConcept
   */
  pattern_index_t rpos() const { return _idx; }

  /**
   * Position of the iterator in global index range.
   * Projects iterator position from its view spec to global index domain.
   *
   * \see DashGlobalIteratorConcept
   */
  pattern_index_t gpos() const {
    return _pattern.memory_layout().at(glob_coords(_idx));
  }

  std::array<pattern_index_t, NumDimensions> gcoords() const {
    return glob_coords(_idx);
  }

  typename PatternT::local_index_t lpos() const {
    return _pattern.local_index(glob_coords(_idx));
  }

  const ViewSpec_t viewspec() const { return _region_view; }

  inline bool is_relative() const noexcept { return true; }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  const GlobMem_t& globmem() const { return _globmem; }

  /**
   * Prefix increment operator.
   */
  Self_t& operator++() {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  Self_t operator++(int) {
    Self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  Self_t& operator--() {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  Self_t operator--(int) {
    Self_t result = *this;
    --_idx;
    return result;
  }

  Self_t& operator+=(pattern_index_t n) {
    _idx += n;
    return *this;
  }

  Self_t& operator-=(pattern_index_t n) {
    _idx -= n;
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

  const PatternT& pattern() const { return _pattern; }

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
    if(&_region_view == &(other._region_view)
       || _region_view == other._region_view) {
      return gidx_cmp(_idx, other._idx);
    }
    // TODO not the best solution
    return false;
  }

  std::array<pattern_index_t, NumDimensions> glob_coords(
    pattern_index_t idx) const {
    return _pattern.memory_layout().coords(idx, _region_view);
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem_t& _globmem;
  /// Pattern that created the encapsulated block.
  const PatternT& _pattern;

  const ViewSpec_t _region_view;
  /// Iterator's position relative to the block border's iteration space.
  pattern_index_t _idx{ 0 };
  /// Maximum iterator position in the block border's iteration space.
  pattern_index_t _max_idx{ 0 };
  /// Unit id of the active unit
  team_unit_t _myid;

  ElementT* _lbegin;

};  // class HaloBlockIter

template <typename ElementT, typename PatternT, typename PointerT,
          typename ReferenceT>
std::ostream& operator<<(
  std::ostream&                                               os,
  const RegionIter<ElementT, PatternT, PointerT, ReferenceT>& i) {
  std::ostringstream                ss;
  dash::GlobPtr<ElementT, PatternT> ptr(i);
  ss << "dash::HaloBlockIter<" << typeid(ElementT).name() << ">("
     << "idx:" << i._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

template <typename ElementT, typename PatternT, typename PointerT,
          typename ReferenceT>
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
  using GlobMem_t =
    GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using RegionSpec_t   = RegionSpec<NumDimensions>;
  using ViewSpec_t     = typename PatternT::viewspec_type;
  using region_index_t = typename RegionSpec_t::region_index_t;
  using pattern_size_t = typename PatternT::size_type;
  using Border_t       = std::array<bool, NumDimensions>;

public:
  using iterator       = RegionIter<ElementT, PatternT>;
  using const_iterator = const iterator;

public:
  Region(const RegionSpec_t& region_spec, const ViewSpec_t& region,
         GlobMem_t& globmem, const PatternT& pattern, const Border_t& border)
  : _region_spec(region_spec), _region(region), _border(border),
    _border_region(
      std::any_of(border.begin(), border.end(),
                  [](bool border_dim) { return border_dim == true; })),
    _beg(globmem, pattern, _region, 0, _region.size()),
    _end(globmem, pattern, _region, _region.size(), _region.size()) {}

  const region_index_t index() const { return _region_spec.index(); }

  const RegionSpec_t& spec() const { return _region_spec; }

  const ViewSpec_t& region() const { return _region; }

  constexpr pattern_size_t size() const { return _region.size(); }

  constexpr Border_t border() const { return _border; }

  bool is_border_region() const { return _border_region; };

  constexpr bool border_dim(dim_t dim) const { return _border[dim]; }

  iterator begin() const { return _beg; }

  iterator end() const { return _end; }

private:
  const RegionSpec_t _region_spec;
  const ViewSpec_t   _region;
  Border_t           _border;
  bool               _border_region;
  iterator           _beg;
  iterator           _end;
};  // Region

template <typename ElementT, typename PatternT>
class HaloBlock {
private:
  static constexpr auto NumDimensions = PatternT::ndim();

  using Self_t = HaloBlock<ElementT, PatternT>;
  using GlobMem_t =
    GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using CycleSpec_t     = CycleSpec<NumDimensions>;
  using pattern_index_t = typename PatternT::index_type;
  using pattern_size_t  = typename PatternT::size_type;
  using ViewSpec_t      = typename PatternT::viewspec_type;
  using HaloSpec_t      = HaloSpec<NumDimensions>;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Region_t        = Region<ElementT, PatternT, NumDimensions>;
  using RegionVector_t  = std::vector<Region_t>;
  using RegionCoords_t  = RegionCoords<NumDimensions>;
  using region_index_t  = typename RegionSpec_t::region_index_t;
  using region_extent_t = typename RegionSpec_t::region_extent_t;
  using HaloExtsMax_t =
    std::array<std::pair<region_extent_t, region_extent_t>, NumDimensions>;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;

public:
  using Element_t = ElementT;
  using Pattern_t = PatternT;

public:
  HaloBlock(GlobMem_t& globmem, const PatternT& pattern, const ViewSpec_t& view,
            const HaloSpec_t&  halo_reg_spec,
            const CycleSpec_t& cycle_spec = CycleSpec_t{})
  : _globmem(globmem), _pattern(pattern), _view(view),
    _halo_reg_spec(halo_reg_spec) {
    _view_inner = view;
    _view_safe  = view;

    // TODO put functionallity to HaloSpec
    HaloExtsMax_t halo_extents_max{};
    _halo_regions.reserve(_halo_reg_spec.num_regions());
    _boundary_regions.reserve(_halo_reg_spec.num_regions());
    for(const auto& spec : _halo_reg_spec.specs()) {
      auto halo_extent = spec.extent();
      if(!halo_extent)
        continue;

      std::array<bool, NumDimensions> border{};

      auto halo_region_offsets = view.offsets();
      auto halo_region_extents = view.extents();
      auto bnd_region_offsets  = view.offsets();
      auto bnd_region_extents  = view.extents();

      for(auto d(0); d < NumDimensions; ++d) {
        if(spec[d] == 1)
          continue;

        auto view_offset = view.offset(d);
        auto view_extent = view.extent(d);

        if(spec[d] < 1) {
          halo_extents_max[d].first =
            std::max(halo_extents_max[d].first, halo_extent);
          if(view_offset < halo_extents_max[d].first) {
            border[d] = true;

            if(cycle_spec[d] == Cycle::NONE) {
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
        } else {
          halo_extents_max[d].second =
            std::max(halo_extents_max[d].second, halo_extent);
          auto check_extent =
            view_offset + view_extent + halo_extents_max[d].second;
          if(check_extent > _pattern.extent(d)) {
            border[d] = true;

            if(cycle_spec[d] == Cycle::NONE) {
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
      _halo_regions.push_back(
        Region_t(spec, ViewSpec_t(halo_region_offsets, halo_region_extents),
                 _globmem, _pattern, border));
      auto& region_tmp = _halo_regions.back();
      _size_halo_elems += region_tmp.size();
      _halo_reg_mapping[index] = &region_tmp;
      _boundary_regions.push_back(
        Region_t(spec, ViewSpec_t(bnd_region_offsets, bnd_region_extents),
                 _globmem, _pattern, border));
      _boundary_reg_mapping[index] = &_boundary_regions.back();
    }

    for(auto d = 0; d < NumDimensions; ++d) {
      const auto view_offset = view.offset(d);
      const auto view_extent = view.extent(d);

      auto bnd_elem_offsets = _view.offsets();
      auto bnd_elem_extents = _view.extents();
      bnd_elem_extents[d]   = halo_extents_max[d].first;
      for(auto d_tmp = 0; d_tmp < d; ++d_tmp) {
        bnd_elem_offsets[d_tmp] += halo_extents_max[d_tmp].first;
        bnd_elem_extents[d_tmp] -=
          halo_extents_max[d_tmp].first + halo_extents_max[d_tmp].second;
      }

      _view_inner.resize_dim(
        d, view_offset + halo_extents_max[d].first,
        view_extent - halo_extents_max[d].first - halo_extents_max[d].second);
      if(cycle_spec[d] == Cycle::NONE) {
        auto safe_offset = view_offset;
        auto safe_extent = view_extent;
        if(view_offset < halo_extents_max[d].first) {
          safe_offset = halo_extents_max[d].first;
          safe_extent -= halo_extents_max[d].first - view_offset;
        } else {
          push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents,
                         halo_extents_max, cycle_spec);
        }
        auto check_extent =
          view_offset + view_extent + halo_extents_max[d].second;
        if(check_extent > _pattern.extent(d)) {
          safe_extent -= check_extent - _pattern.extent(d);
        } else {
          bnd_elem_offsets[d] += view_extent - halo_extents_max[d].first;
          bnd_elem_extents[d] = halo_extents_max[d].second;
          push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents,
                         halo_extents_max, cycle_spec);
        }
        _view_safe.resize_dim(d, safe_offset, safe_extent);
      } else {
        push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents, halo_extents_max,
                       cycle_spec);
        bnd_elem_offsets[d] += view_extent - halo_extents_max[d].first;
        bnd_elem_extents[d] = halo_extents_max[d].second;
        push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents, halo_extents_max,
                       cycle_spec);
      }
    }
  }

  HaloBlock() = delete;

  /**
   * Copy constructor.
   */
  HaloBlock(const Self_t& other) = default;

  /**
   * Assignment operator.
   */
  Self_t& operator=(const Self_t& other) = default;

  static constexpr dim_t ndim() { return NumDimensions; }

  /**
   * The pattern instance that created the encapsulated block.
   */
  const PatternT& pattern() const { return _pattern; }

  const GlobMem_t& globmem() const { return _globmem; }

  const Region_t* halo_region(const region_index_t index) const {
    return _halo_reg_mapping[index];
  }

  const RegionVector_t& halo_regions() const { return _halo_regions; }

  const Region_t* boundary_region(const region_index_t index) const {
    return _boundary_reg_mapping[index];
  }

  const RegionVector_t& boundary_regions() const { return _boundary_regions; }

  // const halo_regs_t& halo_regions() const { return _halo_regions; }

  const ViewSpec_t& view() const { return _view; }

  const ViewSpec_t& view_safe() const { return _view_safe; }

  const ViewSpec_t& view_inner() const { return _view_inner; }

  const std::vector<ViewSpec_t>& boundary_elements() const {
    return _boundary_elements;
  }

  pattern_size_t halo_size() const { return _size_halo_elems; }

  pattern_size_t boundary_size() const { return _size_bnd_elems; }

  region_index_t index_at(const ViewSpec_t&      view,
                          const ElementCoords_t& coords) const {
    using signed_extent_t = typename std::make_signed<pattern_size_t>::type;
    const auto& extents   = view.extents();
    const auto& offsets   = view.offsets();

    region_index_t index = 0;
    if(coords[0] >= offsets[0]
       && coords[0] < static_cast<signed_extent_t>(extents[0]))
      index = 1;
    else if(coords[0] >= static_cast<signed_extent_t>(extents[0]))
      index = 2;
    for(auto d = 1; d < NumDimensions; ++d) {
      if(coords[d] < offsets[d])
        index *= RegionCoords_t::REGION_INDEX_BASE;
      else if(coords[d] < static_cast<signed_extent_t>(extents[d]))
        index = 1 + index * RegionCoords_t::REGION_INDEX_BASE;
      else
        index = 2 + index * RegionCoords_t::REGION_INDEX_BASE;
    }

    return index;
  }

private:
  void push_bnd_elems(dim_t                                       dim,
                      std::array<pattern_index_t, NumDimensions>& offsets,
                      std::array<pattern_size_t, NumDimensions>&  extents,
                      const HaloExtsMax_t&                        halo_exts_max,
                      const CycleSpec_t&                          cycle_spec) {
    for(auto d_tmp = dim + 1; d_tmp < NumDimensions; ++d_tmp) {
      if(cycle_spec[d_tmp] == Cycle::NONE) {
        if(offsets[d_tmp] < halo_exts_max[d_tmp].first) {
          offsets[d_tmp] = halo_exts_max[d_tmp].first;
          extents[d_tmp] -= halo_exts_max[d_tmp].first;
          if((offsets[d_tmp] + extents[d_tmp] + halo_exts_max[d_tmp].second)
             > _pattern.extent(d_tmp))
            extents[d_tmp] -= halo_exts_max[d_tmp].second;
          continue;
        }
        auto check_extent_tmp =
          offsets[d_tmp] + extents[d_tmp] + halo_exts_max[d_tmp].second;
        if(check_extent_tmp > _pattern.extent(d_tmp)) {
          extents[d_tmp] -= halo_exts_max[d_tmp].second;
        }
      }
    }
    ViewSpec_t boundary_next(offsets, extents);
    _size_bnd_elems += boundary_next.size();
    _boundary_elements.push_back(std::move(boundary_next));
  }

private:
  GlobMem_t& _globmem;

  const PatternT& _pattern;

  const ViewSpec_t& _view;

  const HaloSpec_t& _halo_reg_spec;

  ViewSpec_t _view_safe;

  ViewSpec_t _view_inner;

  RegionVector_t _halo_regions;

  std::array<Region_t*, RegionCoords_t::MaxIndex> _halo_reg_mapping{};

  RegionVector_t _boundary_regions;

  std::array<Region_t*, RegionCoords_t::MaxIndex> _boundary_reg_mapping{};

  std::vector<ViewSpec_t> _boundary_elements;

  pattern_size_t _size_bnd_elems = 0;

  pattern_size_t _size_halo_elems = 0;

};  // class HaloBlock

template <typename HaloBlockT>
class HaloMemory {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();
  using RegionCoords_t                = RegionCoords<NumDimensions>;
  using region_index_t                = typename RegionCoords_t::region_index_t;
  using Pattern_t                     = typename HaloBlockT::Pattern_t;
  using pattern_index_t               = typename Pattern_t::index_type;
  using pattern_size_t                = typename Pattern_t::size_type;
  using Element_t                     = typename HaloBlockT::Element_t;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;

  static constexpr auto MaxIndex      = RegionCoords_t::MaxIndex;
  static constexpr auto MemoryArrange = Pattern_t::memory_order();

public:
  HaloMemory(const HaloBlockT& haloblock) : _haloblock(haloblock) {
    _halobuffer.resize(haloblock.halo_size());
    auto* offset = _halobuffer.data();

    for(const auto& region : haloblock.halo_regions()) {
      _halo_offsets[region.index()] = offset;
      offset += region.size();
    }
  }

  Element_t* pos_at(region_index_t index) { return _halo_offsets[index]; }

  Element_t* pos_start() { return _halobuffer.data(); }

  const std::vector<Element_t>& buffer() const { return _halobuffer; }

  bool to_halo_mem_coords_check(const region_index_t region_index,
                                ElementCoords_t&     coords) {
    const auto& extents =
      _haloblock.halo_region(region_index)->region().extents();
    for(auto d = 0; d < NumDimensions; ++d) {
      if(coords[d] < 0)
        coords[d] += extents[d];
      else if(coords[d] >= _haloblock.view().extent(d))
        coords[d] -= _haloblock.view().extent(d);

      if(coords[d] >= extents[d] || coords[d] < 0)
        return false;
    }

    return true;
  }

  void to_halo_mem_coords(const region_index_t region_index,
                          ElementCoords_t&     coords) {
    const auto& extents =
      _haloblock.halo_region(region_index)->region().extents();
    for(auto d = 0; d < NumDimensions; ++d) {
      if(coords[d] < 0) {
        coords[d] += extents[d];
        continue;
      }

      if(coords[d] >= _haloblock.view().extent(d))
        coords[d] -= _haloblock.view().extent(d);
    }
  }

  pattern_size_t value_at(const region_index_t   region_index,
                          const ElementCoords_t& coords) {
    const auto& extents =
      _haloblock.halo_region(region_index)->region().extents();
    pattern_size_t off = 0;
    if(MemoryArrange == ROW_MAJOR) {
      off = coords[0];
      for(auto d = 1; d < NumDimensions; ++d)
        off = off * extents[d] + coords[d];
    } else {
      off = coords[NumDimensions - 1];
      for(auto d = NumDimensions - 2; d >= 0; --d)
        off = off * extents[d] + coords[d];
    }

    return off;
  }

private:
  const HaloBlockT&                _haloblock;
  std::vector<Element_t>           _halobuffer;
  std::array<Element_t*, MaxIndex> _halo_offsets{};
};  // class HaloMemory

}  // namespace dash

#endif  // DASH__HALO_HALO_H__
