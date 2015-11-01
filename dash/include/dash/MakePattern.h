#ifndef DASH__MAKE_PATTERN_H_
#define DASH__MAKE_PATTERN_H_

#include <dash/Pattern.h>

namespace dash {

//////////////////////////////////////////////////////////////////////////////
// Pattern linearization properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_linearization_tag
{
  typedef enum {
    any,
    in_block,
    strided
  } type;
};

template< pattern_linearization_tag::type >
struct pattern_linearization_traits
{
  static const bool contiguous = false;
};

template<>
struct pattern_linearization_traits<
  pattern_linearization_tag::in_block >
{
  static const bool contiguous = true;
};

//////////////////////////////////////////////////////////////////////////////
// Pattern mapping properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_mapping_tag
{
  typedef enum {
    any,
    /// Same number of blocks for every process
    balanced,
    /// Every process mapped in every row and column
    diagonal
  } type;
};

template< pattern_mapping_tag::type >
struct pattern_mapping_traits
{
  static const bool balanced = false;
  static const bool diagnoal = false;
};

template<>
struct pattern_mapping_traits<
  pattern_mapping_tag::balanced >
{
  static const bool balanced = true;
  static const bool diagnoal = false;
};


//////////////////////////////////////////////////////////////////////////////
// Pattern blocking properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_blocking_tag
{
  typedef enum {
    any,
    balanced
  } type;
};

template< pattern_blocking_tag::type >
struct pattern_blocking_traits
{
  static const bool balanced = false;
};

template<>
struct pattern_blocking_traits<
  pattern_blocking_tag::balanced >
{
  static const bool balanced = true;
};


//////////////////////////////////////////////////////////////////////////////
// Generic Abstract Pattern Factories (dash::make_pattern)
//////////////////////////////////////////////////////////////////////////////

/**
 * Generic Abstract Factory for instances of dash::DistributionSpec.
 * Creates a DistributionSpec object from given pattern traits.
 */
template<
  typename BlockingTraits,
  typename MappingTraits,
  typename LinearizationTraits,
  class SizeSpecType,
  class TeamSpecType
>
DistributionSpec<SizeSpecType::ndim()>
make_distribution_spec(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  DASH_LOG_TRACE("dash::make_distribution_spec()");
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Array of distribution specifiers in all dimensions,
  // e.g. { TILE(10), TILE(120) }:
  std::array<dash::Distribution, ndim> distributions;
  // Resolve balanced tile extents from size spec and team spec:
  for (auto d = 0; d < SizeSpecType::ndim(); ++d) {
    auto extent_d    = sizespec.extent(d);
    auto nunits_d    = teamspec.extent(d);
    auto nblocks_d   = nunits_d;
    auto tilesize_d  = extent_d / nblocks_d;
    DASH_LOG_TRACE_VAR("dash::make_distribution_spec", tilesize_d);
    if (MappingTraits::balanced) {
      // Balanced mapping, i.e. same number of blocks for every unit
      if (nblocks_d % teamspec.extent(d) > 0) {
        // Extent in this dimension is not a multiple of number of units,
        // balanced mapping property cannot be satisfied:
        DASH_THROW(dash::exception::InvalidArgument,
                   "dash::make_pattern: cannot distribute " <<
                   nblocks_d << " blocks to " <<
                   nunits_d  << " units in dimension " << d);
      }
    }
    if (BlockingTraits::balanced) {
      // Balanced blocking, i.e. same number of elements in every block
      if (extent_d % tilesize_d > 0) {
        // Extent in this dimension is not a multiple of tile size,
        // balanced blocking property cannot be satisfied:
        DASH_THROW(dash::exception::InvalidArgument,
                   "dash::make_pattern: cannot distribute " <<
                   extent_d   << " elements to " <<
                   nblocks_d  << " blocks in dimension " << d);
      }
    }
    distributions[d] = dash::TILE(tilesize_d);
  }
  // Make distribution spec from template- and run time parameters:
  dash::DistributionSpec<
      SizeSpecType::ndim()
  > distspec(
      distributions);
  return distspec;
}

/**
 * Generic Abstract Factory for models of the Pattern concept.
 *
 * Creates an instance of a Pattern model that satisfies the contiguos
 * linearization property from given pattern traits.
 */
template<
  typename BlockingTraits,
  typename MappingTraits,
  typename LinearizationTraits,
  class SizeSpecType,
  class TeamSpecType
>
typename std::enable_if<
  LinearizationTraits::contiguous,
  TilePattern<SizeSpecType::ndim()>
>::type
make_pattern(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  DASH_LOG_TRACE("dash::make_pattern", "-> contiguous");
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      BlockingTraits,
      MappingTraits,
      LinearizationTraits,
      SizeSpecType,
      TeamSpecType
    >(sizespec,
      teamspec);
  // Make pattern from template- and run time parameters:
  dash::TilePattern<
    ndim
  > pattern(
      sizespec,
      distspec,
      teamspec);
  return pattern;
}

/**
 * Generic Abstract Factory for models of the Pattern concept.
 *
 * Creates an instance of a Pattern model that satisfies the strided
 * linearization property from given pattern traits.
 */
template<
  typename BlockingTraits,
  typename MappingTraits,
  typename LinearizationTraits,
  class SizeSpecType,
  class TeamSpecType
>
typename std::enable_if<
  !LinearizationTraits::contiguous,
  Pattern<SizeSpecType::ndim()>
>::type
make_pattern(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  DASH_LOG_TRACE("dash::make_pattern", "-> not contiguous");
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      BlockingTraits,
      MappingTraits,
      LinearizationTraits,
      SizeSpecType,
      TeamSpecType
    >(sizespec,
      teamspec);
  // Make pattern from template- and run time parameters:
  dash::Pattern<
    ndim
  > pattern(
      sizespec,
      distspec,
      teamspec);
  return pattern;
}

}; // namespace dash

#endif // DASH__MAKE_PATTERN_H_
