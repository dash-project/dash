
#include "Enums.h"

namespace dash {

DistEnum TILE(int blockSize) {
  return DistEnum { DistEnum::TILE, blockSize };
}

DistEnum BLOCKCYCLIC(int blockSize) {
  return DistEnum { DistEnum::BLOCKCYCLIC, blockSize };
}

} // namespace dash
