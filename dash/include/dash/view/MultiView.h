#ifndef DASH__VIEW__MULTI_VIEW_H__INCLUDED
#define DASH__VIEW__MULTI_VIEW_H__INCLUDED

#include <dash/Types.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>

#include <vector>


namespace dash {

/**
 * Multidimensional view.
 *
 * Extends concepts outlined in the TS:
 *   
 *   "Multidimensional bounds, index and array_view"
 *   
 *   OpenSTD document number M3851
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3851.pdf
 *
 * For a related implementation in C++14, see:
 *
 * - https://github.com/Microsoft/GSL/blob/master/gsl/multi_span
 *
 * \par Terminology:
 *
 * \b Multidimensional View Properties
 *
 * - ndim:      number of dimensions in the view's origin domain
 * - rank:      number of view dimensions
 * - size:      total number of elements
 * - shape:     extents ordered by dimension
 * - offset:    base indices ordered by dimension
 * - bounds:    tuples of first and final index in all dimensions d as
 *              (offset(d), offset(d) + shape(d))
 *
 * In each dimension:
 *
 * - extent:    number of elements in the dimension, consequently same as
 *              the size of its range
 * - stride:    number of elements in a slice of the dimension
 *
 * \b Modifying Operations on Multidimensional Views
 *
 * - reshape:   change extents of rectangular view such that rank and size
 *              are unchanged.
 * - resize:    change size of rectangular view, rank is unchanged
 * - sub:       create sub-view from index range
 * - section:   sub-view with same rank
 * - slice:     sub-view with lower rank
 * - intersect: intersection of two or more multidimensional rectangular
 *              views; the resulting rectangular view could also be
 *              obtained from a sequence of resize operations
 *
 * \b Access Operations on Multidimensional Views
 *
 * C-style access:
 *
 * slice at offset 2 in first dimension and sub-range [3,5) in
 * second dimension:
 *
 * \code
 * nview[2][range(3,5)]
 * // same as:
 * sub<0>(2, sub<1>(3,5, nview))
 * \endcode
 *
 * Specifying unchanged dimensions of sub-views:
 *
 * \code
 * nview[2]*[4]
 * // same as:
 * sub<0>(2, sub<2>(4. nview))
 * \endcode
 *
 */
template <dim_t NDim>
class MultiView
{

public:

};


} // namespace dash

#endif // DASH__VIEW__MULTI_VIEW_H__INCLUDED
