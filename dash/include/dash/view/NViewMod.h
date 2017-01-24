#ifndef DASH__VIEW__NVIEW_MOD_H__INCLUDED
#define DASH__VIEW__NVIEW_MOD_H__INCLUDED

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


#ifndef DOXYGEN

// Related: boost::multi_array
//
// http://www.boost.org/doc/libs/1_63_0/libs/multi_array/doc/user.html
//

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------

template <dim_t NDim>
class NViewOrigin;

template <
  class ViewModType,
  class DomainType,
  dim_t NDim >
class NViewModBase;

template <
  class DomainType = NViewOrigin<1>,
  dim_t NDim       = dash::view_traits<DomainType>::rank::value >
class NViewLocalMod;

template <
  class DomainType = NViewOrigin<1>,
  dim_t NDim       = dash::view_traits<DomainType>::rank::value >
class NViewGlobalMod;

template <
  class DomainType = NViewOrigin<1>,
  dim_t SubDim     = 0,
  dim_t NDim       = dash::view_traits<DomainType>::rank::value >
class NViewSubMod;

// --------------------------------------------------------------------
// ViewOrigin
// --------------------------------------------------------------------

/**
 * Monotype for the logical symbol that represents a view origin.
 */
template <dim_t NDim>
class NViewOrigin
{
  typedef NViewOrigin self_t;

public:
  typedef dash::default_index_t                                 index_type;
  typedef self_t                                               domain_type;
  typedef IndexSetIdentity<self_t>                          index_set_type;

public:
  typedef std::integral_constant<bool, false>   is_local;
  typedef std::integral_constant<dim_t, NDim>   rank;

private:
  std::array<index_type, NDim>                  _extents    = { };
  index_set_type                                _index_set;
public:
  constexpr NViewOrigin()               = delete;
  constexpr NViewOrigin(self_t &&)      = default;
  constexpr NViewOrigin(const self_t &) = default;
  ~NViewOrigin()                        = default;
  self_t & operator=(self_t &&)         = default;
  self_t & operator=(const self_t &)    = default;

  constexpr explicit NViewOrigin(
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
    return extent<SizeDim>() *
             (SizeDim < NDim
               ? _size<SizeDim + 1>()
               : 1);
  }
};

template <dim_t NDim>
struct view_traits<NViewOrigin<NDim>> {
  typedef NViewOrigin<NDim>                                      origin_type;
  typedef NViewOrigin<NDim>                                      domain_type;
  typedef NViewOrigin<NDim>                                       image_type;
  typedef typename NViewOrigin<NDim>::index_type                  index_type;
  typedef typename NViewOrigin<NDim>::index_set_type          index_set_type;

  typedef std::integral_constant<bool, false>                  is_projection;
  typedef std::integral_constant<bool, true>                   is_view;
  typedef std::integral_constant<bool, true>                   is_origin;
  typedef std::integral_constant<bool, false>                  is_local;

  typedef std::integral_constant<dim_t, NDim>                  rank;
};


// ------------------------------------------------------------------------
// NViewModBase
// ------------------------------------------------------------------------

template <
  class NViewModType,
  class DomainType,
  dim_t NDim >
class NViewModBase
{
  typedef NViewModBase<NViewModType, DomainType, NDim> self_t;
public:
  typedef DomainType                                             domain_type;
  typedef typename view_traits<DomainType>::index_type            index_type;

  typedef std::integral_constant<dim_t, DomainType::rank::value>        rank;

protected:
  const DomainType * _domain;

  NViewModType & derived() {
    return static_cast<NViewModType &>(*this);
  }
  const NViewModType & derived() const {
    return static_cast<const NViewModType &>(*this);
  }

  constexpr explicit NViewModBase(const domain_type & domain)
  : _domain(&domain)
  { }

  constexpr NViewModBase()               = delete;
  constexpr NViewModBase(self_t &&)      = default;
  constexpr NViewModBase(const self_t &) = default;
  ~NViewModBase()                        = default;

public:
  self_t & operator=(self_t &&)         = default;
  self_t & operator=(const self_t &)    = default;

  constexpr const domain_type & domain() const {
    return *_domain;
  }

  constexpr bool operator==(const NViewModType & rhs) const {
    return &derived() == &rhs;
  }
  
  constexpr bool operator!=(const NViewModType & rhs) const {
    return !(derived() == rhs);
  }

  constexpr bool is_local() const {
    return view_traits<NViewModType>::is_local::value;
  }

