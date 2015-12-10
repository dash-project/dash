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
 * <tt>index_type</tt>      | <tt>at</tt>                | <tt>index[d] gp</tt>            | Linear offset of the global point <i>gp</i> in local memory                                                |
 * <tt>index_type</tt>      | <tt>local_at</tt>          | <tt>index[d] lp</tt>            | Linear local offset of the local point <i>gp</i> in local memory                                           |
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

struct pattern_indexing_tag
{
  typedef enum {
    any,
    local_phase,
    local_strided
  } type;
};

template< pattern_indexing_tag::type >
struct pattern_indexing_properties
{
  static const bool local_phase   = false;
  static const bool local_strided = true;
};

template<>
struct pattern_indexing_properties<
  pattern_indexing_tag::local_phase >
{
  static const bool local_phase   = true;
  static const bool local_strided = false;
};

//////////////////////////////////////////////////////////////////////////////
// Pattern mapping properties
//////////////////////////////////////////////////////////////////////////////

/**
 * Container type for mapping properties of models satisfying the Pattern
 * concept.
 */
struct pattern_topology_tag
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
 *   typedef dash::pattern_topology_properties<
 *             dash::pattern_topology_tag::balanced,
 *             dash::pattern_topology_tag::diagonal
 *           > my_pattern_topology_properties;
 *
 *   auto pattern = dash::make_pattern<
 *                    // ...
 *                    my_pattern_topology_properties
 *                    // ...
 *                  >(sizespec, teamspec);
 * \endcode
 *
 * Template parameter list is processed recursively by specializations of
 * \c dash::pattern_topology_properties.
 */
template<
  pattern_topology_tag::type ... Tags >
struct pattern_topology_properties
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced   = false;
  static const bool diagonal   = false;
  static const bool neighbor   = false;
};

/**
 * Specialization of \c dash::pattern_topology_properties to process tag
 * \c dash::pattern_topology_tag::type::balanced in template parameter list.
 */
template<
  pattern_topology_tag::type ... Tags >
struct pattern_topology_properties<
    pattern_topology_tag::type::balanced,
    Tags ...>
: public pattern_topology_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced   = true;
};

/**
 * Specialization of \c dash::pattern_topology_properties to process tag
 * \c dash::pattern_topology_tag::type::diagonal in template parameter list.
 */
template<
  pattern_topology_tag::type ... Tags >
struct pattern_topology_properties<
    pattern_topology_tag::type::diagonal,
    Tags ...>
: public pattern_topology_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool diagonal = true;
};

/**
 * Specialization of \c dash::pattern_topology_properties to process tag
 * \c dash::pattern_topology_tag::type::neighbor in template parameter list.
 */
template<
  pattern_topology_tag::type ... Tags >
struct pattern_topology_properties<
    pattern_topology_tag::type::remote_neighbors,
    Tags ...>
: public pattern_topology_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
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
    unbalanced,
    cache_align
  } type;
};

template<
  pattern_blocking_tag::type ... Tags >
struct pattern_blocking_properties
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced    = false;
  static const bool cache_align = false;
};

template<
  pattern_blocking_tag::type ... Tags >
struct pattern_blocking_properties<
    pattern_blocking_tag::type::balanced,
    Tags ...>
: public pattern_blocking_properties<
    Tags ...>
{
  // TODO: Should be
  //   typedef typename std::integral_constant<bool, false> the_property;
  static const bool balanced = true;
};

template<
  pattern_blocking_tag::type ... Tags >
struct pattern_blocking_properties<
    pattern_blocking_tag::type::cache_align,
    Tags ...>
: public pattern_blocking_properties<
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
struct pattern_blocking_traits
{
  typedef typename PatternType::blocking_properties
          type;
};

template<typename PatternType>
struct pattern_topology_traits
{
  typedef typename PatternType::topology_properties
          type;
};

template<typename PatternType>
struct pattern_indexing_traits
{
  typedef typename PatternType::indexing_properties
          type;
};

template<typename PatternType>
struct pattern_traits
{
  typedef typename PatternType::index_type
          index_type;
  typedef typename PatternType::size_type
          size_type;
  typedef typename dash::pattern_blocking_traits<PatternType>::type
          blocking;
  typedef typename dash::pattern_topology_traits<PatternType>::type
          topology;
  typedef typename dash::pattern_indexing_traits<PatternType>::type
          indexing;
};

//////////////////////////////////////////////////////////////////////////////
// Verifying Pattern Properties
//////////////////////////////////////////////////////////////////////////////

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
  typedef typename dash::pattern_traits< PatternType >::blocking
          pattern_blocking_traits;
  // Pattern property traits of category Topology
  typedef typename dash::pattern_traits< PatternType >::topology
          pattern_topology_traits;
  // Pattern property traits of category Indexing
  typedef typename dash::pattern_traits< PatternType >::indexing
          pattern_indexing_traits;
  // Check compile-time invariants:
  static_assert(!BlockingConstraints::balanced ||
                pattern_blocking_traits::balanced,
                "Pattern does not implement balanced blocking property");
  static_assert(!IndexingConstraints::local_phase ||
                pattern_indexing_traits::local_phase,
                "Pattern does not implement local phase indexing property");
  static_assert(!TopologyConstraints::diagonal ||
                pattern_topology_traits::diagonal,
                "Pattern does not implement diagonal topology property");
  return true;
}

//////////////////////////////////////////////////////////////////////////////
// Default Pattern Traits Definitions
//////////////////////////////////////////////////////////////////////////////

typedef dash::pattern_blocking_properties<
          pattern_blocking_tag::any >
  pattern_blocking_default_properties;

typedef dash::pattern_topology_properties<
          pattern_topology_tag::any >
  pattern_topology_default_properties;

typedef dash::pattern_indexing_properties<
          pattern_indexing_tag::any >
  pattern_indexing_default_properties;

} // namespace dash

#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/PatternIterator.h>

#endif // DASH__PATTERN_H_
