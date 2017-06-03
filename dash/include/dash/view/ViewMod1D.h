#ifndef DASH__VIEW__VIEW_MOD_1D_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_1D_H__INCLUDED

#include <dash/Types.h>
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

#include <dash/view/ViewMod.h>

namespace dash {


template <class IndexSetType, class DomainType, std::size_t NDim>
class IndexSetBase;

template <class DomainType>
class IndexSetIdentity;

template <class DomainType>
class IndexSetLocal;

template <class DomainType>
class IndexSetGlobal;

template <class DomainType, std::size_t SubDim>
class IndexSetSub;

template <class RangeOrigin>
class IteratorRange;

template <class Iterator, class Sentinel>
class IteratorRangeOrigin;

template <typename Iterator, typename Sentinel>
class IteratorRangeOrigin<Iterator *, Sentinel *>;


#ifndef DOXYGEN

// -----------------------------------------------------------------------
// ViewSubMod
// -----------------------------------------------------------------------

template <
  class DomainType,
  dim_t SubDim >
class ViewSubMod<DomainType, SubDim, 1>
: public ViewModBase<
           ViewSubMod<DomainType, SubDim, 1>, DomainType, 1
         > {
 public:
  typedef DomainType                                          domain_type;
 private:
  typedef ViewSubMod<domain_type, SubDim, 1>                       self_t;
  typedef ViewModBase<
            ViewSubMod<domain_type, SubDim>, domain_type >         base_t;
 public:
  typedef typename base_t::origin_type                        origin_type;

  typedef typename view_traits<domain_type>::index_type        index_type;
  typedef typename view_traits<domain_type>::size_type          size_type;
 public:
  typedef dash::IndexSetSub<domain_type, SubDim>           index_set_type;
  typedef dash::ViewLocalMod<self_t, 1>                        local_type;
  typedef self_t                                              global_type;

  typedef std::integral_constant<bool, false>                    is_local;

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
    DomainType  && domain,
    index_type     begin,
    index_type     end)
  : base_t(std::move(domain))
  , _index_set(this->domain(), begin, end)
  { }

  constexpr ViewSubMod(
    const domain_type & domain,
    index_type          begin,
    index_type          end)
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

  constexpr const local_type local() const {
    return local_type(*this);
  }

  local_type local() {
    return local_type(*this);
  }

  constexpr const self_t & global() const {
    return *this;
  }

  self_t & global() {
    return *this;
  }
}; // class ViewSubMod


// -----------------------------------------------------------------------
// ViewLocalMod
// -----------------------------------------------------------------------

#if 0 // HERE !!!
template <class RangeOrigin>
constexpr ViewLocalMod<IteratorRange<RangeOrigin>, 1>
local(const IteratorRange<RangeOrigin> & ir) {
  return ViewLocalMod<IteratorRange<RangeOrigin>, 1>(ir);
}

template <
  class    Iterator,
  class    Sentinel >
constexpr ViewLocalMod<IteratorRangeOrigin<Iterator, Sentinel>, 1>
local(const IteratorRangeOrigin<Iterator, Sentinel> & ir) {
  return ViewLocalMod<IteratorRangeOrigin<Iterator, Sentinel>, 1>(ir);
}
#endif


template <
  class DomainType >
