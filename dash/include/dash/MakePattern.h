#ifndef DASH__MAKE_PATTERN_H_
#define DASH__MAKE_PATTERN_H_

#include <dash/Pattern.h>

namespace dash {

//////////////////////////////////////////////////////////////////////////////
// Generic Abstract Pattern Factories (dash::make_pattern)
//////////////////////////////////////////////////////////////////////////////

/**
 * Generic Abstract Factory for instances of \c dash::DistributionSpec.
 * Creates a DistributionSpec object from given pattern traits.
 */
template<
  typename PartitioningTraits,
  typename MappingTraits,
  typename LayoutTraits,
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
    if (PartitioningTraits::balanced) {
      // Balanced partitioning, i.e. same number of elements in every block
      if (extent_d % tilesize_d > 0) {
        // Extent in this dimension is not a multiple of tile size,
        // balanced partitioning property cannot be satisfied:
        DASH_THROW(dash::exception::InvalidArgument,
                   "dash::make_pattern: cannot distribute " <<
                   extent_d   << " elements to " <<
                   nblocks_d  << " blocks in dimension " << d);
      }
    }
    if (LayoutTraits::local_phase) {
      distributions[d] = dash::TILE(tilesize_d);
    } else {
      distributions[d] = dash::BLOCKCYCLIC(tilesize_d);
    }
  }
  // Make distribution spec from template- and run time parameters:
  dash::DistributionSpec<
      SizeSpecType::ndim()
  > distspec(
      distributions);
  return distspec;
}

#if __EXP__
/**
 * Usage:
 *
 *    typedef dash::deduce_pattern_model<
 *              pattern_partitioning_properties<...>,
 *              pattern_mapping_properties<...>,
 *              pattern_layout_properties<...>
 *            >::type
 *      pattern_class;
 */
template<
  typename PartitioningTraits,
  typename MappingTraits,
  typename LayoutTraits
>
struct deduce_pattern_model
{
  
}
#endif

/**
 * Generic Abstract Factory for models of the Pattern concept.
 *
 * Creates an instance of a Pattern model that satisfies the contiguos
 * linearization property from given pattern traits.
 */
template<
  typename PartitioningTraits = dash::pattern_partitioning_default_properties,
  typename MappingTraits = dash::pattern_mapping_default_properties,
  typename LayoutTraits = dash::pattern_layout_default_properties,
  class SizeSpecType,
  class TeamSpecType
>
typename std::enable_if<
  LayoutTraits::local_phase,
  TilePattern<SizeSpecType::ndim()>
>::type
make_pattern(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  // Tags in pattern property category 'Linearization'
  DASH_LOG_TRACE("dash::make_pattern", "-> Linearization properties:");
  DASH_LOG_TRACE("dash::make_pattern", "   - local_phase");
  DASH_LOG_TRACE("dash::make_pattern", "-> Partitioning properties:");

  // Tags in pattern property category 'Partitioning'
  if (PartitioningTraits::balanced) {
    DASH_LOG_TRACE("dash::make_pattern", "   - balanced blocks");
  }
  if (PartitioningTraits::cache_align) {
    DASH_LOG_TRACE("dash::make_pattern", "   - cache aligned blocks");
  }

  // Tags in pattern property category 'Mapping'
  static_assert(!MappingTraits::neighbor || !MappingTraits::diagonal,
                "Pattern mapping properties 'diagonal' contradicts "
                "mapping property 'remote_neighbors'");
  DASH_LOG_TRACE("dash::make_pattern", "-> Mapping properties:");
  if (MappingTraits::balanced) {
    DASH_LOG_TRACE("dash::make_pattern", "   - balanced mapping");
  }
  if (MappingTraits::diagonal) {
    DASH_LOG_TRACE("dash::make_pattern", "   - diagonal mapping");
  }
  if (MappingTraits::neighbor) {
    DASH_LOG_TRACE("dash::make_pattern", "   - remote neighbors");
  }
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      PartitioningTraits,
      MappingTraits,
      LayoutTraits,
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
  typename PartitioningTraits = dash::pattern_partitioning_default_properties,
  typename MappingTraits = dash::pattern_mapping_default_properties,
  typename LayoutTraits = dash::pattern_layout_default_properties,
  class SizeSpecType,
  class TeamSpecType
>
typename std::enable_if<
  !LayoutTraits::local_phase,
  Pattern<SizeSpecType::ndim()>
>::type
make_pattern(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  // Tags in pattern property category 'Linearization'
  DASH_LOG_TRACE("dash::make_pattern", "-> Linearization properties:");
  DASH_LOG_TRACE("dash::make_pattern", "   - local_strided");

  // Tags in pattern property category 'Partitioning'
  DASH_LOG_TRACE("dash::make_pattern", "-> Partitioning properties:");
  if (PartitioningTraits::balanced) {
    DASH_LOG_TRACE("dash::make_pattern", "   - balanced blocks");
  }
  if (PartitioningTraits::cache_align) {
    DASH_LOG_TRACE("dash::make_pattern", "   - cache aligned blocks");
  }

  // Tags in pattern property category 'Mapping'
  static_assert(!MappingTraits::neighbor || !MappingTraits::diagonal,
                "Pattern mapping properties 'diagonal' contradicts "
                "mapping property 'remote_neighbors'");
  DASH_LOG_TRACE("dash::make_pattern", "-> Mapping properties:");
  if (MappingTraits::balanced) {
    DASH_LOG_TRACE("dash::make_pattern", "   - balanced mapping");
  }
  if (MappingTraits::diagonal) {
    DASH_LOG_TRACE("dash::make_pattern", "   - diagonal mapping");
  }
  if (MappingTraits::neighbor) {
    DASH_LOG_TRACE("dash::make_pattern", "   - remote neighbors");
  }
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      PartitioningTraits,
      MappingTraits,
      LayoutTraits,
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
