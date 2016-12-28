#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/View/ViewTraits.h>

/* TODO: Eventually, these probably are not public definitions.
 *       Move to namespace internal.
 *
 * Implementing view modifier chain as combination of command pattern
 * and chain of responsibility pattern.
 * For now, only compile-time projections/slices are supported such as:
 *
 *   sub<0>(10,20).sub<1>(30,40)
 *
 * but not run-time projections/slices like:
 *
 *   sub(0, { 10,20 }).sub(1, { 30,40 })
 *
 * A view composition is a chained application of view modifier types
 * that depend on the type of their predecessor in the chain.
 *
 * Example:
 *
 *  sub<0>(2).sub<1>(3,4)
 *  :         :
 *  |         |
 *  |         '--> ViewSubMod<0, ViewSubMod<-1, ViewOrigin> >
 *  |                            '------------.-----------'
 *  |                                         '--> parent
 *  '--> ViewSubMod<-1, ViewOrigin >
 *                      '----.---'
 *                           '--> parent
 *
 * Consequently, specific ViewMod types are defined for every modifier
 * category.
 * As an alternative, all view modifications could be stored in command
 * objects of a single ViewMod type. Expressions then could not be evalated
 * at compile-time, however.
 *
 * Currently, only two view modifier types seem to be required:
 * - ViewSubMod
 * - ViewBlockMod (-> ViewSubMod)
 * - ViewLocalMod
 *
 * However, View modifier types should subclass a common ViewMod base
 * class - or vice versa, following the policy pattern with the operation
 * specified as policy:
 *
 *
 *   template <dim_t DimDiff, class ViewModOperation>
 *   class ViewMod : ViewModOperation
 *   {
 *      // ...
 *   }
 *
 *   class ViewModSubOperation;
 *   // defines
 *   // - sub<N>(...)
 *   // - view_mod_op() { return sub<N>(...); }
 *
 *   ViewMod<0, ViewModSubOperation> view_sub(initializer_list);
 *   // - either calls view_mod_op(initializer_list) in constructor
 *   // - or provides method sub<N>(...) directly
 *
 */


namespace dash {

/*
 * TODO: The ViewMod types don't satisfy the View concept entirely as
 *       methods like extents(), offsets(), cannot be defined without
 *       a known pattern type.
 *       Also, view modifiers are not bound to a data domain (like an
 *       array address space), they do not provide access to elements.
 *
 *       Clarify/ensure that these unbound/unmaterialized/lightweight
 *       views cannot appear in expressions where they are considered
 *       as models of the View concept.
 *
 *       Does not seem problematic so far as bound- and unbound views
 *       have different types (would lead to compiler errors in worst
 *       case) and users should not be tempted to access elements 
 *       without specifying a data domain first:
 *
 *          matrix.sub<1>(1,2);            is bound to matrix element
 *                                         domain
 *
 *          os << sub<1>(1,2);             view unbound at this point
 *                                         and value access cannot be
 *                                         specified.
 *          os << sub<3>(10,20) << mat;    bound once container `mat`
 *                                         is passed.
 *
 *
 */


template <
  dim_t DimDiff,
  class OriginType,
  class IndexType = typename OriginType::IndexType >
class ViewSubMod
{
  typedef ViewSubMod<DimDiff, OriginType, IndexType> self_t;

public:
  constexpr static dim_t dimdiff = DimDiff;

public:
  ViewSubMod() = delete;

  ViewSubMod(
    OriginType & origin,
    IndexType    begin,
    IndexType    end)
  : _origin(origin), _begin(begin), _end(end)
  { }

  constexpr OriginType & origin() const {
    return _origin;
  }

private:
  OriginType & _origin;
  IndexType    _begin;
  IndexType    _end;

}; // class ViewSubMod


template <
  // Difference of view and origin dimensionality.
  // For example, when selecting a single row in the origin domain,
  // the view projection eliminates one dimension of the origin domain
  // difference of dimensionality is \f$(vdim - odim) = -1\f$.
  dim_t DimDiff,
  class IndexType
>
class ViewMod
{
  typedef ViewMod<DimDiff, IndexType> self_t;

public:

  constexpr static dim_t dimdiff = DimDiff;

  template <dim_t SubDim>
  inline self_t & sub(IndexType begin, IndexType end) {
    // record this sub operation:
    _begin = begin;
    _end   = end;
    return *this;
  }

  template <dim_t SubDim>
  inline self_t & sub(IndexType offset) {
    // record this sub operation:
    _begin = offset;
    _end   = offset;
    return *this;
  }

private:

  IndexType _begin;
  IndexType _end;

}; // class ViewMod

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
