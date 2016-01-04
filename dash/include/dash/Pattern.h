#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <dash/Types.h>

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
 * <tt>index</tt>           | <tt>at</tt>                | <tt>index[d] gp</tt>            | Linear offset of the global point <i>gp</i> in local memory                                                |
 * <tt>index</tt>           | <tt>local_at</tt>          | <tt>index[d] lp</tt>            | Linear local offset of the local point <i>gp</i> in local memory                                           |
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

//////////////////////////////////////////////////////////////////////////////
// Pattern linearization properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_layout_tag
{
  typedef enum {
    any,
    local_phase,
    local_strided
  } type;
};

template< pattern_layout_tag::type >
struct pattern_layout_properties
{
  static const bool local_phase   = false;
  static const bool local_strided = true;
};

template<>
struct pattern_layout_properties<
  pattern_layout_tag::local_phase >
{
  static const bool local_phase   = true;
  static const bool local_strided = false;
};

template<>
struct pattern_layout_properties<
  pattern_layout_tag::local_strided >
{
  static const bool local_phase   = false;
  static const bool local_strided = true;
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
    /// Same number of blocks mapped to every process
    balanced,
    /// Varying number of blocks mapped to processes
    unbalanced,
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
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_properties
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced   = false;
  static const bool unbalanced = false;
  static const bool diagonal   = false;
  static const bool neighbor   = false;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::balanced in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_properties<
    pattern_mapping_tag::type::balanced,
    Tags ...>
: public pattern_mapping_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced   = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::unbalanced in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_properties<
    pattern_mapping_tag::type::unbalanced,
    Tags ...>
: public pattern_mapping_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced   = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::diagonal in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_properties<
    pattern_mapping_tag::type::diagonal,
    Tags ...>
: public pattern_mapping_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool diagonal = true;
};

/**
 * Specialization of \c dash::pattern_mapping_properties to process tag
 * \c dash::pattern_mapping_tag::type::neighbor in template parameter list.
 */
template<
  pattern_mapping_tag::type ... Tags >
struct pattern_mapping_properties<
    pattern_mapping_tag::type::remote_neighbors,
    Tags ...>
: public pattern_mapping_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool neighbor = true;
};

//////////////////////////////////////////////////////////////////////////////
// Pattern partitioning properties
//////////////////////////////////////////////////////////////////////////////

struct pattern_partitioning_tag
{
  typedef enum {
    any,
    balanced,
    unbalanced,
    cache_align
  } type;
};

template<
  pattern_partitioning_tag::type ... Tags >
struct pattern_partitioning_properties
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced    = false;
  static const bool cache_align = false;
};

template<
  pattern_partitioning_tag::type ... Tags >
struct pattern_partitioning_properties<
    pattern_partitioning_tag::type::balanced,
    Tags ...>
: public pattern_partitioning_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced = true;
};

template<
  pattern_partitioning_tag::type ... Tags >
struct pattern_partitioning_properties<
    pattern_partitioning_tag::type::cache_align,
    Tags ...>
: public pattern_partitioning_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool cache_align = true;
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
  typename BlockingConstraints,
  typename TopologyConstraints,
  typename IndexingConstraints,
  typename PatternType
>
bool check_pattern_constraints(
  const PatternType & pattern)
{
  // Pattern property traits of category Blocking
  typedef typename dash::pattern_traits< PatternType >::partitioning
          pattern_partitioning_traits;
  // Pattern property traits of category Topology
  typedef typename dash::pattern_traits< PatternType >::mapping
          pattern_mapping_traits;
  // Pattern property traits of category Indexing
  typedef typename dash::pattern_traits< PatternType >::layout
          pattern_layout_traits;
  // Check compile-time invariants:
  static_assert(!BlockingConstraints::balanced ||
                pattern_partitioning_traits::balanced,
                "Pattern does not implement balanced partitioning property");
  static_assert(!IndexingConstraints::local_phase ||
                pattern_layout_traits::local_phase,
                "Pattern does not implement local phase layout property");
  static_assert(!TopologyConstraints::diagonal ||
                pattern_mapping_traits::diagonal,
                "Pattern does not implement diagonal mapping property");
  return true;
}

/**
 * Traits for compile-time pattern constraints checking, suitable as a helper
 * for template definitions employing SFINAE where no verbose error reporting
 * is required.
 * 
 */
template<
  typename BlockingConstraints,
  typename TopologyConstraints,
  typename IndexingConstraints,
  typename PatternType
>
struct pattern_constraints
{
  // Pattern property traits of category Blocking
  typedef typename dash::pattern_traits< PatternType >::partitioning
          pattern_partitioning_traits;
  // Pattern property traits of category Topology
  typedef typename dash::pattern_traits< PatternType >::mapping
          pattern_mapping_traits;
  // Pattern property traits of category Indexing
  typedef typename dash::pattern_traits< PatternType >::layout
          pattern_layout_traits;

  typedef std::integral_constant<
            bool,
            ( !BlockingConstraints::balanced ||
              pattern_partitioning_traits::balanced )
            &&
            ( !IndexingConstraints::local_phase ||
              pattern_layout_traits::local_phase )
            &&
            ( !TopologyConstraints::diagonal ||
              pattern_mapping_traits::diagonal ) >
          satisfied;
};

//////////////////////////////////////////////////////////////////////////////
// Default Pattern Traits Definitions
//////////////////////////////////////////////////////////////////////////////

typedef dash::pattern_partitioning_properties<
          pattern_partitioning_tag::any >
  pattern_partitioning_default_properties;

typedef dash::pattern_mapping_properties<
          pattern_mapping_tag::any >
  pattern_mapping_default_properties;

typedef dash::pattern_layout_properties<
          pattern_layout_tag::any >
  pattern_layout_default_properties;

} // namespace dash

#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/CSRPattern.h>
#include <dash/PatternIterator.h>
#include <dash/MakePattern.h>

#endif // DASH__PATTERN_H_
