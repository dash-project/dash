#ifndef DASH__VIEW__VIEW_MOD_1D_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_1D_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/util/UniversalMember.h>
#include <dash/util/ArrayExpr.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewIterator.h>

#include <dash/view/Local.h>
#include <dash/view/Global.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/Apply.h>


namespace dash {

#ifndef DOXYGEN

// ------------------------------------------------------------------------
// ViewModBase
// ------------------------------------------------------------------------

template <
  class ViewModType,
  class DomainType >
class ViewModBase<ViewModType, DomainType, 1>
{
  typedef ViewModBase<ViewModType, DomainType, 1> self_t;
 public:
  typedef DomainType                                             domain_type;

  typedef typename std::conditional<
                     view_traits<domain_type>::is_origin::value,
                     const domain_type &,
                     domain_type
                   >::type
    domain_member_type;

  typedef typename std::conditional<
            view_traits<domain_type>::is_local::value,
            domain_type,
            typename view_traits<domain_type>::origin_type
          >::type
    origin_type;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<origin_type>::type
              >() ))
    origin_iterator;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<const origin_type>::type
              >() ))
    const_origin_iterator;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<origin_type>::type
               >() ))
    reference;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<const origin_type>::type
               >() ))
    const_reference;

  typedef typename view_traits<domain_type>::index_type           index_type;
  typedef typename view_traits<domain_type>::size_type             size_type;
  typedef typename origin_type::value_type                        value_type;

  typedef std::integral_constant<dim_t, domain_type::rank::value>       rank;

  static constexpr std::size_t ndim() { return domain_type::rank::value; }
 protected:
  // References related to reference / temporary binding:
  //
  // - `shared_view` in range-v3, seems similar top the `std::shared_ptr`
  //   variant:
  //   https://github.com/ericniebler/range-v3/pull/557/files
  //
  // - `common_reference` proposal:
  //    http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0022r2.html
  //
  // - ref-qualified member functions:
  //   http://kukuruku.co/hub/cpp/ref-qualified-member-functions
  domain_member_type _domain;

  ViewModType & derived() {
    return static_cast<ViewModType &>(*this);
  }
  constexpr const ViewModType & derived() const {
    return static_cast<const ViewModType &>(*this);
  }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewModBase(domain_type && domain)
  : _domain(std::forward<domain_type>(domain))
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewModBase(const domain_type & domain)
  : _domain(domain)
  { }

  constexpr ViewModBase()               = delete;
  ~ViewModBase()                        = default;
 public:
  constexpr ViewModBase(const self_t &) = default;
  constexpr ViewModBase(self_t &&)      = default;
  self_t & operator=(const self_t &)    = default;
  self_t & operator=(self_t &&)         = default;

  constexpr const domain_member_type & domain() const & {
    return _domain;
  }

  domain_member_type & domain() & {
    return _domain;
  }

  constexpr domain_member_type domain() const && {
    return _domain;
  }

  constexpr bool operator==(const ViewModType & rhs) const {
    return &derived() == &rhs;
  }
  
  constexpr bool operator!=(const ViewModType & rhs) const {
    return !(derived() == rhs);
  }

  constexpr index_type size() const {
    return dash::index(derived()).size();
  }
};


// ------------------------------------------------------------------------
// ViewSubMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t SubDim >
struct view_traits<ViewSubMod<DomainType, SubDim, 1> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename view_traits<domain_type>::pattern_type     pattern_type;
  typedef ViewSubMod<DomainType, SubDim, 1>                     image_type;
  typedef ViewSubMod<DomainType, SubDim, 1>                     local_type;
  typedef ViewSubMod<DomainType, SubDim, 1>                    global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef typename DomainType::size_type                         size_type;
  typedef dash::IndexSetSub<DomainType, SubDim>             index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;

  typedef std::integral_constant<bool,
    view_traits<domain_type>::is_local::value >              is_local;

  typedef std::integral_constant<dim_t, 1>                            rank;
};


template <
  class DomainType,
  dim_t SubDim >
