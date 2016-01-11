#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <dash/Types.h>
#include <sstream>
#include <iostream>

namespace dash {

/**
 * \defgroup  DashPatternConcept  Pattern Concept
 * Concept for distribution pattern of n-dimensional containers to
 * units in a team.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * A pattern realizes a projection of a global index range to a
 * local view:
 *
 * Team Spec                    | Container                     |
 * ---------------------------- | ----------------------------- |
 * <tt>[ unit 0 : unit 1 ]</tt> | <tt>[ 0  1  2  3  4  5 ]</tt> |
 * <tt>[ unit 1 : unit 0 ]</tt> | <tt>[ 6  7  8  9 10 11 ]</tt> |
 *
 * This pattern would assign local indices to teams like this:
 *
 * Team            | Local indices                 |
 * --------------- | ----------------------------- |
 * <tt>unit 0</tt> | <tt>[ 0  1  2  9 10 11 ]</tt> |
 * <tt>unit 1</tt> | <tt>[ 3  4  5  6  7  8 ]</tt> |
 *
 * \par Methods
 *
 * Return Type              | Method                     | Parameters                      | Description                                                                                                |
 * ------------------------ | -------------------------- | ------------------------------- | ---------------------------------------------------------------------------------------------------------- |
 * <tt>index</tt>           | <tt>local_at</tt>          | <tt>index[d] lp</tt>            | Linear local offset of the local point <i>lp</i> in local memory.                                          |
 * <tt>index</tt>           | <tt>global_at</tt>         | <tt>index[d] gp</tt>            | Global offset of the global point <i>gp</i> in the pattern's iteration order.                                 |
 * <tt>dart_unit_t</tt>     | <tt>unit_at</tt>           | <tt>index[d] gp</tt>            | Unit id mapped to the element at global point <i>p</i>                                                     |
 * <b>global to local</b>   | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
 * <tt>{unit,index}</tt>    | <tt>local</tt>             | <tt>index gi</tt>               | Unit and linear local offset at the global index <i>gi</i>                                                 |
 * <tt>{unit,index[d]}</tt> | <tt>local</tt>             | <tt>index[d] gp</tt>            | Unit and local coordinates at the global point <i>gp</i>                                                   |
 * <tt>{unit,index}</tt>    | <tt>local_index</tt>       | <tt>index[d] gp</tt>            | Unit and local linear offset at the global point <i>gp</i>                                                 |
 * <tt>point[d]</tt>        | <tt>local_coords</tt>      | <tt>index[d] gp</tt>            | Local coordinates at the global point <i>gp</i>                                                            |
 * <b>local to global<b>    | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
 * <tt>index[d]</tt>        | <tt>global</tt>            | <tt>unit u, index[d] lp</tt>    | Local coordinates <i>lp</i> of unit <i>u</i> to global coordinates                                         |
 * <tt>index</tt>           | <tt>global_index</tt>      | <tt>unit u, index[d] lp</tt>    | Local coordinates <i>lp</i> of unit <i>u</i> to global index                                               |
 * <tt>index[d]</tt>        | <tt>global</tt>            | <tt>index[d] lp</tt>            | Local coordinates <i>lp</i> of active unit to global coordinates                                           |
 * <tt>index</tt>           | <tt>global</tt>            | <tt>unit u, index li</tt>       | Local offset <i>li</i> of unit <i>u</i> to global index                                                    |
 * <tt>index</tt>           | <tt>global</tt>            | <tt>index li</tt>               | Local offset <i>li</i> of active unit to global index                                                      |
 * <b>blocks<b>             | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
 * <tt>size[d]</tt>         | <tt>blockspec</tt>         | &nbsp;                          | Number of blocks in all dimensions.                                                                        |
 * <tt>index</tt>           | <tt>block_at</tt>          | <tt>index[d] gp</tt>            | Global index of block at global coordinates <i>gp</i>                                                      |
 * <tt>viewspec</tt>        | <tt>block</tt>             | <tt>index gbi</tt>              | Offset and extent in global cartesian space of block at global block index <i>gbi</i>                      |
 * <tt>viewspec</tt>        | <tt>local_block</tt>       | <tt>index lbi</tt>              | Offset and extent in global cartesian space of block at local block index <i>lbi</i>                       |
 * <tt>viewspec</tt>        | <tt>local_block_local</tt> | <tt>index lbi</tt>              | Offset and extent in local cartesian space of block at local block index <i>lbi</i>                        |
 * <b>locality test</b>     | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
 * <tt>bool</tt>            | <tt>is_local</tt>          | <tt>index gi, unit u</tt>       | Whether the global index <i>gi</i> is mapped to unit <i>u</i>                                              |
 * <tt>bool</tt>            | <tt>is_local</tt>          | <tt>dim d, index o, unit u</tt> | (proposed) Whether any element in dimension <i>d</i> at global offset <i>o</i> is local to unit <i>u</i>   |
 * <b>size</b>              | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
 * <tt>size</tt>            | <tt>capacity</tt>          | &nbsp;                          | Maximum number of elements in the pattern in total                                                         |
 * <tt>size</tt>            | <tt>local_capacity</tt>    | &nbsp;                          | Maximum number of elements assigned to a single unit                                                       |
 * <tt>size</tt>            | <tt>size</tt>              | &nbsp;                          | Number of elements indexed in the pattern                                                                  |
 * <tt>size</tt>            | <tt>local_size</tt>        | &nbsp;                          | Number of elements local to the calling unit                                                               |
 * <tt>size[d]</tt>         | <tt>extents</tt>           | &nbsp;                          | Number of elements in the pattern in all dimension                                                         |
 * <tt>size</tt>            | <tt>extent</tt>            | <tt>dim d</tt>                  | Number of elements in the pattern in dimension <i>d</i>                                                    |
 * <tt>size[d]</tt>         | <tt>local_extents</tt>     | <tt>unit u</tt>                 | Number of elements local to the given unit, by dimension                                                   |
 * <tt>size</tt>            | <tt>local_extent</tt>      | <tt>dim d</tt>                  | Number of elements local to the calling unit in dimension <i>d</i>                                         |
 *
 * \}
 */


/* TODO:
 * Properties should not be specified as boolean (satisfiable: yes/no) but
 * as one of the states:
 *
 * - unspecified
 * - satisfiable
 * - unsatisfiable
 *
 * And constraints as one of the states:
 *
 * - strong
 * - weak
 */


//////////////////////////////////////////////////////////////////////////////
// Pattern linearization properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_layout_tag
{
  typedef enum {
    /// Unspecified layout property.
    any,

    /// Row major storage order, used by default.
    row_major,

    /// Column major storage order.
    col_major,

    /// Elements are contiguous in local memory within a single block.
    blocked,

    /// All local indices are mapped to a single logical index domain.
    canonical,

    /// Local element order corresponds to a logical linearization
    /// within single blocks (blocked) or within entire local memory
    /// (canonical).
    linear

  } type;
};

template< pattern_layout_tag::type ... Tags >
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

/**
 * Specialization of \c dash::pattern_layout_properties to process tag
 * \c dash::pattern_layout_tag::type::blocked in template parameter list.
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties<
         pattern_layout_tag::type::blocked, Tags ...>
: public pattern_layout_properties<Tags ...>
{
  /// Elements are contiguous in local memory within a single block.
  static const bool blocked   = true;
  static const bool canonical = false;
};

/**
 * Specialization of \c dash::pattern_layout_properties to process tag
 * \c dash::pattern_layout_tag::type::canonical in template parameter list.
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties<
         pattern_layout_tag::type::canonical, Tags ...>
: public pattern_layout_properties<Tags ...>
{
  /// All local indices are mapped to a single logical index domain.
  static const bool canonical = true;
  static const bool blocked   = false;
};

/**
 * Specialization of \c dash::pattern_layout_properties to process tag
 * \c dash::pattern_layout_tag::type::linear in template parameter list.
 */
template<pattern_layout_tag::type ... Tags>
struct pattern_layout_properties<
         pattern_layout_tag::type::linear, Tags ...>
: public pattern_layout_properties<Tags ...>
{
  /// Local element order corresponds to a logical linearization
  /// within single blocks (blocked) or within entire local memory
  /// (canonical).
  static const bool linear    = true;
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
 */
template< pattern_mapping_tag::type ... Tags >
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

  /// Blocks are assigned to processes like dealt from a deck of
  /// cards in every hyperplane, starting from first unit.
  static const bool cyclic     = false;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::balanced in template parameter list.
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::balanced, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// The number of assigned blocks is identical for every unit.
  static const bool balanced   = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::unbalanced in template parameter list.
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::unbalanced, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// The number of blocks assigned to units may differ.
  static const bool unbalanced = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::neighbor in template parameter list.
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::neighbor, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Adjacent blocks in any dimension are located at a remote unit.
  static const bool neighbor   = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::shifted in template parameter list.
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::shifted, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Units are mapped to blocks in diagonal chains in at least one
  /// hyperplane
  static const bool shifted    = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::diagonal in template parameter list.
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::diagonal, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Units are mapped to blocks in diagonal chains in all hyperplanes.
  static const bool diagonal   = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::cyclic in template parameter list.
 */
template<pattern_mapping_tag::type ... Tags>
struct pattern_mapping_properties<
         pattern_mapping_tag::type::cyclic, Tags ...>
: public pattern_mapping_properties<Tags ...>
{
  /// Blocks are assigned to processes like dealt from a deck of
  /// cards in every hyperplane, starting from first unit.
  static const bool cyclic     = true;
};

//////////////////////////////////////////////////////////////////////////////
// Pattern partitioning properties
//////////////////////////////////////////////////////////////////////////////

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
    unbalanced

  } type;
};

