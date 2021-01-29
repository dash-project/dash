#ifndef DASH__HALO_REGION_H
#define DASH__HALO_REGION_H

#include <dash/halo/Types.h>
#include <dash/halo/Stencil.h>
#include <dash/iterator/GlobViewIter.h>

#include <cstdlib>

namespace dash {

namespace halo {

using namespace internal;

/**
 * N-Dimensional region coordinates and associated indices for all possible
 * Halo/Boundary regions of a \ref HaloBlock. The center (all values = 1) is the
 * local NArray memory block used by the \ref HaloBlock.
 *
 * Example for 2-D
 *
 * .-------..-------..-------.
 * |   0   ||   1   ||   2 <-|-- region index
 * | (0,0) || (0,1) || (0,2)<|-- region coordinates
 * |  NW   ||   N   ||   NE <|-- north east (only for explanation)
 * '-------''-------''-------'
 * .-------..-------..-------.
 * |   3   ||   4   ||   5   |
 * | (1,0) || (1,1) || (1,2) |
 * |   W   ||   C   ||   E   |
 * '-------''-------''-------'
 * .-------..-------..-------.
 * |   6   ||   7   ||   8   |
 * | (2,0) || (2,1) || (2,2) |
 * |  SW   ||   S   ||   SE  |
 * '-------''-------''-------'
 */
template <dim_t NumDimensions>
class RegionCoords : public Dimensional<region_coord_t, NumDimensions> {
private:
  using Self_t = RegionCoords<NumDimensions>;
  using Base_t = Dimensional<uint8_t, NumDimensions>;

  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

public:
  using Coords_t       = std::array<uint8_t, NumDimensions>;
  using CoordsVec_t    = std::vector<Self_t>;
  using RegIndDepVec_t = std::vector<region_index_t>;
  using RegIndexDim_t  = std::pair<region_index_t, region_index_t>;



public:
  /**
   * Default Constructor
   *
   * All region coordinate values are 1 and pointing to the center.
   */
  RegionCoords() {
    for(dim_t i = 0; i < NumDimensions; ++i) {
      this->_values[i] = 1;
    }
  }

  /**
   * Constructor allows custom coordinate values and calculates the fitting
   * region index.
   */
  template <typename... Values>
  RegionCoords(region_coord_t value, Values... values)
  : Base_t::Dimensional(value, values...) {
  }

  /**
   * Constructor takes a region index to set up the region coordinates
   */
  RegionCoords(region_index_t index) {
    this->_values = Self_t::coords(index);
  }

  static constexpr region_index_t center_index() {
    return NumRegionsMax<NumDimensions> / 2;
  }

  static Coords_t center_coords() {
    Coords_t reg_coords;
    reg_coords.fill(1);

    return reg_coords;
  }

  /**
   * \return region index
   */
  constexpr region_index_t index() const { return index(this->_values); }

  /**
   * Returns a region index for a given dimension and \ref RegionPos
   */
  static constexpr RegIndexDim_t index(dim_t dim) {
    RegIndexDim_t index_dim = std::make_pair(0,0);

    for(dim_t d = 0; d < NumDimensions; ++d)
      if(dim == d) {
        index_dim.first = index_dim.first * REGION_INDEX_BASE;
        index_dim.second = 2 + index_dim.second * REGION_INDEX_BASE;
      }
      else {
        index_dim.first = 1 + index_dim.first * REGION_INDEX_BASE;
        index_dim.second = 1 + index_dim.second * REGION_INDEX_BASE;
      }

    return index_dim;
  }

  /**
   * Returns a region index for a given dimension and \ref RegionPos
   */
  static constexpr region_index_t index(dim_t dim, RegionPos pos) {
    region_coord_t coord = (pos == RegionPos::PRE) ? 0 : 2;

    region_index_t index = 0;
    for(dim_t d = 0; d < NumDimensions; ++d)
      if(dim == d)
        index = coord + index * REGION_INDEX_BASE;
      else
        index = 1 + index * REGION_INDEX_BASE;

    return index;
  }

