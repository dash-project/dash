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
 * In effect, a view expression can be understood as a composite function on
 * an index set.
 * The \b Origin of a view is the first domain in the view chain that has not
 * been created by a view expression; simply put, a view origin is usually a
 * container.
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

#include <dash/view/SetUnion.h>
#include <dash/view/SetIntersect.h>

#include <dash/view/IndexSet.h>
#include <dash/view/CartView.h>

#include <dash/view/ViewMod.h>
#include <dash/view/ViewTraits.h>

#include <dash/Range.h>


namespace dash {

template <
  class Iterator,
  class Sentinel >
class IteratorViewOrigin;

// Currently only supporting
//  - global iterators
//  - 1-dimensional IteratorViewOrigin types.

template <
  class Iterator,
  class Sentinel >
struct view_traits<IteratorViewOrigin<Iterator, Sentinel> > {
  typedef IteratorViewOrigin<Iterator, Sentinel>               domain_type;
  typedef IteratorViewOrigin<Iterator, Sentinel>               origin_type;
  typedef IteratorViewOrigin<Iterator, Sentinel>                image_type;

  // Uses container::local_type directly, e.g. dash::LocalArrayRef:
//typedef typename dash::view_traits<domain_type>::local_type   local_type;
  // Uses ViewLocalMod wrapper on domain, e.g. ViewLocalMod<dash::Array>:
  typedef ViewLocalMod<domain_type>                             local_type;
  typedef ViewGlobalMod<domain_type>                           global_type;

  typedef typename Iterator::index_type                         index_type;
  typedef dash::IndexSetIdentity< 
            IteratorViewOrigin<Iterator, Sentinel> >        index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, true>                 is_origin;
  typedef std::integral_constant<bool, false>                is_local;
};

template <
  class Iterator,
  class Sentinel
//class DomainType = dash::IteratorViewOriginOrigin<1>
>
class IteratorViewOrigin
: public dash::IteratorRange<Iterator, Sentinel>
{
public:
  typedef typename Iterator::index_type                         index_type;
private:
  typedef IteratorViewOrigin<Iterator, Sentinel>  self_t;
  typedef IteratorRange<Iterator, Sentinel>       base_t;
public:
  typedef self_t                                               domain_type;
  typedef self_t                                               origin_type;
  typedef self_t                                                image_type;
  // Alternative: IteratorLocalView<self_t, Iterator, Sentinel>
//typedef ViewLocalMod<domain_type>                             local_type;
//typedef ViewGlobalMod<domain_type>                           global_type;
  typedef typename view_traits<domain_type>::local_type         local_type;
  typedef typename view_traits<domain_type>::global_type       global_type;

  typedef typename Iterator::pattern_type                     pattern_type;
public:
  constexpr IteratorViewOrigin(Iterator begin, Iterator end)
  : base_t(std::move(begin), std::move(end)) {
  }

  constexpr const pattern_type & pattern() const {
    return this->begin().pattern();
  }

  constexpr local_type local() const {
    // for local_type: IteratorLocalView<self_t, Iterator, Sentinel>
    // return local_type(this->begin(), this->end());
    return local_type(*this);
  }

};


template <class Iterator, class Sentinel>
constexpr dash::IteratorViewOrigin<Iterator, Sentinel>
make_view(Iterator begin, Sentinel end) {
  return dash::IteratorViewOrigin<Iterator, Sentinel>(
           std::move(begin),
           std::move(end));
}

} // namespace dash


#endif // DASH__VIEW_H__INCLUDED
