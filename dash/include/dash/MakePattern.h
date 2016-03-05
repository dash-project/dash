#ifndef DASH__MAKE_PATTERN_H_
#define DASH__MAKE_PATTERN_H_

#include <dash/PatternProperties.h>
#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/ShiftTilePattern.h>
#include <dash/util/Locality.h>

namespace dash {

template<
  typename PartitioningTags,
  typename MappingTags,
  typename LayoutTags,
  class SizeSpecType
>
TeamSpec<SizeSpecType::ndim()>
make_team_spec(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType & sizespec)
{
  typedef typename SizeSpecType::size_type extent_t;

  DASH_LOG_TRACE("dash::make_team_spec()");
  // Deduce number of dimensions from size spec:
  const dim_t ndim   = SizeSpecType::ndim();
  // Number of processing nodes:
  auto  n_nodes      = dash::util::Locality::NumNodes();
  // Number of NUMA domains:
  auto  n_numa_dom   = dash::util::Locality::NumNumaNodes();
  // Default team spec:
  dash::TeamSpec<ndim> teamspec;
  // Check for trivial case first:
  if (ndim == 1) {
    return teamspec;
  }
  auto  n_team_units = teamspec.size();
  auto  n_elem_total = sizespec.size();

  // Multi-dimensional case:
  if (n_nodes == 1 ||
      PartitioningTags::minimal ||
      (!MappingTags::diagonal && !MappingTags::neighbor &&
       !MappingTags::multiple)) {
    // Optimize for surface-to-volume ratio:
    teamspec.balance_extents();
  }
  // Create copy of team extents for rebalancing:
  std::array<extent_t, ndim> team_extents = teamspec.extents();

  auto team_factors = dash::math::factorize(n_team_units);
  auto size_factors = dash::math::factorize(n_elem_total);

  DASH_LOG_TRACE("dash::make_team_spec",
                 "team size:",    n_team_units,
                 "factorized:",   team_factors,
                 "data extents:", n_elem_total,
                 "factorized:",   size_factors);

  // Maximum balanced block size for this sizespec and team:
  size_t max_balanced_block_size = sizespec.size();
  if (PartitioningTags::ndimensional) {
    // Partitioning in at least two dimensions required.
    // This divides the maximum balanced block size by the smallest prime
    // factor of the team size:
    max_balanced_block_size /= team_factors.begin()->first;
  }

  if (!MappingTags::multiple) {
    return teamspec;
  }
  // Do not rebalance team on single node if its arrangement is square:
  if (n_nodes == 1 && ndim > 1 && team_extents[0] == team_extents[1]) {
    return teamspec;
  }

  // Rebalance team extents by topology measures by applying:
  //    teamsize[0] /= split_factor
  //    teamsize[1] *= split_factor
  auto split_factor = n_nodes > 1 ? n_nodes : 2;
  if (team_extents[0] % split_factor != 0) {
    split_factor = 1;
  }
  for (auto d = 0; d < ndim; ++d) {
    auto extent_d  = sizespec.extent(d);
    auto nunits_d  = teamspec.extent(d);
    DASH_LOG_TRACE("dash::make_team_spec",
                   "d:",          d,
                   "extent[d]:",  extent_d,
                   "nunits[d]:",  nunits_d);
    auto nblocks_d = nunits_d;
    if (MappingTags::multiple) {
      if (d == 0) {
        nunits_d /= split_factor;
      } else if (d == 1) {
        nunits_d *= split_factor;
      }
    }
    team_extents[d] = nunits_d;
  }
  // Make distribution spec from template- and run time parameters:
  teamspec.resize(team_extents);
  return teamspec;
}

//////////////////////////////////////////////////////////////////////////////
// Generic Abstract Pattern Factories (dash::make_pattern)
//////////////////////////////////////////////////////////////////////////////

/**
 * Generic Abstract Factory for instances of \c dash::DistributionSpec.
 * Creates a DistributionSpec object from given pattern traits.
 */
template<
  typename PartitioningTags,
  typename MappingTags,
  typename LayoutTags,
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
  typedef typename SizeSpecType::size_type extent_t;