  /**
   * Returns a region index for a given dimension and \ref RegionPos
   */
  template<typename StencilPointT>
  static constexpr region_index_t index(const StencilPointT& stencil) {
    region_index_t index = 0;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(stencil[d] < 0) {
        index *= REGION_INDEX_BASE;
        continue;
      }

      if(stencil[d] > 0) {
        index = 2 + index * REGION_INDEX_BASE;
        continue;
      }

      index = 1 + index * REGION_INDEX_BASE;
    }

    return index;
  }

  /**
   * Returns the region index for a given \ref RegionCoords
   *
   * \return region index
   */
  static region_index_t index(const Coords_t& coords) {
    region_index_t index = coords[0];
    for(dim_t d = 1; d < NumDimensions; ++d) {
      // in case a wrong region coordinate was set
      if(coords[d] > 2) {
        index = coords[d] + index * REGION_INDEX_BASE;
      } else {
        index = coords[d] + index * REGION_INDEX_BASE;
      }
    }

    return index;
  }

  /**
   * \param index region index
   *
   * \return region coordinates
   */
  static Coords_t coords(const region_index_t index) {
    Coords_t       coords{};
    region_index_t index_tmp =index;
    for(auto i = (NumDimensions - 1); i >= 1; --i) {
      auto res  = std::div(static_cast<long>(index_tmp), static_cast<long>(REGION_INDEX_BASE));
      coords[i] = res.rem;
      index_tmp = res.quot;
    }
    coords[0] = index_tmp;

    return coords;
  }

  /**
   * Returns the highest dimension with region values != 1
   */
  static dim_t relevant_dim(const Coords_t& coords) {
    dim_t dim = 1;
    for(auto d = 1; d < NumDimensions; ++d) {
      if(coords[d] != 1)
        dim = d + 1;
    }

    return dim;
  }

  /**auto max = stencil.max();
   * level = 0 -> center (1,1)
   * level = 1 -> main regions (e.g. 2D: (0,1) (2,1) (1,0) (1,2)
   * level = 2 e.g. 2D corner regions or 3D edge regions
   * ... for dimensions higher than 2D relevant
   */
  static dim_t level(const Coords_t& coords) {
    dim_t level = 0;
    for(auto d = 0; d < NumDimensions; ++d) {
      if(coords[d] != 1)
        ++level;
    }
    return level;
  }

  /**
   * returns the number of coordinates unequal to the center (1) for
   * all dimensions
   *
   * level = 0 -> center (1,1)
   * level = 1 -> main regions (e.g. 2D: (0,1) (2,1) (1,0) (1,2)
   * level = 2 e.g. 2D corner regions or 3D edge regions
   * ... for dimensions higher than 2D relevant
   */
  dim_t level() { return level(this->_values); }

  static RegIndDepVec_t boundary_dependencies(region_index_t index) {
    RegIndDepVec_t index_dep{};

    if(index >= RegionsMax) {
      DASH_LOG_ERROR("Invalid region index: %d", index);

      return index_dep;
    }

    auto region_coords = Self_t(index);
    auto level = region_coords.level();

    if(level == 0) {
      return index_dep;
    }

    if(level == 1) {
      index_dep.push_back(index);

      return index_dep;
    }

    CoordsVec_t found_coords{};
    find_dep_regions(0, region_coords, found_coords);

    for(auto& reg_coords : found_coords) {
      index_dep.push_back(reg_coords.index());
    }

    return index_dep;
  }

  constexpr bool operator==(const Self_t& other) const {

    return this->_values == other._values;
  }

  constexpr bool operator!=(const Self_t& other) const {
    return !(*this == other);
  }

private:

  static void find_dep_regions(dim_t dim_change, const Self_t& current_coords, CoordsVec_t& dep_coords) {
    dep_coords.push_back(current_coords);

    for(dim_t d = dim_change; d < NumDimensions; ++d) {
      if(current_coords[d] != 1) {
        auto new_coords = current_coords;
        new_coords[d] = 1;
        find_dep_regions(d+1, new_coords, dep_coords);
      }
    }
  }
};  // RegionCoords

/**
 * Region specification connecting \ref RegionCoords with an extent.
 * The region extent applies to all dimensions.
 */
template <dim_t NumDimensions>
class RegionSpec {
private:
  using Self_t = RegionSpec<NumDimensions>;

public:
  using RegionCoords_t  = RegionCoords<NumDimensions>;
  using region_extent_t = uint16_t;

public:
  /**
   * Constructor using RegionCoords and the extent
   */
  RegionSpec(const RegionCoords_t& coords, const region_extent_t extent)
  : _coords(coords), _index(coords.index()), _extent(extent),
    _rel_dim(RegionCoords_t::relevant_dim(coords.values())),
    _level(RegionCoords_t::level(coords.values())) {}

