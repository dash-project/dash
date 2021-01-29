#ifndef DASH__HALO__HALO_H__
#define DASH__HALO__HALO_H__

#include <dash/internal/Logging.h>


#include <dash/halo/Types.h>
#include <dash/halo/Region.h>
#include <dash/halo/Stencil.h>

#include <functional>

namespace dash {

namespace halo {

using namespace internal;

/**
 * Global boundary property specification for every dimension
 */
template <dim_t NumDimensions>
class GlobalBoundarySpec : public Dimensional<BoundaryProp, NumDimensions> {
private:
  using Base_t = Dimensional<BoundaryProp, NumDimensions>;

public:
  // TODO constexpr
  /**
   * Default constructor
   *
   * All \ref BoundaryProp = BoundaryProp::NONE
   */
  GlobalBoundarySpec() {
    for(dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = BoundaryProp::NONE;
    }
  }
  /**
   * Constructor to define custom \ref BoundaryProp values
   */
  template <typename... Values>
  constexpr GlobalBoundarySpec(BoundaryProp value, Values... values)
  : Base_t::Dimensional(value, values...) {}
};  // GlobalBoundarySpec

template <dim_t NumDimensions>
std::ostream& operator<<(std::ostream&                            os,
                         const GlobalBoundarySpec<NumDimensions>& spec) {
  os << "dash::halo::GlobalBoundarySpec<" << NumDimensions << ">"
     << "(";
  for(auto d = 0; d < NumDimensions; ++d) {
    if(d > 0) {
      os << ",";
    }
    os << spec[d];
  }
  os << ")";

  return os;
}



/**
 * Contains all specified Halo regions. HaloSpec can be build with
 * \ref StencilSpec.
 */
template <dim_t NumDimensions>
class HaloSpec {
private:
  using Self_t         = HaloSpec<NumDimensions>;
  using RegionCoords_t = RegionCoords<NumDimensions>;
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

public:
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Specs_t         = std::array<RegionSpec_t, RegionsMax>;
  using region_extent_t = typename RegionSpec_t::region_extent_t;
  using HaloExtsMaxPair_t = std::pair<region_extent_t, region_extent_t>;
  using HaloExtsMax_t     = std::array<HaloExtsMaxPair_t, NumDimensions>;

public:
  constexpr HaloSpec(const Specs_t& specs) : _specs(specs) {}

  template <typename StencilSpecT>
  HaloSpec(const StencilSpecT& stencil_spec) {
    init_region_specs();
    read_stencil_points(stencil_spec);
  }

  template <typename StencilSpecT, typename... Args>
  HaloSpec(const StencilSpecT& stencil_spec, const Args&... stencil_specs)
  : HaloSpec(stencil_specs...) {
    read_stencil_points(stencil_spec);
  }

  template <typename... ARGS>
  HaloSpec(const RegionSpec_t& region_spec, const ARGS&... args) {
    init_region_specs();
    std::array<RegionSpec_t, sizeof...(ARGS) + 1> tmp{ region_spec, args... };
    for(auto& spec : tmp) {
      auto& current_spec = _specs[spec.index()];
      if(current_spec.extent() == 0 && spec.extent() > 0) {
        ++_num_regions;
      }

      if(current_spec.extent() < spec.extent()) {
        current_spec = spec;
        set_max_halo_dist(current_spec.coords(), current_spec.extent());
      }
    }
  }

  HaloSpec(const Self_t& other) { _specs = other._specs; }

  static constexpr dim_t ndim() { return NumDimensions; }

  /**
   * Matching \ref RegionSpec for a given region index
   */
  constexpr RegionSpec_t spec(const region_index_t index) const {
    return _specs[index];
  }

  /**
   * Extent for a given region index
   */
  constexpr region_extent_t extent(const region_index_t index) const {
    return _specs[index].extent();
  }

  /**
   * Number of specified regions
   */
  constexpr region_size_t num_regions() const { return _num_regions; }

  /**
   * Used \ref RegionSpec instance
   */
  const Specs_t& specs() const { return _specs; }

  /**
   * Returns the maximal extension for a  specific dimension
   */
  const HaloExtsMaxPair_t& halo_extension_max(dim_t dim) const {
    return _halo_extents_max[dim];
  }

  /**
   * Returns the maximal halo extension for every dimension
   */
  const HaloExtsMax_t& halo_extension_max() const { return _halo_extents_max; }

private:
  void init_region_specs() {
    for(region_index_t r = 0; r < RegionsMax; ++r) {
      _specs[r] = RegionSpec_t(r, 0);
    }
  }