  template <dim_t ShapeDim>
  constexpr index_type extent() const {
    return _domain->extent<ShapeDim>();
  }

  constexpr index_type extent(dim_t shape_dim) const {
    return _domain->extent(shape_dim);
  }
};


// ------------------------------------------------------------------------
// NViewLocalMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t NDim >
struct view_traits<NViewLocalMod<DomainType, NDim> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef NViewLocalMod<DomainType, NDim>                       local_type;
  typedef domain_type                                          global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef dash::IndexSetLocal< NViewLocalMod<DomainType, NDim> >
                                                            index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool, true>                 is_local;

  typedef std::integral_constant<dim_t, NDim>                rank;
};

template <
  class DomainType,
  dim_t NDim >
class NViewLocalMod
: public NViewModBase<
           NViewLocalMod<DomainType, NDim>,
           DomainType,
           NDim >
{
public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef typename DomainType::index_type                       index_type;
private:
  typedef NViewLocalMod<DomainType, NDim>                           self_t;
  typedef NViewModBase<
            NViewLocalMod<DomainType, NDim>, DomainType, NDim >     base_t;
public:
  typedef dash::IndexSetLocal<
            NViewLocalMod<DomainType, NDim> >               index_set_type;
  typedef self_t                                                local_type;
  typedef typename domain_type::global_type                    global_type;

  typedef std::integral_constant<bool, true>                      is_local;

private:
  index_set_type  _index_set;
public:
  constexpr NViewLocalMod()               = delete;
  constexpr NViewLocalMod(self_t &&)      = default;
  constexpr NViewLocalMod(const self_t &) = default;
  ~NViewLocalMod()                        = default;
  self_t & operator=(self_t &&)           = default;
  self_t & operator=(const self_t &)      = default;

  constexpr explicit NViewLocalMod(
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
         + dash::index(dash::local(dash::domain(*this))).pre()[
             (*dash::end(dash::index(dash::local(dash::domain(*this)))))-1
           ] + 1;
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
// NViewSubMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t SubDim,
  dim_t NDim >
struct view_traits<NViewSubMod<DomainType, SubDim, NDim> > {
  typedef DomainType                                           domain_type;
  typedef typename dash::view_traits<domain_type>::origin_type origin_type;
  typedef NViewSubMod<DomainType, SubDim>                       image_type;
  typedef NViewSubMod<DomainType, SubDim>                       local_type;
  typedef NViewSubMod<DomainType, SubDim>                      global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef dash::IndexSetSub<
            NViewSubMod<DomainType, SubDim, NDim> >         index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool,
    view_traits<domain_type>::is_local::value >              is_local;

  typedef std::integral_constant<dim_t, NDim>                rank;
};


template <
  class DomainType,
  dim_t SubDim,
  dim_t NDim >
class NViewSubMod
: public NViewModBase<
           NViewSubMod<DomainType, SubDim, NDim>,
           DomainType,
           NDim >
{
public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
private:
  typedef NViewSubMod<DomainType, SubDim, NDim>                     self_t;
  typedef NViewModBase<
            NViewSubMod<DomainType, SubDim, NDim>, DomainType, NDim
          >                                                         base_t;
public:
  typedef dash::IndexSetSub<
            NViewSubMod<DomainType, SubDim, NDim> >         index_set_type;
  typedef NViewLocalMod<self_t>                                 local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, false>                     is_local;

private:
  index_type     _begin_idx;
  index_type     _end_idx;
  index_set_type _index_set;

public:
  constexpr NViewSubMod()               = delete;
  constexpr NViewSubMod(self_t &&)      = default;
  constexpr NViewSubMod(const self_t &) = default;
  ~NViewSubMod()                        = default;
  self_t & operator=(self_t &&)         = default;
  self_t & operator=(const self_t &)    = default;

  constexpr NViewSubMod(
    const DomainType & domain,
    index_type         begin,
    index_type         end)
  : base_t(domain)
  , _begin_idx(begin)
  , _end_idx(end)
  , _index_set(*this, begin, end)
  { }

  template <dim_t ExtDim = SubDim>
  constexpr index_type extent() const {
    return _end_idx - _begin_idx;
  }

  constexpr index_type extent(dim_t shape_dim) const {
    return ( shape_dim == SubDim
             ? base_t::extent(shape_dim)
             : _end_idx - _begin_idx
           );
  }

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


#endif // DOXYGEN

} // namespace dash

#endif // DASH__NVIEW__VIEW_MOD_H__INCLUDED
