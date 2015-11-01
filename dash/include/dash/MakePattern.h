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

/**
 * Container type for mapping properties of models satisfying the Pattern
 * concept.
 */
struct pattern_mapping_tag
{
  typedef enum {
    any,
    /// Same number of blocks for every process
    balanced,
    /// Every process mapped in every row and column
    diagonal,
    /// Unit mapped to block different from units mapped to adjacent blocks
    remote_neighbors
  } type;
};

/**
 * Generic type of mapping properties of a model satisfying the Pattern
 * concept.
 *
 * Example: 
 *
 * \code
 *   typedef dash::pattern_mapping_traits<
 *             dash::pattern_mapping_tag::balanced,
 *             dash::pattern_mapping_tag::diagonal
 *           > my_pattern_mapping_traits;
 *
 *   auto pattern = dash::make_pattern<
 *                    // ...
 *                    my_pattern_mapping_traits
 *                    // ...
 *                  >(sizespec, teamspec);
 * \endcode
 *
 * Template parameter list is processed recursively by specializations of
 * \c dash::pattern_mapping_traits.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_traits
{
  static const bool balanced = false;
  static const bool diagonal = false;
  static const bool neighbor = false;
};

/**
 * Specialization of \c dash::pattern_mapping_traits to process tag
 * \c dash::pattern_mapping_tag::type::balanced in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_traits<
    pattern_mapping_tag::type::balanced,
    Tags ...>
: public pattern_mapping_traits<
    Tags ...>
{
  static const bool balanced = true;
};

/**
 * Specialization of \c dash::pattern_mapping_traits to process tag
 * \c dash::pattern_mapping_tag::type::diagonal in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_traits<
    pattern_mapping_tag::type::diagonal,
    Tags ...>
: public pattern_mapping_traits<
    Tags ...>
{
  static const bool diagonal = true;
};

/**
 * Specialization of \c dash::pattern_mapping_traits to process tag
 * \c dash::pattern_mapping_tag::type::neighbor in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_traits<
    pattern_mapping_tag::type::remote_neighbors,
    Tags ...>
: public pattern_mapping_traits<
    Tags ...>
{
  static const bool neighbor = true;
};

//////////////////////////////////////////////////////////////////////////////
// Pattern blocking properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_blocking_tag
{
  typedef enum {
    any,
    balanced,
    cache_sized
  } type;
};

template<
  pattern_blocking_tag::type ... Tags >
struct pattern_blocking_traits
{
  static const bool balanced    = false;
  static const bool cache_sized = false;
};

template<
  pattern_blocking_tag::type ... Tags >
struct pattern_blocking_traits<
    pattern_blocking_tag::type::balanced,
    Tags ...>
: public pattern_blocking_traits<
    Tags ...>
{
  static const bool balanced = true;
};

template<
  pattern_blocking_tag::type ... Tags >
struct pattern_blocking_traits<
    pattern_blocking_tag::type::cache_sized,
    Tags ...>
: public pattern_blocking_traits<
    Tags ...>
{
  static const bool cache_sized = true;
};

//////////////////////////////////////////////////////////////////////////////
// Default Pattern Traits Definitions
//////////////////////////////////////////////////////////////////////////////

typedef dash::pattern_blocking_traits<
          pattern_blocking_tag::any >
  pattern_blocking_default_traits;

typedef dash::pattern_mapping_traits<
          pattern_mapping_tag::any >
  pattern_mapping_default_traits;

typedef dash::pattern_linearization_traits<
          pattern_linearization_tag::any >
  pattern_linearization_default_traits;

//////////////////////////////////////////////////////////////////////////////
// Generic Abstract Pattern Factories (dash::make_pattern)
//////////////////////////////////////////////////////////////////////////////

/**
 * Generic Abstract Factory for instances of \c dash::DistributionSpec.
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
  typename BlockingTraits      = dash::pattern_blocking_default_traits,
  typename MappingTraits       = dash::pattern_mapping_default_traits,
  typename LinearizationTraits = dash::pattern_linearization_default_traits,
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
  // Tags in pattern property category 'Linearization'
  DASH_LOG_TRACE("dash::make_pattern", "-> Linearization properties:");
  DASH_LOG_TRACE("dash::make_pattern", "   - contiguous");
  DASH_LOG_TRACE("dash::make_pattern", "-> Blocking properties:");

  // Tags in pattern property category 'Blocking'
  if (BlockingTraits::balanced) {
    DASH_LOG_TRACE("dash::make_pattern", "   - balanced blocks");
  }
  if (BlockingTraits::cache_sized) {
    DASH_LOG_TRACE("dash::make_pattern", "   - cache sized blocks");
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
  typename BlockingTraits      = dash::pattern_blocking_default_traits,
  typename MappingTraits       = dash::pattern_mapping_default_traits,
  typename LinearizationTraits = dash::pattern_linearization_default_traits,
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
  // Tags in pattern property category 'Linearization'
  DASH_LOG_TRACE("dash::make_pattern", "-> Linearization properties:");
  DASH_LOG_TRACE("dash::make_pattern", "   - not contiguous");

  // Tags in pattern property category 'Blocking'
  DASH_LOG_TRACE("dash::make_pattern", "-> Blocking properties:");
  if (BlockingTraits::balanced) {
    DASH_LOG_TRACE("dash::make_pattern", "   - balanced blocks");
  }
  if (BlockingTraits::cache_sized) {
    DASH_LOG_TRACE("dash::make_pattern", "   - cache sized blocks");
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
