#ifndef DASH__MAKE_PATTERN_H_
#define DASH__MAKE_PATTERN_H_

#include <dash/Pattern.h>
#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>

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
    auto extent_d  = sizespec.extent(d);
    auto nunits_d  = teamspec.extent(d);
    auto nblocks_d = nunits_d;
    DASH_LOG_TRACE("dash::make_distribution_spec",
                   "d:",          d,
                   "extent[d]:",  extent_d,
                   "nunits[d]:",  nunits_d,
                   "nblocks[d]:", nblocks_d);
    if (MappingTraits::diagonal || MappingTraits::neighbor) {
      // Diagonal and neighbor mapping properties require occurrence of every
      // unit in any hyperplane. Use total number of units in every dimension:
      nblocks_d = teamspec.size();
    } else {
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
    }
    auto tilesize_d = extent_d / nblocks_d;
    DASH_LOG_TRACE_VAR("dash::make_distribution_spec", tilesize_d);
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
    if (LayoutTraits::linear && LayoutTraits::blocked) {
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
 *
 * \returns  An instance of \c dash::TilePattern if the following constraints are
 *           specified:
 *           - Layout: blocked
 *           or
 *           - Partitioning: balanced
 *           - Dimensions:   1
 */
template<
  typename PartitioningTraits = dash::pattern_partitioning_default_properties,
  typename MappingTraits      = dash::pattern_mapping_default_properties,
  typename LayoutTraits       = dash::pattern_layout_default_properties,
  class    SizeSpecType,
  class    TeamSpecType
>
typename std::enable_if<
  (LayoutTraits::blocked) ||
  (PartitioningTraits::balanced && SizeSpecType::ndim() == 1),
  TilePattern<SizeSpecType::ndim(), dash::ROW_MAJOR, typename SizeSpecType::index_type>
>::type
make_pattern(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Deduce index type from size spec:
  typedef typename SizeSpecType::index_type                 index_t;
  typedef dash::TilePattern<ndim, dash::ROW_MAJOR, index_t> pattern_t;
  DASH_LOG_TRACE("dash::make_pattern", PartitioningTraits());
  DASH_LOG_TRACE("dash::make_pattern", MappingTraits());
  DASH_LOG_TRACE("dash::make_pattern", LayoutTraits());
  DASH_LOG_TRACE_VAR("dash::make_pattern", sizespec.extents());
  DASH_LOG_TRACE_VAR("dash::make_pattern", teamspec.extents());
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
  pattern_t pattern(sizespec,
		    distspec,
		    teamspec);
  return pattern;
}

/**
 * Generic Abstract Factory for models of the Pattern concept.
 *
 * Creates an instance of a Pattern model that satisfies the canonical
 * (strided) layout property from given pattern traits.
 *
 * \returns  An instance of \c dash::BlockPattern if the following constraints are
 *           specified:
 *           - Layout: canonical
 */
template<
  typename PartitioningTraits = dash::pattern_partitioning_default_properties,
  typename MappingTraits      = dash::pattern_mapping_default_properties,
  typename LayoutTraits       = dash::pattern_layout_default_properties,
  class    SizeSpecType,
  class    TeamSpecType
>
typename std::enable_if<
  LayoutTraits::canonical,
  Pattern<SizeSpecType::ndim(),
          dash::ROW_MAJOR,
          typename SizeSpecType::index_type>
>::type
make_pattern(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec,
  /// Team spec containing layout of units mapped by the pattern.
  const TeamSpecType & teamspec)
{
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Deduce index type from size spec:
  typedef typename SizeSpecType::index_type             index_t;
  typedef dash::Pattern<ndim, dash::ROW_MAJOR, index_t> pattern_t;
  DASH_LOG_TRACE("dash::make_pattern", PartitioningTraits());
  DASH_LOG_TRACE("dash::make_pattern", MappingTraits());
  DASH_LOG_TRACE("dash::make_pattern", LayoutTraits());
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
  pattern_t pattern(sizespec,
                    distspec,
                    teamspec);
  return pattern;
}

} // namespace dash

#endif // DASH__MAKE_PATTERN_H_
