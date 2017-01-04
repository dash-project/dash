#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/view/ViewTraits.h>
#include <dash/view/Local.h>

#ifndef DOXYGEN

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

#ifdef __TODO__

template <class ViewModType>
class ViewRangeExpression
{

public:

  ViewRangeExpression(
    ViewModType      & view_mod)
  : _view_mod(view_mod)
  { }

  constexpr auto begin() const -> decltype(_view_mod.apply()) {
    // lazy evaluation of _range here
    return _range.begin;
  }

  constexpr auto end() const -> decltype(_view_mod.apply()) {
    // lazy evaluation of _range here
    return _range.end;
  }

private:
  ViewModType & _view_mod;

};

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
  typedef ViewOrigin                                         origin_type;
  typedef ViewOrigin                                           view_type;
  typedef typename ViewOrigin::index_type                     index_type;

  typedef std::integral_constant<bool, false>              is_projection;
  typedef std::integral_constant<bool, true>               is_view;
  typedef std::integral_constant<bool, true>               is_origin;
  typedef std::integral_constant<bool, false>              is_local;
};

// ----------------------------------------------------------------------
// Forward-declaration: ViewSubMod
// ----------------------------------------------------------------------

template <
  dim_t DimDiff,
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::IndexType >
class ViewSubMod;


// ----------------------------------------------------------------------
// Forward-declaration: ViewLocalMod
// ----------------------------------------------------------------------

template <
  dim_t DimDiff,
  class OriginType = ViewOrigin,
  class IndexType  = typename OriginType::IndexType >
class ViewLocalMod;

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType >
struct view_traits<ViewLocalMod<DimDiff, OriginType, IndexType> > {
  typedef OriginType                                         origin_type;
  typedef ViewLocalMod<DimDiff, OriginType, IndexType>         view_type;
  typedef IndexType                                           index_type;

  typedef std::integral_constant<bool, (DimDiff != 0)>     is_projection;
  typedef std::integral_constant<bool, true>               is_view;
  typedef std::integral_constant<bool, false>              is_origin;
  typedef std::integral_constant<bool, true>               is_local;
};

// ----------------------------------------------------------------------
// ViewLocalMod
// ----------------------------------------------------------------------

template <
  class ViewLocalModType >
class ViewSubLocalIndexSet
{
  typedef ViewSubLocalIndexSet<ViewLocalModType> self_t;

public:
  typedef typename ViewLocalModType::index_type index_type;

public:
  ViewSubLocalIndexSet(ViewLocalModType & view_local_mod)
  : _view_local_mod(view_local_mod)
  { }

  self_t & apply() {
    if (!_ready) {
      typename ViewLocalModType::origin_type & origin_sub =
        dash::origin(_view_local_mod);

      // global index range of origin sub view:
      index_type sub_begin_gidx  = dash::index(dash::begin(origin_sub));
      index_type sub_end_gidx    = dash::index(dash::end(origin_sub));
      // global index range of local view:
      index_type sub_lbegin_gidx = dash::index(dash::begin(origin_sub));
      index_type sub_lend_gidx   = dash::index(dash::end(origin_sub));
      // local index range of sub local view:
      index_type sub_lbegin_lidx = 3; // dash::index(
                                      //   dash::begin(origin_sub));
      index_type sub_lend_lidx   = 5; // dash::index(
                                      //   dash::end(origin_sub));

      _begin_index = sub_lbegin_lidx;
      _end_index   = sub_lend_lidx;

      _ready = true;
    }
    return *this;
  }

  constexpr index_type begin() const {
    return _ready ? _begin_index : apply().begin();
  }

  constexpr index_type end() const {
    return _ready ? _end_index : apply().end();
  }

private:
  ViewLocalModType & _view_local_mod;
  index_type         _begin_index;
  index_type         _end_index;
  bool               _ready = false;
};

// ----------------------------------------------------------------------
// ViewLocalModBase
// ----------------------------------------------------------------------

template <
  class ViewLocalModType,
  bool  LocalOfSub >
