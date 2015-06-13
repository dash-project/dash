#ifndef DASH__DISTRIBUTION_H_
#define DASH__DISTRIBUTION_H_

namespace dash {

/**
 * Concept \c Distribution, specifies a distribution of a
 * one-dimensional range to a number of units.
 */
template<DistType DistributionType>
class DistributionFunctor {
  /**
   * The capacity of a single block in the given range for
   * a given total number of blocks.
   */
  size_t blocksize_of_range(
    size_t range,
    size_t num_blocks) const;

  /**
   * The number of elements of all blocks in the given range for a
   * single unit.
   */
  size_t local_capacity_of_range(
    size_t range,
    size_t num_blocks) const;

  /**
   * Retreive the assigned unit id to a given index in a range.
   */
  size_t index_to_unit(
    size_t range,
    size_t num_blocks,
    long long index) const;
};

template<>
class DistributionFunctor<BLOCKED> {
public:
  size_t blocksize_of_range(
    size_t range,
    size_t num_blocks) const {
    return div_ceil(range, num_blocks);
  }

  size_t local_capacity_of_range(
    size_t range,
    size_t num_blocks) const {
    
  }

  size_t index_to_unit(
    size_t range,
    size_t num_blocks,
    long long index) const {
    return index / num_blocks;
  }
};

template<>
class DistributionFunctor<CYCLIC> {
public:
  size_t blocksize_of_range(
    size_t range,
    size_t num_blocks) const {
    return 1;
  }

  size_t local_capacity_of_range(
    size_t range,
    size_t num_blocks) const {
    
  }

  size_t index_to_unit(
    size_t range,
    size_t num_blocks,
    long long index) const {
    
  }
};

template<>
class DistributionFunctor<BLOCKCYCLIC> {
public:
  Distribution(size_t blocksize)
  : _blocksize(blocksize) {
  }

  size_t blocksize_of_range(
    size_t range,
    size_t num_blocks) const {
    return _blocksize;
  }

  size_t local_capacity_of_range(
    size_t range,
    size_t num_blocks) const {
    return _blocksize * num_blocks;
  }

  size_t index_to_unit(
    size_t range,
    size_t num_blocks,
    long long index) const {
    
  }

private:
  size_t _blocksize;
};

template<>
class DistributionFunctor<TILE> {
public:
  Distribution(size_t tilesize)
  : _tilesize(tilesize) {
  }

  size_t blocksize_of_range(
    size_t range,
    size_t num_blocks) const {
    return _blocksize;
  }

  size_t local_capacity_of_range(
    size_t range,
    size_t num_blocks) const {
    // Number of tiles in the given range:
    size_t num_tiles = div_ceil(range, _blocksize);
    // Number of elements in all tiles:
    return _blocksize * num_tiles;
  }

  size_t index_to_unit(
    size_t range,
    size_t num_blocks,
    long long index) const {
    
  }

private:
  size_t _tilesize;
};

} // namespace dash

#endif // DASH__DISTRIBUTION_H_
