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
 * \see \c dash::view_traits
 *
 * \par Terminology
 *
 * A \b View is a mapping between two index sets, from a \b Domain space to
 * an \b Image space in the view's codomain.
 * Views can be chained such that the image obtained from the application of
 * a view expression can again act as the domain of other views.
 * The \b Origin of a view is the first domain in the view chain that has not
 * been created by a view expression.
 *
 * \par Expressions
 *
 * View Specifier        | Synopsis
 * --------------------- | --------------------------------------------------
 * <tt>dash::sub</tt>    | Subrange of domain in a specified dimension
 * <tt>dash::local</tt>  | Local subspace of domain
 * <tt>dash::global</tt> | Maps subspace to elements in global domain
 * <tt>dash::apply</tt>  | Obtain image of domain view (inverse of \c domain)
 * <tt>dash::domain</tt> | Obtain domain of view image (inverse of \c apply)
 * <tt>dash::origin</tt> | Obtain the view origin (root domain)
 * <tt>dash::blocks</tt> | Decompose domain into blocks
 * <tt>dash::block</tt>  | Subspace of decomposed domain in a specific block
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
 *
 * \endcode
 *
 * \}
 */

#include <dash/view/Sub.h>
#include <dash/view/Local.h>
#include <dash/view/Global.h>
#include <dash/view/Apply.h>
#include <dash/view/Domain.h>
#include <dash/view/Origin.h>

#include <dash/view/IndexSet.h>
#include <dash/view/CartView.h>

#include <dash/view/ViewMod.h>
#include <dash/view/ViewTraits.h>


#endif // DASH__VIEW_H__INCLUDED
