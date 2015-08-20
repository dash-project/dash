#ifndef DASH__CONTAINER_H_
#define DASH__CONTAINER_H_

/**
 * \defgroup  DashContainerConcept  Container Concept
 * Concept of a distributed container.
 *
 * \see DashArrayConcept
 * \see DashMatrixConcept
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * A container in DASH is a set of elements of a the same type that 
 * is distributed to units in a team according to a specified distribution 
 * pattern. Container elements can be iterated canonically in global and
 * local memory.
 *
 * \par Types
 *
 * <table>
 *   <tr>
 *     <th>Type name</th>
 *     <th>Description</th>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::index_type
 *       \endcode
 *     </td>
 *     <td>
 *       Integer denoting an offset/coordinate in cartesian index space
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::size_type
 *       \endcode
 *     </td>
 *     <td>
 *       Integer denoting an extent in cartesian index space
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::pattern_type
 *       \endcode
 *     </td>
 *     <td>
 *       Concrete type implementing the Pattern concept that is used to
 *       distribute elements to units in a team.
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::local_type
 *       \endcode
 *     </td>
 *     <td>
 *       Reference to local range of the container, allows range-based
 *       iteration
 *     </td>
 *   </tr>
 *
 * </table>
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
 *       const pattern_type & pattern()
 *       \endcode
 *     </td>
 *     <td>
 *       Pattern used to distribute container elements
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *        local_type local
 *       \endcode
 *     </td>
 *     <td>
 *       Local range of the container, allows range-based
 *       iteration
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         GlobIter<Value> begin()
 *       \endcode
 *     </td>
 *     <td>
 *       Global iterator referencing the initial element
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         GlobIter<Value> end()
 *       \endcode
 *     </td>
 *     <td>
 *       Global iterator referencing past the final element
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         Value * lbegin()
 *       \endcode
 *     </td>
 *     <td>
 *       Native pointer to the initial local container element
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         Value * lend()
 *       \endcode
 *     </td>
 *     <td>
 *       Native pointer past the final local container element
 *     </td>
 *   </tr>
 *
 * </table>
 * \}
 */

#include<dash/Array.h>
#include<dash/Matrix.h>

#endif // DASH__CONTAINER_H_
