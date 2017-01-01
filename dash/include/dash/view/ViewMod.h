#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/view/ViewTraits.h>
#include <dash/view/Local.h>


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
 *
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
 * TODO: Define dereference operator*() for view types, delegating to
 *       origin::operator* recursively.
 */


namespace dash {

#if 0
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

  typedef typename dash::view_traits<OriginType>::is_local is_local;

  typedef OriginType                                    origin_type;
  typedef IndexType                                      index_type;

  typedef ViewLocalMod<0, self_t, index_type>            local_type;

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
  
  inline Team & team(){
    return _origin.team();
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
#endif

// --------------------------------------------------------------------
// ViewOrigin
// --------------------------------------------------------------------

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
  typedef std::integral_constant<bool, false> is_local;

public:
  constexpr const origin_type & origin() const {
    return *this;
  }

  constexpr bool operator==(const self_t & rhs) const {
    return (this == &rhs);
  }
  
  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }
};

template <>
struct view_traits<ViewOrigin> {
  typedef std::integral_constant<bool, false> is_projection;
  typedef std::integral_constant<bool, true>  is_view;
  typedef std::integral_constant<bool, true>  is_origin;
  typedef std::integral_constant<bool, false> is_local;
};

// --------------------------------------------------------------------
// ViewLocalMod
// --------------------------------------------------------------------

template <
  dim_t DimDiff,
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::IndexType >
class ViewLocalMod;

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType >
class ViewLocalMod
{
  typedef ViewLocalMod<DimDiff, OriginType, IndexType> self_t;

public:
  constexpr static dim_t dimdiff  = DimDiff;

  typedef std::integral_constant<bool, true> is_local;

  typedef OriginType    origin_type;
  typedef IndexType      index_type;
  typedef self_t         local_type;

private:
  typedef typename origin_type::local_type origin_local_t;

public:

  ViewLocalMod() = delete;

  ViewLocalMod(OriginType & origin)
  : _origin(origin)
//, _begin(origin._begin)
//, _end(origin._end)
  { }

#if 0
  ViewLocalMod(OriginType & origin, index_type begin, index_type end)
  : _origin(origin)
//, _begin(begin)
//, _end(end)
  { }
#endif

  constexpr bool operator==(const self_t & rhs) const {
    return (this      == &rhs ||
            // Note: testing _origin for identity (identical address)
            //       instead of equality (identical value)
            (   &_origin == &rhs._origin
    //       && _begin   == rhs._begin &&
    //       && _end     == rhs._end
            ));
  }
  
  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  constexpr typename origin_type::local_type & begin() const {
    // return std::is_same<self_t, origin_local_t>::value
    //        ? origin_local_t(
    //            const_cast<origin_type &>(_origin), _begin, _begin+1)
    //        : dash::begin(dash::local(_origin)) + _begin;
    return dash::begin(dash::local(_origin));
  }

  inline typename origin_type::local_type & begin() {
    return dash::begin(dash::local(_origin));
  }

  constexpr typename origin_type::local_type & end() const {
    return dash::end(dash::local(_origin));
  }

  inline typename origin_type::local_type & end() {
    return dash::end(dash::local(_origin));
  }

  constexpr const origin_type & origin() const {
    return _origin;
  }

  inline origin_type & origin() {
    return _origin;
  }
  
  inline Team & team(){
    return _origin.team();
  }

  constexpr const local_type & local() const {
    return *this;
  }

  inline local_type & local() {
    return *this;
  }

  constexpr index_type size() const {
    return dash::distance(dash::begin(*this), dash::end(*this));
  }
  
  constexpr bool empty() const {
    return size() == 0;
  }

private:
  origin_type & _origin;
//index_type    _begin;
//index_type    _end;

}; // class ViewLocalMod

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType >
struct view_traits<ViewLocalMod<DimDiff, OriginType, IndexType> > {
  typedef std::integral_constant<bool, (DimDiff != 0)> is_projection;
  typedef std::integral_constant<bool, true>           is_view;
  typedef std::integral_constant<bool, false>          is_origin;
  typedef std::integral_constant<bool, true>           is_local;
};

// --------------------------------------------------------------------
// ViewSubMod
// --------------------------------------------------------------------

template <
  dim_t DimDiff,
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::IndexType >
class ViewSubMod;

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType >
class ViewSubMod
{
  typedef ViewSubMod<DimDiff, OriginType, IndexType> self_t;

  template <dim_t DD_, class OT_, class IT_>
  friend class ViewLocalMod;

public:
  constexpr static dim_t dimdiff  = DimDiff;

  typedef typename dash::view_traits<OriginType>::is_local is_local;

  typedef OriginType                              origin_type;
  typedef IndexType                                index_type;
  typedef ViewLocalMod<0, self_t, index_type>      local_type;

public:
  ViewSubMod() = delete;

  ViewSubMod(
    OriginType & origin,
    IndexType    begin,
    IndexType    end)
  : _origin(origin), _local(*this), _begin(begin), _end(end)
  { }

  constexpr bool operator==(const self_t & rhs) const {
    return (this      == &rhs ||
            // Note: testing _origin for identity (identical address)
            //       instead of equality (identical value)
            (&_origin == &rhs._origin &&
             _begin   == rhs._begin &&
             _end     == rhs._end));
  }
  
  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  constexpr auto begin() const
    -> decltype(dash::begin(dash::origin(*this))) {
    return dash::begin(_origin) + _begin;
  }

  inline auto begin()
    -> decltype(dash::begin(dash::origin(*this))) {
    return dash::begin(_origin) + _begin;
  }

  constexpr auto end() const
    -> decltype(dash::begin(dash::origin(*this))) {
    return dash::begin(_origin) + _end;
  }

  inline auto end()
    -> decltype(dash::begin(dash::origin(*this))) {
    return dash::begin(_origin) + _end;
  }

  constexpr const origin_type & origin() const {
    return _origin;
  }

  inline origin_type & origin() {
    return _origin;
  }
  
  inline Team & team(){
    return _origin.team();
  }

  constexpr const local_type & local() const {
    return _local;
  }

  inline local_type & local() {
    return _local;
  }

  constexpr index_type size() const {
    return dash::distance(dash::begin(*this), dash::end(*this));
  }
  
  constexpr bool empty() const {
    return size() == 0;
  }

private:
  origin_type & _origin;
  local_type    _local;
  index_type    _begin;
  index_type    _end;

}; // class ViewSubMod

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