class ViewLocalModBase;

// ----------------------------------------------------------------------
// View Local of Sub (array.sub.local)
//                             |
//                             '--> non-trivial case, range calculations
//
template <
  class ViewLocalModType >
class ViewLocalModBase<ViewLocalModType, true>
{
  typedef ViewLocalModBase<ViewLocalModType, true> self_t;
public:
  typedef typename view_traits<ViewLocalModType>::origin_type origin_type;
  typedef typename dash::ViewSubLocalMod<ViewLocalModType>      view_type;
  typedef typename view_traits<origin_type>::index_type        index_type;

  ViewLocalModBase(origin_type & origin_sub)
  : _origin(origin_sub), _range(*this)
  { }

public:
  
  constexpr view_type & apply() const {
    // e.g. called via
    //   dash::begin(dash::apply(*this))
    return _range;
  }

protected:
  origin_type                & _origin;
  // range created by application of this view modifier.
  view_type                    _range;
};


// ----------------------------------------------------------------------
// View Local of Origin (array.local.sub)
//                            |
//                            '--> trivial case
template <
  class ViewLocalModType >
class ViewLocalModBase<ViewLocalModType, false>
{
public:
  typedef typename view_traits<ViewLocalModType>::origin_type origin_type;
  typedef typename origin_type::local_type                      view_type;
  typedef typename origin_type::index_type                     index_type;

  ViewLocalModBase(origin_type & origin)
  : _origin(origin)
  { }

public:
  constexpr view_type & apply() const {
    // e.g. called via
    //   dash::begin(dash::apply(*this))
    return dash::local(_origin);
  }

protected:
  origin_type & _origin;
};

// ======================================================================

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType >
class ViewLocalMod
: public dash::ViewLocalModBase<
           ViewLocalMod<DimDiff, OriginType, IndexType>,
           dash::view_traits<OriginType>::is_view::value >
{
  typedef ViewLocalMod<DimDiff, OriginType, IndexType> self_t;

public:
  constexpr static dim_t dimdiff  = DimDiff;

  typedef std::integral_constant<bool, true>  is_local;

  typedef OriginType                       origin_type;
  typedef IndexType                         index_type;
  typedef self_t                            local_type;
  typedef typename origin_type::local_type   view_type;

public:

  ViewLocalMod() = delete;

  ViewLocalMod(OriginType & origin)
  : ViewLocalModBase<
      ViewLocalMod<DimDiff, OriginType, IndexType>,
      dash::view_traits<OriginType>::is_view::value
    >(origin)
  { }

  constexpr bool operator==(const self_t & rhs) const {
    return (this      == &rhs ||
            // Note: testing _origin for identity (identical address)
            //       instead of equality (identical value)
            &this->_origin == &rhs._origin);
  }
  
  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  constexpr view_type & begin() const {
    return dash::begin(dash::apply(*this)); // , dash::origin(*this)));
  }

  inline view_type & begin() {
    return dash::begin(dash::apply(*this)); // , dash::origin(*this)));
  }

  constexpr view_type & end() const {
    return dash::end(dash::apply(*this)); // , dash::origin(*this)));
  }

  inline view_type & end() {
    return dash::end(dash::apply(*this)); // , dash::origin(*this)));
  }

  constexpr const origin_type & origin() const {
    return this->_origin;
  }

  inline origin_type & origin() {
    return this->_origin;
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

}; // class ViewLocalMod

// --------------------------------------------------------------------
// ViewSubMod
// --------------------------------------------------------------------

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
  typedef ViewSubLocalMod<self_t>                  local_type;

public:
  ViewSubMod() = delete;

  ViewSubMod(
    OriginType & origin,
    IndexType    begin,
    IndexType    end)
  : _origin(origin), _begin(begin), _end(end), _local(*this)
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
  index_type    _begin;
  index_type    _end;
  local_type    _local;

}; // class ViewSubMod

} // namespace dash

#endif // DOXYGEN
#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
