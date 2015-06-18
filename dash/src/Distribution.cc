
#include <dash/Enums.h>
#include <dash/Distribution.h>

namespace dash {

Distribution TILE(int blockSize) {
  return Distribution(dash::internal::DIST_TILE, blockSize);
}

Distribution BLOCKCYCLIC(int blockSize) {
  return Distribution(dash::internal::DIST_BLOCKCYCLIC, blockSize);
}

} // namespace dash