class ViewSubMod<DomainType, SubDim, 1>
: public ViewModBase<
           ViewSubMod<DomainType, SubDim, 1>,
           DomainType >
{
 public:
  typedef DomainType                                             domain_type;
 private:
  typedef ViewSubMod<domain_type, SubDim, 1>                          self_t;
  typedef ViewModBase< ViewSubMod<domain_type, SubDim>, domain_type > base_t;
 public:
  typedef typename base_t::origin_type                           origin_type;

  typedef typename view_traits<domain_type>::index_type           index_type;
  typedef typename view_traits<domain_type>::size_type             size_type;
 public:
  typedef dash::IndexSetSub<domain_type, SubDim>              index_set_type;
  typedef ViewLocalMod<self_t, 1>                                 local_type;
  typedef self_t                                                 global_type;

  typedef std::integral_constant<bool, false>                       is_local;

  typedef ViewIterator<
            typename base_t::origin_iterator, index_set_type >
    iterator;
  typedef ViewIterator<
            typename base_t::const_origin_iterator, index_set_type >
    const_iterator;

  using reference       = typename base_t::reference;
  using const_reference = typename base_t::const_reference;

 private:
  index_set_type _index_set;

 public:
  constexpr ViewSubMod()               = delete;
  constexpr ViewSubMod(self_t &&)      = default;
  constexpr ViewSubMod(const self_t &) = default;
  ~ViewSubMod()                        = default;
  self_t & operator=(self_t &&)        = default;
  self_t & operator=(const self_t &)   = default;

  constexpr ViewSubMod(
    domain_type && domain,
    index_type     begin,
    index_type     end)
  : base_t(std::forward<domain_type>(domain))
  , _index_set(this->domain(), begin, end)
  { }

  constexpr ViewSubMod(
    const domain_type  & domain,
    index_type     begin,
    index_type     end)
  : base_t(domain)
  , _index_set(domain, begin, end)
  { }

  constexpr const_iterator begin() const {
    return const_iterator(dash::origin(*this).begin(),
                          _index_set, 0);
  }

  iterator begin() {
    return iterator(const_cast<origin_type &>(
                      dash::origin(*this)
                    ).begin(),
                    _index_set, 0);
  }

  constexpr const_iterator end() const {
    return const_iterator(dash::origin(*this).begin(),
                          _index_set, _index_set.size());
  }

  iterator end() {
    return iterator(const_cast<origin_type &>(
                      dash::origin(*this)
                    ).begin(),
                    _index_set, _index_set.size());
  }

  constexpr const_reference operator[](int offset) const {
    return *(const_iterator(dash::origin(*this).begin(),
                            _index_set, offset));
  }

  reference operator[](int offset) {
    return *(iterator(const_cast<origin_type &>(
                        dash::origin(*this)
                      ).begin(),
                      _index_set, offset));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr local_type local() const {
    return local_type(*this);
  }
}; // class ViewSubMod


// ------------------------------------------------------------------------
// ViewLocalMod
// ------------------------------------------------------------------------

template <
  class DomainType >
struct view_traits<ViewLocalMod<DomainType, 1> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename view_traits<domain_type>::pattern_type     pattern_type;
  typedef typename domain_type::local_type                      image_type;
  typedef ViewLocalMod<DomainType, 1>                           local_type;
  typedef domain_type                                          global_type;

  typedef typename view_traits<domain_type>::index_type         index_type;
  typedef typename view_traits<domain_type>::size_type           size_type;
  typedef dash::IndexSetLocal<DomainType>                   index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool, true>                 is_local;

  typedef std::integral_constant<dim_t, DomainType::rank::value> rank;
};

template <
  class DomainType >
