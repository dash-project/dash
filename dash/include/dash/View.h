#ifndef DASH__VIEW_H__INCLUDED
#define DASH__VIEW_H__INCLUDED

/**
 * \defgroup  DashViewConcept  Multidimensional View Concept
 *
 * \ingroup DashNDimConcepts
 * \{
 * \par Description
 *
 * Definitions for multidimensional view expressions.
 * A view expression consumes a view object (its origin) and returns a view
 * object that applies the expression's modification on the consumed origin.
 *
 * The result of a view expression satisfies the multidimensional Range
 * concept.
 *
 * \see DashDimensionalConcept
 * \see DashRangeConcept
 * \see DashIteratorConcept
 * \see DashIndexSetConcept
 * \see \c dash::view_traits
 *
 * \par Terminology
 *
 * A \b View is a mapping from a \b Domain space to an \b Image space in the
 * view's codomain defined by their underlying index sets.
 * Views can be chained such that the image obtained from the application of
 * a view expression can again act as the domain of other views.
 * In effect, a view expression can be understood as a composite function on
 * an index set.
 * The \b Origin of a view is the first domain in the view chain that has not
 * been created by a view expression; simply put, a view origin is usually a
 * container.
 *
 * \par Expressions
 *
 * View Specifier            | Synopsis
 * ------------------------- | --------------------------------------------------
 * <tt>dash::sub</tt>        | Subrange of domain in a specified dimension
 * <tt>dash::intersect</tt>  | View from intersection of two domains
 * <tt>dash::difference</tt> | View from difference of two domains
 * <tt>dash::combine</tt>    | Composite view of two possibply unconnected domains
 * <tt>dash::local</tt>      | Local subspace of domain
 * <tt>dash::global</tt>     | Maps subspace to elements in global domain
 * <tt>dash::apply</tt>      | Obtain image of domain view (inverse of \c domain)
 * <tt>dash::domain</tt>     | Obtain domain of view image (inverse of \c apply)
 * <tt>dash::origin</tt>     | Obtain the view origin (root domain)
 * <tt>dash::blocks</tt>     | Decompose domain into blocks
 * <tt>dash::block</tt>      | Subspace of decomposed domain in a specific block
 * <tt>dash::index</tt>      | Returns a view's index set
 *
 * \par Examples
 *
 * \code
 * auto matrix_rect = dash::sub<0>(10,20,
 *                    dash::sub<1>(30,40,
 *                    matrix));
 *
 * auto matrix_rect_size = dash::size(matrix_rect);
 * // -> 10x10 = 100
 *
 * auto matrix_rect_begin_gidx = dash::index(dash::begin(matrix_rect));
 * auto matrix_rect_end_gidx   = dash::index(dash::end(matrix_rect));
 *
 * for (auto elem : matrix_rect) {
 *   // ...
 * }
 * \endcode
 *
 * \}
 */

#include <dash/view/Block.h>
#include <dash/view/Chunked.h>
#include <dash/view/Domain.h>
#include <dash/view/Origin.h>
#include <dash/view/Global.h>
#include <dash/view/Local.h>
#include <dash/view/Remote.h>
#include <dash/view/Apply.h>
#include <dash/view/Sub.h>

#include <dash/view/SetUnion.h>
#include <dash/view/SetIntersect.h>

#include <dash/view/IndexSet.h>
#include <dash/view/IndexRange.h>
#include <dash/view/CartView.h>

#include <dash/view/MultiView.h>
#include <dash/view/StridedView.h>

#include <dash/view/ViewTraits.h>

#include <dash/view/ViewMod.h>
#include <dash/view/ViewBlocksMod.h>

#endif // DASH__VIEW_H__INCLUDED
