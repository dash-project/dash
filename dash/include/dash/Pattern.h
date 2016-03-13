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
 * <b>local to global</b>   | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
 * <tt>index[d]</tt>        | <tt>global</tt>            | <tt>unit u, index[d] lp</tt>    | Local coordinates <i>lp</i> of unit <i>u</i> to global coordinates                                         |
 * <tt>index</tt>           | <tt>global_index</tt>      | <tt>unit u, index[d] lp</tt>    | Local coordinates <i>lp</i> of unit <i>u</i> to global index                                               |
 * <tt>index[d]</tt>        | <tt>global</tt>            | <tt>index[d] lp</tt>            | Local coordinates <i>lp</i> of active unit to global coordinates                                           |
 * <tt>index</tt>           | <tt>global</tt>            | <tt>unit u, index li</tt>       | Local offset <i>li</i> of unit <i>u</i> to global index                                                    |
 * <tt>index</tt>           | <tt>global</tt>            | <tt>index li</tt>               | Local offset <i>li</i> of active unit to global index                                                      |
 * <b>blocks</b>            | &nbsp;                     | &nbsp;                          | &nbsp;                                                                                                     |
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


} // namespace dash

#include <dash/BlockPattern.h>
#include <dash/TilePattern.h>
#include <dash/ShiftTilePattern.h>
#include <dash/SeqTilePattern.h>
#include <dash/CSRPattern.h>
#include <dash/PatternIterator.h>
#include <dash/PatternProperties.h>
#include <dash/MakePattern.h>

#endif // DASH__PATTERN_H_
