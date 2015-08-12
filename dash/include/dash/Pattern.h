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
 * Distribution                 | Container                     |
 * ---------------------------- | ----------------------------- |
 * <tt>[ team 0 : team 1 ]</tt> | <tt>[ 0  1  2  3  4  5 ]</tt> |
 * <tt>[ team 1 : team 0 ]</tt> | <tt>[ 6  7  8  9 10 11 ]</tt> |
 *
 * This pattern would assign local indices to teams like this:
 * 
 * Team            | Local indices                 |
 * --------------- | ----------------------------- |
 * <tt>team 0</tt> | <tt>[ 0  1  2  9 10 11 ]</tt> |
 * <tt>team 1</tt> | <tt>[ 3  4  5  6  7  8 ]</tt> |
 *
 * \par Methods
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Index[D] coords(Index gi)
 *     </td>
 *     <td>
 *       Global linear offset to global cartesian coordinates
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Index at(Index[D] gp)
 *     </td>
 *     <td>
 *       Global point to local linear offset
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Index local_at(Index[D] lp)
 *     </td>
 *     <td>
 *       Lobal point to local linear offset
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c { dart_unit_t u, Index li } local(Index[D] gp)
 *     </td>
 *     <td>
 *       Global point to unit and local linear offset, inverse of
 *       \c global
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Index[D] global(dart_unit_t u, Index gi)
 *     </td>
 *     <td>
 *       Unit and lobal linear offset to global cartesian coordinates,
 *       inverse of \c local.
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c bool is_local(Index gi, dart_unit_t u)
 *     </td>
 *     <td>
 *       Whether element at global index \gi is local to unit \c u
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c bool is_local(dim_t d, Index go, dart_unit_t u)
 *     </td>
 *     <td>
 *       Whether any element in dimension \c d at global offset \go (such
 *       as a matrix row) is local to unit \c u
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Size capacity()
 *     </td>
 *     <td>
 *       Size of the cartesian index space, total number of elements in
 *       the pattern, alias of \c size()
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Size local_capacity()
 *     </td>
 *     <td>
 *       Maximum size of the local cartesian index space for any unit, i.e.
 *       maximum number of elements assigned to a single unit
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Size size()
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
 *       \c Size local_size()
 *     </td>
 *     <td>
 *       Actual number of elements assigned to the active unit
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Size extent(dim_t d)
 *     </td>
 *     <td>
 *       Number of elements in the pattern in dimension \c d
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \c Size local_extent(dim_t d)
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