  /*
   * Reads all stencil points of the given stencil spec and sets the region
   * specification.
   */
  template <typename StencilSpecT>
  void read_stencil_points(const StencilSpecT& stencil_spec) {
    for(const auto& stencil : stencil_spec.specs()) {
      auto stencil_combination = stencil;

      set_region_spec(stencil_combination);
      while(next_region(stencil, stencil_combination)) {
        set_region_spec(stencil_combination);
      }
    }
  }

  /*
   * Sets the region extent dependent on the given stencil point
   */
  template <typename StencilPointT>
  void set_region_spec(const StencilPointT& stencil) {
    auto index = RegionSpec_t::index(stencil);

    auto max = stencil.max();
    auto reg_extent = _specs[index].extent();
    if(reg_extent == 0 && max > 0) {
      ++_num_regions;
    }

    if(max > reg_extent) {
      _specs[index] = RegionSpec_t(index, max);
      set_max_halo_dist(_specs[index].coords(), max);
    }
  }

  /*
   * Makes sure, that all necessary regions are covered for a stencil point.
   * E.g. 2-D stencil point (-1,-1) needs not only region 0, it needs also
   * region 1 when the stencil is shifted to the right.
   */
  template <typename StencilPointT>
  bool next_region(const StencilPointT& stencil,
                   StencilPointT&       stencil_combination) {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(stencil[d] == 0)
        continue;
      stencil_combination[d] = (stencil_combination[d] == 0) ? stencil[d] : 0;
      if(stencil_combination[d] == 0) {
        return true;
      }
    }

    return false;
  }

  void set_max_halo_dist(RegionCoords_t reg_coords, region_extent_t extent) {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(reg_coords[d] == 1) {
        continue;
      }

      if(reg_coords[d] < 1) {
        _halo_extents_max[d].first = std::max(_halo_extents_max[d].first, extent);
        continue;
      }
      _halo_extents_max[d].second = std::max(_halo_extents_max[d].second, extent);
    }
  }

private:
  Specs_t       _specs{};
  HaloExtsMax_t _halo_extents_max{};
  region_size_t _num_regions{ 0 };
};  // HaloSpec


template <dim_t NumDimensions>
std::ostream& operator<<(std::ostream& os, const HaloSpec<NumDimensions>& hs) {
  os << "dash::halo::HaloSpec<" << NumDimensions << ">(";
  bool begin = true;
  for(const auto& region_spec : hs.specs()) {
    if(region_spec.extent() > 0) {
      if(begin) {
        os << region_spec;
        begin = false;
      } else {
        os << "," << region_spec;
      }
    }
  }
  os << "; number region: " << hs.num_regions();
  os << ")";

  return os;
}

template<typename ViewSpecT>
class BoundaryRegionCheck {
  static constexpr auto NumDimensions = ViewSpecT::ndim();
  static constexpr auto CenterIndex   = RegionCoords<NumDimensions>::center_index();
public:
  using GlobalBndSpec_t = GlobalBoundarySpec<NumDimensions>;
  using EnvRegInfo_t    = EnvironmentRegionInfo<ViewSpecT>;
  using RegionBorders_t = typename EnvRegInfo_t::RegionBorders_t;
  using RegionData_t    = typename EnvRegInfo_t::RegionData_t;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using MaxDistPair_t   = std::pair<region_extent_t, region_extent_t>;
  using MaxDist_t       = std::array<MaxDistPair_t, NumDimensions>;
  using BlockViewSpec_t = BlockViewSpec<ViewSpecT>;


public:
  BoundaryRegionCheck(const ViewSpecT& view, const MaxDist_t& max_dist, const GlobalBndSpec_t& glob_bnd_spec, const RegionBorders_t& borders)
  : _view(&view), _max_dist(max_dist) {
    std::array<BS, NumDimensions> to_small;

    const auto& view_extents = _view->extents();

    for(dim_t d = 0; d < NumDimensions; ++d) {
      auto minmax_dim = max_dist[d];
      auto dist = minmax_dim.first + minmax_dim.second;

      to_small[d] = BS::BIGGER;
      if(view_extents[d] <= dist) {
        to_small[d] = (view_extents[d] > minmax_dim.first) ? BS::EQUALS_LESS : BS::PRE_ONLY;
      }
    }

    for(int d = 0; d < NumDimensions; ++d) {
      bool test_small = false;
      for(int d_tmp = 0; d_tmp < d; ++d_tmp) {
        if(to_small[d_tmp] != BS::BIGGER) {
          test_small = true;
          break;
        }
      }

      if(test_small) {
        if(borders[d].first && glob_bnd_spec[d] == BoundaryProp::NONE) {
          _valid_main[d].first = {false, REASON::BORDER};
        } else {
          _valid_main[d].first = {false, REASON::TO_SMALL};
        }

        if(borders[d].second && glob_bnd_spec[d] == BoundaryProp::NONE) {
          _valid_main[d].second = {false, REASON::BORDER};
        } else {
          _valid_main[d].second = {false, REASON::TO_SMALL};
        }
        continue;
      }
      if(borders[d].first && glob_bnd_spec[d] == BoundaryProp::NONE) {
        _valid_main[d].first = {false, REASON::BORDER};
      } else  {
        _valid_main[d].first = {true, REASON::NONE};
      }

      if(borders[d].second && glob_bnd_spec[d] == BoundaryProp::NONE) {
        _valid_main[d].second = {false, REASON::BORDER};
      } else  {
        if(_valid_main[d].first.valid && to_small[d] == BS::PRE_ONLY) {
          _valid_main[d].second = {false, REASON::TO_SMALL};
        } else {
          _valid_main[d].second = {true, REASON::NONE};
        }
      }
    }
  }

