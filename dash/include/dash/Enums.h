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

  size_t num_elements_of_unit(size_t unit_id) {

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
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Distribution type undefined in blocksize_in_range");
    }
  }

  /**
   * Resolve the associated unit id offset of the given block offset.
   */
  size_t block_coord_to_unit_offset(
    long long block_coord,
    int dimension,
    long long num_units) const {
    switch (type) {
      case DistEnum::disttype::NONE:
        // Unit id is unchanged:
        return 0;
      case DistEnum::disttype::BLOCKED:
      case DistEnum::disttype::CYCLIC:
      case DistEnum::disttype::BLOCKCYCLIC:
        // Advance one unit id per block coordinate:
        return block_coord;
      case DistEnum::disttype::TILE:
        // Advance one unit id per block coordinate and
        // one unit id per dimension:
        return block_coord + dimension;
      default:
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Distribution type undefined in block_coord_to_unit_offset");
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
