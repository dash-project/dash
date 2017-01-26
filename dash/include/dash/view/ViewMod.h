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

/**
 * \defgroup  DashViewExpressionConcept  Multidimensional View Expressions
 *
 * \ingroup DashViewConcept
 * \{
 * \par Description
 *
 * Implementing view modifier chain as combination of command pattern
 * and chain of responsibility pattern.
 * For now, only compile-time projections/slices are supported such as:
 *
 * \code
 *   sub<0>(10,20).sub<1>(30,40)
 * \endcode
 *
 * but not run-time projections/slices like:
 *
 * \code
 *   sub(0, { 10,20 }).sub(1, { 30,40 })
 * \endcode
 *
 * \par Implementation Notes
 *
 * A view composition is a chained application of view modifier types
 * that depend on the type of their predecessor in the chain.
 *
 * Example:
 *
 * \code
 *  sub<0>(2).sub<1>(3,4)
 *  :         :
 *  |         |
 *  |         '--> ViewSubMod<0, ViewSubMod<-1, ViewOrigin> >
 *  |                            '------------.-----------'
 *  |                                         '--> parent
 *  '--> ViewSubMod<-1, ViewOrigin >
 *                      '----.---'
 *                           '--> parent
 * \endcode
 *
 * Consequently, specific ViewMod types are defined for every modifier
 * category.
 *
 * \}
 *
 *
 * \note
 *
 * As an alternative, all view modifications could be stored in command
 * objects of a single ViewMod type. Expressions then could not be
 * evalated at compile-time, however.
 *
 * However, View modifier types should subclass a common ViewMod base
 * class - or vice versa, following the policy pattern with the
 * operation specified as policy:
 *
 * \code
 *   template <dim_t DimDiff, class DomainType>
 *   class ViewMod : DomainType
 *   {
 *      // ...
 *   }
 * \endcode
 *
 * or:
 *
 * \code
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
 * \endcode
 *
 * \todo Eventually, these probably are not public definitions.
 *       Move to namespace internal.
 *       Define dereference operator*() for view types, delegating to
 *       domain::operator* recursively.
 */


#ifndef DOXYGEN

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------

template <
  dim_t NDim = 1>
class ViewOrigin;

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

// --------------------------------------------------------------------
// ViewOrigin
// --------------------------------------------------------------------

/**
 * Monotype for the logical symbol that represents a view origin.
 */
template <dim_t NDim>
class ViewOrigin
{
  typedef ViewOrigin self_t;

public:
  typedef dash::default_index_t                                 index_type;
  typedef self_t                                               domain_type;
  typedef IndexSetIdentity<self_t>                          index_set_type;

public:
  typedef std::integral_constant<bool, false> is_local;

private:
  std::array<index_type, NDim>   _extents    = { };
  index_set_type                 _index_set;
public:
  constexpr ViewOrigin()               = delete;
  constexpr ViewOrigin(self_t &&)      = default;
  constexpr ViewOrigin(const self_t &) = default;
  ~ViewOrigin()                        = default;
  self_t & operator=(self_t &&)        = default;
  self_t & operator=(const self_t &)   = default;

  constexpr explicit ViewOrigin(
      std::initializer_list<index_type> extents)
  : _extents(extents)
  , _index_set(*this)
  { }

  constexpr const domain_type & domain() const {
    return *this;
  }

