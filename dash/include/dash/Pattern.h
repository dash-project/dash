#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/PatternIterator.h>

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

} // namespace dash

#endif // DASH__PATTERN_H_
