#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

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
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Index[D] coords(Index gi)
 *       \endcode
 *     </td>
 *     <td>
 *       Global linear offset to global cartesian coordinates
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Index at(Index[D] gp)
 *       \endcode
 *     </td>
 *     <td>
 *       Global point to local linear offset
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Index local_at(Index[D] lp)
 *       \endcode
 *     </td>
 *     <td>
 *       Lobal point to local linear offset
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       { dart_unit_t u, Index li } local(Index[D] gp)
 *       \endcode
 *     </td>
 *     <td>
 *       Global point to unit and local linear offset, inverse of
 *       \c global
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Index[D] global(dart_unit_t u, Index gi)
 *       \endcode
 *     </td>
 *     <td>
 *       Unit and lobal linear offset to global cartesian coordinates,
 *       inverse of \c local.
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       bool is_local(Index gi, dart_unit_t u)
 *       \endcode
 *     </td>
 *     <td>
 *       Whether element at global index \gi is local to unit \c u
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       bool is_local(dim_t d, Index go, dart_unit_t u)
 *       \endcode
 *     </td>
 *     <td>
 *       Whether any element in dimension \c d at global offset \go (such
 *       as a matrix row) is local to unit \c u
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Size capacity()
 *       \endcode
 *     </td>
 *     <td>
 *       Size of the cartesian index space, total number of elements in
 *       the pattern, alias of \c size()
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Size local_capacity()
 *       \endcode
 *     </td>
 *     <td>
 *       Maximum size of the local cartesian index space for any unit, i.e.
 *       maximum number of elements assigned to a single unit
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Size size()
 *       \endcode
 *     </td>
 *     <td>
 *       Maximum size of the local cartesian index space for any unit, i.e.
 *       maximum number of elements assigned to a single unit, alias of
 *       \c capacity().
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Size local_size()
 *       \endcode
 *     </td>
 *     <td>
 *       Actual number of elements assigned to the active unit
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Size extent(dim_t d)
 *       \endcode
 *     </td>
 *     <td>
 *       Number of elements in the pattern in dimension \c d
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       Size local_extent(dim_t d)
 *       \endcode
 *     </td>
 *     <td>
 *       Number of elements in the pattern in dimension \c d that are local
 *       to the active unit
 *     </td>
 *   </tr>
 *
 * </table>
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

#include <dash/LinearPattern.h>
#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/PatternIterator.h>

#endif // DASH__PATTERN_H_
