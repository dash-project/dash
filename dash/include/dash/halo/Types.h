#ifndef DASH__HALO_TYPES_H
#define DASH__HALO_TYPES_H

#include <dash/Types.h>
#include <dash/util/FunctionalExpr.h>
#include <type_traits>

namespace dash {

namespace halo {

namespace internal {

// Stencil point type
using spoint_value_t = int16_t;
using spoint_distance_t = std::make_unsigned_t<spoint_value_t>;


using region_coord_t = uint8_t;
using region_index_t = uint32_t;
using region_size_t  = region_index_t;

using region_extent_t = spoint_distance_t;

/// index calculation base - 3^N regions for N-Dimensions
static constexpr region_index_t REGION_INDEX_BASE = 3;

/// number of maximal possible regions
template<dim_t NumDimensions>
static constexpr region_index_t NumRegionsMax =
    ce::pow(REGION_INDEX_BASE, static_cast<std::make_unsigned<dim_t>::type>(NumDimensions));

/**
 * View property of the StencilIterator
 */
enum class StencilViewScope : std::uint8_t {
  /// inner elements only
  INNER,
  /// Boundary elements only
  BOUNDARY,
  /// Inner and boundary elements
  ALL
};

inline std::ostream& operator<<(std::ostream&           os,
                                const StencilViewScope& scope) {
  if(scope == StencilViewScope::INNER)
    os << "INNER";
  else if(scope == StencilViewScope::BOUNDARY)
    os << "BOUNDARY";
  else
    os << "ALL";

  return os;
}

} // namespace internal

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

enum class SharedType : uint8_t {
  STL, OMP, NONE
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
 * Switch to turn on halo update signaling in both directions
 */
enum class SignalReady : bool {
  /// Region before center
  ON,
  /// Region behind center
  OFF
};

inline std::ostream& operator<<(std::ostream& os, const SignalReady& pos) {
  if(pos == SignalReady::ON)
    os << "ON";
  else
    os << "OFF";

  return os;
}

namespace internal {

template<typename ViewSpecT>
struct BlockViewSpec {
  ViewSpecT inner;
  ViewSpecT inner_bound;
};

/**
 * Region information describing the global border connections and
 * direct neighbor ids
 */
template<typename ViewSpecT>
struct RegionData {
  ViewSpecT view{};
  /// while neighbor_id_from is DART_UNDEFINED_UNIT_ID this flag shows the
  /// status of this region
  bool      valid{false};
};

template<typename ViewSpecT>
std::ostream& operator<<(std::ostream& os, const RegionData<ViewSpecT>& region_data) {
  os << "RegionData(";
  os << "valid_region: " << std::boolalpha << region_data.valid << "; ";
  os << region_data.view << ")";

  return os;
}

/**
 * Region information describing the global border connections and
 * direct neighbor ids
 */
template<typename ViewSpecT>
struct EnvironmentRegionInfo {
  static constexpr auto NumDimensions = ViewSpecT::ndim();

  using PrePostBool_t   = std::pair<bool,bool>;
  using RegionBorders_t = std::array<PrePostBool_t, NumDimensions>;
  using RegionData_t    = RegionData<ViewSpecT>;
  using region_extent_t = std::array<typename ViewSpecT::size_type, NumDimensions>;


  /// neighbor id of the region the halo data got from
  /// if id is DART_UNDEFINED_UNIT_ID, no neighbor is defined
  dart_unit_t     neighbor_id_from{DART_UNDEFINED_UNIT_ID};
  /// neighbor id of the region the halo data need to prepared for
  /// if id is DART_UNDEFINED_UNIT_ID, no neighbor is defined
  dart_unit_t     neighbor_id_to{DART_UNDEFINED_UNIT_ID};
  /// halo extents and validation for halo preparation
  RegionData_t    halo_reg_data{};
  /// defines the \ref BoundaryProp in case this region is a border region
  BoundaryProp    boundary_prop{BoundaryProp::NONE};
  /// defines whether a region is located at the narray global border
  bool            border_region{false};
  /// stores all borders the region is connected to
  /// -> each dimension has to possible border locations pre and post center
  RegionBorders_t region_borders{};
  /// halo extents and validation for halo preparation
  RegionData_t    bnd_reg_data;
};

template<typename ViewSpecT>
std::ostream& operator<<(std::ostream& os, const EnvironmentRegionInfo<ViewSpecT>& env_reg_info) {
  static constexpr auto NumDimensions = ViewSpecT::ndim();

  os << "neighbor_id_from: " << env_reg_info.neighbor_id_from << "; "
     << "neighbor_id_to: " << std::boolalpha << env_reg_info.neighbor_id_to << "; ";
  os << "boundary_prop: ";
     if(env_reg_info.border_region) {
       os << env_reg_info.boundary_prop;
     } else if(env_reg_info.halo_reg_data.valid) {
       os << "INNER";
     } else {
       os << "UNUSED";
     }
  os <<  "; ";
  os << "is border region: " << std::boolalpha << env_reg_info.border_region << ";"
     << "region_borders[" << std::boolalpha;
     for(dim_t d = 0; d < NumDimensions; ++d) {
       os << "(" << std::boolalpha
          << env_reg_info.region_borders[d].first << ","
          << env_reg_info.region_borders[d].second << ") ";
     }

  os << "];";
  os << "halo region: " << env_reg_info.halo_reg_data << "; ";
  os << "boundary region: " << env_reg_info.bnd_reg_data << "; ";

  return os;
}

}  // namespace internal

}  // namespace halo

}  // namespace dash

#endif // DASH__HALO_TYPES_H