  BlockViewSpec_t block_views() {
    // TODO PRE and POST Region true but not full POST region possible
    auto offsets_inner     = _view->offsets();
    auto extents_inner     = _view->extents();
    auto offsets_inner_bnd = _view->offsets();
    auto extents_inner_bnd = _view->extents();

    for(int d = 0; d < NumDimensions; ++d) {

      offsets_inner[d] = _max_dist[d].first;
      extents_inner[d] -= _max_dist[d].first + _max_dist[d].second;

      offsets_inner_bnd[d] = 0;

      if(!_valid_main[d].first.valid) {
        offsets_inner_bnd[d] = _max_dist[d].first;
        extents_inner_bnd[d] -= _max_dist[d].first;
      }

      if(!_valid_main[d].second.valid) {
        extents_inner_bnd[d] -= _max_dist[d].second;
      }
    }

    return {ViewSpecT(offsets_inner, extents_inner), ViewSpecT(offsets_inner_bnd, extents_inner_bnd)};
  }

  bool is_bnd_region_valid(const RegionSpec_t& region) {

    auto& coords = region.coords();
    for(int d = 0; d < NumDimensions; ++d) {
      if(coords[d] == 0 && !_valid_main[d].first.valid) {
        return false;
      }

      if(coords[d] > 1 && !_valid_main[d].second.valid) {
        return false;
      }
    }

    return true;
  }

  RegionData_t region_data(const RegionSpec_t& region, bool local_offsets = true) {

    if(region.index() == CenterIndex) {
      return {ViewSpecT(), false};
    }

    RegionData_t region_data;
    auto& coords = region.coords();
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(coords[d] == 0 && !_valid_main[d].first.valid) {
        return {ViewSpecT(), false};
      }

      if(coords[d] > 1 && !_valid_main[d].second.valid) {
        return {ViewSpecT(), false};
      }
    }

    auto offsets = _view->offsets();
    auto extents = _view->extents();

    if(local_offsets) {
      std::fill(offsets.begin(), offsets.end(), 0);
    }

    // TODO PRE and POST Region true but not full POST region possible
    for(dim_t d = 0; d < NumDimensions; ++d) {

      if(coords[d] < 1) {
        extents[d] = (region.extent() == 0) ? _max_dist[d].first : region.extent();
        continue;
      }

      if(coords[d] == 1) {
         if(_valid_main[d].first.valid ||
           (!_valid_main[d].first.valid && _valid_main[d].first.reason == REASON::BORDER)) {
           extents[d] -= _max_dist[d].first;
           offsets[d] += _max_dist[d].first;
         }

         if(_valid_main[d].second.valid ||
           (!_valid_main[d].second.valid && _valid_main[d].second.reason == REASON::BORDER)) {
           extents[d] -= _max_dist[d].second;
         }
         continue;
      }

      offsets[d] = extents[d] - _max_dist[d].second;
      extents[d] = (region.extent() == 0) ? _max_dist[d].second : region.extent();
    }

