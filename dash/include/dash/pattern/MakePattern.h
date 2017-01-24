#ifndef DASH__MAKE_PATTERN_H_
#define DASH__MAKE_PATTERN_H_

#include <dash/pattern/PatternProperties.h>
#include <dash/pattern/BlockPattern.h>
#include <dash/pattern/TilePattern.h>
#include <dash/pattern/ShiftTilePattern.h>

#include <dash/util/UnitLocality.h>
#include <dash/util/TeamLocality.h>
#include <dash/util/Locality.h>
#include <dash/util/Config.h>

#include <dash/Distribution.h>
#include <dash/Dimensional.h>

#include <set>


namespace dash {

template<
  typename PartitioningTags,
  typename MappingTags,
  typename LayoutTags,
  class    SizeSpecType
>
TeamSpec<SizeSpecType::ndim(), typename SizeSpecType::index_type>
make_team_spec(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType               & sizespec,
  int                                n_units,
  typename SizeSpecType::size_type   n_nodes      = 0,
  typename SizeSpecType::size_type   n_numa_dom   = 0,
  typename SizeSpecType::size_type   n_cores      = 0)
{
  typedef typename SizeSpecType::size_type  extent_t;
  typedef typename SizeSpecType::index_type index_t;

  // Deduce number of dimensions from size spec:
  const dim_t ndim  = SizeSpecType::ndim();

  // Default team spec:
  std::array<extent_t, ndim> team_extents;
  team_extents.fill(1);
  team_extents[1] = n_units;
  dash::TeamSpec<ndim, index_t> teamspec(team_extents);

  DASH_LOG_TRACE("dash::make_team_spec",
                 "step 1 - initial team extents:", team_extents);

  // Check for trivial case first:
  if (ndim == 1) {
    return teamspec;
  }

  auto n_elem_total = sizespec.size();
  // Configure preferable blocking factors:
  std::set<extent_t> blocking;
  if (n_nodes == 1) {
    // blocking by NUMA domains:
    blocking.insert(n_numa_dom);
    team_extents = dash::math::balance_extents(team_extents, blocking);
  } else {
    blocking.insert(n_cores);
  }

  DASH_LOG_TRACE("dash::make_team_spec",
                 "step 2 - team extents after balancing on NUMA domains:",
                 team_extents);
  DASH_LOG_TRACE_VAR("dash::make_team_spec", blocking);
  // Next simple case: Minimal partitioning, i.e. optimizing for minimum
  // number of blocks.
  // In this case, blocking will be minimal with respect to prefered blocking
  // factors:
  if (n_nodes > 1 &&
      (PartitioningTags::minimal ||
       (!MappingTags::diagonal && !MappingTags::neighbor &&
        !MappingTags::multiple))) {
    // Optimize for surface-to-volume ratio:
    DASH_LOG_TRACE("dash::make_team_spec",
                   "- optimizing for minimal number of blocks");
    team_extents = dash::math::balance_extents(team_extents, blocking);
    if (team_extents[0] == n_units) {
      // Could not balance with preferred blocking factors.
      DASH_LOG_TRACE("dash::make_team_spec",
                     "- minimize number of blocks for blocking", blocking);
    }
  }
  DASH_LOG_TRACE("dash::make_team_spec",
                 "step 3 - team extents after minimal partitioning:",
                 team_extents);
  // For minimal partitioning and multiple mapping, the first dimension is
  // partitioned using the smallest possible blocking factor.

  // The smallest factor s.t. team- and data extents are divisible by it:
  extent_t small_factor_found = 0;
  auto team_factors_d0 = dash::math::factorize(team_extents[0]);
  auto team_factors_d1 = dash::math::factorize(team_extents[1]);
  DASH_LOG_TRACE("dash::make_team_spec",
                 "- team extent factors in dim 0:", team_factors_d0);
  DASH_LOG_TRACE("dash::make_team_spec",
                 "- team extent factors in dim 1:", team_factors_d1);
  if (PartitioningTags::minimal && MappingTags::multiple) {
    DASH_LOG_TRACE("dash::make_team_spec",
                   "optimizing for multiple blocks per unit");
    for (auto small_factor_kv : team_factors_d0) {
      auto small_factor = small_factor_kv.first;
      // Find smallest factor s.t. team- and data extents are divisible by it:
      if (team_extents[0] % small_factor &&
          sizespec.extent(0) % small_factor == 0) {
        team_extents[0] /= small_factor;
        team_extents[1] *= small_factor;
        small_factor_found = small_factor;
        break;
      }
    }
    // No matching factor found in first dimension, try in second dimension:
    if (small_factor_found == 0) {
      for (auto small_factor_kv : team_factors_d1) {
        auto small_factor = small_factor_kv.first;
        if (team_extents[1] % small_factor &&
            sizespec.extent(1) % small_factor == 0) {
          team_extents[1] /= small_factor;
          team_extents[0] *= small_factor;
          small_factor_found = small_factor;
          break;
        }
      }
    }
  }
  DASH_LOG_TRACE("dash::make_team_spec",
                 "- smallest blocking factor:", small_factor_found);
  DASH_LOG_TRACE("dash::make_team_spec",
                 "step 4 - team extents after multiple mapping:",
                 team_extents);

  // Check if the resulting block sizes are within prefered bounds:
  extent_t bulk_min = std::max<extent_t>(
                        dash::util::Config::get<extent_t>(
                          "DASH_BULK_MIN_SIZE_BYTES"),
                        4096);
  if (bulk_min > 0) {
    DASH_LOG_TRACE("dash::make_team_spec",
                   "- optimizing for bulk min size", bulk_min);
    auto team_factors = dash::math::factorize(n_units);
    extent_t block_size = 1;
    for (auto d = 0; d < ndim; ++d) {
      auto block_extent_d = sizespec.extent(d) / team_extents[d];
      block_size *= block_extent_d;
    }
    // TODO: Need sizeof(T) instead of 8:
    if (block_size * 8 < bulk_min && small_factor_found > 0) {
      // Unbalance extents to increase block size:
      auto unbalance_factor = team_factors_d1.begin()->first;
      DASH_LOG_TRACE("dash::make_team_spec",
                     "- unbalancing with factor", unbalance_factor);
      team_extents[0] *= unbalance_factor;
      team_extents[1] /= unbalance_factor;
    }
  }

  DASH_LOG_TRACE("dash::make_team_spec >",
                 "step 5 - team extents after adjusting for bulk min size:",
                 team_extents);

  // Make distribution spec from template- and run time parameters:
  teamspec.resize(team_extents);
  return teamspec;
}

/**
 * \ingroup{DashPatternConcept}
 */
template<
  typename PartitioningTags,
  typename MappingTags,
  typename LayoutTags,
  class    SizeSpecType
>
TeamSpec<SizeSpecType::ndim(), typename SizeSpecType::index_type>
make_team_spec(
  /// Size spec of cartesian space to be distributed by the pattern.
  const SizeSpecType               & sizespec,
  dash::Team &                       team         = dash::Team::All(),
  typename SizeSpecType::size_type   n_nodes      = 0,
  typename SizeSpecType::size_type   n_numa_dom   = 0,
  typename SizeSpecType::size_type   n_cores      = 0)
{
  DASH_LOG_TRACE_VAR("dash::make_team_spec()", sizespec.extents());
  DASH_LOG_TRACE_VAR("dash::make_team_spec", team.size());

  dash::util::TeamLocality tloc(dash::Team::All());
  if (0 >= n_nodes) {
    n_nodes    = tloc.num_nodes();
    if (0 >= n_nodes) { n_nodes = 1; }
  }
  if (0 >= n_numa_dom) {
    n_numa_dom = tloc.domain().scope_domains(
                   dash::util::Locality::Scope::NUMA).size() / n_nodes;
    if (0 >= n_numa_dom) {
      n_numa_dom = tloc.domain().scope_domains(
                     dash::util::Locality::Scope::Package).size() / n_nodes;
    }
    if (0 >= n_numa_dom) { n_numa_dom = 1; }
  }
  if (0 >= n_cores) {
    n_cores    = tloc.num_cores();
    if (0 >= n_cores) { n_cores = 1; }
  }

  return make_team_spec<
           PartitioningTags,
           MappingTags,
           LayoutTags,
           SizeSpecType>(
             sizespec,
             team.size(),
             n_nodes,
             n_numa_dom,
             n_cores);
}

//////////////////////////////////////////////////////////////////////////////
// Generic Abstract Pattern Factories (dash::make_pattern)
//////////////////////////////////////////////////////////////////////////////

/**
 * Generic Abstract Factory for instances of \c dash::DistributionSpec.
 * Creates a DistributionSpec object from given pattern traits.
 *
 * \ingroup{DashPatternConcept}
 *
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
  std::array<dash::Distribution, ndim> distributions = {{ }};
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
    DASH_LOG_TRACE_VAR("dash::make_distribution_spec", distributions[d]);
  }
  // Make distribution spec from template- and run time parameters:
  DASH_LOG_TRACE_VAR("dash::make_distribution_spec >", distributions);
  dash::DistributionSpec<ndim> distspec(distributions);
  DASH_LOG_TRACE_VAR("dash::make_distribution_spec >", distspec);
  return distspec;
}

/**
 * Generic Abstract Factory for models of the Pattern concept.
 *
 * Creates an instance of a Pattern model that satisfies the contiguos
 * linearization property from given pattern traits.
 *
 * \ingroup{DashPatternConcept}
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
 * \ingroup{DashPatternConcept}
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
  typedef typename SizeSpecType::index_type                      index_t;
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
 * \ingroup{DashPatternConcept}
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
