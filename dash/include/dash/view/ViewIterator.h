#ifndef DASH__VIEW__VIEW_ITERATOR_H__INCLUDED
#define DASH__VIEW__VIEW_ITERATOR_H__INCLUDED

#include <dash/GlobPtr.h>
#include <dash/iterator/internal/IteratorBase.h>
#include <dash/meta/TypeInfo.h>

#include <iostream>
#include <sstream>


namespace dash {

// --------------------------------------------------------------------
// ViewIterator
// --------------------------------------------------------------------

template <
  class DomainIterator,
  class IndexSetType >
class ViewIterator
  : public dash::internal::IndexIteratorBase<
      ViewIterator<DomainIterator, IndexSetType>,
      typename DomainIterator::value_type,      // value type
      typename DomainIterator::difference_type, // difference type
      typename DomainIterator::pointer,         // pointer
      typename DomainIterator::reference        // reference
    > {
  typedef ViewIterator<DomainIterator, IndexSetType>       self_t;
  typedef dash::internal::IndexIteratorBase<
            ViewIterator<DomainIterator, IndexSetType>,
            typename DomainIterator::value_type,
            typename DomainIterator::difference_type,
            typename DomainIterator::pointer,
            typename DomainIterator::reference >           base_t;

  template<class DomainT, class IndexSetT>
  friend std::ostream & operator<<(
    std::ostream                           & os,
    const ViewIterator<DomainT, IndexSetT> & view_it);
public:
  typedef typename base_t::reference                    reference;
  typedef typename base_t::value_type                  value_type;
  typedef typename IndexSetType::index_type            index_type;
  typedef typename DomainIterator::pattern_type      pattern_type;
  typedef typename DomainIterator::local_type          local_type;

  typedef std::integral_constant<bool, false>            has_view;

  typedef IndexSetType                             index_set_type;

  typedef DomainIterator                          domain_iterator;
private:
  DomainIterator  _domain_it;
  IndexSetType    _index_set;
public:
  constexpr ViewIterator()                 = delete;

  ViewIterator(self_t && other)            = default;
  self_t & operator=(self_t && other)      = default;

  ViewIterator(const self_t & other)       = default;
  self_t & operator=(const self_t & other) = default;

  template <class DomainItType>
  ViewIterator(
    const DomainItType   & domain_it, 
    const IndexSetType   & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(domain_it)
  , _index_set(index_set)
  { }

  template <class DomainItType>
  ViewIterator(
    DomainItType        && domain_it, 
    const IndexSetType   & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(std::forward<DomainItType>(domain_it))
  , _index_set(index_set)
  { }

  ViewIterator(
    const self_t         & other, 
    index_type             position)
  : base_t(position)
  , _domain_it(other._domain_it)
  , _index_set(other._index_set)
  { }

  constexpr reference dereference(index_type idx) const {
    return *(_domain_it + (_index_set[idx]));
  }

  constexpr index_type gpos() const {
    return (_index_set)[this->pos()];
  }

  constexpr bool is_local() const {
    return (_domain_it + (_index_set[this->pos()])).is_local();
  }

  constexpr local_type local() const {
    return (_domain_it + (_index_set[this->pos()])).local();
  }

  constexpr const self_t & global() const {
    return *this;
  }

  constexpr dart_gptr_t dart_gptr() const {
    return (_domain_it + _index_set[this->pos()]).dart_gptr();
  }

  constexpr explicit operator dart_gptr_t() const {
    return dart_gptr();
  }

  constexpr const dash::Team & team() const {
    return _domain_it.team();
  }

  constexpr explicit operator domain_iterator() const {
    return (_domain_it + _index_set[this->pos()]);
  }

  explicit operator domain_iterator() {
    return (_domain_it + _index_set[this->pos()]);
  }

  constexpr domain_iterator domain() const {
    return (_domain_it + _index_set[this->pos()]);
  }

  constexpr const pattern_type & pattern() const {
    return _domain_it.pattern();
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }
};

template <
  class DomainIterator,
  class IndexSetType >
class ViewIterator<DomainIterator *, IndexSetType>
  : public dash::internal::IndexIteratorBase<
      ViewIterator<DomainIterator *, IndexSetType>,
      DomainIterator,
      std::ptrdiff_t,
      DomainIterator *,
      DomainIterator & >
{
  typedef ViewIterator<DomainIterator *, IndexSetType>     self_t;
  typedef dash::internal::IndexIteratorBase<
            ViewIterator<DomainIterator *, IndexSetType>,
            DomainIterator,
            std::ptrdiff_t,
            DomainIterator *,
            DomainIterator & >                             base_t;

  template<class DomainT, class IndexSetT>
  friend std::ostream & operator<<(
    std::ostream                           & os,
    const ViewIterator<DomainT, IndexSetT> & view_it);
public:
  typedef DomainIterator &                              reference;
  typedef DomainIterator                               value_type;
  typedef std::ptrdiff_t                               index_type;
  typedef self_t                                       local_type;
private:
  DomainIterator * _domain_it;
  IndexSetType     _index_set;
public:
  constexpr ViewIterator()            = delete;

  ViewIterator(const self_t & other)
  : base_t(other.pos())
  , _domain_it(other._domain_it)
  , _index_set(other._index_set)
  { }

  ViewIterator(self_t && other)
  : base_t(other.pos())
  , _domain_it(other._domain_it)
  , _index_set(other._index_set)
  { }

  self_t & operator=(const self_t & other) {
    base_t::operator=(other);
    _domain_it = other._domain_it;
    _index_set = other._index_set;
  }

  self_t & operator=(self_t && other) {
    base_t::operator=(other);
    _domain_it = other._domain_it;
    _index_set = other._index_set;
  }

  template <class DomainItType, class IndexSetO>
  ViewIterator(
    DomainItType         * domain_it, 
    const IndexSetO      & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(domain_it)
  , _index_set(index_set)
  { }

  template <class IndexSetO>
  ViewIterator(
    const self_t         & other, 
    const IndexSetO      & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(other._domain_it)
  , _index_set(index_set)
  { }

  template <class ViewIteratorO>
  ViewIterator(
    const ViewIteratorO  & other, 
    index_type             position)
  : base_t(position)
  , _domain_it(other._domain_it)
  , _index_set(other._index_set)
  { }

  constexpr reference dereference(index_type idx) const {
    return (_domain_it)[ (_index_set)[idx] ];
  }

  constexpr index_type gpos() const {
    return (_index_set)[this->pos()];
  }

  constexpr const value_type * local() const {
    return (_domain_it + (_index_set[this->pos()])).local();
  }

  inline value_type * local() {
    return (_domain_it + (_index_set[this->pos()])).local();
  }

  constexpr explicit operator const value_type *() const {
    return (_domain_it + (_index_set[this->pos()])).local();
  }

  explicit operator value_type *() {
    return (_domain_it + (_index_set[this->pos()])).local();
  }

  constexpr explicit operator DomainIterator() const {
    return (_domain_it + _index_set[this->pos()]);
  }
};

template<class DomainT, class IndexSetT>
std::ostream & operator<<(
  std::ostream                           & os,
  const ViewIterator<DomainT, IndexSetT> & view_it)
{
  std::ostringstream ss;
  ss << dash::typestr(view_it) << " "
     << "{ "
     << "domain_it: " << view_it._domain_it << ", "
     << "rpos: "      << view_it.pos()      << ", "
     << "gpos: "      << view_it.gpos()
     << " }";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__VIEW__VIEW_ITERATOR_H__INCLUDED