class ViewLocalMod<DomainType, 1>
: public ViewModBase<
           ViewLocalMod<DomainType, 1>,
           DomainType > {
 public:
  typedef DomainType                                          domain_type;
  typedef typename view_traits<DomainType>::origin_type       origin_type;
  typedef typename view_traits<DomainType>::local_type         image_type;
  typedef typename domain_type::index_type                     index_type;
  typedef typename domain_type::size_type                       size_type;
 private:
  typedef ViewLocalMod<DomainType, 1>                              self_t;
  typedef ViewModBase<
            ViewLocalMod<DomainType, 1>, DomainType >              base_t;
 public:
  typedef dash::IndexSetLocal<DomainType>                  index_set_type;
  typedef self_t                                               local_type;
  typedef typename domain_type::global_type                   global_type;

  typedef std::integral_constant<bool, true>                     is_local;

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
            typename std::add_lvalue_reference<
              const domain_type>::type >()
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
              typename std::add_lvalue_reference<
                const domain_type>::type >()
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
  : base_t(std::move(domain))
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

// -------------------------------------------------------------------------
// ViewGlobalMod
// -------------------------------------------------------------------------

template <
  class DomainType >
class ViewGlobalMod<DomainType, 1>
: public ViewModBase< ViewGlobalMod<DomainType, 1>, DomainType > {
 public:
  typedef DomainType                                          domain_type;
 private:
  typedef ViewGlobalMod<DomainType, 1>                             self_t;
  typedef ViewModBase< ViewLocalMod<DomainType, 1>, DomainType >   base_t;
 public:
  typedef typename base_t::origin_type                        origin_type;
  typedef typename domain_type::global_type                    image_type;
  typedef typename view_traits<domain_type>::index_type        index_type;
  typedef typename view_traits<domain_type>::size_type          size_type;
 public:
  typedef dash::IndexSetGlobal< DomainType >               index_set_type;
  typedef self_t                                              global_type;
  typedef typename domain_type::local_type                     local_type;

  typedef std::integral_constant<bool, false>                    is_local;

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
  : base_t(std::move(domain))
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



// -----------------------------------------------------------------------
// Iterator Range Local Origin
// -----------------------------------------------------------------------

template <
  class Iterator,
  class Sentinel >
class IteratorRangeLocalOrigin;

template <
  class Iterator,
  class Sentinel >
struct view_traits<IteratorRangeLocalOrigin<Iterator, Sentinel> > {
 private:
  typedef IteratorRangeLocalOrigin<Iterator, Sentinel>             RangeT;
 public:
  typedef IteratorRangeOrigin<Iterator, Sentinel>             domain_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>             origin_type;
  typedef RangeT                                               image_type;

  typedef typename Iterator::pattern_type                    pattern_type;
  typedef std::integral_constant<dim_t, pattern_type::ndim()>        rank;

  typedef RangeT                                               local_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>             global_type;

  typedef typename Iterator::index_type                        index_type;
  typedef typename std::make_unsigned<index_type>::type         size_type;

  typedef dash::IndexSetLocal<domain_type>                 index_set_type;

  typedef std::integral_constant<bool, false> is_projection;
  typedef std::integral_constant<bool, true > is_view;
  typedef std::integral_constant<bool, false> is_origin;
  typedef std::integral_constant<bool, true > is_local;
};

template <
  class OriginIter,
  class Sentinel >
class IteratorRangeLocalOrigin
: public ViewModBase<
           IteratorRangeLocalOrigin<OriginIter, Sentinel>,
           IteratorRangeOrigin<OriginIter, Sentinel>,
           IteratorRangeOrigin<OriginIter, Sentinel>::rank::value >
{
  // Do not depend on DomainType (IndexRangeOrigin):
  // Use OriginIter, OriginSntl in template parameters to decouple
  // cyclic type dependency.
  typedef OriginIter                                    g_origin_iterator;
  typedef OriginIter                              const_g_origin_iterator;

  typedef IteratorRangeLocalOrigin<OriginIter, Sentinel>           self_t;
 public:
  typedef IteratorRangeOrigin<OriginIter, Sentinel>           domain_type;
  typedef IteratorRangeOrigin<OriginIter, Sentinel>           origin_type;
  typedef IteratorRangeLocalOrigin<OriginIter, Sentinel>       image_type;
 private:
  typedef ViewModBase<
            self_t,
            domain_type,
            domain_type::rank::value > base_t;
 public:

  typedef typename g_origin_iterator::value_type               value_type;

  typedef typename g_origin_iterator::index_type               index_type;
  typedef typename std::make_unsigned<index_type>::type         size_type;

  typedef typename g_origin_iterator::pattern_type           pattern_type;

  typedef IndexSetLocal<domain_type>                       index_set_type;

  typedef std::integral_constant<dim_t, pattern_type::ndim()>        rank;

  typedef std::integral_constant<bool, true>                     is_local;

  typedef typename g_origin_iterator::local_type
    origin_iterator;

  typedef typename g_origin_iterator::local_type
    const_origin_iterator;

  typedef ViewIterator<
            origin_iterator, index_set_type >
    iterator;
  typedef ViewIterator<
            const_origin_iterator, index_set_type >
    const_iterator;

  typedef IteratorRangeOrigin<OriginIter, Sentinel>           global_type;
  typedef self_t                                               local_type;

 private:
  index_set_type _index_set;

 public:
  constexpr explicit IteratorRangeLocalOrigin(
      const IteratorRangeOrigin<OriginIter, Sentinel> & range_origin)
  : base_t(range_origin)
  , _index_set(this->domain())
  { }

  constexpr const_iterator begin() const {
    return const_iterator(
             dash::begin(
               dash::local(
                 dash::origin(*this) )),
               _index_set, 0);
  }

  iterator begin() {
    return iterator(
             dash::begin(
               dash::local(
                 const_cast<origin_type &>(
                   dash::origin(*this)) )),
             _index_set, 0);
  }

  constexpr const_iterator end() const {
    return const_iterator(
             dash::begin(
               dash::local(
                 dash::origin(*this) )),
               _index_set, _index_set.size());
  }

  iterator end() {
    return iterator(
             dash::begin(
               dash::local(
                 const_cast<origin_type &>(
                   dash::origin(*this)) )),
             _index_set, _index_set.size());
  }

  constexpr size_type size() const { return _index_set.size(); }

  constexpr const pattern_type & pattern() const {
    return this->begin().pattern();
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr const local_type & local() const {
    return *this;
  }

  local_type & local() {
    return *this;
  }

  constexpr const global_type & global() const {
    return this->domain();
  }

  global_type & global() {
    return this->domain();
  }
};

// -----------------------------------------------------------------------
// Iterator Range Origin
// -----------------------------------------------------------------------

template <
  class Iterator,
  class Sentinel >
class IteratorRangeOrigin;

template <
  class Iterator,
  class Sentinel >
struct view_traits<IteratorRangeOrigin<Iterator, Sentinel> > {
 private:
  typedef IteratorRangeOrigin<Iterator, Sentinel>                  RangeT;
 public:
  typedef IteratorRangeOrigin<Iterator, Sentinel>             domain_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>             origin_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>              image_type;

  typedef typename Iterator::pattern_type                    pattern_type;
  typedef std::integral_constant<dim_t, pattern_type::ndim()>        rank;

  typedef RangeT                                              global_type;
  typedef IteratorRangeLocalOrigin<Iterator, Sentinel>         local_type;

  typedef typename Iterator::index_type                        index_type;
  typedef typename std::make_unsigned<index_type>::type         size_type;

  typedef dash::IndexSetIdentity< 
            IteratorRangeOrigin<Iterator, Sentinel> >      index_set_type;

  typedef std::integral_constant<bool, false> is_projection;
  typedef std::integral_constant<bool, false> is_view;
  typedef std::integral_constant<bool, true > is_origin;
  typedef std::integral_constant<bool, false> is_local;
};


template <
  class Iterator,
  class Sentinel >
class IteratorRangeOrigin
{
  typedef IteratorRangeOrigin<Iterator, Sentinel>                  self_t;
 public:
  typedef Iterator                                               iterator;
  typedef Iterator                                         const_iterator;
  typedef Sentinel                                               sentinel;
  typedef Sentinel                                         const_sentinel;

  typedef IteratorRangeOrigin<Iterator, Sentinel>             domain_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>             origin_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>              image_type;

  typedef typename iterator::value_type                        value_type;

  typedef typename Iterator::index_type                        index_type;
  typedef typename std::make_unsigned<index_type>::type         size_type;

  typedef typename iterator::pattern_type                    pattern_type;

  typedef std::integral_constant<bool, false>                    is_local;

  typedef self_t                                              global_type;
  typedef IteratorRangeLocalOrigin<Iterator, Sentinel>         local_type;

  typedef std::integral_constant<dim_t, pattern_type::ndim()>        rank;

 private:
  iterator             _begin;
  sentinel             _end;

 public:
  constexpr IteratorRangeOrigin(
    const iterator & begin,
    const sentinel & end)
  : _begin(begin)
  , _end(end)
  { }

  constexpr IteratorRangeOrigin(iterator && begin, sentinel && end)
  : _begin(std::move(begin))
  , _end(std::move(end))
  { }

  constexpr IteratorRangeOrigin()                     = delete;
  constexpr IteratorRangeOrigin(const self_t & other) = default;
  constexpr IteratorRangeOrigin(self_t && other)      = default;
  self_t & operator=(const self_t & other)            = default;
  self_t & operator=(self_t && other)                 = default;

  constexpr const_iterator begin() const { return _begin; }
  constexpr const_sentinel end()   const { return _end;   }

  iterator begin() { return _begin; }
  sentinel end()   { return _end;   }

  constexpr size_type size() const noexcept {
    return std::distance(_begin, _end);
  }

  constexpr const pattern_type & pattern() const noexcept {
    return this->begin().pattern();
  }

  constexpr const local_type local() const noexcept {
    return local_type(*this);
  }

  constexpr const self_t & global() const {
    return *this;
  }

  self_t & global() {
    return *this;
  }
};



// -----------------------------------------------------------------------
// Iterator Range Origin (local pointers)
// -----------------------------------------------------------------------

template <
  class LocalIterator,
  class LocalSentinel,
  class IndexSet >
struct view_traits<
         IteratorRangeOrigin<
           ViewIterator<LocalIterator *, IndexSet>,
           ViewIterator<LocalSentinel *, IndexSet>
         > > {
 private:
  typedef ViewIterator<LocalIterator *, IndexSet>                iterator;
  typedef ViewIterator<LocalSentinel *, IndexSet>                sentinel;

  typedef IteratorRangeOrigin<iterator, sentinel>                  RangeT;
 public:
  typedef RangeT                                              domain_type;
  typedef RangeT                                              origin_type;
  typedef RangeT                                               image_type;

  typedef std::integral_constant<dim_t, 1>                           rank;

  typedef RangeT                                              global_type;
  typedef RangeT                                               local_type;

  typedef typename iterator::index_type                        index_type;
  typedef typename std::make_unsigned<index_type>::type         size_type;

  typedef dash::IndexSetIdentity< 
            IteratorRangeOrigin<iterator, sentinel> >      index_set_type;

  typedef std::integral_constant<bool, false> is_projection;
  typedef std::integral_constant<bool, false> is_view;
  typedef std::integral_constant<bool, true > is_origin;
  typedef std::integral_constant<bool, true > is_local;
};

template <
  typename LocalIterator,
  typename LocalSentinel,
  typename IndexSet >
class IteratorRangeOrigin<
        ViewIterator<LocalIterator *, IndexSet>,
        ViewIterator<LocalSentinel *, IndexSet> >
{
  typedef IteratorRangeOrigin<
            ViewIterator<LocalIterator *, IndexSet>,
            ViewIterator<LocalSentinel *, IndexSet> >
    self_t;

 public:
  typedef ViewIterator<LocalIterator *, IndexSet>                iterator;
  typedef ViewIterator<LocalSentinel *, IndexSet>                sentinel;
  typedef ViewIterator<const LocalIterator *, IndexSet>    const_iterator;
  typedef ViewIterator<const LocalSentinel *, IndexSet>    const_sentinel;

  typedef dash::default_index_t                                index_type;
  typedef dash::default_size_t                                  size_type;

  typedef LocalIterator                                        value_type;

  typedef self_t                                              global_type;

  typedef typename IndexSet::pattern_type                    pattern_type;

  typedef std::integral_constant<bool, true>                     is_local;

  typedef std::integral_constant<dim_t, 1>                           rank;

 private:
  iterator _begin;
  sentinel _end;

 public:
  constexpr IteratorRangeOrigin(iterator begin, sentinel end)
  : _begin(begin)
  , _end(end)
  { }

  constexpr IteratorRangeOrigin()                     = delete;
  constexpr IteratorRangeOrigin(const self_t & other) = default;
  constexpr IteratorRangeOrigin(self_t && other)      = default;
  self_t & operator=(const self_t & other)            = default;
  self_t & operator=(self_t && other)                 = default;

  constexpr const_iterator begin() const { return _begin; }
  constexpr const_sentinel end()   const { return _end;   }

  iterator begin() { return _begin; }
  sentinel end()   { return _end;   }

  constexpr size_type size() const {
    return std::distance(_begin, _end);
  }
};


// ----------------------------------------------------------------------
// Iterator Range
// ----------------------------------------------------------------------

/**
 * Specialization of \c dash::view_traits for IteratorRange.
 */
template <class RangeOrigin>
struct view_traits<IteratorRange<RangeOrigin> > {
 private:
  typedef IteratorRange<RangeOrigin>                               RangeT;
 public:
  typedef RangeOrigin                                         domain_type;
  typedef RangeOrigin                                         origin_type;
  typedef RangeT                                               image_type;

  typedef typename RangeOrigin::pattern_type                 pattern_type;

  typedef std::integral_constant<dim_t, pattern_type::ndim()>        rank;

  typedef RangeT                                              global_type;
  typedef ViewLocalMod<RangeT, 1>                              local_type;

  typedef typename RangeT::index_type                          index_type;
  typedef typename RangeT::size_type                            size_type;

  typedef dash::IndexSetSub<domain_type, 0>                index_set_type;

  typedef std::integral_constant<bool, false> is_projection;
  typedef std::integral_constant<bool, true > is_view;
  typedef std::integral_constant<bool, false> is_origin;
//typedef typename view_traits<
//                   typename std::decay<RangeOrigin>::type
//                 >::is_local is_local;
  typedef std::integral_constant<bool, false> is_local;
};

/**
 * Adapter template for range concept, wraps `begin` and `end` iterators
 * in range type.
 */
template <class RangeOrigin>
class IteratorRange
: public ViewModBase<
           IteratorRange<RangeOrigin>,
           RangeOrigin,
           1 >
{
  typedef IteratorRange<RangeOrigin>
    self_t;
  typedef ViewModBase<IteratorRange<RangeOrigin>, RangeOrigin, 1 >
    base_t;
 public:
  typedef typename RangeOrigin::iterator                         iterator;
  typedef typename RangeOrigin::const_iterator             const_iterator;
  typedef typename RangeOrigin::sentinel                         sentinel;
  typedef typename RangeOrigin::const_sentinel             const_sentinel;

  typedef RangeOrigin                                         domain_type;
  typedef RangeOrigin                                         origin_type;
  typedef self_t                                               image_type;

  typedef typename domain_type::value_type                     value_type;

//typedef typename domain_type::pattern_type                 pattern_type;
//typedef std::integral_constant<dim_t, pattern_type::ndim()>        rank;
  typedef std::integral_constant<dim_t, 1>                           rank;

  typedef self_t                                              global_type;
  typedef ViewLocalMod<self_t, 1>                              local_type;

  typedef typename view_traits<
                     typename std::decay<RangeOrigin>::type
                   >::is_local is_local;

  typedef dash::default_index_t                                index_type;
  typedef dash::default_size_t                                  size_type;

  typedef dash::IndexSetSub<domain_type, 0>                index_set_type;

  typedef typename
            std::conditional<
              std::is_pointer<iterator>::value,
              iterator,
              typename iterator::local_type
            >::type
    local_iterator;

  typedef typename
            std::conditional<
              std::is_pointer<sentinel>::value,
              sentinel,
              typename sentinel::local_type
            >::type
    local_sentinel;

  using       reference = typename       iterator::reference;
  using const_reference = typename iterator::const_reference;

 private:
  static const dim_t   NDim         = rank::value;
  index_set_type       _index_set;

 public:
  constexpr IteratorRange(const iterator & begin, const sentinel & end)
  : base_t(domain_type(
             // Move begin iterator first position of its iteration scope:
             begin - begin.pos(),
             // Move end iterator to end position of its iteration scope:
             begin + (begin.pattern().size() - begin.pos()) ))
    // Convert iterator positions to sub-range index set:
  , _index_set(this->domain(), begin.pos(), end.pos())
  { }

  constexpr IteratorRange(iterator && begin, sentinel && end)
  : base_t(domain_type(
             // Move begin iterator first position of its iteration scope:
             std::move(begin) - begin.pos(),
             // Move end iterator to end position of its iteration scope:
             std::move(begin) + (begin.pattern().size() - begin.pos()) ))
    // Convert iterator positions to sub-range index set:
  , _index_set(this->domain(), begin.pos(), end.pos())
  { }

  constexpr IteratorRange()                     = delete;
  constexpr IteratorRange(const self_t & other) = default;
  constexpr IteratorRange(self_t && other)      = default;
  self_t & operator=(const self_t & other)      = default;
  self_t & operator=(self_t && other)           = default;

  // ---- extents -------------------------------------------------------

  constexpr std::array<size_type, NDim> extents() const {
    return _index_set.extents();
  }

  template <dim_t ExtDim>
  constexpr size_type extent() const {
    return _index_set.template extent<ExtDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return _index_set.extent(shape_dim);
  }

  // ---- offsets -------------------------------------------------------

  template <dim_t ExtDim>
  constexpr index_type offset() const {
    return _index_set.template offset<ExtDim>();
  }

  constexpr std::array<index_type, NDim> offsets() const {
    return _index_set.offsets();
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return _index_set.offset(shape_dim);
  }

  // ---- size ----------------------------------------------------------

  constexpr size_type size(dim_t sub_dim = 0) const {
    return _index_set.size(sub_dim);
  }

  // ---- access --------------------------------------------------------

  constexpr const_iterator begin() const {
    return dash::domain(*this).begin() + _index_set[0];
  }

  iterator begin() {
    return dash::domain(*this).begin() + _index_set[0];
  }

  constexpr const_iterator end() const {
    return dash::domain(*this).end();
  }

  iterator end() {
    return dash::domain(*this).end();
  }

  constexpr const_reference operator[](int offset) const {
    return *(dash::domain(*this).begin() + _index_set[offset]);
  }

  reference operator[](int offset) {
    return *(dash::domain(*this).begin() + _index_set[offset]);
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr const local_type local() const {
    return local_type(*this);
  }

  local_type local() {
    return local_type(*this);
  }

  constexpr const self_t & global() const {
    return *this;
  }

  self_t & global() {
    return *this;
  }
};

// -----------------------------------------------------------------------
// dash::make_range
// -----------------------------------------------------------------------

template <class Iterator, class Sentinel>
constexpr dash::IteratorRange<
            dash::IteratorRangeOrigin<
              typename std::decay<Iterator>::type,
              typename std::decay<Sentinel>::type > >
make_range(Iterator && begin, Sentinel && end) {
  return dash::IteratorRange<
           dash::IteratorRangeOrigin<
             typename std::decay<Iterator>::type,
             typename std::decay<Sentinel>::type >
         >(std::forward<Iterator>(begin),
           std::forward<Sentinel>(end));
}

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_1D_H__INCLUDED
