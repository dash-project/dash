#ifndef DASH__HALO__HALO_H__
#define DASH__HALO__HALO_H__

#include <dash/internal/Logging.h>


#include <dash/halo/Types.h>
#include <dash/halo/Region.h>
#include <dash/halo/Stencil.h>
#include <dash/halo/HaloMemory.h>

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

public:
  constexpr HaloSpec(const Specs_t& specs) : _specs(specs) {}

  template <typename StencilSpecT>
  HaloSpec(const StencilSpecT& stencil_spec) {
    
    read_stencil_points(stencil_spec);
  }

  template <typename StencilSpecT, typename... Args>
  HaloSpec(const StencilSpecT& stencil_spec, const Args&... stencil_specs)
  : HaloSpec(stencil_specs...) {
    read_stencil_points(stencil_spec);
  }

  template <typename... ARGS>
  HaloSpec(const RegionSpec_t& region_spec, const ARGS&... args) {
    std::array<RegionSpec_t, sizeof...(ARGS) + 1> tmp{ region_spec, args... };
    for(auto& spec : tmp) {
      auto& current_spec = _specs[spec.index()];
      if(current_spec.extent() == 0 && spec.extent() > 0) {
        ++_num_regions;
      }

      if(current_spec.extent() < spec.extent()) {
        current_spec = spec;
      }
    }
  }

  HaloSpec(const Self_t& other) { _specs = other._specs; }

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

private:
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
    if(_specs[index].extent() == 0 && max > 0)
      ++_num_regions;
    
    
    if(max > _specs[index].extent())
      _specs[index] = RegionSpec_t(index, max);
  }

  /*
   * Makes sure, that all necessary regions are covered for a stencil point.
   * E.g. 2-D stencil point (-1,-1) needs not only region 0, it needs also
   * region 1 when the stencil is shifted to the right.
   */
  template <typename StencilPointT>
  bool next_region(const StencilPointT& stencil,
                   StencilPointT&       stencil_combination) {
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

/**
 * Contains all Boundary regions defined via a \ref StencilSpec.
 */
template <typename HaloBlockT>
class BoundaryRegionMapping {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();

  using Self_t         = BoundaryRegionMapping<HaloBlockT>;
  using RegionCoords_t = RegionCoords<NumDimensions>;
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

public:
  using ViewSpec_t      = typename HaloBlockT::ViewSpec_t;
  using GlobalBndSpec_t = GlobalBoundarySpec<NumDimensions>;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Views_t         = std::array<ViewSpec_t, RegionsMax>;
  using view_size_t     = typename ViewSpec_t::size_type;

public:
  constexpr BoundaryRegionMapping(const Views_t& views) : _views(views) {
    for(const auto& view : _views) {
      _num_elems += _views.size();
    }
  }

  template <typename StencilSpecT>
  BoundaryRegionMapping(const StencilSpecT& stencil_spec, HaloBlockT halo_block) {
    read_stencil_points(stencil_spec, halo_block);
  }

  BoundaryRegionMapping(const Self_t& other) { _views = other._views; }

  /**
   * Matching \ref RegionSpec for a given region index
   */
  constexpr ViewSpec_t view(const region_index_t index) const {
    return _views[index];
  }

  /**
   * Number of specified regions
   */
  constexpr region_size_t num_regions() const { return RegionsMax; }

  /**
   * Number of elements reflected by all boundary views
   */
  region_size_t num_elements() const { return _num_elems; }

  /**
   * Used \ref RegionSpec instance
   */
  const Views_t& views() const { return _views; }

private:
  template<typename CoordsT, typename DistT>
  bool check_valid_region(const CoordsT& coords, region_index_t index, const DistT& dist,
    const HaloBlockT& halo_block) {

    const auto& glob_bnd_spec = halo_block.global_boundary_spec();
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(coords[d] == 1) {
        continue;
      }

      if(glob_bnd_spec[d] == BoundaryProp::NONE) {
        auto reg_index_dim = RegionCoords_t::index(d);
        if(coords[d] < 1) {
          auto bnd_region = halo_block.boundary_region(reg_index_dim.first);
          if(bnd_region == nullptr || bnd_region->border_dim(d).first) {
            return false;
          }
        }

        if(coords[d] > 1) {
          auto bnd_region = halo_block.boundary_region(reg_index_dim.second);
          if(bnd_region == nullptr || bnd_region->border_dim(d).second) {
            return false;
          }
        }
      }

      auto min = dist[d].first;
      if(coords[d] < 1 && min == 0)
        return false;

      auto max = dist[d].second;
      if(coords[d] > 1 && max == 0)
        return false;
    }

    return true;
  }

  /*
   * Reads all stencil points of the given stencil spec and sets the region
   * specification.
   */
  template <typename StencilSpecT>
  void read_stencil_points(const StencilSpecT& stencil_spec, const HaloBlockT& halo_block) {

    using view_ext_t = typename ViewSpec_t::size_type;
    const auto& view = halo_block.view_local();

    _num_elems = 0;
    auto center = RegionCoords_t::center_index();
    auto dist_dims = stencil_spec.minmax_distances();
    for(auto r = 0u;  r < RegionsMax; ++r) {
      auto reg_coords = RegionCoords_t::coords(r);
      if(r == center || !check_valid_region(reg_coords, r, dist_dims, halo_block)){
        continue;
      }

      auto offsets = _views[r].offsets();
      auto extents = _views[r].extents();

      for(dim_t d = 0; d < NumDimensions; ++d) {
        auto dist_min = dist_dims[d].first;
        if(reg_coords[d] < 1 && dist_min < 0) {
          extents[d] = static_cast<view_ext_t>(dist_min * -1);

          continue;
        }

        auto dist_max = dist_dims[d].second;
        if(reg_coords[d] == 1) {
          offsets[d] = static_cast<view_ext_t>(dist_min * -1);
          if(extents[d] == 0) {
            extents[d] = view.extent(d);
          }
          extents[d] = extents[d] - offsets[d] - dist_max;
          continue;
        }

        if(dist_max > 0) {
          extents[d] = static_cast<view_ext_t>(dist_max);
          offsets[d] = view.extent(d) - extents[d];
        }
      }
      _views[r] = ViewSpec_t(offsets, extents);
      _num_elems += _views[r].size();
    }
  }

private:
  Views_t     _views{};
  view_size_t _num_elems;
};  // BoundaryRegionMapping

