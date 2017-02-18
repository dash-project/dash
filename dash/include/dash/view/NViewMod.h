#ifndef DASH__VIEW__NVIEW_MOD_H__INCLUDED
#define DASH__VIEW__NVIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/util/UniversalMember.h>
#include <dash/util/ArrayExpr.h>

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

template <
  dim_t NDim = 1>
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
  dim_t SubDim     = 0,
  dim_t NDim       = dash::view_traits<DomainType>::rank::value >
class NViewSubMod;

template <
  class DomainType = NViewOrigin<1>,
  dim_t NDim       = dash::view_traits<DomainType>::rank::value >
class NViewGlobalMod;

// --------------------------------------------------------------------
// NViewOrigin
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
  typedef dash::default_extent_t                                 size_type;
  typedef self_t                                               domain_type;
  typedef IndexSetIdentity<self_t>                          index_set_type;

public:
  typedef std::integral_constant<bool, false>        is_local;
  typedef std::integral_constant<dim_t, NDim>  rank;

private:
  std::array<size_type, NDim>                  _extents    = { };
  std::array<index_type, NDim>                 _offsets    = { };
  index_set_type                               _index_set;
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

  // ---- extents ---------------------------------------------------------

  constexpr const std::array<size_type, NDim> extents() const {
    return _extents;
  }

  template <dim_t ExtentDim = 0>
  constexpr index_type extent() const {
    return _extents[ExtentDim];
  }

  constexpr index_type extent(dim_t extent_dim) const {
    return _extents[extent_dim];
  }

  // ---- offsets ---------------------------------------------------------

  constexpr const std::array<index_type, NDim> & offsets() const {
    return _offsets;
  }

  template <dim_t OffsetDim = 0>
  constexpr index_type offset() const {
    return _offsets[OffsetDim];
  }

  constexpr index_type offset(dim_t offset_dim) const {
    return _offsets[offset_dim];
  }

  // ---- size ------------------------------------------------------------
  
  template <dim_t SizeDim = 0>
  constexpr index_type size() const {
    return extent<SizeDim>() *
             (SizeDim + 1 < NDim
               ? size<SizeDim + 1>()
               : 1);
  }
};

template <dim_t NDim>
struct view_traits<NViewOrigin<NDim>> {
  typedef NViewOrigin<NDim>                                      origin_type;
  typedef NViewOrigin<NDim>                                      domain_type;
  typedef NViewOrigin<NDim>                                       image_type;
  typedef typename NViewOrigin<NDim>::index_type                  index_type;
  typedef typename NViewOrigin<NDim>::size_type                    size_type;
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
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;
  typedef typename origin_type::value_type                      value_type;

  typedef std::integral_constant<dim_t, DomainType::rank::value>
    rank;

  static constexpr dim_t ndim() { return NDim; }

protected:
  dash::UniversalMember<domain_type> _domain;

  NViewModType & derived() {
    return static_cast<NViewModType &>(*this);
  }
  const NViewModType & derived() const {
    return static_cast<const NViewModType &>(*this);
  }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit NViewModBase(domain_type && domain)
  : _domain(std::forward<domain_type>(domain))
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit NViewModBase(const domain_type & domain)
  : _domain(domain)
  { }

  constexpr NViewModBase()               = delete;
  constexpr NViewModBase(self_t &&)      = default;
  constexpr NViewModBase(const self_t &) = default;
  ~NViewModBase()                        = default;

public:
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;

