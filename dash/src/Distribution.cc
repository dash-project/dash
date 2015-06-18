
#include <dash/Enums.h>

namespace dash {

Distribution TILE(int blockSize) {
  return Distribution(Distribution::TILE, blockSize);
}

Distribution BLOCKCYCLIC(int blockSize) {
  return Distribution(Distribution::BLOCKCYCLIC, blockSize);
}

} // namespace dash
