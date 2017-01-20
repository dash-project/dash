#ifndef DASH__PATTERN_PROPERTIES_H__
#define DASH__PATTERN_PROPERTIES_H__

#include <dash/Types.h>
#include <sstream>
#include <iostream>

namespace dash {

/**
 * \defgroup  DashPatternProperties  Pattern Properties
 *
 * Property system for specification and deduction of pattern types.
 *
 * \ingroup DashPatternConcept
 * \{
 * \par Description
 *
 * The Pattern property system is based on type traits that provide a
 * classification of pattern types by their properties.
 *
 * All patterns realize a mapping of elements to addresses in physical memory
 * in three stages:
 *
 * - Partitioning: how elements are partitioned into blocks
 * - Mapping: how blocks are mapped to units
 * - Layout: how elements are arranged in the units' local memory
 *
 * Semantics of a pattern type are fully described by its properties in these
 * categories.
 *
 * \}
 *
 *
 * \todo
 * Properties should not be specified as boolean (satisfiable: yes/no) but
 * as one of the states:
 *
 * - unspecified
 * - satisfiable
 * - unsatisfiable
 *
 * ... and constraints as one of the states:
 *
 * - strong
 * - weak
 *
 * Traits concepts should work like this:
 *
 *   dash::pattern_traits<PatternType>::partitioning::balanced()
 *
 * and properties (currently named traits) should be renamed to tags.
 */


//////////////////////////////////////////////////////////////////////////////
// Pattern Layout Properties
//////////////////////////////////////////////////////////////////////////////

/**
 * \defgroup  DashPatternLayoutProperties  Pattern Layout Properties
 *
 * Pattern layout property category for specification and deduction of
 * pattern types.
 *
 * \ingroup DashPatternProperties
 * \{
 * \par Description
 *
 * Pattern properties describing the arrangement of distributed elements in
 * the units' physical memory.
 *
 * \}
 */

/**
 * \ingroup{DashPatternLayoutProperties}
 */
struct pattern_layout_tag
{
  typedef enum {
    /// Unspecified layout property.
    any,

    /// Row major storage order, used by default.
    row_major,

    /// Column major storage order.
    col_major,

    /// Elements are contiguous in local memory within a single block
    /// and thus indexed blockwise.
    blocked,

    /// All local indices are mapped to a single logical index domain
    /// and thus not indexed blockwise.
    canonical,

    /// Local element order corresponds to a logical linearization
    /// within single blocks (if blocked) or within entire local memory
    /// (if canonical).
    linear

  } type;
};

/**
 * \ingroup{DashPatternLayoutProperties}
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties
{
  typedef pattern_layout_tag::type tag_type;

  /// Layout properties defaults:

  /// Row major storage order.
  static const bool row_major = true;

  /// Column major storage order.
  static const bool col_major = false;

  /// Elements are contiguous in local memory within a single block.
  static const bool blocked   = false;

  /// All local indices are mapped to a single logical index domain.
  static const bool canonical = true;

  /// Local element order corresponds to a logical linearization
  /// within single blocks (blocked) or within entire local memory
  /// (canonical).
  static const bool linear    = false;
};

#ifndef DOXYGEN

/**
 * Specialization of \c dash::pattern_layout_properties to process tag
 * \c dash::pattern_layout_tag::type::blocked in template parameter list.
 *
 * \ingroup{DashPatternLayoutProperties}
 *
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties<
         pattern_layout_tag::type::blocked, Tags ...>
: public pattern_layout_properties<Tags ...>
{
  /// Elements are contiguous in local memory within a single block.
  static const bool blocked;
  static const bool canonical;
};

template<pattern_layout_tag::type ... Tags>
const bool
pattern_layout_properties<
  pattern_layout_tag::type::blocked, Tags ...
>::blocked = true;

template<pattern_layout_tag::type ... Tags>
const bool
pattern_layout_properties<
  pattern_layout_tag::type::blocked, Tags ...
>::canonical = false;

/**
 * Specialization of \c dash::pattern_layout_properties to process tag
 * \c dash::pattern_layout_tag::type::canonical in template parameter list.
 *
 * \ingroup{DashPatternLayoutProperties}
 *
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties<
         pattern_layout_tag::type::canonical, Tags ...>
: public pattern_layout_properties<Tags ...>
{
  /// All local indices are mapped to a single logical index domain.
  static const bool canonical;
  static const bool blocked;
};

template<pattern_layout_tag::type ... Tags>
const bool
pattern_layout_properties<
  pattern_layout_tag::type::canonical, Tags ...
>::blocked = false;

template<pattern_layout_tag::type ... Tags>
const bool
pattern_layout_properties<
  pattern_layout_tag::type::canonical, Tags ...
>::canonical = true;

/**
 * Specialization of \c dash::pattern_layout_properties to process tag
 * \c dash::pattern_layout_tag::type::linear in template parameter list.
 *
 * \ingroup{DashPatternLayoutProperties}
 *
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties<
         pattern_layout_tag::type::linear, Tags ...>
: public pattern_layout_properties<Tags ...>
{
  /// Local element order corresponds to a logical linearization
  /// within single blocks (blocked) or within entire local memory
  /// (canonical).
  static const bool linear;
};

template<pattern_layout_tag::type ... Tags>
const bool
pattern_layout_properties<
  pattern_layout_tag::type::linear, Tags ...
>::linear = true;

#endif // DOXYGEN

//////////////////////////////////////////////////////////////////////////////
// Pattern Mapping Properties
//////////////////////////////////////////////////////////////////////////////

/**
 * \defgroup  DashPatternMappingProperties  Pattern Mapping Properties
 *
 * Pattern mapping property category for specification and deduction of
 * pattern types.
 *
 * \ingroup DashPatternProperties
 * \{
 * \par Description
 *
 * Pattern properties describing the mapping element blocks to units in a
 * team.
 *
 * \}
 */

/**
 * Container type for mapping properties of models satisfying the Pattern
 * concept.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
struct pattern_mapping_tag
{
  typedef enum {
    /// Unspecified mapping property.
    any,

    /// The number of assigned blocks is identical for every unit.
    balanced,

    /// The number of blocks assigned to units may differ.
    unbalanced,

    /// Adjacent blocks in any dimension are located at a remote unit.
    neighbor,

    /// Units are mapped to blocks in diagonal chains in at least one
    /// hyperplane
    shifted,

    /// Units are mapped to blocks in diagonal chains in all hyperplanes.
    diagonal,

    /// Units are mapped to more than one block. For minimal partitioning,
    /// every unit is mapped to two blocks.
    multiple,

    /// Blocks are assigned to processes like dealt from a deck of
    /// cards in every hyperplane, starting from first unit.
    cyclic

  } type;
};

/**
 * Generic type of mapping properties of a model satisfying the Pattern
 * concept.
 *
 * Example:
 *
 * \code
 *   typedef dash::pattern_mapping_properties<
 *             dash::pattern_mapping_tag::balanced,
 *             dash::pattern_mapping_tag::diagonal
 *           > my_pattern_mapping_properties;
 *
 *   auto pattern = dash::make_pattern<
 *                    // ...
 *                    my_pattern_mapping_properties
 *                    // ...
 *                  >(sizespec, teamspec);
 * \endcode
 *
 * Template parameter list is processed recursively by specializations of
 * \c dash::pattern_mapping_properties.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties
{
  typedef pattern_mapping_tag::type tag_type;

  /// Mapping properties defaults:

  /// The number of assigned blocks is identical for every unit.
  static const bool balanced   = false;

  /// The number of blocks assigned to units may differ.
  static const bool unbalanced = false;

  /// Adjacent blocks in any dimension are located at a remote unit.
  static const bool neighbor   = false;

  /// Units are mapped to blocks in diagonal chains in at least one
  /// hyperplane
  static const bool shifted    = false;

  /// Units are mapped to blocks in diagonal chains in all hyperplanes.
  static const bool diagonal   = false;

  /// Units are mapped to more than one block. For minimal partitioning,
  /// every unit is mapped to two blocks.
  static const bool multiple   = false;

  /// Blocks are assigned to processes like dealt from a deck of
  /// cards in every hyperplane, starting from first unit.
  static const bool cyclic     = false;
};

#ifndef DOXYGEN

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::balanced in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::balanced, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// The number of assigned blocks is identical for every unit.
  static const bool balanced;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::balanced, Tags ...
>::balanced = true;

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::unbalanced in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::unbalanced, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// The number of blocks assigned to units may differ.
  static const bool unbalanced;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::unbalanced, Tags ...
>::unbalanced = true;

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::neighbor in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::neighbor, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Adjacent blocks in any dimension are located at a remote unit.
  static const bool neighbor;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::neighbor, Tags ...
>::neighbor = true;

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::shifted in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::shifted, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Units are mapped to blocks in diagonal chains in at least one
  /// hyperplane
  static const bool shifted;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::shifted, Tags ...
>::shifted = true;

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::diagonal in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::diagonal, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Units are mapped to blocks in diagonal chains in all hyperplanes.
  static const bool diagonal;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::diagonal, Tags ...
>::diagonal = true;

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::multiple in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::multiple, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Units are mapped to more than one block. For minimal partitioning,
  /// every unit is mapped to two blocks.
  static const bool multiple;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::multiple, Tags ...
>::multiple = true;

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::cyclic in template parameter list.
 *
 * \ingroup{DashPatternMappingProperties}
 *
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::cyclic, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Blocks are assigned to processes like dealt from a deck of
  /// cards in every hyperplane, starting from first unit.
  static const bool cyclic;
};

template<pattern_mapping_tag::type ... Tags>
const bool
pattern_mapping_properties<
  pattern_mapping_tag::type::cyclic, Tags ...
>::cyclic = true;

#endif // DOXYGEN

//////////////////////////////////////////////////////////////////////////////
// Pattern partitioning properties
//////////////////////////////////////////////////////////////////////////////

/**
 * \defgroup DashPatternPartitioningProperties Pattern Partitioning Properties
 *
 * Pattern partitioning property category for specification and deduction of
 * pattern types.
 *
 * \ingroup DashPatternProperties
 * \{
 * \par Description
 *
 * Pattern properties describing the partitioning of distributed elements
 * into blocks.
 *
 * \}
 */

/**
 * \ingroup{DashPatternPartitioningProperties}
 */
struct pattern_partitioning_tag
{
  typedef enum {
    any,

    /// Block extents are constant for every dimension.
    rectangular,

    /// Minimal number of blocks in every dimension, typically at most one
    /// block per unit.
    minimal,

    /// All blocks have identical extents.
    regular,

    /// All blocks have identical size.
    balanced,

    /// Size of blocks may differ.
    unbalanced,

    /// Data range is partitioned in at least two dimensions.
    ndimensional,

    /// Data range is partitioned dynamically.
    dynamic
  } type;
};

/**
 * \ingroup{DashPatternPartitioningProperties}
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties
{
  typedef pattern_partitioning_tag::type tag_type;

  /// Partitioning properties defaults:

  /// Block extents are constant for every dimension.
  static const bool rectangular  = false;

  /// Minimal number of blocks in every dimension, typically at most one
  /// block per unit.
  static const bool minimal      = false;

  /// All blocks have identical extents.
  static const bool regular      = false;

  /// All blocks have identical size.
  static const bool balanced     = false;

  /// Size of blocks may differ.
  static const bool unbalanced   = false;

  /// Data range is partitioned in at least two dimensions.
  static const bool ndimensional = false;

  /// Data range is partitioned dynamically.
  static const bool dynamic      = false;
};

#ifndef DOXYGEN

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::rectangular in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::rectangular, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// Blocks are assigned to processes like dealt from a deck of
  /// cards in every hyperplane, starting from first unit.
  static const bool rectangular;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::rectangular, Tags ...
>::rectangular = true;

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::minimal in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::minimal, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// Minimal number of blocks in every dimension, typically at most one
  /// block per unit.
  static const bool minimal;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::minimal, Tags ...
>::minimal = true;

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::regular in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::regular, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// All blocks have identical extents.
  static const bool regular;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::regular, Tags ...
>::regular = true;

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::balanced in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::balanced, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// All blocks have identical size.
  static const bool balanced;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::balanced, Tags ...
>::balanced = true;

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::unbalanced in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::unbalanced, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// All blocks have identical size.
  static const bool unbalanced;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::unbalanced, Tags ...
>::unbalanced = true;

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::ndimensional in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::ndimensional, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// Data range is partitioned in at least two dimensions.
  static const bool ndimensional;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::ndimensional, Tags ...
>::ndimensional = true;

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::dynamic in template parameter
 * list.
 *
 * \ingroup{DashPatternPartitioningProperties}
 *
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::dynamic, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// Data range is partitioned dynamically.
  static const bool dynamic;
};

template<pattern_partitioning_tag::type ... Tags>
const bool
pattern_partitioning_properties<
  pattern_partitioning_tag::type::dynamic, Tags ...
>::dynamic = true;

#endif // DOXYGEN

//////////////////////////////////////////////////////////////////////////////
// Pattern Traits Default Definitions
//////////////////////////////////////////////////////////////////////////////

template<typename PatternType>
struct pattern_partitioning_traits
{
  typedef typename PatternType::partitioning_properties
          type;
};

template<typename PatternType>
struct pattern_mapping_traits
{
  typedef typename PatternType::mapping_properties
          type;
};

template<typename PatternType>
struct pattern_layout_traits
{
  typedef typename PatternType::layout_properties
          type;
};

template<typename PatternType>
struct pattern_traits
{
  typedef typename PatternType::index_type
          index_type;
  typedef typename PatternType::size_type
          size_type;
  typedef typename dash::pattern_partitioning_traits<PatternType>::type
          partitioning;
  typedef typename dash::pattern_mapping_traits<PatternType>::type
          mapping;
  typedef typename dash::pattern_layout_traits<PatternType>::type
          layout;
};

//////////////////////////////////////////////////////////////////////////////
// Verifying Pattern Properties
//////////////////////////////////////////////////////////////////////////////

/**
 * Traits for compile- and run-time pattern constraints checking, suitable for
 * property checks where detailed error reporting is desired.
 *
 * \ingroup{DashPatternProperties}
 *
 */
template<
  typename PartitioningConstraints,
  typename MappingConstraints,
  typename LayoutConstraints,
  typename PatternType
>
bool check_pattern_constraints(
  const PatternType & pattern)
{
  // Pattern property traits of category Partitioning
  typedef typename dash::pattern_traits< PatternType >::partitioning
          partitioning_traits;
  // Pattern property traits of category Mapping
  typedef typename dash::pattern_traits< PatternType >::mapping
          mapping_traits;
  // Pattern property traits of category Layout
  typedef typename dash::pattern_traits< PatternType >::layout
          layout_traits;
  // Check compile-time invariants:

  // Partitioning properties:
  //
  static_assert(!PartitioningConstraints::rectangular ||
                partitioning_traits::rectangular,
                "Pattern does not implement rectangular partitioning");
  static_assert(!PartitioningConstraints::minimal ||
                partitioning_traits::minimal,
                "Pattern does not implement minimal partitioning");
  static_assert(!PartitioningConstraints::regular ||
                partitioning_traits::regular,
                "Pattern does not implement regular partitioning");
  static_assert(!PartitioningConstraints::balanced ||
                partitioning_traits::balanced,
                "Pattern does not implement balanced partitioning");
  static_assert(!PartitioningConstraints::unbalanced ||
                partitioning_traits::unbalanced,
                "Pattern does not implement unbalanced partitioning");
  // Mapping properties:
  //
  static_assert(!MappingConstraints::balanced ||
                mapping_traits::balanced,
                "Pattern does not implement balanced mapping");
  static_assert(!MappingConstraints::unbalanced ||
                mapping_traits::unbalanced,
                "Pattern does not implement unbalanced mapping");
  static_assert(!MappingConstraints::neighbor ||
                mapping_traits::neighbor,
                "Pattern does not implement neighbor mapping");
  static_assert(!MappingConstraints::shifted ||
                mapping_traits::shifted,
                "Pattern does not implement shifted mapping");
  static_assert(!MappingConstraints::diagonal ||
                mapping_traits::diagonal,
                "Pattern does not implement diagonal mapping");
  static_assert(!MappingConstraints::cyclic ||
                mapping_traits::cyclic,
                "Pattern does not implement cyclic mapping");
  // Layout properties:
  //
  static_assert(!LayoutConstraints::blocked ||
                layout_traits::blocked,
                "Pattern does not implement blocked layout");
  static_assert(!LayoutConstraints::canonical ||
                layout_traits::canonical,
                "Pattern does not implement canonical layout");
  static_assert(!LayoutConstraints::linear ||
                layout_traits::linear,
                "Pattern does not implement linear layout");
  return true;
}

/**
 * Traits for compile-time pattern constraints checking, suitable as a helper
 * for template definitions employing SFINAE where no verbose error reporting
 * is required.
 *
 * \ingroup{DashPatternProperties}
 *
 */
template<
  typename PartitioningConstraints,
  typename MappingConstraints,
  typename LayoutConstraints,
  typename PatternType
>
struct pattern_constraints
{
  // Pattern property traits of category Partitioning
  typedef typename dash::pattern_traits< PatternType >::partitioning
          partitioning_traits;
  // Pattern property traits of category Mapping
  typedef typename dash::pattern_traits< PatternType >::mapping
          mapping_traits;
  // Pattern property traits of category Layout
  typedef typename dash::pattern_traits< PatternType >::layout
          layout_traits;

