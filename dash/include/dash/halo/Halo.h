#ifndef DASH__HALO__HALO_H__
#define DASH__HALO__HALO_H__

#include <dash/iterator/GlobIter.h>

#include <dash/internal/Logging.h>
#include <dash/util/FunctionalExpr.h>

#include <functional>

namespace dash {

namespace halo {

/**
 * Stencil point with raletive coordinates for N dimensions
 * e.g. StencilPoint<2>(-1,-1) -> north west
 */
template <dim_t NumDimensions, typename CoeffT = double>
class StencilPoint : public Dimensional<int16_t, NumDimensions> {
public:
  using point_value_t = int16_t;
  using coefficient_t = CoeffT;

private:
  using Base_t = Dimensional<point_value_t, NumDimensions>;

public:
  // TODO constexpr
  /**
   * Default Contructor
   *
   * All stencil point values are 0 and default coefficient = 1.0.
   */
  StencilPoint() {
    for(dim_t i(0); i < NumDimensions; ++i) {
      this->_values[i] = 0;
    }
  }

  /**
   * Constructor
   *
   * Custom stencil point values for all dimensions and default
   * coefficient = 1.0.
   */
  template <typename... Values>
  constexpr StencilPoint(
    typename std::enable_if<sizeof...(Values) == NumDimensions - 1,
                            point_value_t>::type value,
    Values... values)
  : Base_t::Dimensional(value, (point_value_t) values...) {}

  /**
   * Constructor
   *
   * Custom values and custom coefficient.
   */
  template <typename... Values>
  constexpr StencilPoint(
    typename std::enable_if<sizeof...(Values) == NumDimensions - 1,
                            CoeffT>::type coefficient,
    point_value_t                         value, Values... values)
  : Base_t::Dimensional(value, (point_value_t) values...),
    _coefficient(coefficient) {}

  // TODO as constexpr
  /**
   * Returns maximum distance to center over all dimensions
   */
  int max() const {
    int max = 0;
    for(dim_t i(0); i < NumDimensions; ++i)
      max = std::max(max, (int) std::abs(this->_values[i]));
    return max;
  }

  /**
   * Returns coordinates adjusted by stencil point
   */
  template <typename ElementCoordsT>
  ElementCoordsT stencil_coords(ElementCoordsT& coords) const {
    return StencilPoint<NumDimensions, CoeffT>::stencil_coords(coords, this);
  }

  /**
   * Returns coordinates adjusted by a given stencil point
   */
  template <typename ElementCoordsT>
  static ElementCoordsT stencil_coords(
    ElementCoordsT                             coords,
    const StencilPoint<NumDimensions, CoeffT>& stencilp) {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      coords[d] += stencilp[d];
    }

    return coords;
  }

  /**
   * Returns coordinates adjusted by a stencil point and a boolean to indicate
   * a if the adjusted coordinate points to elements out of the given
   * \ref ViewSpecpossible (inside: true, else: false).
   */
  template <typename ElementCoordsT, typename ViewSpecT>
  std::pair<ElementCoordsT, bool> stencil_coords_check(
    ElementCoordsT coords, const ViewSpecT& view) const {
    bool halo = false;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      coords[d] += this->_values[d];
      if(coords[d] < view.offset(d) || coords[d] >= view.offset(d) + view.extent(d))
        halo = true;
    }

    return std::make_pair(coords, halo);
  }

  /**
   * Returns coordinates adjusted by a stencil point and a boolean to indicate
   * a if the adjusted coordinate points to elements out of the given
   * \ref ViewSpec:  possible (inside: true, else: false).
   * If one dimension points to an element outside the \ref ViewSpec this method
   * returns immediately the unfinished adjusted coordinate and true. Otherwise
   * the adjusted coordinate and false is returned,
   */
  template <typename ElementCoordsT, typename ViewSpecT>
  std::pair<ElementCoordsT, bool> stencil_coords_check_abort(
    ElementCoordsT coords, const ViewSpecT& view) const {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      coords[d] += this->_values[d];
      if(coords[d] < view.offset(d) || coords[d] >= view.offset(d) + view.extent(d))
        return std::make_pair(coords, true);
    }

    return std::make_pair(coords, false);
  }

  /**
   * Returns the coefficient for this stencil point
   */
  CoeffT coefficient() const { return _coefficient; }

private:
  CoeffT _coefficient = 1.0;
};  // StencilPoint

template <dim_t NumDimensions, typename CoeffT>
std::ostream& operator<<(
  std::ostream& os, const StencilPoint<NumDimensions, CoeffT>& stencil_point) {
  os << "dash::halo::StencilPoint<" << NumDimensions << ">"
     << "(coefficient = " << stencil_point.coefficient() << " - points: ";
  for(auto d = 0; d < NumDimensions; ++d) {
    if(d > 0) {
      os << ",";
    }
    os << stencil_point[d];
  }
  os << ")";

  return os;
}

