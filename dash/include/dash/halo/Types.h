#ifndef DASH__HALO_TYPES_H
#define DASH__HALO_TYPES_H

#include <dash/Types.h>
#include <dash/util/FunctionalExpr.h>

namespace dash {

namespace halo {

namespace internal {

// Stencil point type
using spoint_value_t = int16_t;


using region_coord_t = uint8_t;
using region_index_t = uint32_t;
using region_size_t  = region_index_t;

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
  
}  // namespace halo

}  // namespace dash

#endif // DASH__HALO_TYPES_H