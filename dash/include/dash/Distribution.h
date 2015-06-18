#ifndef DASH__DISTRIBUTION_H_
#define DASH__DISTRIBUTION_H_

#include <dash/Enums.h>
#include <dash/internal/DistributionFunc.h>

namespace dash {

class Distribution {
public:
  dash::internal::DistributionType type;
  long long blocksz;

  Distribution()
  : type(dash::internal::DIST_NONE),
    blocksz(-1) {
  }

  Distribution(
    dash::internal::DistributionType distType,
    long long blockSize)
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
      case dash::internal::DIST_NONE:
        return 1;
      case dash::internal::DIST_BLOCKED:
        return 1;
      case dash::internal::DIST_CYCLIC:
        // same as block cyclic with blocksz = 1
        return dash::math::div_ceil(range, num_units);
      case dash::internal::DIST_BLOCKCYCLIC:
        // extent to blocks:
        num_blocks = dash::math::div_ceil(range, blocksz);
        // blocks to units:
        return dash::math::div_ceil(num_blocks, num_units);
      case dash::internal::DIST_TILE:
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
      case dash::internal::DIST_NONE:
        return range;
      case dash::internal::DIST_BLOCKED:
        return dash::math::div_ceil(range, num_units);
      case dash::internal::DIST_CYCLIC:
        return 1;
      case dash::internal::DIST_BLOCKCYCLIC:
        return blocksz;
      case dash::internal::DIST_TILE:
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
      case dash::internal::DIST_NONE:
        // Unit id is unchanged:
        return 0;
      case dash::internal::DIST_BLOCKED:
      case dash::internal::DIST_CYCLIC:
      case dash::internal::DIST_BLOCKCYCLIC:
        // Advance one unit id per block coordinate:
        return block_coord % num_units;
      case dash::internal::DIST_TILE:
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
  bool operator==(const Distribution & other) const {
    return (this->type == other.type &&
            this->blocksz == other.blocksz);
  }
  /**
   * Inequality comparison operator.
   */
  bool operator!=(const Distribution & other) const {
    return !(*this == other);
  }
};

static Distribution BLOCKED(dash::internal::DIST_BLOCKED, -1);
static Distribution CYCLIC(dash::internal::DIST_BLOCKCYCLIC, 1);
static Distribution NONE(dash::internal::DIST_NONE, -1);

Distribution TILE(int blockSize);

Distribution BLOCKCYCLIC(int blockSize);

} // namespace dash

#endif // DASH__DISTRIBUTION_H_
