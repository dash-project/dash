#ifndef DASH__ENUMS_H_ 
#define DASH__ENUMS_H_

#include <dash/internal/Math.h>
#include <cstddef>
#include <functional>

namespace dash {

typedef enum MemArrange {
  Undefined = 0,
  ROW_MAJOR,
  COL_MAJOR
} MemArrange;

class DistEnum {
public:
  typedef enum disttype {
    BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
    CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
    BLOCKCYCLIC,
    TILE,
    NONE
  } disttype; // general blocked distribution

public:
  disttype type;
  long long blocksz;

  DistEnum()
  : type(disttype::NONE),
    blocksz(-1) {
  }

  DistEnum(disttype distType, long long blockSize)
  : type(distType),
    blocksz(blockSize) {
  }

  long long blocksize_in_range(
    size_t range,
    size_t num_blocks) const {
    switch (type) {
      case DistEnum::disttype::NONE:
        return range;
      case DistEnum::disttype::BLOCKED:
        return dash::math::div_ceil(range, num_blocks);
      case DistEnum::disttype::CYCLIC:
        return 1;
      case DistEnum::disttype::BLOCKCYCLIC:
        return blocksz;
      case DistEnum::disttype::TILE:
        return blocksz;
      default:
        return -1;
    }
  }

  bool operator==(const DistEnum & other) const {
    return (this->type == other.type &&
            this->blocksz == other.blocksz);
  }
  bool operator!=(const DistEnum & other) const {
    return !(*this == other);
  }
};

static DistEnum BLOCKED(DistEnum::BLOCKED, -1);
static DistEnum CYCLIC(DistEnum::BLOCKCYCLIC, 1);
static DistEnum NONE(DistEnum::NONE, -1);

DistEnum TILE(int blockSize);

DistEnum BLOCKCYCLIC(int blockSize);

} // namespace dash

#endif // DASH__ENUMS_H_
