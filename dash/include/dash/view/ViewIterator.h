#ifndef DASH__VIEW__VIEW_ITERATOR_H__INCLUDED
#define DASH__VIEW__VIEW_ITERATOR_H__INCLUDED

#include <dash/util/internal/IteratorBase.h>
#include <dash/internal/TypeInfo.h>

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
  typedef typename IndexSetType::index_type            index_type;
private:
  DomainIterator  _domain_it;
  IndexSetType    _index_set;
public:
  constexpr ViewIterator() = delete;

  ViewIterator(
    const DomainIterator & domain_it, 
    const IndexSetType   & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(domain_it)
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
    return (_domain_it)[ (_index_set)[idx] ];
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
  typedef ViewIterator<DomainIterator, IndexSetType>       self_t;
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
  typedef std::ptrdiff_t                               index_type;
private:
  DomainIterator * _domain_it;
  IndexSetType     _index_set;
public:
  constexpr ViewIterator() = delete;

  ViewIterator(
    DomainIterator       * domain_it, 
    const IndexSetType   & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(domain_it)
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
    return (_domain_it)[ (_index_set)[idx] ];
  }
};

template<class DomainT, class IndexSetT>
std::ostream & operator<<(
  std::ostream                           & os,
  const ViewIterator<DomainT, IndexSetT> & view_it)
{
  std::ostringstream ss;
  ss << dash::internal::typestr(view_it) << " "
     << "{ "
     << "domain_it: " << view_it._domain_it << ", "
     << "position: "  << view_it.pos()
     << " }";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__VIEW__VIEW_ITERATOR_H__INCLUDED