/**
 * Adapts all views \ref HaloBlock provides to the given \ref StencilSpec.
 */
template <typename HaloBlockT, typename StencilSpecT>
class StencilSpecificViews {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();

  using Pattern_t = typename HaloBlockT::Pattern_t;

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
    auto minmax_dist = stencil_spec.minmax_distances();
    for(auto& dist : minmax_dist)
      dist.first = std::abs(dist.first);

    auto inner_off       = halo_block.view_inner().offsets();
    auto inner_ext       = halo_block.view_inner().extents();
    auto inner_bound_off = halo_block.view_inner_with_boundaries().offsets();
    auto inner_bound_ext = halo_block.view_inner_with_boundaries().extents();
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

    BoundaryRegionMapping<HaloBlockT> bound_mapping(stencil_spec, halo_block);

    auto bound_regions = bound_mapping.views();
    _size_bnd_elems = bound_mapping.num_elements();

    for(auto r = 0u; r < bound_regions.size(); ++r ) {
      _boundary_views.push_back(bound_regions[r]);
    }

    _boundary_views_dim = halo_block.boundary_views();
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
   * Returns all boundary views including all boundary elements (no dublicates)
   */
  const BoundaryViews_t& boundary_views_dim() const { return _boundary_views_dim; }

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
  BoundaryViews_t      _boundary_views_dim;
  pattern_size_t       _size_bnd_elems = 0;
};

