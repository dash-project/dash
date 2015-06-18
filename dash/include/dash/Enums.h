#ifndef DASH__ENUMS_H_ 
#define DASH__ENUMS_H_

#include <dash/Exception.h>
#include <dash/internal/Math.h>
#include <cstddef>
#include <functional>

namespace dash {

typedef enum MemArrange {
  Undefined = 0,
  ROW_MAJOR,
  COL_MAJOR
} MemArrange;

namespace internal {

typedef enum DistributionType {
  Undefined = 0,
  DIST_NONE,
  DIST_BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
  DIST_CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
  DIST_BLOCKCYCLIC,
  DIST_TILE
} DistributionType; // general blocked distribution

} // namespace internal
} // namespace dash

#endif // DASH__ENUMS_H_