class ViewLocalMod<DomainType, 1>
: public ViewModBase< ViewLocalMod<DomainType, 1>, DomainType > {
 public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef typename domain_type::index_type                      index_type;
  typedef typename domain_type::size_type                        size_type;
 private:
  typedef ViewLocalMod<DomainType, 1>                               self_t;
  typedef ViewModBase< ViewLocalMod<DomainType, 1>, DomainType >    base_t;
 public:
  typedef dash::IndexSetLocal<DomainType>                   index_set_type;
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
  constexpr ViewLocalMod()               = delete;
  constexpr ViewLocalMod(self_t &&)      = default;
  constexpr ViewLocalMod(const self_t &) = default;
  ~ViewLocalMod()                        = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewLocalMod(
    domain_type && domain)
  : base_t(std::forward<domain_type>(domain))
  , _index_set(this->domain())
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewLocalMod(
    const DomainType & domain)
  : base_t(domain)
  , _index_set(domain)
  { }

  constexpr bool operator==(const self_t & rhs) const {
    return (this == &rhs ||
             ( base_t::operator==(rhs) &&
               _index_set == rhs._index_set ) );
  }

  constexpr bool operator!=(const self_t & rhs) const {
    return !(*this == rhs);
  }

  constexpr const_iterator begin() const {
    return dash::begin(
             dash::local(
               dash::origin(*this) ))
           + _index_set[0];
  }

  iterator begin() {
    return dash::begin(
             dash::local(
               const_cast<origin_type &>(dash::origin(*this))
             ))
           + _index_set[0];
  }

  constexpr const_iterator end() const {
    return dash::begin(
             dash::local(
               dash::origin(*this) ))
           + _index_set[_index_set.size() - 1] + 1;
  }

  iterator end() {
    return dash::begin(
             dash::local(
               const_cast<origin_type &>(dash::origin(*this))
             ))
           + _index_set[_index_set.size() - 1] + 1;
  }

  constexpr const_reference operator[](int offset) const {
    return *(dash::begin(
               dash::local(
                 dash::origin(*this) ))
             + _index_set[offset]);
  }

  reference operator[](int offset) {
    return *(dash::begin(
               dash::local(
                 const_cast<origin_type &>(dash::origin(*this))
               ))
             + _index_set[offset]);
  }

  constexpr const local_type & local() const {
    return *this;
  }

  constexpr const global_type & global() const {
    return dash::global(dash::domain(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }
}; // class ViewLocalMod


// ------------------------------------------------------------------------
// ViewGlobalMod
// ------------------------------------------------------------------------

template <
  class DomainType >
struct view_traits<ViewGlobalMod<DomainType, 1> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename view_traits<domain_type>::pattern_type     pattern_type;
  typedef typename domain_type::global_type                     image_type;
  typedef typename domain_type::local_type                      local_type;
  typedef ViewGlobalMod<DomainType, 1>                         global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef typename DomainType::size_type                         size_type;
  typedef dash::IndexSetLocal< DomainType >                 index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool, false>                is_local;

  typedef std::integral_constant<dim_t, DomainType::rank::value> rank;
};

template <
  class DomainType >
class ViewGlobalMod<DomainType, 1>
: public ViewModBase< ViewGlobalMod<DomainType, 1>, DomainType > {
 public:
  typedef DomainType                                             domain_type;
 private:
  typedef ViewGlobalMod<DomainType, 1>                                self_t;
  typedef ViewModBase< ViewLocalMod<DomainType, 1>, DomainType >      base_t;
 public:
  typedef typename base_t::origin_type                           origin_type;
  typedef typename domain_type::global_type                       image_type;
  typedef typename view_traits<domain_type>::index_type           index_type;
  typedef typename view_traits<domain_type>::size_type             size_type;
 public:
  typedef dash::IndexSetGlobal< DomainType >                  index_set_type;
  typedef self_t                                                 global_type;
  typedef typename domain_type::local_type                        local_type;

  typedef std::integral_constant<bool, false>                       is_local;

 private:
  index_set_type  _index_set;
 public:
  constexpr ViewGlobalMod()               = delete;
  constexpr ViewGlobalMod(self_t &&)      = default;
  constexpr ViewGlobalMod(const self_t &) = default;
  ~ViewGlobalMod()                        = default;
  self_t & operator=(self_t &&)           = default;
  self_t & operator=(const self_t &)      = default;

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewGlobalMod(
    domain_type && domain)
  : base_t(std::forward<domain_type>(domain))
  , _index_set(this->domain())
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewGlobalMod(
    const domain_type & domain)
  : base_t(domain)
  , _index_set(domain)
  { }

  constexpr auto begin() const
  -> decltype(dash::begin(dash::global(dash::domain(*this)))) {
    return dash::begin(
             dash::global(
               dash::domain(
                 *this)));
  }

  constexpr auto end() const
  -> decltype(dash::end(dash::global(dash::domain(*this)))) {
    return dash::begin(
             dash::global(
               dash::domain(
                 *this)))
           + *dash::end(dash::index(dash::domain(*this)));
  }

  constexpr auto operator[](int offset) const
  -> decltype(*(dash::begin(
                 dash::global(dash::domain(*this))))) {
    return *(this->begin() + offset);
  }

  constexpr const local_type & local() const {
    // if any parent domain is local, it will return *this
    // and in effect eliminate dash::global( ... dash::local( ... ))
    return dash::local(dash::domain(*this));
  }

  inline local_type & local() {
    // if any parent domain is local, it will return *this
    // and in effect eliminate dash::global( ... dash::local( ... ))
    return dash::local(dash::domain(*this));
  }

  constexpr const global_type & global() const {
    return *this;
  }

  inline global_type & global() {
    return *this;
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }
}; // class ViewGlobalMod

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_1D_H__INCLUDED