template <typename HaloBlockT, typename StencilSpecT>
std::ostream& operator<<(
  std::ostream&                                         os,
  const StencilSpecificViews<HaloBlockT, StencilSpecT>& stencil_views) {
  std::ostringstream ss;
  ss << "dash::halo::StencilSpecificViews"
     << "(local: " << stencil_views.view()
     << "; inner: " << stencil_views.inner()
     << "; inner_bound: " << stencil_views.inner_with_boundaries()
     << "; boundary_views: " << stencil_views.boundary_views()
     << "; boundary elems: " << stencil_views.boundary_size() << ")";

  return operator<<(os, ss.str());
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
  using BorderMeta_t    = typename Region_t::BorderMeta_t;
  using Border_t        = typename Region_t::Border_t;

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

  using HaloExtsMaxPair_t = std::pair<region_extent_t, region_extent_t>;
  using HaloExtsMax_t     = std::array<HaloExtsMaxPair_t, NumDimensions>;
  using RegIndDepVec_t    = typename RegionCoords_t::RegIndDepVec_t;

public:
  /**
   * Constructor
   */
  HaloBlock(GlobMem_t& globmem, const PatternT& pattern, const ViewSpec_t& view,
            const HaloSpec_t&      halo_reg_spec,
            const GlobBoundSpec_t& bound_spec)
  : _globmem(globmem), _pattern(pattern), _view(view),
    _halo_reg_spec(halo_reg_spec), _view_local(_view.extents()),
    _glob_bound_spec(bound_spec) {
    // setup local views
    _view_inner                 = _view_local;
    _view_inner_with_boundaries = _view_local;

    // TODO put functionallity to HaloSpec
    _halo_regions.reserve(_halo_reg_spec.num_regions());
    _boundary_regions.reserve(_halo_reg_spec.num_regions());

    Border_t border{};
    /*
     * Setup for all halo and boundary regions and properties like:
     * is the region a global boundary region and is the region custom or not
     */
    for(const auto& spec : _halo_reg_spec.specs()) {
      auto halo_extent = spec.extent();
      if(!halo_extent)
        continue;

      Border_t border_region{};
      bool     custom_region = false;

      auto halo_region_offsets = view.offsets();
      auto halo_region_extents = view.extents();
      auto bnd_region_offsets  = view.offsets();
      auto bnd_region_extents  = view.extents();

      for(dim_t d = 0; d < NumDimensions; ++d) {
        if(spec[d] == 1)
          continue;

        auto view_offset = view.offset(d);
        auto view_extent = view.extent(d);

        if(spec[d] < 1) {
          _halo_extents_max[d].first =
            std::max(_halo_extents_max[d].first, halo_extent);
          if(view_offset < _halo_extents_max[d].first) {
            border_region[d].first = true;
            border[d].first = true;

            if(bound_spec[d] == BoundaryProp::NONE) {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = 0;
              bnd_region_offsets[d]  = 0;
              bnd_region_extents[d]  = 0;
            } else {
              if(bound_spec[d] == BoundaryProp::CUSTOM)
                custom_region = true;
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
          _halo_extents_max[d].second =
            std::max(_halo_extents_max[d].second, halo_extent);
          auto check_extent =
            view_offset + view_extent + _halo_extents_max[d].second;
          if(static_cast<pattern_index_t>(check_extent) > _pattern.extent(d)) {
            border_region[d].second = true;
            border[d].second = true;

            if(bound_spec[d] == BoundaryProp::NONE) {
              halo_region_offsets[d] = 0;
              halo_region_extents[d] = 0;
              bnd_region_offsets[d]  = 0;
              bnd_region_extents[d]  = 0;
            } else {
              if(bound_spec[d] == BoundaryProp::CUSTOM)
                custom_region = true;
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
                 _globmem, _pattern, border_region, custom_region));
      auto& region_tmp = _halo_regions.back();
      _size_halo_elems += region_tmp.size();
      _halo_reg_mapping[index] = &region_tmp;
      _boundary_regions.push_back(
        Region_t(spec, ViewSpec_t(bnd_region_offsets, bnd_region_extents),
                 _globmem, _pattern, border_region, custom_region));
      _boundary_reg_mapping[index] = &_boundary_regions.back();
    }

    /*
     * Setup for the non duplicate boundary elements and the views: inner,
     * boundary and inner + boundary
     */
    for(dim_t d = 0; d < NumDimensions; ++d) {
      const auto global_offset = view.offset(d);
      const auto view_extent   = _view_local.extent(d);

      auto bnd_elem_offsets = _view.offsets();
      auto bnd_elem_extents = _view_local.extents();
      bnd_elem_extents[d]   = _halo_extents_max[d].first;
      for(auto d_tmp = 0; d_tmp < d; ++d_tmp) {
        bnd_elem_offsets[d_tmp] -=
          _view.offset(d_tmp) - _halo_extents_max[d_tmp].first;
        bnd_elem_extents[d_tmp] -=
          _halo_extents_max[d_tmp].first + _halo_extents_max[d_tmp].second;
      }

      _view_inner.resize_dim(
        d, _halo_extents_max[d].first,
        view_extent - _halo_extents_max[d].first - _halo_extents_max[d].second);

      auto safe_offset = global_offset;
      auto safe_extent = view_extent;
      if(border[d].first && bound_spec[d] == BoundaryProp::NONE) {
        safe_offset = _halo_extents_max[d].first;
        safe_extent -= _halo_extents_max[d].first;
      } else {
        bnd_elem_offsets[d] -= global_offset;
        push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents,
                        _halo_extents_max, bound_spec);
      }

      if(border[d].second && bound_spec[d] == BoundaryProp::NONE) {
        safe_extent -= _halo_extents_max[d].second;
      } else {
        bnd_elem_offsets[d] += view_extent - _halo_extents_max[d].first;
        bnd_elem_extents[d] = _halo_extents_max[d].second;
        push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents,
                       _halo_extents_max, bound_spec);
      }
      _view_inner_with_boundaries.resize_dim(d, safe_offset - global_offset,
                                               safe_extent);
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
   * Returns the maximal extension for a  specific dimension
   */
  const HaloExtsMaxPair_t& halo_extension_max(dim_t dim) const {
    return _halo_extents_max[dim];
  }

  /**
   * Returns the maximal halo extension for every dimension
   */
  const HaloExtsMax_t& halo_extension_max() const { return _halo_extents_max; }

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
  void push_bnd_elems(dim_t                                       dim,
                      std::array<pattern_index_t, NumDimensions>& offsets,
                      std::array<pattern_size_t, NumDimensions>&  extents,
                      const HaloExtsMax_t&                        halo_exts_max,
                      const GlobBoundSpec_t&                      bound_spec) {
    auto tmp = offsets;
    for(auto d_tmp = dim + 1; d_tmp < NumDimensions; ++d_tmp) {
      if(bound_spec[d_tmp] == BoundaryProp::NONE) {
        if(offsets[d_tmp] < halo_exts_max[d_tmp].first) {
          offsets[d_tmp] = halo_exts_max[d_tmp].first;
          tmp[d_tmp]     = halo_exts_max[d_tmp].first;
          extents[d_tmp] -= halo_exts_max[d_tmp].first;
        }
        auto check_extent_tmp =
          offsets[d_tmp] + extents[d_tmp] + halo_exts_max[d_tmp].second;
        if(static_cast<pattern_index_t>(check_extent_tmp) > _pattern.extent(d_tmp))
          extents[d_tmp] -= halo_exts_max[d_tmp].second;
      }

      tmp[d_tmp] -= _view.offset(d_tmp);
    }

    ViewSpec_t boundary_next(tmp, extents);
    _size_bnd_elems += boundary_next.size();
    _boundary_views.push_back(std::move(boundary_next));
  }

private:
  GlobMem_t& _globmem;

  const PatternT& _pattern;

  const ViewSpec_t& _view;

  const HaloSpec_t& _halo_reg_spec;

  const ViewSpec_t _view_local;

  const GlobBoundSpec_t& _glob_bound_spec;

  ViewSpec_t _view_inner_with_boundaries;

  ViewSpec_t _view_inner;

  RegionVector_t _halo_regions;

  std::array<Region_t*, RegionsMax> _halo_reg_mapping{};

  RegionVector_t _boundary_regions;

  std::array<Region_t*, RegionsMax> _boundary_reg_mapping{};

  BoundaryViews_t _boundary_views;

  pattern_size_t _size_bnd_elems = 0;

  pattern_size_t _size_halo_elems = 0;

  HaloExtsMax_t _halo_extents_max{};
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