    region_data.valid = true;
    region_data.view = ViewSpecT(offsets, extents);
    return region_data;
  }

  RegionData_t region_data_duplicate(const RegionSpec_t& region, bool local_offsets = true) {
    if(region.extent() == 0) {
      return {ViewSpecT(), false};
    }

    RegionData_t region_data;
    auto& coords = region.coords();
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(coords[d] == 0 && !_valid_main[d].first.valid) {
        return {ViewSpecT(), false};
      }

      if(coords[d] > 1 && !_valid_main[d].second.valid) {
        return {ViewSpecT(), false};
      }
    }

    auto offsets = _view->offsets();
    auto extents = _view->extents();

    if(local_offsets) {
      std::fill(offsets.begin(), offsets.end(), 0);
    }

    // TODO PRE and POST Region true but not full POST region possible
    for(dim_t d = 0; d < NumDimensions; ++d) {

      if(coords[d] < 1) {
        extents[d] = region.extent();
        continue;
      }

      if(coords[d] == 1) {
         continue;
      }

      offsets[d] = extents[d] - _max_dist[d].second;
      extents[d] = region.extent();
    }

    region_data.valid = true;
    region_data.view = ViewSpecT(offsets, extents);
    return region_data;
  }

private:
    /*
   * Defines the relation between block extent and stencil distance
   * PRE_ONLY -> only the boundary on the pre center side is valid
   * EQUALS_LESS -> both bboundaries (pre and post center) are valid, but are equal or less than matrix extent
   * BIGGER -> matrix extent is bigger than stencil distance
  */
  enum class BS{
    PRE_ONLY,
    EQUALS_LESS,
    BIGGER
  };

  enum class REASON {
    NONE,
    TO_SMALL,
    BORDER
  };

  struct valid_region {
    bool   valid{};
    REASON reason{};

  };


private:
  const ViewSpecT* _view;
  const MaxDist_t  _max_dist;
  std::array<std::pair<valid_region, valid_region>,NumDimensions> _valid_main;
};


template<typename PatternT>
class EnvironmentInfo {
  static constexpr auto NumDimensions = PatternT::ndim();
  static constexpr auto RegionsMax    = NumRegionsMax<NumDimensions>;

public:
  using ViewSpec_t     = typename PatternT::viewspec_type;

private:
  using RegionData_t = RegionData<ViewSpec_t>;
  using BndInfos_t   = std::array<RegionData_t, RegionsMax>;
  using EnvRegInfo_t = EnvironmentRegionInfo<ViewSpec_t>;
  using EnvInfo_t    = std::array<EnvRegInfo_t, RegionsMax>;
  using HaloSpec_t   = HaloSpec<NumDimensions>;
  using HaloExtsMaxPair_t = typename HaloSpec_t::HaloExtsMaxPair_t;
  using HaloExtsMax_t     = typename HaloSpec_t::HaloExtsMax_t;
  using RegionCoords_t = RegionCoords<NumDimensions>;

public:
  using GlobalBndSpec_t = GlobalBoundarySpec<NumDimensions>;

  using RegionBorders_t = typename EnvRegInfo_t::RegionBorders_t;
  using BndRegCheck_t   = BoundaryRegionCheck<ViewSpec_t>;
  using RegIdxMain_t   = std::array<typename RegionCoords_t::RegIndexDim_t, NumDimensions>;
  using BlockViewSpec_t = BlockViewSpec<ViewSpec_t>;


  EnvironmentInfo(const PatternT& pattern, const HaloSpec_t& halo_spec,
                  const ViewSpec_t& view_glob, const GlobalBndSpec_t& glob_bound_spec)
  : _view(&view_glob), _glob_bnd_spec(&glob_bound_spec) {
    set_environment(pattern, halo_spec);
  }

  BndRegCheck_t boundary_region_check(const HaloSpec_t& halo_spec) const {
    return BndRegCheck_t(*_view, halo_spec.halo_extension_max(), *_glob_bnd_spec, _borders);
  }

  template <typename StencilPointT, std::size_t NumStencilPoints>
  BndRegCheck_t boundary_region_check(const StencilSpec<StencilPointT, NumStencilPoints>& stencil_spec) const {
    auto minmax = stencil_spec.minmax_distances();
    HaloExtsMax_t max_dist{};
    for(dim_t d = 0; d < NumDimensions; ++d) {
      max_dist[d] = {std::abs(minmax[d].first), minmax[d].second};
    }

    return BndRegCheck_t(*_view, max_dist, *_glob_bnd_spec, _borders);
  }

