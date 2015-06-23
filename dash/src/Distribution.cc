
#include <dash/Enums.h>
#include <dash/Distribution.h>

dash::Distribution dash::BLOCKED(dash::internal::DIST_BLOCKED, -1);

dash::Distribution dash::CYCLIC(dash::internal::DIST_CYCLIC, 1);

dash::Distribution dash::NONE(dash::internal::DIST_NONE, -1);

dash::Distribution dash::TILE(int blockSize) {
  return Distribution(dash::internal::DIST_TILE, blockSize);
}

dash::Distribution dash::BLOCKCYCLIC(int blockSize) {
  return Distribution(dash::internal::DIST_BLOCKCYCLIC, blockSize);
}