  constexpr const domain_type & domain() const {
    return _domain;
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

  // ---- extents ---------------------------------------------------------

  constexpr const std::array<size_type, NDim> extents() const {
    return domain().extents();
  }

  template <dim_t ShapeDim>
  constexpr size_type extent() const {
    return domain().template extent<ShapeDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return domain().extent(shape_dim);
  }

  // ---- offsets ---------------------------------------------------------

  constexpr const std::array<index_type, NDim> & offsets() const {
    return domain().offsets();
  }

  template <dim_t ShapeDim>
  constexpr index_type offset() const {
    return domain().template offset<ShapeDim>();
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return domain().offset(shape_dim);
  }

  // ---- size ------------------------------------------------------------
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

  typedef typename view_traits<domain_type>::index_type         index_type;
  typedef typename view_traits<domain_type>::size_type           size_type;
  typedef dash::IndexSetLocal<
            NViewLocalMod<DomainType, NDim> >               index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool, true>                 is_local;

  typedef std::integral_constant<dim_t, NDim>          rank;
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
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;
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

  typedef
    decltype(
      dash::begin(
        dash::local(dash::origin(
          std::declval<
            typename std::add_lvalue_reference<domain_type>::type >()
      ))))
    iterator;

  typedef
    decltype(
      dash::begin(
        dash::local(dash::origin(
          std::declval<
            typename std::add_lvalue_reference<const domain_type>::type >()
      ))))
    const_iterator;

  typedef
    decltype(
      *(dash::begin(
          dash::local(dash::origin(
            std::declval<
              typename std::add_lvalue_reference<domain_type>::type >()
          )))))
    reference;

  typedef
    decltype(
      *(dash::begin(
          dash::local(dash::origin(
            std::declval<
              typename std::add_lvalue_reference<const domain_type>::type >()
          )))))
    const_reference;

private:
  index_set_type  _index_set;
public:
  constexpr NViewLocalMod()               = delete;
  constexpr NViewLocalMod(self_t &&)      = default;
  constexpr NViewLocalMod(const self_t &) = default;
  ~NViewLocalMod()                        = default;
  self_t & operator=(self_t &&)           = default;
  self_t & operator=(const self_t &)      = default;

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit NViewLocalMod(
    domain_type && domain)
  : base_t(std::forward<domain_type>(domain))
  , _index_set(*this)
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
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

  // ---- extents ---------------------------------------------------------

  constexpr const std::array<size_type, NDim> extents() const {
    return _index_set.extents();
  }

  template <dim_t ShapeDim>
  constexpr size_type extent() const {
    return _index_set.template extent<ShapeDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return _index_set.extent(shape_dim);
  }

  // ---- offsets ---------------------------------------------------------

  constexpr const std::array<index_type, NDim> & offsets() const {
    return _index_set.offsets();
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size(dim_t sub_dim = 0) const {
    return index_set().size(sub_dim);
  }

  // ---- access ----------------------------------------------------------

  constexpr const_iterator begin() const {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this ) ) )
         + _index_set.pre()[
             _index_set.first()
           ];
  }

  iterator begin() {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this ) ) )
         + _index_set.pre()[
             _index_set.first()
           ];
  }

  constexpr const_iterator end() const {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this ) ) )
         + _index_set.pre()[
             _index_set.last()
           ] + 1;
  }

  iterator end() {
    return dash::begin(
             dash::local(
               dash::origin(
                 *this ) ) )
         + _index_set.pre()[
             _index_set.last()
           ] + 1;
  }

  constexpr const_reference operator[](int offset) const {
    return *(this->begin() + offset);
  }

  reference operator[](int offset) {
    return *(this->begin() + offset);
  }

  constexpr const local_type & local() const {
    return *this;
  }

  local_type & local() {
    return *this;
  }

  constexpr const global_type & global() const {
    return dash::global(dash::domain(*this));
  }

  global_type & global() {
    return dash::global(dash::domain(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }
};

#if 0
template <class ViewType>
constexpr auto
local(const ViewType & v)
-> typename std::enable_if<
     (dash::view_traits<ViewType>::rank::value > 1),
     NViewLocalMod< ViewType, dash::view_traits<ViewType>::rank::value >
   >::type {
  return NViewLocalMod<
           ViewType,
           dash::view_traits<ViewType>::rank::value >(
             v);
}
#endif


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
  typedef typename DomainType::size_type                         size_type;
  typedef dash::IndexSetSub<
            NViewSubMod<DomainType, SubDim, NDim>, SubDim > index_set_type;

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
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;

  using value_type      = typename origin_type::value_type;
  using reference       = typename origin_type::reference;
  using const_reference = typename origin_type::const_reference;
private:
  typedef NViewSubMod<DomainType, SubDim, NDim>                     self_t;
  typedef NViewModBase<
            NViewSubMod<DomainType, SubDim, NDim>, DomainType, NDim
          >                                                         base_t;
public:
  typedef dash::IndexSetSub<
            NViewSubMod<DomainType, SubDim, NDim>, SubDim>  index_set_type;
  typedef NViewLocalMod<self_t>                                 local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, false>                     is_local;

  typedef decltype(
            dash::begin(
              std::declval<
            //  typename std::add_lvalue_reference<origin_type>::type
                typename std::add_lvalue_reference<domain_type>::type
              >() ))
    iterator;

  typedef decltype(
            dash::begin(
              std::declval<
            //  typename std::add_lvalue_reference<const origin_type>::type
                typename std::add_lvalue_reference<const domain_type>::type
              >() ))
    const_iterator;

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
    domain_type && domain,
    index_type     begin,
    index_type     end)
  : base_t(std::forward<domain_type>(domain))
  , _begin_idx(begin)
  , _end_idx(end)
  , _index_set(*this, begin, end)
  { }

  constexpr NViewSubMod(
    domain_type  & domain,
    index_type     begin,
    index_type     end)
  : base_t(domain)
  , _begin_idx(begin)
  , _end_idx(end)
  , _index_set(*this, begin, end)
  { }

  // ---- extents ---------------------------------------------------------

  template <dim_t ExtDim>
  constexpr size_type extent() const {
    return _index_set.template extent<ExtDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return _index_set.extent(shape_dim);
  }

  constexpr std::array<size_type, NDim> extents() const {
    return _index_set.extents();
  }

  // ---- offsets ---------------------------------------------------------

  template <dim_t ExtDim>
  constexpr index_type offset() const {
    return ( ExtDim == SubDim
             ? _begin_idx
             : base_t::offset(ExtDim)
           );
  }

  constexpr auto offsets() const
    -> decltype(
         std::declval<
           typename std::add_lvalue_reference<domain_type>::type
         >().offsets()) {
    return dash::ce::replace_nth<SubDim>(
             static_cast<
               typename std::remove_reference<
                 decltype( std::get<0>(dash::domain(*this).offsets()) )
               >::type
             >(_begin_idx),
             dash::domain(*this).offsets());
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return ( shape_dim == SubDim
             ? _begin_idx
             : base_t::offset(shape_dim)
           );
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size(dim_t sub_dim = 0) const {
    return _index_set.size(sub_dim);
  }

  // ---- access ----------------------------------------------------------

  constexpr const_iterator begin() const {
    return this->domain().begin() + _index_set[0];
  }

  constexpr const_iterator end() const {
    return this->domain().begin() + *_index_set.end();
  }

  constexpr const_reference operator[](int offset) const {
    return this->domain().begin()[offset];
  }

  reference operator[](int offset) {
    return this->domain().begin()[offset];
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