  auto info_dim(dim_t dim) const {
    return std::make_pair(std::ref(_env_info[_reg_idx_main[dim].first]), std::ref(_env_info[_reg_idx_main[dim].second]));
  }

  const EnvRegInfo_t& info(region_index_t region_index) const {
    return _env_info[region_index];
  }

  const EnvInfo_t& info() const {
    return _env_info;
  }

  const auto& view_inner() const {
    return _block_views.inner;
  }

  const auto& view_inner_boundary() const {
    return _block_views.inner_bound;
  }

  const auto& views() const {
    return _block_views;
  }

private:

  void set_environment(const PatternT& pattern, const HaloSpec_t& halo_spec) {
    const auto& view_offsets = _view->offsets();
    const auto& view_extents = _view->extents();

    const auto& glob_extent = pattern.extents();
    for(dim_t d = 0; d < NumDimensions; ++d) {
      _reg_idx_main[d] = RegionCoords_t::index(d);
      if(view_offsets[d] == 0) {
        _borders[d].first = true;
      }
      if(view_offsets[d] + view_extents[d] == glob_extent[d]) {
        _borders[d].second = true;
      }
    }

    BndRegCheck_t bnd_check(*_view, halo_spec.halo_extension_max(), *_glob_bnd_spec, _borders);
    _block_views = bnd_check.block_views();

    const auto& team_spec   = pattern.teamspec();
    for(const auto& spec : halo_spec.specs()) {
      auto halo_extent = spec.extent();
      if(!halo_extent) {
        continue;
      }

      auto& env_md = _env_info[spec.index()];

      env_md.bnd_reg_data = bnd_check.region_data(spec);

      std::array<int, NumDimensions> neighbor_coords_rem;
      std::array<int, NumDimensions> neighbor_coords;
      auto reg_coords = spec.coords();
      auto reg_coords_rem = RegionCoords_t::coords(RegionsMax - 1 - spec.index());

      env_md.boundary_prop     = BoundaryProp::CYCLIC;
      BoundaryProp bnd_prop_to = BoundaryProp::CYCLIC;

      const auto& halo_ext_max = halo_spec.halo_extension_max();

      auto halo_region_offsets = _view->offsets();
      auto halo_region_extents = _view->extents();
      for(dim_t d = 0; d < NumDimensions; ++d) {

        // region coords uses 1 for the center position, while \ref TeamSpec use 0
        neighbor_coords[d] = static_cast<int>(reg_coords[d]) - 1;
        neighbor_coords_rem[d] = static_cast<int>(reg_coords_rem[d]) - 1;


        if(spec[d] == 1) {
          continue;
        }

        halo_region_extents[d] = halo_extent;

        if(spec[d] < 1) {
          if(_borders[d].first) {
            halo_region_offsets[d] = pattern.extent(d) - halo_extent;
            env_md.boundary_prop = test_bound_prop(env_md.boundary_prop, (*_glob_bnd_spec)[d]);
            env_md.border_region = true;
            env_md.region_borders[d].first = true;
          }else {
            halo_region_offsets[d] -= halo_extent;
          }

          if(_borders[d].second) {
            bnd_prop_to = test_bound_prop(bnd_prop_to, (*_glob_bnd_spec)[d]);
          }
          continue;
        }

        // spec[d] > 1
        if(_borders[d].second) {
          halo_region_offsets[d] = 0;
          env_md.boundary_prop = test_bound_prop(env_md.boundary_prop, (*_glob_bnd_spec)[d]);
          env_md.border_region = true;
          env_md.region_borders[d].second = true;
        } else {
          halo_region_offsets[d] += _view->extent(d);
        }

        if(_borders[d].first) {
          bnd_prop_to = test_bound_prop(bnd_prop_to, (*_glob_bnd_spec)[d]);
        }
      }

      env_md.halo_reg_data = {ViewSpec_t(halo_region_offsets, halo_region_extents), true};
      if(env_md.boundary_prop != BoundaryProp::NONE) {
        if(env_md.boundary_prop == BoundaryProp::CYCLIC) {
          env_md.neighbor_id_from = static_cast<dart_unit_t>(team_spec.periodic_neighbor(neighbor_coords));
        } else {
          env_md.neighbor_id_from = static_cast<dart_unit_t>(team_spec.neighbor(neighbor_coords));
        }
      } else {
        env_md.neighbor_id_from = static_cast<dart_unit_t>(team_spec.neighbor(neighbor_coords));
        env_md.halo_reg_data.valid = false;
      }

      if(bnd_prop_to == BoundaryProp::CYCLIC) {
        env_md.neighbor_id_to = static_cast<dart_unit_t>(team_spec.periodic_neighbor(neighbor_coords_rem));
      } else {
        env_md.neighbor_id_to = static_cast<dart_unit_t>(team_spec.neighbor(neighbor_coords_rem));
      }
    }
  }

