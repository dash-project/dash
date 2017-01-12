#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewTraits.h>

#include <dash/view/Local.h>
#include <dash/view/Global.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/Apply.h>


namespace dash {

} // namespace dash

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
 *   template <dim_t DimDiff, class DomainType>
 *   class ViewMod : DomainType
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
 *       domain::operator* recursively.
 */


namespace dash {

// --------------------------------------------------------------------
// ViewOrigin
// --------------------------------------------------------------------

/**
 * Monotype for the logical symbol that represents a view origin.
 */
template <dim_t NDim = 1>
class ViewOrigin
{
  typedef ViewOrigin self_t;

public:
  typedef dash::default_index_t                                 index_type;
  typedef self_t                                               domain_type;
  typedef IndexSetIdentity<self_t>                          index_set_type;

public:
  typedef std::integral_constant<bool, false> is_local;

public:

  constexpr explicit ViewOrigin(
      std::initializer_list<index_type> extents)
  : _extents(extents)
  { }

  constexpr const domain_type & domain() const {
    return *this;
  }

  inline domain_type & domain() {
    return *this;
  }

  constexpr index_set_type index_set() const {
    return IndexSetIdentity<self_t>(*this);
  }

  constexpr bool operator==(const self_t & rhs) const {
    return (this == &rhs);
  }
  
  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  template <dim_t ExtentDim = 0>
  constexpr index_type extent() const {
    return _extents[ExtentDim];
  }
  
  constexpr index_type size() const {
    return _size<0>();
  }

private:
  template <dim_t SizeDim = 0>
  constexpr index_type _size() const {
    return extent<SizeDim>() +
             (SizeDim < NDim
               ? _size<SizeDim + 1>()
               : 0);
  }

private:
  std::array<index_type, NDim> _extents = { };
};

template <dim_t NDim>
struct view_traits<ViewOrigin<NDim>> {
  typedef ViewOrigin<NDim>                                       origin_type;
  typedef ViewOrigin<NDim>                                       domain_type;
  typedef ViewOrigin<NDim>                                        image_type;
  typedef typename ViewOrigin<NDim>::index_type                   index_type;
  typedef typename ViewOrigin<NDim>::index_set_type           index_set_type;

  typedef std::integral_constant<bool, false>                  is_projection;
  typedef std::integral_constant<bool, true>                   is_view;
  typedef std::integral_constant<bool, true>                   is_origin;
  typedef std::integral_constant<bool, false>                  is_local;
};



// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------

template <
  class ViewModType,
  class DomainType >
class ViewModBase;

template <
  class DomainType = ViewOrigin<1> >
class ViewLocalMod;

template <
  class DomainType = ViewOrigin<1> >
class ViewGlobalMod;

template <
  class DomainType = ViewOrigin<1>,
  dim_t SubDim     = 0 >
class ViewSubMod;


// ------------------------------------------------------------------------
// ViewModBase
// ------------------------------------------------------------------------

template <class ViewModType, class DomainType>
class ViewModBase
{
  typedef ViewModBase<ViewModType, DomainType> self_t;
public:
  typedef DomainType                                             domain_type;
  typedef typename view_traits<DomainType>::index_type            index_type;

  constexpr explicit ViewModBase(
    const domain_type & dom)
  : _domain(&dom)
  { }

  constexpr bool operator==(const ViewModType & rhs) const {
    return (static_cast<const ViewModType *>(this) == &rhs ||
            // Note: testing _domain for identity (identical address)
            //       instead of equality (identical value)
            this->_domain == rhs._domain);
  }
  
  constexpr bool operator!=(const ViewModType & rhs) const {
    return !(*(static_cast<const ViewModType *>(this) == rhs));
  }

  constexpr bool is_local() const {
    return view_traits<ViewModType>::is_local::value;
  }

  constexpr const domain_type & domain() const {
    return *_domain;
  }

  inline domain_type & domain() {
    return *_domain;
  }

private:
//constexpr const ViewModType & this_view() const {
//  return *static_cast<const ViewModType *>(this);
//}

protected:
  const domain_type * _domain = nullptr;
};


// ------------------------------------------------------------------------
// ViewLocalMod
// ------------------------------------------------------------------------

template <
  class DomainType >
struct view_traits<ViewLocalMod<DomainType> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef ViewGlobalMod<DomainType>                             local_type;
  typedef domain_type                                          global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef dash::IndexSetLocal< ViewLocalMod<DomainType> >   index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool, true>                 is_local;
};