  inline domain_type & domain() {
    return *this;
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
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
// ViewModBase
// ------------------------------------------------------------------------

template <
  class ViewModType,
  class DomainType >
class ViewModBase
{
  typedef ViewModBase<ViewModType, DomainType> self_t;
public:
  typedef DomainType                                             domain_type;
  typedef typename view_traits<DomainType>::index_type            index_type;

protected:
  const DomainType * _domain;

  ViewModType & derived() {
    return static_cast<ViewModType &>(*this);
  }
  const ViewModType & derived() const {
    return static_cast<const ViewModType &>(*this);
  }

  constexpr explicit ViewModBase(const domain_type & domain)
  : _domain(&domain)
  { }

  constexpr ViewModBase()               = delete;
  constexpr ViewModBase(self_t &&)      = default;
  constexpr ViewModBase(const self_t &) = default;
  ~ViewModBase()                        = default;

public:
  self_t & operator=(self_t &&)         = default;
  self_t & operator=(const self_t &)    = default;

  constexpr const domain_type & domain() const {
    return *_domain;
  }

  constexpr bool operator==(const ViewModType & rhs) const {
    return &derived() == &rhs;
  }
  
  constexpr bool operator!=(const ViewModType & rhs) const {
    return !(derived() == rhs);
  }

  constexpr bool is_local() const {
    return view_traits<ViewModType>::is_local::value;
  }
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

private:
  index_set_type  _index_set;
public:
  constexpr ViewLocalMod()               = delete;
  constexpr ViewLocalMod(self_t &&)      = default;
  constexpr ViewLocalMod(const self_t &) = default;
  ~ViewLocalMod()                        = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;

  constexpr explicit ViewLocalMod(
    const DomainType & domain)
  : base_t(domain)
  , _index_set(*this)
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
  -> decltype(dash::begin(dash::local(
                std::declval<
                  typename std::add_lvalue_reference<origin_type>::type >()
              ))) {
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
  -> decltype(dash::end(dash::local(
                std::declval<
                  typename std::add_lvalue_reference<origin_type>::type >()
              ))) {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this
               )
             )
           )
    // TODO: Using 
    //
#if 0
         + index_set().size();
#else
    //
    //       ... instead should work here
         + dash::index(dash::local(dash::domain(*this))).pre()[
             (*dash::end(dash::index(dash::local(dash::domain(*this)))))-1
           ] + 1;
#endif
  }

  constexpr auto operator[](int offset) const
  -> decltype(*(dash::begin(
                dash::local(dash::origin(
                  std::declval<
                    typename std::add_lvalue_reference<domain_type>::type >()
                ))))) {
    return *(this->begin() + offset);
  }

  constexpr index_type size() const {
    return index_set().size();
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

private:
  index_type     _begin_idx;
  index_type     _end_idx;
  index_set_type _index_set;

public:
  constexpr ViewSubMod()               = delete;
  constexpr ViewSubMod(self_t &&)      = default;
  constexpr ViewSubMod(const self_t &) = default;
  ~ViewSubMod()                        = default;
  self_t & operator=(self_t &&)        = default;
  self_t & operator=(const self_t &)   = default;

  constexpr ViewSubMod(
    const DomainType & domain,
    index_type         begin,
    index_type         end)
  : base_t(domain)
  , _begin_idx(begin)
  , _end_idx(end)
  , _index_set(*this, begin, end)
  { }

  constexpr auto begin() const
  -> decltype(dash::begin(
                std::declval<
                  typename std::add_lvalue_reference<domain_type>::type
                >() )) {
    return dash::begin(dash::domain(*this)) +
             *dash::begin(dash::index(*this));
  }

  constexpr auto end() const
  -> decltype(dash::begin(
                std::declval<
                  typename std::add_lvalue_reference<domain_type>::type
                >() )) {
    return dash::begin(dash::domain(*this)) +
             *dash::end(dash::index(*this));
  }

  constexpr auto operator[](int offset) const
  -> decltype(*(dash::begin(
                  std::declval<
                    typename std::add_lvalue_reference<domain_type>::type
                  >() ))) {
    return *(this->begin() + offset);
  }

  constexpr index_type size() const {
    return _index_set.size();
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr local_type local() const {
    return local_type(*this);
  }
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
  typedef ViewGlobalMod<DomainType>                                 self_t;
  typedef ViewModBase< ViewLocalMod<DomainType>, DomainType >       base_t;
public:
  typedef dash::IndexSetLocal< ViewLocalMod<DomainType> >   index_set_type;
  typedef self_t                                               global_type;
  typedef typename domain_type::local_type                      local_type;

  typedef std::integral_constant<bool, false>                     is_local;

private:
  index_set_type  _index_set;
public:
  constexpr ViewGlobalMod()               = delete;
  constexpr ViewGlobalMod(self_t &&)      = default;
  constexpr ViewGlobalMod(const self_t &) = default;
  ~ViewGlobalMod()                        = default;
  self_t & operator=(self_t &&)           = default;
  self_t & operator=(const self_t &)      = default;

  constexpr explicit ViewGlobalMod(
    const DomainType & domain)
  : base_t(domain)
  , _index_set(*this)
  { }

  constexpr auto begin() const
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
};

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