  BoundaryProp test_bound_prop(const BoundaryProp& current_prop, const BoundaryProp& new_prop) {
    if(current_prop == BoundaryProp::NONE || new_prop == BoundaryProp::NONE) {
      return BoundaryProp::NONE;
    }

    if(current_prop == BoundaryProp::CUSTOM || new_prop == BoundaryProp::CUSTOM) {
      return BoundaryProp::CUSTOM;
    }

    return BoundaryProp::CYCLIC;
  }

private:
  const ViewSpec_t*      _view;
  const GlobalBndSpec_t* _glob_bnd_spec;
  EnvInfo_t              _env_info;
  RegionBorders_t        _borders{};
  RegIdxMain_t           _reg_idx_main;
  BlockViewSpec_t        _block_views;
};

template<typename PatternT>
std::ostream& operator<<(std::ostream& os, const EnvironmentInfo<PatternT>& env_info) {
  static constexpr auto NumDimensions = PatternT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  const auto& env_mds = env_info.info();

  os << "dash::halo::EnvironmentInfo { ";
  for(region_index_t r = 0; r < RegionsMax; ++r) {
    const auto& env_md = env_mds[r];
    os << dash::myid() << " -> ";
    os << "region_index: " << r << ";"
       << env_md << ")\n";
  }
  os << "}";

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
  using HaloSpec_t = typename HaloBlockT::HaloSpec_t;

public:
  using HaloBlock_t     = HaloBlockT;
  using ViewSpec_t      = typename HaloBlockT::ViewSpec_t;
  using BoundaryViews_t = typename HaloBlockT::BoundaryViews_t;
  using pattern_size_t  = typename Pattern_t::size_type;
  using StencilSpec_t   = StencilSpecT;

public:
  StencilSpecificViews(const HaloBlockT&   halo_block,
                       const StencilSpec_t& stencil_spec,
                       const ViewSpec_t*   view_local)
  : _stencil_spec(&stencil_spec), _view_local(view_local) {
    HaloSpec_t halo_spec(stencil_spec);
    auto bnd_region_check = halo_block.halo_env_info().boundary_region_check(halo_spec);

    auto block_views = bnd_region_check.block_views();
    _view_inner = block_views.inner;
    _view_inner_with_boundaries = block_views.inner_bound;
    for(const auto& region : halo_spec.specs()) {
        auto bnd_region_data = bnd_region_check.region_data(region);
        _size_bnd_elems += bnd_region_data.view.size();
       _boundary_views.push_back(std::move(bnd_region_data.view));
    }
  }

  /**
   * Returns \ref StencilSpec
   */
  const StencilSpec_t& stencil_spec() const { return *_stencil_spec; }

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
    if(diff_ext > static_cast<ExtT>(max))
      extent += diff_ext - max;
  }

private:
  const StencilSpec_t* _stencil_spec;
  const ViewSpec_t*    _view_local;
  ViewSpec_t           _view_inner;
  ViewSpec_t           _view_inner_with_boundaries;
  BoundaryViews_t      _boundary_views;
  pattern_size_t       _size_bnd_elems = 0;
};

template <typename HaloBlockT, typename StencilSpecT>
std::ostream& operator<<(
  std::ostream&                                         os,
  const StencilSpecificViews<HaloBlockT, StencilSpecT>& stencil_views) {
  os << "dash::halo::StencilSpecificViews"
     << "(local: " << stencil_views.view()
     << "; inner: " << stencil_views.inner()
     << "; inner_bound: " << stencil_views.inner_with_boundaries()
     << "; boundary_views: " << stencil_views.boundary_views()
     << "; boundary elems: " << stencil_views.boundary_size() << ")";

  return os;
}

/**
 * Takes the local part of the NArray and builds halo and
 * boundary regions.
 */