  typedef std::integral_constant<
            bool,
            //
            // Partitioning properties:
            //
            ( !PartitioningConstraints::rectangular ||
              partitioning_traits::rectangular )
            &&
            ( !PartitioningConstraints::minimal ||
              partitioning_traits::minimal )
            &&
            ( !PartitioningConstraints::regular ||
              partitioning_traits::regular )
            &&
            ( !PartitioningConstraints::balanced ||
              partitioning_traits::balanced )
            &&
            ( !PartitioningConstraints::unbalanced ||
              partitioning_traits::unbalanced )
            &&
            //
            // Mapping properties:
            //
            ( !MappingConstraints::balanced ||
              mapping_traits::balanced )
            &&
            ( !MappingConstraints::unbalanced ||
              mapping_traits::unbalanced )
            &&
            ( !MappingConstraints::neighbor ||
              mapping_traits::neighbor )
            &&
            ( !MappingConstraints::shifted ||
              mapping_traits::shifted )
            &&
            ( !MappingConstraints::diagonal ||
              mapping_traits::diagonal )
            &&
            ( !MappingConstraints::cyclic ||
              mapping_traits::cyclic )
            &&
            //
            // Layout properties:
            //
            ( !LayoutConstraints::blocked ||
              layout_traits::blocked )
            &&
            ( !LayoutConstraints::canonical ||
              layout_traits::canonical )
            &&
            ( !LayoutConstraints::linear ||
              layout_traits::linear )
          > satisfied;
};

//////////////////////////////////////////////////////////////////////////////
// Default Pattern Traits Definitions
//////////////////////////////////////////////////////////////////////////////

typedef dash::pattern_partitioning_properties<
            pattern_partitioning_tag::rectangular,
            pattern_partitioning_tag::unbalanced >
        pattern_partitioning_default_properties;

typedef dash::pattern_mapping_properties<
            pattern_mapping_tag::unbalanced >
        pattern_mapping_default_properties;

typedef dash::pattern_layout_properties<
            pattern_layout_tag::row_major,
            pattern_layout_tag::canonical,
            pattern_layout_tag::linear >
        pattern_layout_default_properties;


#ifndef DOXYGEN

template<pattern_layout_tag::type ... Tags>
std::ostream & operator<<(
    std::ostream & os,
    const pattern_layout_properties<Tags ...> traits)
{
  std::ostringstream ss;
  ss << "dash::pattern_layout_properties< ";
  if (traits.row_major) {
    ss << "row_major ";
  }
  if (traits.col_major) {
    ss << "col_major ";
  }
  if (traits.blocked) {
    ss << "blocked ";
  }
  if (traits.canonical) {
    ss << "canonical ";
  }
  if (traits.linear) {
    ss << "linear ";
  }
  ss << ">";
  return operator<<(os, ss.str());
}

template<pattern_mapping_tag::type ... Tags>
std::ostream & operator<<(
    std::ostream & os,
    const pattern_mapping_properties<Tags ...> traits)
{
  std::ostringstream ss;
  ss << "dash::pattern_mapping_properties< ";
  if (traits.balanced) {
    ss << "balanced ";
  }
  if (traits.unbalanced) {
    ss << "unbalanced ";
  }
  if (traits.neighbor) {
    ss << "neighbor ";
  }
  if (traits.shifted) {
    ss << "shifted ";
  }
  if (traits.diagonal) {
    ss << "diagonal ";
  }
  if (traits.cyclic) {
    ss << "cyclic ";
  }
  ss << ">";
  return operator<<(os, ss.str());
}

template<pattern_partitioning_tag::type ... Tags>
std::ostream & operator<<(
    std::ostream & os,
    const pattern_partitioning_properties<Tags ...> traits)
{
  std::ostringstream ss;
  ss << "dash::pattern_partitioning_properties< ";
  if (traits.rectangular) {
    ss << "rectangular ";
  }
  if (traits.minimal) {
    ss << "minimal ";
  }
  if (traits.regular) {
    ss << "regular ";
  }
  if (traits.balanced) {
    ss << "balanced ";
  }
  if (traits.unbalanced) {
    ss << "unbalanced ";
  }
  ss << ">";
  return operator<<(os, ss.str());
}

#endif

} // namespace dash

#endif // DASH__PATTERN_PROPERTIES_H__