  DASH_LOG_TRACE("dash::make_distribution_spec()");
  // Deduce number of dimensions from size spec:
  const dim_t ndim = SizeSpecType::ndim();
  // Array of distribution specifiers in all dimensions,
  // e.g. { TILE(10), TILE(120) }:
  std::array<dash::Distribution, ndim> distributions;
  extent_t min_block_extent = sizespec.size();
  if (PartitioningTags::minimal) {
    // Find minimal block size in minimal partitioning, initialize with
    // pattern size (maximum):
    for (auto d = 0; d < SizeSpecType::ndim(); ++d) {
      auto extent_d    = sizespec.extent(d);
      auto nunits_d    = teamspec.extent(d);
      auto blocksize_d = extent_d / nunits_d;
      if (blocksize_d < min_block_extent) {
        min_block_extent = blocksize_d;
      }
    }
    DASH_LOG_TRACE("dash::make_distribution_spec",
                   "minimum block extent for square blocks:",
                   min_block_extent);
  }
  // Resolve balanced tile extents from size spec and team spec:
  for (auto d = 0; d < SizeSpecType::ndim(); ++d) {
    auto extent_d  = sizespec.extent(d);
    auto nunits_d  = teamspec.extent(d);
    DASH_LOG_TRACE("dash::make_distribution_spec",
                   "d:",          d,
                   "extent[d]:",  extent_d,
                   "nunits[d]:",  nunits_d);
    auto nblocks_d = nunits_d;
    if (MappingTags::diagonal || MappingTags::neighbor) {
      // Diagonal and neighbor mapping properties require occurrence of every
      // unit in any hyperplane. Use total number of units in every dimension:
      nblocks_d = teamspec.size();
      DASH_LOG_TRACE("dash::make_distribution_spec",
                     "diagonal or neighbor mapping",
                     "d", d, "nblocks_d", nblocks_d);
    } else if (PartitioningTags::minimal) {
      // Trying to assign one block per unit:
      nblocks_d = nunits_d;
      if (!MappingTags::balanced) {
        // Unbalanced mapping, trying to use same block extent in all
        // dimensions:
        nblocks_d = extent_d / min_block_extent;
        DASH_LOG_TRACE("dash::make_distribution_spec",
                       "minimal partitioning, mapping not balanced",
                       "d", d, "nblocks_d", nblocks_d);
      }
    } else if (MappingTags::balanced) {
      // Balanced mapping, i.e. same number of blocks for every unit
      if (nblocks_d % teamspec.extent(d) > 0) {
        // Extent in this dimension is not a multiple of number of units,
        // balanced mapping property cannot be satisfied:
        DASH_THROW(dash::exception::InvalidArgument,
                   "dash::make_distribution_spec: cannot distribute " <<
                   nblocks_d << " blocks to " <<
                   nunits_d  << " units in dimension " << d);
      }
    }
    auto tilesize_d = extent_d / nblocks_d;
    DASH_LOG_TRACE("dash::make_distribution_spec",
                   "tile size in dimension", d, ":", tilesize_d);
    if (PartitioningTags::balanced) {
      // Balanced partitioning, i.e. same number of elements in every block
      if (extent_d % tilesize_d > 0) {
        // Extent in this dimension is not a multiple of tile size,
        // balanced partitioning property cannot be satisfied:
        DASH_THROW(dash::exception::InvalidArgument,
                   "dash::make_distribution_spec: cannot distribute " <<
                   extent_d   << " elements to " <<
                   nblocks_d  << " blocks in dimension " << d);
      }
    }
    if (LayoutTags::linear && LayoutTags::blocked) {
      distributions[d] = dash::TILE(tilesize_d);
    } else {
      distributions[d] = dash::BLOCKCYCLIC(tilesize_d);
    }
  }
  // Make distribution spec from template- and run time parameters:
  dash::DistributionSpec<ndim> distspec(distributions);
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
  typename PartitioningTags,
  typename MappingTags,
  typename LayoutTags
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
 * \returns  An instance of \c dash::TilePattern if the following
 *           constraints are specified:
 *           (Partitioning: minimal)
 *           and
 *           (Layout:       blocked)
 */
template<
  typename PartitioningTags = dash::pattern_partitioning_default_properties,
  typename MappingTags      = dash::pattern_mapping_default_properties,
  typename LayoutTags       = dash::pattern_layout_default_properties,
  class    SizeSpecType,
  class    TeamSpecType
>
typename std::enable_if<
  PartitioningTags::minimal &&
  LayoutTags::blocked,
  TilePattern<SizeSpecType::ndim(),
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
  typedef typename SizeSpecType::index_type                 index_t;
  typedef dash::TilePattern<ndim, dash::ROW_MAJOR, index_t> pattern_t;
  DASH_LOG_TRACE("dash::make_pattern", PartitioningTags());
  DASH_LOG_TRACE("dash::make_pattern", MappingTags());
  DASH_LOG_TRACE("dash::make_pattern", LayoutTags());
  DASH_LOG_TRACE_VAR("dash::make_pattern", sizespec.extents());
  DASH_LOG_TRACE_VAR("dash::make_pattern", teamspec.extents());
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      PartitioningTags,
      MappingTags,
      LayoutTags,
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
 * Creates an instance of a Pattern model that satisfies the contiguos
 * linearization property from given pattern traits.
 *
 * \returns  An instance of \c dash::ShiftTilePattern if the following
 *           constraints are specified:
 *           (Mapping:       diagonal)
 *           and
 *           (Layout:        blocked
 *            or
 *            (Partitioning: balanced
 *             Dimensions:   1))
 */
template<
  typename PartitioningTags = dash::pattern_partitioning_default_properties,
  typename MappingTags      = dash::pattern_mapping_default_properties,
  typename LayoutTags       = dash::pattern_layout_default_properties,
  class    SizeSpecType,
  class    TeamSpecType
>
typename std::enable_if<
  MappingTags::diagonal &&
  (LayoutTags::blocked ||
   (PartitioningTags::balanced &&
    SizeSpecType::ndim() == 1)),
  ShiftTilePattern<SizeSpecType::ndim(),
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
  typedef typename SizeSpecType::index_type                 index_t;
  typedef dash::ShiftTilePattern<ndim, dash::ROW_MAJOR, index_t> pattern_t;
  DASH_LOG_TRACE("dash::make_pattern", PartitioningTags());
  DASH_LOG_TRACE("dash::make_pattern", MappingTags());
  DASH_LOG_TRACE("dash::make_pattern", LayoutTags());
  DASH_LOG_TRACE_VAR("dash::make_pattern", sizespec.extents());
  DASH_LOG_TRACE_VAR("dash::make_pattern", teamspec.extents());
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      PartitioningTags,
      MappingTags,
      LayoutTags,
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
 * \returns  An instance of \c dash::BlockPattern if the following constraints
 *           are specified:
 *           - Layout: canonical
 */
template<
  typename PartitioningTags = dash::pattern_partitioning_default_properties,
  typename MappingTags      = dash::pattern_mapping_default_properties,
  typename LayoutTags       = dash::pattern_layout_default_properties,
  class    SizeSpecType,
  class    TeamSpecType
>
typename std::enable_if<
  LayoutTags::canonical,
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
  DASH_LOG_TRACE("dash::make_pattern", PartitioningTags());
  DASH_LOG_TRACE("dash::make_pattern", MappingTags());
  DASH_LOG_TRACE("dash::make_pattern", LayoutTags());
  // Make distribution spec from template- and run time parameters:
  auto distspec =
    make_distribution_spec<
      PartitioningTags,
      MappingTags,
      LayoutTags,
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