/**
 * A collection of stencil points (\ref Stencil)
 * e.g. StencilSpec<dash::StencilPoint<2>, 2,2>({StencilPoint<2>(-1,0),
 * StencilPoint<2>(1,0)}) -> north and south
 */
template <typename StencilPointT, std::size_t NumStencilPoints>
class StencilSpec {
private:
  using Self_t = StencilSpec<StencilPointT, NumStencilPoints>;
  static constexpr auto NumDimensions = StencilPointT::ndim();

public:
  using stencil_size_t   = std::size_t;
  using stencil_index_t  = std::size_t;
  using StencilArray_t   = std::array<StencilPointT, NumStencilPoints>;
  using StencilPoint_t   = StencilPointT;
  using point_value_t    = typename StencilPoint_t::point_value_t;
  using DistanceDim_t = std::pair<point_value_t, point_value_t>;
  using DistanceAll_t = std::array<DistanceDim_t, NumDimensions>;

public:
  /**
   * Constructor
   *
   * Takes a list of \ref StencilPoint
   */
  constexpr StencilSpec(const StencilArray_t& specs) : _specs(specs) {}

  /**
   * Constructor
   *
   * Takes all given \ref StencilPoint. The number of arguments has to be the
   * same as the given number of stencil points via the template argument.
   */
  template <typename... Values>
  constexpr StencilSpec(const StencilPointT& value, const Values&... values)
  : _specs{ { value, (StencilPointT) values... } } {
    static_assert(sizeof...(values) == NumStencilPoints - 1,
                  "Invalid number of stencil point arguments");
  }

  // TODO constexpr
  /**
   * Copy Constructor
   */
  StencilSpec(const Self_t& other) { _specs = other._specs; }

  /**
   * \return container storing all stencil points
   */
  constexpr const StencilArray_t& specs() const { return _specs; }

  /**
   * \return number of stencil points
   */
  static constexpr stencil_size_t num_stencil_points() {
    return NumStencilPoints;
  }

  /**
   * Returns the stencil point index for a given \ref StencilPoint
   *
   * \return The index and true if the given stecil point was found,
   *         else the index 0 and false.
   *         Keep in mind that index 0 is only a valid index, if the returned
   *         bool is true
   */
  const std::pair<stencil_index_t, bool> index(StencilPointT stencil) const {
    for(auto i = 0; i < _specs.size(); ++i) {
      if(_specs[i] == stencil)
        return std::make_pair(i, true);
    }

    return std::make_pair(0, false);
  }

  /**
   * Returns the minimal and maximal distances of all stencil points for all
   * dimensions.
   */
  DistanceAll_t minmax_distances() const {
    DistanceAll_t max_dist{};
    for(const auto& stencil_point : _specs) {
      for(auto d = 0; d < NumDimensions; ++d) {
        if(stencil_point[d] < max_dist[d].first) {
          max_dist[d].first = stencil_point[d];
          continue;
        }
        if(stencil_point[d] > max_dist[d].second)
          max_dist[d].second = stencil_point[d];
      }
    }

    return max_dist;
  }

  /**
   * Returns the minimal and maximal distances of all stencil points for the
   * given dimension.
   */
  DistanceDim_t minmax_distances(dim_t dim) const {
    DistanceDim_t max_dist{};
    for(const auto& stencil_point : _specs) {
      if(stencil_point[dim] < max_dist.first) {
        max_dist.first = stencil_point[dim];
        continue;
      }
      if(stencil_point[dim] > max_dist.second)
        max_dist.second = stencil_point[dim];
    }

    return max_dist;
  }
  /**
   * \return stencil point for a given index
   */
  constexpr const StencilPointT& operator[](stencil_index_t index) const {
    return _specs[index];
  }

private:
  StencilArray_t _specs{};
};  // StencilSpec

template <typename StencilPointT, std::size_t NumStencilPoints>
std::ostream& operator<<(
  std::ostream& os, const StencilSpec<StencilPointT, NumStencilPoints>& specs) {
  os << "dash::halo::StencilSpec<" << NumStencilPoints << ">"
     << "(";
  for(auto i = 0; i < NumStencilPoints; ++i) {
    if(i > 0) {
      os << ",";
    }
    os << specs[i];
  }
  os << ")";

  return os;
}

/**
 * Global boundary Halo properties
 */