template <typename ElementT, typename PatternT, typename GlobMemT>
class HaloBlock {
private:
  static constexpr auto NumDimensions = PatternT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  using Self_t          = HaloBlock<ElementT, PatternT, GlobMemT>;
  using pattern_index_t = typename PatternT::index_type;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Region_t        = Region<ElementT, PatternT, GlobMemT>;
  using RegionCoords_t  = RegionCoords<NumDimensions>;
  using Coords_t        = typename RegionCoords_t::Coords_t;
  using region_extent_t = typename RegionSpec_t::region_extent_t;

public:
  using Element_t = ElementT;
  using Pattern_t = PatternT;
  using GlobMem_t = GlobMemT;
  using GlobBoundSpec_t = GlobalBoundarySpec<NumDimensions>;
  using pattern_size_t  = typename PatternT::size_type;
  using ViewSpec_t      = typename PatternT::viewspec_type;
  using BoundaryViews_t = std::vector<ViewSpec_t>;
  using HaloSpec_t      = HaloSpec<NumDimensions>;
  using RegionVector_t  = std::vector<Region_t>;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;
  using HaloExtsMax_t   = typename HaloSpec_t::HaloExtsMax_t;
  using RegIndDepVec_t    = typename RegionCoords_t::RegIndDepVec_t;
  using EnvInfo_t       = EnvironmentInfo<Pattern_t>;

public:
  /**
   * Constructor
   */
  HaloBlock(GlobMem_t& globmem, const PatternT& pattern, const ViewSpec_t& view,
            const HaloSpec_t&      halo_reg_spec,
            const GlobBoundSpec_t& bound_spec)
  : _globmem(globmem), _pattern(pattern), _view(view),
    _halo_reg_spec(halo_reg_spec), _view_local(_view.extents()),
    _glob_bound_spec(bound_spec),
    _env_info(pattern, _halo_reg_spec, _view, _glob_bound_spec) {

    // TODO put functionallity to HaloSpec
    _halo_regions.reserve(_halo_reg_spec.num_regions());
    _boundary_regions.reserve(_halo_reg_spec.num_regions());

    _view_inner = _env_info.view_inner();
    _view_inner_with_boundaries =  _env_info.view_inner_boundary();

    /*
     * Setup for all halo and boundary regions and properties like:
     * is the region a global boundary region and is the region custom or not
     */

    auto bnd_check = _env_info.boundary_region_check(halo_reg_spec);
    for(region_index_t r = 0; r < RegionsMax; ++r) {
      const auto& env_reg_info = _env_info.info(r);
      const auto& spec = _halo_reg_spec.specs()[r];

      _boundary_views.push_back(env_reg_info.bnd_reg_data.view);
      _size_bnd_elems += env_reg_info.bnd_reg_data.view.size();

      auto halo_extent = spec.extent();
      if(!halo_extent) {
        continue;
      }

      if(env_reg_info.halo_reg_data.valid) {
        _halo_regions.push_back(
          Region_t(spec, env_reg_info.halo_reg_data.view,
                  _globmem, _pattern, env_reg_info));
        _halo_reg_mapping[r] = &_halo_regions.back();
        _size_halo_elems += env_reg_info.halo_reg_data.view.size();
      } else {
         _halo_regions.push_back(
          Region_t(spec, ViewSpec_t(),
                  _globmem, _pattern, env_reg_info));
        _halo_reg_mapping[r] = &_halo_regions.back();
      }
      auto bnd_reg_data = bnd_check.region_data_duplicate(spec, false);
      _boundary_regions.push_back(
        Region_t(spec, bnd_reg_data.view,
                 _globmem, _pattern, env_reg_info));
      _boundary_reg_mapping[r] = &_boundary_regions.back();
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
  const Pattern_t& pattern() const { return _pattern; }

  /**
   * The global memory instance that created the encapsulated block.
   */
  const GlobMem_t& globmem() const { return _globmem; }

  /**
   * Returns the \ref GlobalBoundarySpec used by the HaloBlock instance.
   */
  const GlobBoundSpec_t& global_boundary_spec() const {
    return _glob_bound_spec;
  }

  /**
   * Returns used \ref HaloSpec
   */
  const HaloSpec_t& halo_spec() const { return _halo_reg_spec; }

/**
   * Returns the environment information object \ref EnvironmentInfo
   */
  EnvInfo_t halo_env_info() { return _env_info; }

  /**
   * Returns the environment information object \ref EnvironmentInfo
   */
  const EnvInfo_t& halo_env_info() const { return _env_info; }


  /**
   * Returns a specific halo region and nullptr if no region exists
   */
  const Region_t* halo_region(const region_index_t index) const {
    return _halo_reg_mapping[index];
  }

  /**
   * Returns all halo regions
   */
  const RegionVector_t& halo_regions() const { return _halo_regions; }

  /**
   * Returns a specific region and nullptr if no region exists
   */
  const Region_t* boundary_region(const region_index_t index) const {
    return _boundary_reg_mapping[index];
  }

  /**
   * Returns all boundary regions. Between regions element reoccurences are
   * possible.
   */
  const RegionVector_t& boundary_regions() const { return _boundary_regions; }

  RegIndDepVec_t boundary_dependencies(region_index_t index) const {
    RegIndDepVec_t index_dep{};
    for(auto reg_index : RegionCoords_t::boundary_dependencies(index)) {
      auto region = halo_region(reg_index);
      if(region != nullptr) {
        index_dep.push_back(reg_index);
      }
    }

    return index_dep;
  }

  /**
   * Returns the initial global \ref ViewSpec
   */
  const ViewSpec_t& view() const { return _view; }

  /**
   * Returns the initial local \ref ViewSpec
   */
  const ViewSpec_t& view_local() const { return _view_local; }

  /**
   * Returns a local \ref ViewSpec that combines the boundary and inner view
   */
  const ViewSpec_t& view_inner_with_boundaries() const {
    return _view_inner_with_boundaries;
  }

  /**
   * Returns the inner \ref ViewSpec with local offsets depending on the used
   * \ref HaloSpec.
   */
  const ViewSpec_t& view_inner() const { return _view_inner; }

  /**
   * Returns a set of local views that contains all boundary elements.
   * No duplicated elements included.
   */
  const BoundaryViews_t& boundary_views() const { return _boundary_views; }

  /**
   * Number of halo elements
   */
  pattern_size_t halo_size() const { return _size_halo_elems; }

  /**
   * Number of boundary elements (no duplicates)
   */
  pattern_size_t boundary_size() const { return _size_bnd_elems; }

  /**
   * Returns the region index belonging to the given coordinates and \ref ViewSpec
   */
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
        index *= REGION_INDEX_BASE;
      else if(coords[d] < static_cast<signed_extent_t>(extents[d]))
        index = 1 + index * REGION_INDEX_BASE;
      else
        index = 2 + index * REGION_INDEX_BASE;
    }

    return index;
  }

private:
  GlobMem_t& _globmem;

