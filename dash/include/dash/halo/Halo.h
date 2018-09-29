#ifndef DASH__HALO__HALO_H__
#define DASH__HALO__HALO_H__

#include <dash/iterator/GlobIter.h>
#include <dash/memory/GlobStaticMem.h>

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
     << "(coefficient = " << stencil_point.coefficient << " - points: ";
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
  using MaxDistanceDim_t = std::pair<point_value_t, point_value_t>;
  using MaxDistanceAll_t = std::array<MaxDistanceDim_t, NumDimensions>;

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
  MaxDistanceAll_t minmax_distances() const {
    MaxDistanceAll_t max_dist{};
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
  MaxDistanceDim_t minmax_distances(dim_t dim) const {
    MaxDistanceDim_t max_dist{};
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

  /// index calculation base - 3^N regions for N-Dimensions
  static constexpr uint8_t REGION_INDEX_BASE = 3;

  /// number of maximal possible regions
  static constexpr auto MaxIndex =
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
    _index = index(this->_values);
  }

  /**
   * Constructor allows custom coordinate values and calculates the fitting
   * region index.
   */
  template <typename... Values>
  RegionCoords(region_coord_t value, Values... values)
  : Base_t::Dimensional(value, values...) {
    _index = index(this->_values);
  }

  /**
   * Constructor takes a region index to set up the region coordinates
   */
  RegionCoords(region_index_t index) : _index(index) {
    this->_values = coords(index);
  }

  /**
   * \return region index
   */
  constexpr region_index_t index() const { return _index; }

  /**
   * Returns a region index for a given dimension and \ref RegionPos
   */
  static region_index_t index(dim_t dim, RegionPos pos) {
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
   * Returns the region index for a given \ref RegionCoords
   *
   * \return region index
   */
  static region_index_t index(const Coords_t& coords) {
    region_index_t index = coords[0];
    for(auto i = 1; i < NumDimensions; ++i)
      index = coords[i] + index * REGION_INDEX_BASE;

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

  constexpr bool operator==(const Self_t& other) const {
    return _index == other._index && this->_values == other._values;
  }

  constexpr bool operator!=(const Self_t& other) const {
    return !(*this == other);
  }

private:
  region_index_t _index;
};  // RegionCoords

/**
 * Region specification connecting \ref RegionCoords with an extent.
 * The region extent applies to all dimensions.
 */
template <dim_t NumDimensions>
class RegionSpec : public Dimensional<uint8_t, NumDimensions> {
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
  : _coords(coords), _extent(extent), _rel_dim(init_rel_dim()) {
    init_level();
  }

  /**
   * Constructor using a region index and an extent
   */
  RegionSpec(region_index_t index, const region_extent_t extent)
  : _coords(RegionCoords_t(index)), _extent(extent), _rel_dim(init_rel_dim()) {
    init_level();
  }

  constexpr RegionSpec() = default;

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
  constexpr region_index_t index() const { return _coords.index(); }

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
  // TODO put init_rel_dim and level together
  dim_t init_rel_dim() {
    dim_t dim = 1;
    for(auto d = 1; d < NumDimensions; ++d) {
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
  using Specs_t         = std::array<RegionSpec_t, RegionCoords_t::MaxIndex>;
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
 * Iterator to iterate over all region elements defined by \ref Region
 */
template <typename ElementT, typename PatternT,
          typename PointerT   = GlobPtr<ElementT, PatternT>,
          typename ReferenceT = GlobRef<ElementT>>
class RegionIter {
private:
  using Self_t = RegionIter<ElementT, PatternT, PointerT, ReferenceT>;

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

  using GlobMem_t =
    GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using ViewSpec_t      = typename PatternT::viewspec_type;
  using pattern_index_t = typename PatternT::index_type;
  using pattern_size_t  = typename PatternT::size_type;

public:
  /**
   * Constructor, creates a region iterator.
   */
  RegionIter(GlobMem_t* globmem, const PatternT* pattern,
             const ViewSpec_t& _region_view, pattern_index_t pos,
             pattern_size_t size)
  : _globmem(globmem), _pattern(pattern), _region_view(_region_view), _idx(pos),
    _max_idx(size - 1), _myid(pattern->team().myid()),
    _lbegin(globmem->lbegin()) {}

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
    //TODO dhinf: verify if this is correct
    return *GlobIter<ElementT, PatternT>(_globmem, *_pattern, gpos() + n);
  }

  dart_gptr_t dart_gptr() const { return operator[](_idx).dart_gptr(); }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  bool is_local() const { return (_myid == lpos().unit); }

  GlobIter<ElementT, PatternT> global() const {
    auto g_idx = gpos();
    return GlobIter<ElementT, PatternT>(_globmem, *_pattern, g_idx);
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
    return _pattern->memory_layout().at(glob_coords(_idx));
  }

  std::array<pattern_index_t, NumDimensions> gcoords() const {
    return glob_coords(_idx);
  }

  typename PatternT::local_index_t lpos() const {
    return _pattern->local_index(glob_coords(_idx));
  }

  const ViewSpec_t view() const { return _region_view; }

  inline bool is_relative() const noexcept { return true; }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   *
   * \see DashGlobalIteratorConcept
   */
  const GlobMem_t& globmem() const { return *_globmem; }

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

  const PatternT& pattern() const { return *_pattern; }

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
    return _pattern->memory_layout().coords(idx, _region_view);
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMem_t* _globmem;
  /// Pattern that created the encapsulated block.
  const PatternT* _pattern;

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
  const RegionIter<ElementT, PatternT, PointerT, ReferenceT>& it) {
  os << "dash::halo::RegionIter<" << typeid(ElementT).name() << ">("
     << "; idx: " << it.rpos() << "; view: " << it.view() << ")";

  return os;
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

/**
 * Provides \ref RegionIter and some region metadata like \ref RegionSpec,
 * size etc.
 */
template <typename ElementT, typename PatternT>
class Region {
private:
  static constexpr auto NumDimensions = PatternT::ndim();

public:
  using iterator       = RegionIter<ElementT, PatternT>;
  using const_iterator = const iterator;
  using RegionSpec_t   = RegionSpec<NumDimensions>;
  using GlobMem_t =
    GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
  using ViewSpec_t     = typename PatternT::viewspec_type;
  using Border_t       = std::array<bool, NumDimensions>;
  using region_index_t = typename RegionSpec_t::region_index_t;
  using pattern_size_t = typename PatternT::size_type;

public:
  Region(const RegionSpec_t& region_spec, const ViewSpec_t& region,
         GlobMem_t& globmem, const PatternT& pattern, const Border_t& border,
         bool custom_region)
  : _region_spec(region_spec), _region(region), _border(border),
    _border_region(
      std::any_of(border.begin(), border.end(),
                  [](bool border_dim) { return border_dim == true; })),
    _custom_region(custom_region),
    _beg(&globmem, &pattern, _region, 0, _region.size()),
    _end(&globmem, &pattern, _region, _region.size(), _region.size()) {}

  const region_index_t index() const { return _region_spec.index(); }

  const RegionSpec_t& spec() const { return _region_spec; }

  const ViewSpec_t& view() const { return _region; }

  constexpr pattern_size_t size() const { return _region.size(); }

  constexpr Border_t border() const { return _border; }

  bool is_border_region() const { return _border_region; };

  bool is_custom_region() const { return _custom_region; };

  constexpr bool border_dim(dim_t dim) const { return _border[dim]; }

  iterator begin() const { return _beg; }

  iterator end() const { return _end; }

private:
  const RegionSpec_t _region_spec;
  const ViewSpec_t   _region;
  Border_t           _border;
  bool               _border_region;
  bool               _custom_region;
  iterator           _beg;
  iterator           _end;
};  // Region

template <typename ElementT, typename PatternT>
std::ostream& operator<<(std::ostream&                     os,
                         const Region<ElementT, PatternT>& region) {
  os << "dash::halo::Region<" << typeid(ElementT).name() << ">"
     << "( view: " << region.view() << "; region spec: " << region.spec()
     << "; border regions: {";
  const auto& border = region.border();
  for(auto d = 0; d < border.size(); ++d) {
    if(d == 0)
      os << border[d];
    else
      os << "," << border[d];
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
template <typename ElementT, typename PatternT>
class HaloBlock {
private:
  static constexpr auto NumDimensions = PatternT::ndim();

  using Self_t          = HaloBlock<ElementT, PatternT>;
  using pattern_index_t = typename PatternT::index_type;
  using RegionSpec_t    = RegionSpec<NumDimensions>;
  using Region_t        = Region<ElementT, PatternT>;
  using RegionCoords_t  = RegionCoords<NumDimensions>;
  using region_extent_t = typename RegionSpec_t::region_extent_t;

public:
  using Element_t = ElementT;
  using Pattern_t = PatternT;
  using GlobMem_t =
    GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>;
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

public:
  /**
   * Constructor
   */
  HaloBlock(GlobMem_t& globmem, const PatternT& pattern, const ViewSpec_t& view,
            const HaloSpec_t&      halo_reg_spec,
            const GlobBoundSpec_t& bound_spec = GlobBoundSpec_t{})
  : _globmem(globmem), _pattern(pattern), _view(view),
    _halo_reg_spec(halo_reg_spec), _view_local(_view.extents()) {
    // setup local views
    _view_inner                 = _view_local;
    _view_inner_with_boundaries = _view_local;

    // TODO put functionallity to HaloSpec
    _halo_regions.reserve(_halo_reg_spec.num_regions());
    _boundary_regions.reserve(_halo_reg_spec.num_regions());
    /*
     * Setup for all halo and boundary regions and properties like:
     * is the region a global boundary region and is the region custom or not
     */
    for(const auto& spec : _halo_reg_spec.specs()) {
      auto halo_extent = spec.extent();
      if(!halo_extent)
        continue;

      std::array<bool, NumDimensions> border{};
      bool                            custom_region = false;

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
            border[d] = true;

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
            border[d] = true;

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
                 _globmem, _pattern, border, custom_region));
      auto& region_tmp = _halo_regions.back();
      _size_halo_elems += region_tmp.size();
      _halo_reg_mapping[index] = &region_tmp;
      _boundary_regions.push_back(
        Region_t(spec, ViewSpec_t(bnd_region_offsets, bnd_region_extents),
                 _globmem, _pattern, border, custom_region));
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
      if(bound_spec[d] == BoundaryProp::NONE) {
        auto safe_offset = global_offset;
        auto safe_extent = view_extent;
        if(global_offset < _halo_extents_max[d].first) {
          safe_offset = _halo_extents_max[d].first;
          safe_extent -= _halo_extents_max[d].first - global_offset;
        } else {
          bnd_elem_offsets[d] -= global_offset;
          push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents,
                         _halo_extents_max, bound_spec);
        }
        auto check_extent =
          global_offset + view_extent + _halo_extents_max[d].second;
        if(check_extent > _pattern.extent(d)) {
          safe_extent -= check_extent - _pattern.extent(d);
        } else {
          bnd_elem_offsets[d] += view_extent - _halo_extents_max[d].first;
          bnd_elem_extents[d] = _halo_extents_max[d].second;
          push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents,
                         _halo_extents_max, bound_spec);
        }
        _view_inner_with_boundaries.resize_dim(d, safe_offset - global_offset,
                                               safe_extent);
      } else {
        bnd_elem_offsets[d] -= global_offset;
        push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents, _halo_extents_max,
                       bound_spec);
        bnd_elem_offsets[d] += view_extent - _halo_extents_max[d].first;
        bnd_elem_extents[d] = _halo_extents_max[d].second;
        push_bnd_elems(d, bnd_elem_offsets, bnd_elem_extents, _halo_extents_max,
                       bound_spec);
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

  /**
   * The global memory instance that created the encapsulated block.
   */
  const GlobMem_t& globmem() const { return _globmem; }

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

  ViewSpec_t _view_inner_with_boundaries;

  ViewSpec_t _view_inner;

  RegionVector_t _halo_regions;

  std::array<Region_t*, RegionCoords_t::MaxIndex> _halo_reg_mapping{};

  RegionVector_t _boundary_regions;

  std::array<Region_t*, RegionCoords_t::MaxIndex> _boundary_reg_mapping{};

  BoundaryViews_t _boundary_views;

  pattern_size_t _size_bnd_elems = 0;

  pattern_size_t _size_halo_elems = 0;

  HaloExtsMax_t _halo_extents_max{};
};  // class HaloBlock

template <typename ElementT, typename PatternT>
std::ostream& operator<<(std::ostream&                        os,
                         const HaloBlock<ElementT, PatternT>& haloblock) {
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

  static constexpr auto MaxIndex      = RegionCoords_t::MaxIndex;
  static constexpr auto MemoryArrange = Pattern_t::memory_order();

public:
  using Element_t = typename HaloBlockT::Element_t;
  using ElementCoords_t =
    std::array<typename Pattern_t::index_type, NumDimensions>;
  using HaloBuffer_t = std::vector<Element_t>;
  using region_index_t = typename RegionCoords_t::region_index_t;
  using pattern_size_t = typename Pattern_t::size_type;

  using iterator = typename HaloBuffer_t::iterator;
  using const_iterator = const iterator;

  using MemRange_t = std::pair<iterator,iterator>;

public:
  /**
   * Constructor
   */
  HaloMemory(const HaloBlockT& haloblock) : _haloblock(haloblock) {
    _halobuffer.resize(haloblock.halo_size());
    auto it = _halobuffer.begin();

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
   * iReturns the range of all halo elements for the given region index.
   * \param index halo region index
   * \return Pair of iterator. First points ot the beginning and second to the
   *         end.
   */
  MemRange_t range_at(region_index_t index) {
    auto it = _halo_offsets[index];
    if(it == _halobuffer.end())
      return std::make_pair(it,it);

    auto* region = _haloblock.halo_region(index);

    DASH_ASSERT_MSG(region != nullptr,
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
  const HaloBlockT&      _haloblock;
  HaloBuffer_t           _halobuffer;
  std::array<iterator, MaxIndex> _halo_offsets{};
};  // class HaloMemory

}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO_HALO_H__