enum class BoundaryProp : uint8_t {
  /// No global boundary Halos
  NONE,
  /// Global boundary Halos with values from the opposite boundary
  CYCLIC,
  /// Global boundary Halos with predefined custom values
  CUSTOM
};

inline std::ostream& operator<<(std::ostream& os, const BoundaryProp& prop) {
  if(prop == BoundaryProp::NONE)
    os << "NONE";
  else if(prop == BoundaryProp::CYCLIC)
    os << "CYCLIC";
  else
    os << "CUSTOM";

  return os;
}

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
 * Position of a \ref Region in one dimension relating to the center
 */
enum class RegionPos : bool {
  /// Region before center
  PRE,
  /// Region behind center
  POST
};

inline std::ostream& operator<<(std::ostream& os, const RegionPos& pos) {
  if(pos == RegionPos::PRE)
    os << "PRE";
  else
    os << "POST";

  return os;
}

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
class RegionCoords : public Dimensional<uint8_t, NumDimensions> {
private:
  using Self_t = RegionCoords<NumDimensions>;
  using Base_t = Dimensional<uint8_t, NumDimensions>;
  using udim_t = std::make_unsigned<dim_t>::type;

public:
  using region_coord_t = uint8_t;
  using region_index_t = uint32_t;
  using region_size_t  = uint32_t;
  using Coords_t       = std::array<uint8_t, NumDimensions>;
  using CoordsVec_t    = std::vector<Self_t>;
  using RegIndDepVec_t = std::vector<region_index_t>;
  using RegIndexDim_t  = std::pair<region_index_t, region_index_t>;

  /// index calculation base - 3^N regions for N-Dimensions
  static constexpr uint8_t REGION_INDEX_BASE = 3;