template< pattern_partitioning_tag::type ... Tags >
struct pattern_partitioning_properties
{
  typedef pattern_partitioning_tag::type tag_type;

  /// Partitioning properties defaults:

  /// Block extents are constant for every dimension.
  static const bool rectangular = false;

  /// Minimal number of blocks in every dimension, typically at most one
  /// block per unit.
  static const bool minimal     = false;

  /// All blocks have identical extents.
  static const bool regular     = false;

  /// All blocks have identical size.
  static const bool balanced    = false;

  /// Size of blocks may differ.
  static const bool unbalanced  = false;
};

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::rectangular in template parameter
 * list.
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::rectangular, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// Blocks are assigned to processes like dealt from a deck of
  /// cards in every hyperplane, starting from first unit.
  static const bool rectangular = true;
};

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::minimal in template parameter
 * list.
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::minimal, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// Minimal number of blocks in every dimension, typically at most one
  /// block per unit.
  static const bool minimal    = true;
};

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::regular in template parameter
 * list.
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::regular, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// All blocks have identical extents.
  static const bool regular    = true;
};

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::balanced in template parameter
 * list.
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::balanced, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// All blocks have identical size.
  static const bool balanced   = true;
};

/**
 * Specialization of \c dash::pattern_partitioning_properties to process tag
 * \c dash::pattern_partitioning_tag::type::unbalanced in template parameter
 * list.
 */
template<pattern_partitioning_tag::type ... Tags>
struct pattern_partitioning_properties<
         pattern_partitioning_tag::type::unbalanced, Tags ...>
: public pattern_partitioning_properties<Tags ...>
{
  /// All blocks have identical size.
  static const bool unbalanced = true;
};

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

} // namespace dash

#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/CSRPattern.h>
#include <dash/PatternIterator.h>
#include <dash/MakePattern.h>

#endif // DASH__PATTERN_H_
