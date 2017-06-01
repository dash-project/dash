#ifndef DASH__VIEW__MAKE_RANGE_H__INCLUDED
#define DASH__VIEW__MAKE_RANGE_H__INCLUDED

#include <dash/Range.h>
#include <dash/Meta.h>

// #include <dash/view/Sub.h>
// #include <dash/view/Local.h>
// #include <dash/view/ViewMod.h>
#include <dash/view/IndexSet.h>



namespace dash {

#ifndef DOXYGEN

// Related: boost::range
//
// https://github.com/boostorg/range/tree/develop/include/boost/range
//

template <typename ViewT>
struct view_traits;

// Forward-declaration
template <typename ViewType>
class IndexSetIdentity;

template <class DomainType, std::size_t SubDim>
class IndexSetSub;

// Forward-declaration
template <class RangeOrigin>
class IteratorRange;

#endif


#if 0
template <
  typename IteratorT,
  typename SentinelT >
struct view_traits<dash::IteratorRange<IteratorT *, SentinelT *> > {
private:
  typedef IteratorRange<IteratorT *, SentinelT *> RangeT;
public:
  typedef RangeT                                               domain_type;
  typedef RangeT                                               origin_type;
  typedef RangeT                                                image_type;
  typedef RangeT                                               global_type;
  typedef RangeT                                                local_type;
  typedef typename RangeT::index_type                           index_type;
  typedef typename RangeT::size_type                             size_type;
  typedef dash::IndexSetIdentity<RangeT>                    index_set_type;

  /// Whether the view type is a projection (has less dimensions than the
  /// view's domain type).
  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, false>                is_view;
  /// Whether the view is the origin domain.
  typedef std::integral_constant<bool, false>                is_origin;
  /// Whether the view / container type is a local view.
  /// \note A container type is local if it is identical to its
  ///       \c local_type
  typedef std::integral_constant<bool, true>                 is_local;

  typedef std::integral_constant<dim_t, 1>                   rank;
};

/**
 * Specialization of adapter template for range concept, wraps `begin`
 * and `end` pointers in range type.
 */
template <
  typename LocalIterator,
  typename LocalSentinel >
class IteratorRange<LocalIterator *, LocalSentinel *>
{
  typedef IteratorRange<LocalIterator *, LocalSentinel *> self_t;

  LocalIterator * _begin;
  LocalSentinel * _end;

public:
  typedef LocalIterator *                                         iterator;
  typedef LocalSentinel *                                         sentinel;
  typedef const LocalIterator *                             const_iterator;
  typedef const LocalSentinel *                             const_sentinel;

  typedef dash::default_index_t                                 index_type;
  typedef dash::default_size_t                                   size_type;

  typedef LocalIterator                                         value_type;

  typedef iterator local_iterator;
  typedef sentinel local_sentinel;
            
  typedef IteratorRange<local_iterator, local_sentinel>         local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, true>                      is_local;

public:
  constexpr IteratorRange(iterator begin, sentinel end)
  : _begin(begin)
  , _end(end)
  { }

  constexpr const_iterator begin() const { return _begin; }
  constexpr const_sentinel end()   const { return _end;   }

  iterator begin() { return _begin; }
  sentinel end()   { return _end;   }

  constexpr size_type size() const { return std::distance(_begin, _end); }

  constexpr const local_type & local() const {
    return *this;
  }
};

/**
 * Adapter utility function.
 * Wraps `begin` and `end` pointers in range type.
 */
template <
  class    Iterator,
  class    Sentinel,
  typename std::enable_if< std::is_pointer<Iterator>::value > * = nullptr >
constexpr dash::IteratorRange<Iterator *, Sentinel *>
make_range(
  Iterator * begin,
  Sentinel * end) {
  return dash::IteratorRange<Iterator *, Sentinel *>(
           begin,
           end);
}
#endif

#if 1
/*
template <class Iterator, class Sentinel>
constexpr dash::IteratorRange<
            typename std::decay<Iterator>::type,
            typename std::decay<Sentinel>::type >
make_range(Iterator && begin, Sentinel && end) {
  return dash::IteratorRange<
            typename std::decay<Iterator>::type,
            typename std::decay<Sentinel>::type
         >(std::forward<Iterator>(begin),
           std::forward<Sentinel>(end));
}

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
*/
#else
/**
 * Adapter utility function.
 * Wraps `begin` and `end` iterators in range type.
 */
template <
  class    Iterator,
  class    Sentinel,
  typename IteratorDT = typename std::decay<Iterator>::type,
  typename SentinelDT = typename std::decay<Sentinel>::type,
  typename std::enable_if< !std::is_pointer<IteratorDT>::value > * = nullptr >
auto
make_range(
  const Iterator & begin,
  const Sentinel & end)
  -> decltype(
      dash::sub(begin.pos(),
                end.pos(),
                dash::IteratorRange<Iterator, Sentinel>(
                  begin - begin.pos(),
                  begin + (begin.pattern().size() - begin.pos())))) {
  return dash::sub(begin.pos(),
                   end.pos(),
                   dash::IteratorRange<Iterator, Sentinel>(
                     begin - begin.pos(),
                     begin + (begin.pattern().size() - begin.pos())));
}
#endif

} // namespace dash

#endif // DASH__VIEW__MAKE_RANGE_H__INCLUDED
