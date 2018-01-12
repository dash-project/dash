#ifndef DASH__CONTAINER_H_
#define DASH__CONTAINER_H_

/**
 * \defgroup  DashContainerConcept  Container Concept
 * Concept of a distributed container.
 *
 * \see DashArrayConcept
 * \see DashMapConcept
 * \see DashMatrixConcept
 * \see DashViewConcept
 * \see DashRangeConcept
 * \see DashIteratorConcept
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
 * Type name                       | Description
 * ------------------------------- | --------------------------------------------------------------------------------------------------------------------
 * <tt>value_type</tt>             | Type of the container elements.
 * <tt>difference_type</tt>        | Integer type denoting a distance in cartesian index space.
 * <tt>index_type</tt>             | Integer type denoting an offset/coordinate in cartesian index space.
 * <tt>size_type</tt>              | Integer type denoting an extent in cartesian index space.
 * <tt>iterator</tt>               | Iterator on container elements in global index space.
 * <tt>const_iterator</tt>         | Iterator on const container elements in global index space.
 * <tt>reverse_iterator</tt>       | Reverse iterator on container elements in global index space.
 * <tt>const_reverse_iterator</tt> | Reverse iterator on const container elements in global index space.
 * <tt>reference</tt>              | Reference on container elements in global index space.
 * <tt>const_reference</tt>        | Reference on const container elements in global index space.
 * <tt>local_pointer</tt>          | Native pointer on local container elements.
 * <tt>const_local_pointer</tt>    | Native pointer on const local container elements.
 * <tt>view_type</tt>              | View specifier on container elements, model of \c DashViewConcept.
 * <tt>local_type</tt>             | Reference to local element range, allows range-based iteration.
 * <tt>pattern_type</tt>           | Concrete model of the Pattern concept that specifies the container's data distribution and cartesian access pattern.
 *
 * \par Member Functions
 *
 * Return Type              | Method                | Parameters                                              | Description
 * ------------------------ | --------------------- | ------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------
 * <tt>local_type</tt>      | <tt>local</tt>        | &nbsp;                                                  | Container proxy object representing a view specifier on the container's local elements.
 * <tt>pattern_type</tt>    | <tt>pattern</tt>      | &nbsp;                                                  | Object implementing the Pattern concept specifying the container's data distribution and iteration pattern.
 * <tt>iterator</tt>        | <tt>begin</tt>        | &nbsp;                                                  | Iterator referencing the first container element.
 * <tt>iterator</tt>        | <tt>end</tt>          | &nbsp;                                                  | Iterator referencing the element past the last container element.
 * <tt>Element *</tt>       | <tt>lbegin</tt>       | &nbsp;                                                  | Native pointer referencing the first local container element, same as <tt>local().begin()</tt>.
 * <tt>Element *</tt>       | <tt>lend</tt>         | &nbsp;                                                  | Native pointer referencing the element past the last local container element, same as <tt>local().end()</tt>.
 * <tt>size_type</tt>       | <tt>size</tt>         | &nbsp;                                                  | Number of elements in the container.
 * <tt>size_type</tt>       | <tt>local_size</tt>   | &nbsp;                                                  | Number of local elements in the container, same as <tt>local().size()</tt>.
 * <tt>bool</tt>            | <tt>is_local</tt>     | <tt>index_type gi</tt>                                  | Whether the element at the given linear offset in global index space <tt>gi</tt> is local.
 * <tt>bool</tt>            | <tt>allocate</tt>     | <tt>size_type n, DistributionSpec\<DD\> ds, Team t</tt> | Allocation of <tt>n</tt> container elements distributed in Team <tt>t</tt> as specified by distribution spec <tt>ds</tt>
 * <tt>void</tt>            | <tt>deallocate</tt>   | &nbsp;                                                  | Deallocation of the container and its elements.
 *
 * \par Non-member Functions
 *
 * Return Type              | Method                | Parameters                                            | Description
 * ------------------------ | --------------------- | ----------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------
 * <tt>C::size_type</tt>    | <tt>dash::size</tt>   | <tt>const C & container</tt>                          | Returns the size of a container
 * <tt>bool</tt>            | <tt>dash::empty</tt>  | <tt>const C & container</tt>                          | Checks whether a container is empty
 *
 * \}
 */

// Static containers:
#include<dash/Array.h>
#include<dash/Matrix.h>
#include<dash/Coarray.h>

// Dynamic containers:
#include<dash/List.h>
#include<dash/UnorderedMap.h>

#endif // DASH__CONTAINER_H_
