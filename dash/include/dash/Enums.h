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

  /**
   * The maximum number of blocks local to a single unit within an
   * extent for a given total number of units.
   */
  long long max_local_blocks_in_range(
    /// Number of elements to distribute
    size_t range,
    /// Number of units to which elements are distributed
    size_t num_units) const {
    size_t num_blocks;
    switch (type) {
      case DistEnum::disttype::NONE:
        return 1;
      case DistEnum::disttype::BLOCKED:
        return 1;
      case DistEnum::disttype::CYCLIC:
        // same as block cyclic with blocksz = 1
        return dash::math::div_ceil(range, num_units);
      case DistEnum::disttype::BLOCKCYCLIC:
        // extent to blocks:
        num_blocks = dash::math::div_ceil(range, blocksz);
        // blocks to units:
        return dash::math::div_ceil(num_blocks, num_units);
      case DistEnum::disttype::TILE:
        // same as block cyclic
        num_blocks = dash::math::div_ceil(range, blocksz);
        return dash::math::div_ceil(num_blocks, num_units);
      default:
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Distribution type undefined in max_blocksize_in_range");
    }
  }

  /**
   * The maximum size of a single block within an extent for
   * a given total number of units.
   */
  long long max_blocksize_in_range(
    /// Number of elements to distribute
    size_t range,
    /// Number of units to which elements are distributed
    size_t num_units) const {
    switch (type) {
      case DistEnum::disttype::NONE:
        return range;
      case DistEnum::disttype::BLOCKED:
        return dash::math::div_ceil(range, num_units);
      case DistEnum::disttype::CYCLIC:
        return 1;
      case DistEnum::disttype::BLOCKCYCLIC:
        return blocksz;
      case DistEnum::disttype::TILE:
        return blocksz;
      default:
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Distribution type undefined in max_blocksize_in_range");
    }
  }

  /**
   * Resolve the associated unit id offset of the given block offset.
   */
  size_t block_coord_to_unit_offset(
    long long block_coord,
    int dimension,
    int num_units) const {
    switch (type) {
      case DistEnum::disttype::NONE:
        // Unit id is unchanged:
        return 0;
      case DistEnum::disttype::BLOCKED:
      case DistEnum::disttype::CYCLIC:
      case DistEnum::disttype::BLOCKCYCLIC:
        // Advance one unit id per block coordinate:
        return block_coord % num_units;
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

  /**
   * Equality comparison operator.
   */
  bool operator==(const DistEnum & other) const {
    return (this->type == other.type &&
            this->blocksz == other.blocksz);
  }
  /**
   * Inequality comparison operator.
   */
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