template <
  class DomainType >
class ViewLocalMod
: public ViewModBase< ViewLocalMod<DomainType>, DomainType >
{
public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef typename DomainType::index_type                       index_type;
private:
  typedef ViewLocalMod<DomainType>                                  self_t;
  typedef ViewModBase< ViewLocalMod<DomainType>, DomainType >       base_t;
public:
  typedef dash::IndexSetLocal< ViewLocalMod<DomainType> >   index_set_type;
  typedef self_t                                                local_type;
  typedef typename domain_type::global_type                    global_type;

  typedef std::integral_constant<bool, true>                      is_local;

  constexpr explicit ViewLocalMod(
    const DomainType & domain)
  : base_t(domain),
    _index_set(*this)
  { }

  constexpr bool operator==(const self_t & rhs) const {
    return (this == &rhs ||
             ( base_t::operator==(rhs) &&
               _index_set == rhs._index_set ) );
  }

  constexpr bool operator!=(const self_t & rhs) const {
    return not (*this == rhs);
  }

  constexpr auto begin() const
  -> decltype(dash::begin(dash::local(dash::origin(*this)))) {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this
               )
             )
           )
         + dash::index(dash::local(dash::domain(*this))).pre()[
             dash::index(dash::local(dash::domain(*this)))[0]
           ];
  }

  constexpr auto end() const
  -> decltype(dash::end(dash::local(dash::origin(*this)))) {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this
               )
             )
           )
    // TODO: Using 
    //
    //   + _index_set.size()
    //
    //       ... instead should work here
         + dash::index(dash::local(dash::domain(*this))).pre()[
             *dash::end(dash::index(dash::local(dash::domain(*this))))-1
           ] + 1;
  }

  constexpr auto operator[](int offset) const
  -> decltype(*(dash::begin(
                 dash::local(dash::origin(dash::domain(*this)))))) {
    return *(this->begin() + offset);
  }

  constexpr index_type size() const {
    return _index_set.size();
  }

  constexpr const local_type & local() const {
    return *this;
  }

  inline local_type & local() {
    return *this;
  }

  constexpr const global_type & global() const {
    return dash::global(dash::domain(*this));
  }

  inline global_type & global() {
    return dash::global(dash::domain(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

private:
  index_set_type  _index_set;
};


// ------------------------------------------------------------------------
// ViewGlobalMod
// ------------------------------------------------------------------------

template <
  class DomainType >
struct view_traits<ViewGlobalMod<DomainType> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef typename domain_type::local_type                      local_type;
  typedef ViewGlobalMod<DomainType>                            global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef dash::IndexSetLocal< ViewLocalMod<DomainType> >   index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool, false>                is_local;
};

template <
  class DomainType >
class ViewGlobalMod
: public ViewModBase< ViewGlobalMod<DomainType>, DomainType >
{
public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename domain_type::global_type                     image_type;
  typedef typename DomainType::index_type                       index_type;
private:
  typedef ViewLocalMod<DomainType>                                  self_t;
  typedef ViewModBase< ViewLocalMod<DomainType>, DomainType >       base_t;
public:
  typedef dash::IndexSetLocal< ViewLocalMod<DomainType> >   index_set_type;
  typedef self_t                                               global_type;
  typedef typename domain_type::local_type                      local_type;

  typedef std::integral_constant<bool, false>                     is_local;

  constexpr explicit ViewGlobalMod(
    const DomainType & domain)
  : base_t(domain),
    _index_set(*this)
  { }

  inline auto begin() const
  -> decltype(dash::begin(dash::global(dash::domain(*this)))) {
    return dash::begin(
             dash::global(
               dash::domain(
                 *this
               )
             )
           );
  }

  constexpr auto end() const
  -> decltype(dash::end(dash::global(dash::domain(*this)))) {
    return dash::begin(
             dash::global(
               dash::domain(
                 *this
               )
             )
           )
           + *dash::end(dash::index(dash::domain(*this)));
  }

  constexpr auto operator[](int offset) const
  -> decltype(*(dash::begin(
                 dash::global(dash::domain(*this))))) {
    return *(this->begin() + offset);
  }

  constexpr index_type size() const {
    return _index_set.size();
  }

  constexpr const local_type & local() const {
    return dash::local(dash::domain(*this));
  }

  inline local_type & local() {
    return dash::local(dash::domain(*this));
  }

  constexpr const global_type & global() const {
    return dash::global(dash::domain(*this));
  }

  inline global_type & global() {
    return dash::global(dash::domain(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

private:
  index_set_type  _index_set;
};

// ------------------------------------------------------------------------
// ViewSubMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t SubDim >
struct view_traits<ViewSubMod<DomainType, SubDim> > {
  typedef DomainType                                             domain_type;
  typedef typename dash::view_traits<domain_type>::origin_type   origin_type;
  typedef ViewSubMod<DomainType, SubDim>                          image_type;
  typedef ViewSubMod<DomainType, SubDim>                          local_type;
  typedef ViewSubMod<DomainType, SubDim>                         global_type;

  typedef typename DomainType::index_type                         index_type;
  typedef dash::IndexSetSub< ViewSubMod<DomainType, SubDim> > index_set_type;

  typedef std::integral_constant<bool, false>                  is_projection;
  typedef std::integral_constant<bool, true>                   is_view;
  typedef std::integral_constant<bool, false>                  is_origin;
  typedef std::integral_constant<bool,
    view_traits<domain_type>::is_local::value >                is_local;
};


template <
  class DomainType,
  dim_t SubDim >
class ViewSubMod
: public ViewModBase<
           ViewSubMod<DomainType, SubDim>,
           DomainType >
{
public:
  typedef DomainType                                             domain_type;
  typedef typename view_traits<DomainType>::index_type            index_type;
private:
  typedef ViewSubMod<DomainType, SubDim>                              self_t;
  typedef ViewModBase< ViewSubMod<DomainType, SubDim>, DomainType >   base_t;
public:
  typedef dash::IndexSetSub< ViewSubMod<DomainType, SubDim> > index_set_type;
  typedef ViewLocalMod<self_t>                                    local_type;
  typedef self_t                                                 global_type;

  typedef std::integral_constant<bool, false>                       is_local;

  constexpr ViewSubMod(
    const DomainType & domain,
    index_type         begin,
    index_type         end)
  : base_t(domain),
    _index_set(*this, begin, end)
   ,_local(*this)
  { }

  constexpr auto begin() const
  -> decltype(dash::begin(dash::domain(*this))) {
    return dash::begin(dash::domain(*this)) +
             *dash::begin(dash::index(*this));
  }

  constexpr auto end() const
  -> decltype(dash::begin(dash::domain(*this))) {
    return dash::begin(dash::domain(*this)) +
             *dash::end(dash::index(*this));
  }

  constexpr auto operator[](int offset) const
  -> decltype(*(dash::begin(dash::domain(*this)))) {
    return *(this->begin() + offset);
  }

  constexpr index_type size() const {
    return _index_set.size();
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

#if 1
  constexpr const local_type & local() const {
    return _local;
  }

  inline local_type & local() {
    return _local;
  }
#else
  constexpr local_type local() const {
    return local_type(*this);
  }
#endif

private:
  index_set_type  _index_set;
  local_type      _local;
};


template <
  class DomainType >
class ViewBlockMod
: public ViewModBase<
           ViewBlockMod<DomainType>,
           DomainType >
{
public:
  typedef DomainType                                             domain_type;
  typedef typename view_traits<DomainType>::index_type            index_type;
private:
  typedef ViewBlockMod<DomainType>                                    self_t;
  typedef ViewModBase< ViewBlockMod<DomainType>, DomainType >         base_t;
public:
  typedef dash::IndexSetSub< ViewBlockMod<DomainType> >       index_set_type;
  typedef ViewLocalMod<self_t>                                    local_type;

  typedef std::integral_constant<bool, false>                       is_local;

  constexpr ViewBlockMod(
    const DomainType & domain,
    index_type         block_index)
  : base_t(domain),
    _index_set(*this, block_index),
    _local(*this)
  { }

  constexpr auto begin() const
  -> decltype(dash::begin(dash::domain(*this))) {
    return dash::begin(dash::domain(*this)) +
           *dash::begin(dash::index(*this));
  }

  constexpr auto end() const
  -> decltype(dash::begin(dash::domain(*this))) {
    return dash::begin(dash::domain(*this)) +
           *dash::end(dash::index(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr const local_type & local() const {
    return _local;
  }

  inline local_type & local() {
    return _local;
  }

private:
  local_type      _local;
  index_set_type  _index_set;
};

} // namespace dash

#endif // DOXYGEN
#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