  /**
   * Constructor using a region index and an extent
   */
  RegionSpec(region_index_t index, const region_extent_t extent)
  : _coords(RegionCoords_t(index)), _index(index), _extent(extent),
    _rel_dim(RegionCoords_t::relevant_dim(_coords.values())),
    _level(RegionCoords_t::level(_coords.values())) {}

  RegionSpec()
  : _coords(), _index(_coords.index()), _extent(0),
    _rel_dim(RegionCoords_t::relevant_dim(_coords.values())),
    _level(RegionCoords_t::level(_coords.values())) {}

  /**
   * Returns the region index for a given \ref StencilPoint
   */
  template <typename StencilT>
  static region_index_t index(const StencilT& stencil) {
    region_index_t index = 0;
    if(stencil[0] == 0)
      index = 1;
    else if(stencil[0] > 0)
      index = 2;
    for(auto d(1); d < NumDimensions; ++d) {
      if(stencil[d] < 0)
        index *= REGION_INDEX_BASE;
      else if(stencil[d] == 0)
        index = 1 + index * REGION_INDEX_BASE;
      else
        index = 2 + index * REGION_INDEX_BASE;
    }

    return index;
  }

  /**
   * Returns the region index
   */
  constexpr region_index_t index() const { return _index; }

  /**
   * Returns the \ref RegionCoords
   */
  constexpr const RegionCoords_t& coords() const { return _coords; }

  /**
   * Returns the extent
   */
  constexpr region_extent_t extent() const { return _extent; }

  /**
   * Returns the \ref RegionCoords for a given region index
   */
  constexpr region_coord_t operator[](const region_index_t index) const {
    return _coords[index];
  }

  constexpr bool operator==(const Self_t& other) const {
    return _coords.index() == other._coords.index() && _extent == other._extent;
  }

  constexpr bool operator!=(const Self_t& other) const {
    return !(*this == other);
  }

  /**
   * Returns the highest dimension with region values != 1
   */
  dim_t relevant_dim() const { return _rel_dim; }

  /**
   * returns the number of coordinates unequal the center (1) for all
   * dimensions
   */
  dim_t level() const { return _level; }

private:
  RegionCoords_t  _coords{};
  region_index_t  _index;
  region_extent_t _extent  = 0;
  dim_t           _rel_dim = 1;
  dim_t           _level   = 0;
};  // RegionSpec

template <dim_t NumDimensions>
std::ostream& operator<<(std::ostream&                    os,
                         const RegionSpec<NumDimensions>& rs) {
  os << "dash::halo::RegionSpec<" << NumDimensions << ">(" << (uint32_t) rs[0];
  for(auto i = 1; i < NumDimensions; ++i)
    os << "," << (uint32_t) rs[i];
  os << "), Extent:" << rs.extent();

  return os;
}

/**
 * Provides \ref RegionIter and some region metadata like \ref RegionSpec,
 * size etc.
 */
template <typename ElementT, typename PatternT, typename GlobMemT>
class Region {
private:
  using Self_t = Region<ElementT, PatternT, GlobMemT>;
  static constexpr auto NumDimensions = PatternT::ndim();

public:
  using iterator       = GlobViewIter<ElementT, PatternT, GlobMemT>;
  using const_iterator = const iterator;
  using RegionSpec_t   = RegionSpec<NumDimensions>;
  using GlobMem_t      = GlobMemT;
  using ViewSpec_t     = typename PatternT::viewspec_type;
  using extent_size_t  = typename ViewSpec_t::size_type;
  using pattern_size_t = typename PatternT::size_type;
  using EnvRegInfo_t   = EnvironmentRegionInfo<ViewSpec_t>;
  using BorderPair_t   = typename EnvRegInfo_t::PrePostBool_t;
  using RegBorders_t   = typename EnvRegInfo_t::RegionBorders_t;

public:
  Region(const RegionSpec_t& region_spec, const ViewSpec_t& view,
         GlobMem_t& globmem, const PatternT& pattern,
         const EnvRegInfo_t& env_reg_info)
  : _region_spec(&region_spec), _view(view),
    _globmem(&globmem), _pattern(&pattern),
    _env_reg_info(&env_reg_info),
    _beg(&globmem, *_pattern, _view, 0),
    _end(&globmem, *_pattern, _view, _view.size()) {
  }

