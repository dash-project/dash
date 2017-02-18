#ifndef DASH__VIEW__VIEW_ITERATOR_H__INCLUDED
#define DASH__VIEW__VIEW_ITERATOR_H__INCLUDED

#include <dash/util/internal/IteratorBase.h>


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
public:
  typedef typename base_t::reference                    reference;
  typedef typename IndexSetType::index_type            index_type;
private:
  const DomainIterator * _domain_it;
        IndexSetType     _index_set;
public:
  constexpr ViewIterator() = delete;

  ViewIterator(
    const DomainIterator & domain_it, 
    const IndexSetType   & index_set,
    index_type             position)
  : base_t(position)
  , _domain_it(&domain_it)
  , _index_set(index_set)
  { }

  constexpr reference dereference(index_type idx) const {
    return (*_domain_it)[ (_index_set)[idx] ];
  }
};


} // namespace dash

#endif // DASH__VIEW__VIEW_ITERATOR_H__INCLUDED
