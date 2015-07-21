#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>

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
 *   <tr>
 *     <td>
 *       \c coords(gindex)
 *     </td>
 *     <td>
 *       Returns the cartesian coordinates for a linear index
 *       within the pattern.
 *     </td>
 *   </tr>
 * </table>
 * \}
 */

} // namespace dash

#endif // DASH__PATTERN_H_