  const PatternT& _pattern;

  const ViewSpec_t& _view;

  const HaloSpec_t& _halo_reg_spec;

  const ViewSpec_t _view_local;

  const GlobBoundSpec_t& _glob_bound_spec;

  EnvInfo_t       _env_info;

  ViewSpec_t _view_inner_with_boundaries;

  ViewSpec_t _view_inner;

  RegionVector_t _halo_regions;

  std::array<Region_t*, RegionsMax> _halo_reg_mapping{};

  RegionVector_t _boundary_regions;

  std::array<Region_t*, RegionsMax> _boundary_reg_mapping{};

  BoundaryViews_t _boundary_views;

  pattern_size_t _size_bnd_elems = 0;

  pattern_size_t _size_halo_elems = 0;
};  // class HaloBlock

template <typename ElementT, typename PatternT, typename GlobMemT>
std::ostream& operator<<(std::ostream&                        os,
                         const HaloBlock<ElementT, PatternT, GlobMemT>& haloblock) {
  bool begin = true;
  os << "dash::halo::HaloBlock<" << typeid(ElementT).name() << ">("
     << "view global: " << haloblock.view()
     << "; halo spec: " << haloblock.halo_spec()
     << "; view local: " << haloblock.view_local()
     << "; view inner: " << haloblock.view_inner()
     << "; view inner_bnd: " << haloblock.view_inner_with_boundaries()
     << "; halo regions { ";
  for(const auto& region : haloblock.halo_regions()) {
    if(begin) {
      os << region;
      begin = false;
    } else {
      os << "," << region;
    }
  }
  os << " } "
     << "; halo elems: " << haloblock.halo_size() << "; boundary regions: { ";
  for(const auto& region : haloblock.boundary_regions()) {
    if(begin) {
      os << region;
      begin = false;
    } else {
      os << "," << region;
    }
  }
  os << " } "
     << "; boundary views: " << haloblock.boundary_views()
     << "; boundary elems: " << haloblock.boundary_size() << ")";

  return os;
}



}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO_HALO_H__