  Region(const Self_t& other)
  : _region_spec(other._region_spec),
    _view     (other._view),
    _globmem(other._globmem),
    _pattern(other._pattern),
    _env_reg_info(other._env_reg_info),
    _beg(_globmem, *_pattern, _view, 0),
    _end(_globmem, *_pattern, _view, _view.size()) {
  }

  Region(const Self_t&& other)
  : _region_spec(std::move(other._region_spec)),
    _view     (std::move(other._view)),
    _globmem(std::move(other._globmem)),
    _pattern(std::move(other._pattern)),
    _env_reg_info(std::move(other._env_reg_info)),
    _beg(_globmem, *_pattern, _view, 0),
    _end(_globmem, *_pattern, _view, _view.size()) {
  }

  Self_t& operator=(const Self_t& other) {
    _region_spec = other._region_spec;
    _view      = other._view;
    _globmem = other._globmem;
    _pattern = other._pattern;
    _env_reg_info = other._env_reg_info;
    _beg = iterator(_globmem, *_pattern, _view, 0);
    _end = iterator(_globmem, *_pattern, _view, _view.size());

    return *this;
  }

  Self_t& operator=(const Self_t&& other) {
    _region_spec = std::move(other._region_spec);
    _view      = std::move(other._view);
    _globmem = std::move(other._globmem);
    _pattern = std::move(other._pattern);
    _env_reg_info = std::move(other._env_reg_info),
    _beg = iterator(_globmem, *_pattern, _view, 0);
    _end = iterator(_globmem, *_pattern, _view, _view.size());

    return *this;
  }

  const region_index_t index() const { return _region_spec->index(); }

  const RegionSpec_t& spec() const { return *_region_spec; }

  const ViewSpec_t& view() const { return _view; }

  pattern_size_t size() const { return _view.size(); }

  const RegBorders_t& border() const { return _env_reg_info->region_borders; }

  bool is_border_region() const { return _env_reg_info->border_region; }

  bool is_custom_region() const {
    return (_env_reg_info->border_region && _env_reg_info->boundary_prop == BoundaryProp::CUSTOM) ? true : false;
  }

  /**
   * Returns a pair of two booleans for a given dimension.
   * In case the region is the global border in this dimension
   * the value is true, otherwise false
   * first -> Pre center position; second -> Post center position
   */
  BorderPair_t border_dim(dim_t dim) const { return _env_reg_info->region_borders[dim]; }

  bool border_dim(dim_t dim, RegionPos pos) const {
    if(pos == RegionPos::PRE) {
      return _env_reg_info->region_borders[dim].first;
    }

    return _env_reg_info->region_borders[dim].second;
  }

  iterator begin() const { return _beg; }

  iterator end() const { return _end; }

private:
  const RegionSpec_t* _region_spec;
  ViewSpec_t          _view;
  GlobMemT*           _globmem;
  const PatternT*     _pattern;
  const EnvRegInfo_t* _env_reg_info;
  iterator            _beg;
  iterator            _end;
};  // Region

template <typename ElementT, typename PatternT, typename GlobMemT>
std::ostream& operator<<(std::ostream&                     os,
                         const Region<ElementT, PatternT, GlobMemT>& region) {
  os << "dash::halo::Region<" << typeid(ElementT).name() << ">"
     << "( view: " << region.view() << "; region spec: " << region.spec()
     << "; env_reg_info: {";
  const auto& border = region.border();
  for(auto d = 0; d < border.size(); ++d) {
    if(d > 0) {
      os << ",";
    }
    os << "(" << border[d].first << border[d].second  << ")";
  }
  os << "}"
     << "; is border: " << region.is_border_region()
     << "; is custom: " << region.is_custom_region();
     //<< "; begin iterator: " << region.begin()
     //<< "; end iterator: " << region.begin() << ")";

  return os;
}

}  // namespace halo

}  // namespace dash

#endif // DASH__HALO_REGION_H