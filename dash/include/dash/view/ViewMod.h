#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/view/ViewTraits.h>


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
 * objects of a single ViewMod type. Expressions then could not be
 * evalated at compile-time, however.
 *
 * Currently, only two view modifier types seem to be required:
 * - ViewSubMod
 * - ViewBlockMod (-> ViewSubMod)
 * - ViewLocalMod
 *
 * However, View modifier types should subclass a common ViewMod base
 * class - or vice versa, following the policy pattern with the
 * operation specified as policy:
 *
 *   template <dim_t DimDiff, class OriginType>
 *   class ViewMod : OriginType
 *   {
 *      // ...
 *   }
 *
 * or:
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

/**
 * Monotype for the logical symbol that represents a view origin.
 */
class ViewOrigin
{
  typedef ViewOrigin self_t;

public:
  typedef dash::default_index_t   index_type;
  typedef self_t                 origin_type;

public:
  constexpr static bool is_local = false;

public:
  constexpr const origin_type & origin() const {
    return *this;
  }
};

template <>
struct view_traits<ViewOrigin> {
  constexpr static bool is_projection = false;
  constexpr static bool is_origin     = true;
  constexpr static bool is_local      = false;
};


template <
  dim_t DimDiff,
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::IndexType >
class ViewLocalMod
{
  typedef ViewLocalMod<DimDiff, OriginType, IndexType> self_t;

public:
  constexpr static dim_t dimdiff  = DimDiff;
  constexpr static bool  is_local = true;

  typedef OriginType    origin_type;
  typedef IndexType      index_type;

public:
  ViewLocalMod() = delete;

  ViewLocalMod(OriginType & origin)
  : _origin(origin)
  { }

  constexpr index_type begin() const {
    return _begin;
  }

  constexpr index_type end() const {
    return _end;
  }

  constexpr origin_type & origin() const {
    return _origin;
  }

  constexpr index_type size() const {
    return dash::distance(_begin, _end);
  }
  
  constexpr bool empty() const {
    return size() == 0;
  }

private:
  origin_type & _origin;
  index_type    _begin;
  index_type    _end;

}; // class ViewLocalMod

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType >
struct view_traits<ViewLocalMod<DimDiff, OriginType, IndexType> > {
  constexpr static bool is_projection = (DimDiff != 0);
  constexpr static bool is_origin     = false;
  constexpr static bool is_local      = true;
};


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
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::IndexType >
class ViewSubMod
{
  typedef ViewSubMod<DimDiff, OriginType, IndexType> self_t;

public:
  constexpr static dim_t dimdiff  = DimDiff;
  constexpr static bool  is_local =
                            dash::view_traits<OriginType>::is_local::value;

  typedef OriginType                        origin_type;
  typedef IndexType                          index_type;
  typedef typename OriginType::local_type    local_type;

public:
  ViewSubMod() = delete;

  ViewSubMod(
    OriginType & origin,
    IndexType    begin,
    IndexType    end)
  : _origin(origin), _begin(begin), _end(end)
  { }

  constexpr index_type begin() const {
    return _begin;
  }

  constexpr index_type end() const {
    return _end;
  }

  constexpr origin_type & origin() const {
    return _origin;
  }

  constexpr index_type size() const {
    return dash::distance(_begin, _end);
  }
  
  constexpr bool empty() const {
    return size() == 0;
  }

  constexpr local_type local() const {
    return local_type(*this);
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
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::index_type >
class ViewMod
{
  typedef ViewMod<DimDiff, OriginType, IndexType> self_t;

public:

  constexpr static dim_t dimdiff = DimDiff;

  typedef OriginType    origin_type;
  typedef IndexType      index_type;

public:
  ViewMod() = delete;

  template <dim_t SubDim>
  inline self_t & sub(index_type begin, index_type end) {
    // record this sub operation:
    _begin = begin;
    _end   = end;
    return *this;
  }

  template <dim_t SubDim>
  inline self_t & sub(index_type offset) {
    // record this sub operation:
    _begin = offset;
    _end   = offset;
    return *this;
  }

  constexpr index_type begin() const {
    return _begin;
  }

  constexpr index_type end() const {
    return _end;
  }

  constexpr const origin_type & origin() const {
    return _origin;
  }

  constexpr index_type size() const {
    return dash::distance(_begin, _end);
  }
  
  constexpr bool empty() const {
    return size() == 0;
  }

private:

  origin_type & _origin;
  index_type    _begin;
  index_type    _end;

}; // class ViewMod

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