  /// number of maximal possible regions
  static constexpr auto NumRegionsMax =
    ce::pow(REGION_INDEX_BASE, static_cast<udim_t>(NumDimensions));

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
    return NumRegionsMax / 2;
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
      // in case a wrong region coordinate was set
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
    region_index_t index_tmp = static_cast<long>(index);
    for(auto i = (NumDimensions - 1); i >= 1; --i) {
      auto res  = std::div(index_tmp, REGION_INDEX_BASE);
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

  /**
   * Returns the highest dimension with region values != 1
   */
  dim_t relevant_dim() { return relevant_dim(this->_values); }

  /**
   * returns the number of coordinates unequal to the center (1) for
   * all dimensions
   *
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

    if(index >= NumRegionsMax) {
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
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using region_extent_t = uint16_t;
  using region_coord_t  = typename RegionCoords_t::region_coord_t;

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
        index *= RegionCoords_t::REGION_INDEX_BASE;
      else if(stencil[d] == 0)
        index = 1 + index * RegionCoords_t::REGION_INDEX_BASE;
      else
        index = 2 + index * RegionCoords_t::REGION_INDEX_BASE;
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
 * Contains all specified Halo regions. HaloSpec can be build with
 * \ref StencilSpec.
 */
template <dim_t NumDimensions>
class HaloSpec {
private:
  using Self_t         = HaloSpec<NumDimensions>;
  using RegionCoords_t = RegionCoords<NumDimensions>;

public:
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Specs_t         = std::array<RegionSpec_t, RegionCoords_t::NumRegionsMax>;
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using region_size_t   = typename RegionCoords_t::region_index_t;
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
      _specs[spec.index()] = spec;
      ++_num_regions;
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

    if(_specs[index].extent() == 0)
      ++_num_regions;

    auto max = stencil.max();
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

public:
  using ViewSpec_t      = typename HaloBlockT::ViewSpec_t;
  using GlobalBndSpec_t = GlobalBoundarySpec<NumDimensions>;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Views_t         = std::array<ViewSpec_t, RegionCoords_t::NumRegionsMax>;
  using region_index_t  = typename RegionCoords_t::region_index_t;
  using region_size_t   = typename RegionCoords_t::region_index_t;
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
  constexpr region_size_t num_regions() const { return RegionCoords_t::NumRegionsMax; }

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
    for(auto r = 0;  r < RegionCoords_t::NumRegionsMax; ++r) {
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
  using BorderMeta_t   = std::pair<bool, bool>;
  using Border_t       = std::array<BorderMeta_t, NumDimensions>;
  using region_index_t = typename RegionSpec_t::region_index_t;
  using pattern_size_t = typename PatternT::size_type;

public:
  Region(const RegionSpec_t& region_spec, const ViewSpec_t& view,
         GlobMem_t& globmem, const PatternT& pattern, const Border_t& border,
         bool custom_region)
  : _region_spec(&region_spec), _view(view),
    _globmem(&globmem), _pattern(&pattern), _border(border),
    _border_region(
      std::any_of(border.begin(), border.end(),
                  [](BorderMeta_t border_dim) {
                    return border_dim.first == true ||
                           border_dim.second == true; })),
    _custom_region(custom_region),
    _beg(&globmem, *_pattern, _view, 0),
    _end(&globmem, *_pattern, _view, _view.size()) {
  }

  Region(const Self_t& other)
  : _region_spec(other._region_spec),
    _view     (other._view),
    _globmem(other._globmem),
    _pattern(other._pattern),
    _border(other._border),
    _border_region(other._border_region),
    _custom_region(other._custom_region),
    _beg(_globmem, *_pattern, _view, 0),
    _end(_globmem, *_pattern, _view, _view.size()) {
  }

  Region(const Self_t&& other)
  : _region_spec(std::move(other._region_spec)),
    _view     (std::move(other._view)),
    _globmem(std::move(other._globmem)),
    _pattern(std::move(other._pattern)),
    _border(std::move(other._border)),
    _border_region(std::move(other._border_region)),
    _custom_region(std::move(other._custom_region)),
    _beg(_globmem, *_pattern, _view, 0),
    _end(_globmem, *_pattern, _view, _view.size()) {
  }

  Self_t& operator=(const Self_t& other) {
    _region_spec = other._region_spec;
    _view      = other._view;
    _globmem = other._globmem;
    _pattern = other._pattern;
    _border = other._border;
    _border_region = other._border_region;
    _custom_region = other._custom_region;
    _beg = iterator(_globmem, *_pattern, _view, 0);
    _end = iterator(_globmem, *_pattern, _view, _view.size());

    return *this;
  }

  Self_t& operator=(const Self_t&& other) {
    _region_spec = std::move(other._region_spec);
    _view      = std::move(other._view);
    _globmem = std::move(other._globmem);
    _pattern = std::move(other._pattern);
    _border = std::move(other._border);
    _border_region = std::move(other._border_region);
    _custom_region = std::move(other._custom_region);
    _beg = iterator(_globmem, *_pattern, _view, 0);
    _end = iterator(_globmem, *_pattern, _view, _view.size());

    return *this;
  }

  const region_index_t index() const { return _region_spec->index(); }

  const RegionSpec_t& spec() const { return *_region_spec; }

  const ViewSpec_t& view() const { return _view; }

  pattern_size_t size() const { return _view.size(); }

  const Border_t& border() const { return _border; }

  bool is_border_region() const { return _border_region; };

  bool is_custom_region() const { return _custom_region; };


  /**
   * Returns a pair of two booleans for a given dimension.
   * In case the region is the global border in this dimension
   * the value is true, otherwise false
   * first -> Pre center position; second -> Post center position
   */
  BorderMeta_t border_dim(dim_t dim) const { return _border[dim]; }

  bool border_dim(dim_t dim, RegionPos pos) const {
    if(pos == RegionPos::PRE) {
      return _border[dim].first;
    }

    return _border[dim].second;
  }

  iterator begin() const { return _beg; }

  iterator end() const { return _end; }

private:
  const RegionSpec_t* _region_spec;
  ViewSpec_t          _view;
  GlobMemT*     _globmem;
  const PatternT*     _pattern;
  Border_t            _border;
  bool                _border_region;
  bool                _custom_region;
  iterator            _beg;
  iterator            _end;
};  // Region

template <typename ElementT, typename PatternT, typename GlobMemT>
std::ostream& operator<<(std::ostream&                     os,
                         const Region<ElementT, PatternT, GlobMemT>& region) {
  os << "dash::halo::Region<" << typeid(ElementT).name() << ">"
     << "( view: " << region.view() << "; region spec: " << region.spec()
     << "; border regions: {";
  const auto& border = region.border();
  for(auto d = 0; d < border.size(); ++d) {
    if(d == 0)
      os << "(" << border[d].first << border[d].first  << ")";
    else
      os << ",(" << border[d].first << border[d].first  << ")";
  }
  os << "}"
     << "; is border: " << region.is_border_region()
     << "; is custom: " << region.is_custom_region()
     << "; begin iterator: " << region.begin()
     << "; end iterator: " << region.begin() << ")";

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
  using region_index_t  = typename RegionSpec_t::region_index_t;
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
          if(check_extent > _pattern.extent(d)) {
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
  const PatternT& pattern() const { return _pattern; }

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
   * Returns the inner view with global offsets depending on the used
   * \ref HaloSpec.
   */
  const ViewSpec_t& view_inner() const { return _view_inner; }

  /**
   * Returns a set of local views that contains all boundary elements.
   * No duplicates of elements included.
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
   * Returns the index belonging to the given coordinates and \ref ViewSpec
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
        if(check_extent_tmp > _pattern.extent(d_tmp))
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

  std::array<Region_t*, RegionCoords_t::NumRegionsMax> _halo_reg_mapping{};

  RegionVector_t _boundary_regions;

  std::array<Region_t*, RegionCoords_t::NumRegionsMax> _boundary_reg_mapping{};

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

/**
 * Mangages the memory for all halo regions provided by the given
 * \ref HaloBlock
 */
template <typename HaloBlockT>
class HaloMemory {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();

  using RegionCoords_t = RegionCoords<NumDimensions>;
  using Pattern_t      = typename HaloBlockT::Pattern_t;

  static constexpr auto NumRegionsMax      = RegionCoords_t::NumRegionsMax;
  static constexpr auto MemoryArrange = Pattern_t::memory_order();

public:
  using Element_t = typename HaloBlockT::Element_t;
  using ElementCoords_t =
    std::array<typename Pattern_t::index_type, NumDimensions>;
  using HaloBuffer_t   = std::vector<Element_t>;
  using region_index_t = typename RegionCoords_t::region_index_t;
  using pattern_size_t = typename Pattern_t::size_type;

  using iterator       = typename HaloBuffer_t::iterator;
  using const_iterator = const iterator;

  using MemRange_t = std::pair<iterator, iterator>;

public:
  /**
   * Constructor
   */
  HaloMemory(const HaloBlockT& haloblock) : _haloblock(haloblock) {
    _halobuffer.resize(haloblock.halo_size());
    auto it = _halobuffer.begin();
    std::fill(_halo_offsets.begin(), _halo_offsets.end(), _halobuffer.end());
    for(const auto& region : haloblock.halo_regions()) {
      _halo_offsets[region.index()] = it;
      it += region.size();
    }
  }

  /**
   * Iterator to the first halo element for the given region index
   * \param index halo region index
   * \return Iterator to the first halo element. If no region exists the
   *         end iterator will be returned.
   */
  iterator first_element_at(region_index_t index) {
    return _halo_offsets[index];
  }

  /**
   * Returns the range of all halo elements for the given region index.
   * \param index halo region index
   * \return Pair of iterator. First points ot the beginning and second to the
   *         end.
   */
  MemRange_t range_at(region_index_t index) {
    auto it = _halo_offsets[index];
    if(it == _halobuffer.end())
      return std::make_pair(it, it);

    auto* region = _haloblock.halo_region(index);

    DASH_ASSERT_MSG(
      region != nullptr,
      "HaloMemory manages memory for a region that seemed to be empty.");

    return std::make_pair(it, it + region->size());
  }

  /**
   * Returns an iterator to the first halo element
   */
  iterator begin() { return _halobuffer.begin(); }

  /**
   * Returns a const iterator to the first halo element
   */
  const_iterator begin() const { return _halobuffer.begin(); }

  /**
   * Returns an iterator to the end of the halo elements
   */
  iterator end() { return _halobuffer.end(); }

  /**
   * Returns a const iterator to the end of the halo elements
   */
  const_iterator end() const { return _halobuffer.end(); }

  /**
   * Container storing all halo elements
   *
   * \return Reference to the container storing all halo elements
   */
  const HaloBuffer_t& buffer() const { return _halobuffer; }

  /**
   * Converts coordinates to halo memory coordinates for a given
   * region index and returns true if the coordinates are valid and
   * false if not.
   */
  bool to_halo_mem_coords_check(const region_index_t region_index,
                                ElementCoords_t&     coords) const {
    const auto& extents =
      _haloblock.halo_region(region_index)->view().extents();
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

  /**
   * Converts coordinates to halo memory coordinates for a given region index.
   */
  void to_halo_mem_coords(const region_index_t region_index,
                          ElementCoords_t&     coords) const {
    const auto& extents =
      _haloblock.halo_region(region_index)->view().extents();
    for(auto d = 0; d < NumDimensions; ++d) {
      if(coords[d] < 0) {
        coords[d] += extents[d];
        continue;
      }

      if(coords[d] >= _haloblock.view().extent(d))
        coords[d] -= _haloblock.view().extent(d);
    }
  }

  /*
   * Retuns the offset for a given region index and coordinates within the
   * region.
   */
  pattern_size_t offset(const region_index_t   region_index,
                        const ElementCoords_t& coords) const {
    const auto& extents =
      _haloblock.halo_region(region_index)->view().extents();
    pattern_size_t off = 0;
    if(MemoryArrange == ROW_MAJOR) {
      off = coords[0];
      for(dim_t d = 1; d < NumDimensions; ++d)
        off = off * extents[d] + coords[d];
    } else {
      off = coords[NumDimensions - 1];
      for(dim_t d = NumDimensions - 1; d > 0;) {
        --d;
        off = off * extents[d] + coords[d];
      }
    }

    return off;
  }

private:
  const HaloBlockT&              _haloblock;
  HaloBuffer_t                   _halobuffer;
  std::array<iterator, NumRegionsMax> _halo_offsets{};
};  // class HaloMemory

template <typename HaloBlockT>
class HaloPackBuffer {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();

  using RegionCoords_t = RegionCoords<NumDimensions>;
  using Team_t         = dash::Team;
  using Pattern_t      = typename HaloBlockT::Pattern_t;
  using signed_pattern_size_t =
    typename std::make_signed<typename Pattern_t::size_type>::type;
  using ViewSpec_t     = typename Pattern_t::viewspec_type;


  static constexpr auto NumRegionsMax = RegionCoords_t::NumRegionsMax;
  static constexpr auto MemoryArrange = Pattern_t::memory_order();
  // value not related to array index
  static constexpr auto FastestDim    =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;
  static constexpr auto ContiguousDim    =
    MemoryArrange == ROW_MAJOR ? 1 : NumDimensions;

public:
  using Element_t = typename HaloBlockT::Element_t;
  using ElementCoords_t =
    std::array<typename Pattern_t::index_type, NumDimensions>;
  using HaloBuffer_t   = dash::Array<Element_t>;
  using HaloSignalBuffer_t   = dash::Array<bool>;
  using region_index_t = typename RegionCoords_t::region_index_t;
  using pattern_size_t = typename Pattern_t::size_type;

  //using iterator       = typename HaloBuffer_t::iterator;
  //using const_iterator = const iterator;

  //using MemRange_t = std::pair<iterator, iterator>;

public:
  /**
   * Constructor
   */
  HaloPackBuffer(const HaloBlockT& halo_block, Element_t* local_memory, Team_t& team)
  : _halo_block(halo_block),
    _local_memory(local_memory),
    _num_halo_elems(num_halo_elems()),
    _halo_buffer(_num_halo_elems * team.size(), team),
    _signal_buffer(NumRegionsMax * team.size(), team) {

    init_block_data();
    for(auto& signal : _signal_buffer.local) {
      signal = 0;
    }

    _signal = 1;
  }

  void pack() {
    region_index_t handle_pos = 0;
    for(auto r = 0; r < NumRegionsMax; ++r) {
      const auto& update_data = _halo_update_data[r];
      if(!update_data.put_data.needs_signal) {
        continue;
      }

      if(update_data.pack_data.needs_packing) {
        auto buffer_offset = _halo_buffer.lbegin() + update_data.pack_data.buffer_offset;
        for(auto& offset : update_data.pack_data.block_offs) {
          auto block_begin = _local_memory + offset;
          std::copy(block_begin, block_begin + update_data.pack_data.block_len, buffer_offset);
          buffer_offset += update_data.pack_data.block_len;
        }
      }

      dash::internal::put_handle(update_data.put_data.signal_gptr, &_signal, 1, &_signal_handles[handle_pos]);
      ++handle_pos;
    }

    dart_waitall_local(_signal_handles.data(), _signal_handles.size());
  }

  void pack(region_index_t region_index) {
    const auto& update_data = _halo_update_data[region_index];
    if(!update_data.needs_signal) {
      return;
    }

    if(update_data.needs_packing) {
      auto buffer_offset = _halo_buffer.lbegin() + update_data.halo_offset;
      for(auto& block : update_data.block_data) {
        auto block_begin = _local_memory + block.offset;
        std::copy(block_begin, block_begin + block.blength, buffer_offset);
        buffer_offset += block.blength;
      }
    }

    dash::internal::put_blocking(update_data.neighbor_signal, &_signal, 1);
  }

  dart_gptr_t buffer_region(region_index_t region_index) {
    return _halo_update_data[region_index].get_data.halo_gptr;
  }

  void update_ready(region_index_t region_index) {
    auto& get_data = _halo_update_data[region_index].get_data;
    if(!get_data.awaits_signal) {
      return;
    }


    bool signal = false;
    while(!signal) {
      dash::internal::get_blocking(get_data.signal_gptr, &signal, 1);
    }
    _signal_buffer.lbegin()[region_index] = 0;
  }

  void print_block_data() {
    std::cout << "BlockData:\n";
    for(auto r = 0; r < NumRegionsMax; ++r) {
      std::cout << "region [" << r << "] {";
      for(auto& offset : _halo_update_data[r].pack_data.block_offs) {
        std::cout << " (" << offset << "," << _halo_update_data[r].pack_data.block_len << ")";
      }
      std::cout << " }\n";
    }
    std::cout << std::endl;
  }

  void print_buffer_data() {
    std::cout << "bufferData: { ";
    for(auto& elem : _halo_buffer.local) {
      std::cout << elem << ",";
    }
    std::cout << " }" << std::endl;
  }

  void print_signal_data() {
    std::cout << "signalData: { ";
    for(auto& elem : _signal_buffer.local) {
      std::cout << elem << ",";
    }
    std::cout << " }" << std::endl;
  }

  void print_pack_data() {
    for(auto r = 0; r < NumRegionsMax; ++r) {
      auto& data = _halo_update_data[r];
      std::cout << "Halo Update Data (" << r << ")\n"
                << "  Get Data:\n"
                << "    awaits signal: " << data.get_data.awaits_signal << "\n"
                << "    signal gptr: " << " uid: " << data.get_data.signal_gptr.unitid << " off: " << data.get_data.signal_gptr.addr_or_offs.offset << "\n"
                << "    halo gptr: " << " uid: " << data.get_data.halo_gptr.unitid << " off: " << data.get_data.halo_gptr.addr_or_offs.offset << "\n"
                << "  Put Data:\n"
                << "    needs signal: " << data.put_data.needs_signal << "\n"
                << "    halo gptr: " << " uid: " << data.put_data.signal_gptr.unitid << " off: " << data.put_data.signal_gptr.addr_or_offs.offset << "\n"
                << "  Pack Data:\n"
                << "    needs packed: " << data.pack_data.needs_packing << "\n"
                << "    halo offset buffer: " << data.pack_data.buffer_offset << "\n"
                << "    block length: " << data.pack_data.block_len << "\n";
      std::cout << "    Block Offsets: { ";
      for(auto& offset : data.pack_data.block_offs) {
        std::cout << offset << " ";
      }
      std::cout << " }\n";
    }
    std::cout << std::endl;
  }

  void print_pack_data(region_index_t reg) {
    auto& data = _halo_update_data[reg];
    std::cout << "Halo Update Data (" << reg << ")\n"
              << "  Get Data:\n"
              << "    awaits signal: " << data.get_data.awaits_signal << "\n"
              << "    signal gptr: " << " uid: " << data.get_data.signal_gptr.unitid << " off: " << data.get_data.signal_gptr.addr_or_offs.offset << "\n"
              << "    halo gptr: " << " uid: " << data.get_data.halo_gptr.unitid << " off: " << data.get_data.halo_gptr.addr_or_offs.offset << "\n"
              << "  Put Data:\n"
              << "    needs signal: " << data.put_data.needs_signal << "\n"
              << "    halo gptr: " << " uid: " << data.put_data.signal_gptr.unitid << " off: " << data.put_data.signal_gptr.addr_or_offs.offset << "\n"
              << "  Pack Data:\n"
              << "    needs packed: " << data.pack_data.needs_packing << "\n"
              << "    halo offset buffer: " << data.put_data.buffer_offset << "\n"
              << "    block length: " << data.pack_data.block_len << "\n";
    std::cout << "    Block Offsets: { ";
    for(auto& offset : data.pack_data.block_offs) {
      std::cout << offset << " ";
    }
    std::cout << std::endl;
  }

  void print_gptr(dart_gptr_t gptr, region_index_t reg, const char* location) {
    std::cout << "[" << dash::myid() << "] loc: " << location << " reg: " << reg << " uid: " << gptr.unitid << " off: " << gptr.addr_or_offs.offset << std::endl;
  }
private:
  struct GetData {
    bool        awaits_signal{false};
    dart_gptr_t signal_gptr{DART_GPTR_NULL};
    dart_gptr_t halo_gptr{DART_GPTR_NULL};
  };

  struct PutData {
    bool        needs_signal{false};
    dart_gptr_t signal_gptr{DART_GPTR_NULL};
  };

  struct PackData{
    bool                        needs_packing{false};
    std::vector<pattern_size_t> block_offs{};
    pattern_size_t              block_len{0};
    signed_pattern_size_t       buffer_offset{-1};
  };

  struct HaloUpdateData {
    PackData pack_data;
    PutData  put_data;
    GetData  get_data;
  };

  pattern_size_t num_halo_elems() {
    const auto& halo_spec = _halo_block.halo_spec();
    const auto& view_local = _halo_block.view_local();
    team_unit_t rank_0(0);
    auto max_local_extents = _halo_block.pattern().local_extents(rank_0);

    pattern_size_t num_halo_elems = 0;
    for(auto r = 0; r < NumRegionsMax; ++r) {
      const auto& region_spec = halo_spec.spec(r);
      auto& pack_data = _halo_update_data[r].pack_data;
      if(region_spec.extent() == 0 ||
         (region_spec.level() == 1 && region_spec.relevant_dim() == ContiguousDim)) {
        pack_data.buffer_offset = -1;
        continue;
      }

      pattern_size_t reg_size = 1;
      for(auto d = 0; d < NumDimensions; ++d) {
        if(region_spec[d] != 1) {
          reg_size *= region_spec.extent();
        } else {
          reg_size *= max_local_extents[d];
        }
      }
      pack_data.buffer_offset = num_halo_elems;
      num_halo_elems += reg_size;
    }

    return num_halo_elems;
  }

  void init_block_data() {
    for(auto r = 0; r < NumRegionsMax; ++r) {
      auto region = _halo_block.halo_region(r);
      if(region == nullptr || region->size() == 0) {
        continue;
      }

      auto remote_region_index = NumRegionsMax - 1 - r;
      auto& update_data_loc = _halo_update_data[r];
      auto& update_data_rem = _halo_update_data[remote_region_index];

      update_data_rem.put_data.needs_signal = true;
      update_data_loc.get_data.awaits_signal = true;
      _signal_handles.push_back(nullptr);

      auto signal_gptr = _signal_buffer.begin().dart_gptr();
      // sets local signal gptr -> necessary for dart_get
      update_data_loc.get_data.signal_gptr = signal_gptr;
      update_data_loc.get_data.signal_gptr.unitid = _halo_block.pattern().team().myid();
      update_data_loc.get_data.signal_gptr.addr_or_offs.offset = r * sizeof(bool);

      // sets remote neighbor signal gptr -> necessary for dart_put
      auto neighbor_id = region->begin().dart_gptr().unitid;
      update_data_rem.put_data.signal_gptr = signal_gptr;
      update_data_rem.put_data.signal_gptr.unitid = neighbor_id;
      update_data_rem.put_data.signal_gptr.addr_or_offs.offset = remote_region_index * sizeof(bool);

      // Halo elements can be updated with one request
      if(region->spec().relevant_dim() == ContiguousDim && region->spec().level() == 1) {
        update_data_loc.get_data.halo_gptr = region->begin().dart_gptr();
        continue;
      }

      update_data_loc.get_data.halo_gptr = _halo_buffer.begin().dart_gptr();
      update_data_loc.get_data.halo_gptr.unitid = neighbor_id;
      update_data_loc.get_data.halo_gptr.addr_or_offs.offset = update_data_loc.pack_data.buffer_offset * sizeof(Element_t);

      // Setting all packing data
      // no packing needed -> all elements are contiguous
      const auto& reg_spec = _halo_block.halo_spec().spec(remote_region_index);
      if(reg_spec.extent() == 0 ||
         (reg_spec.relevant_dim() == ContiguousDim && reg_spec.level() == 1)) {
        update_data_rem.pack_data.buffer_offset = -1;
        continue;
      }

      auto& pack_data = update_data_rem.pack_data;
      pack_data.needs_packing = true;


      const auto& view_glob = _halo_block.view();
      auto reg_extents = view_glob.extents();
      auto reg_offsets = view_glob.offsets();

      for(dim_t d = 0; d < NumDimensions; ++d) {
        if(reg_spec[d] == 1) {
          continue;
        }

        reg_extents[d] = reg_spec.extent();
        if(reg_spec[d] == 0) {
          reg_offsets[d] += view_glob.extent(d) - reg_extents[d];
        } else {
          reg_offsets[d] = view_glob.offset(d);
        }
      }
      ViewSpec_t view_pack(reg_offsets, reg_extents);

      pattern_size_t num_elems_block = reg_extents[FastestDim];
      pattern_size_t num_blocks      = view_pack.size() / num_elems_block;

      pack_data.block_len = num_elems_block;
      pack_data.block_offs.resize(num_blocks);

      auto it_region = region->begin();
      decltype(it_region) it_pack_data(&(it_region.globmem()), it_region.pattern(), view_pack);
      for(auto& offset : pack_data.block_offs) {
        offset  = it_pack_data.lpos().index;
        it_pack_data += num_elems_block;
      }
    }
  }

private:
  const HaloBlockT&              _halo_block;
  std::vector<dart_handle_t>     _signal_handles;
  bool                           _signal;
  Element_t*                     _local_memory;
  std::array<HaloUpdateData,NumRegionsMax> _halo_update_data;
  pattern_size_t                 _num_halo_elems;
  HaloBuffer_t                   _halo_buffer;
  HaloSignalBuffer_t             _signal_buffer;

};  // class HaloPackBuffer

}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO_HALO_H__
