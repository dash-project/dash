#ifndef DASH__VIEW__MAKE_RANGE_H__INCLUDED
#define DASH__VIEW__MAKE_RANGE_H__INCLUDED

#include <dash/Range.h>
#include <dash/Meta.h>

#include <dash/view/Sub.h>



namespace dash {

template <
  class Iterator,
  class Sentinel >
class IteratorRangeOrigin;

template <
  class Iterator,
  class Sentinel >
struct view_traits<IteratorRangeOrigin<Iterator, Sentinel> > {
  typedef IteratorRangeOrigin<Iterator, Sentinel>               domain_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>               origin_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>                image_type;

  typedef typename dash::view_traits<domain_type>::local_type    local_type;
  typedef typename dash::view_traits<domain_type>::global_type  global_type;

  typedef typename Iterator::index_type                          index_type;
  typedef typename std::make_unsigned<index_type>::type           size_type;

  typedef typename Iterator::pattern_type                      pattern_type;

  typedef dash::IndexSetIdentity< 
            IteratorRangeOrigin<Iterator, Sentinel> >        index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, true>                 is_origin;
  typedef std::integral_constant<bool, false>                is_local;

  typedef std::integral_constant<dim_t, 1>                   rank;
};

template <
  class Iterator,
  class Sentinel
>
class IteratorRangeOrigin
{
  typedef IteratorRangeOrigin<Iterator, Sentinel>                    self_t;
public:
  typedef Iterator                                                 iterator;
  typedef Iterator                                           const_iterator;
  typedef Sentinel                                                 sentinel;
  typedef Sentinel                                           const_sentinel;

  typedef IteratorRangeOrigin<Iterator, Sentinel>               domain_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>               origin_type;
  typedef IteratorRangeOrigin<Iterator, Sentinel>                image_type;

  typedef typename iterator::value_type                          value_type;

  typedef typename Iterator::index_type                          index_type;
  typedef typename std::make_unsigned<index_type>::type           size_type;

  typedef typename iterator::pattern_type                      pattern_type;

  typedef ViewLocalMod<domain_type>                              local_type;
  typedef ViewGlobalMod<domain_type>                            global_type;

  typedef std::integral_constant<dim_t, 1>                             rank;

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


private:
  Iterator             _begin;
  Sentinel             _end;

public:
  constexpr IteratorRangeOrigin(const iterator & begin, const sentinel & end)
  : _begin(begin)
  , _end(end)
  { }

  constexpr IteratorRangeOrigin(iterator && begin, sentinel && end)
  : _begin(std::move(begin))
  , _end(std::move(end))
  { }

  constexpr const_iterator begin() const { return _begin; }
  constexpr const_sentinel end()   const { return _end;   }

  iterator begin() { return _begin; }
  sentinel end()   { return _end;   }

  constexpr size_type size() const { return std::distance(_begin, _end); }

  constexpr const pattern_type & pattern() const {
    return this->begin().pattern();
  }

  constexpr local_type local() const {
    // for local_type: IteratorLocalView<self_t, Iterator, Sentinel>
    // return local_type(this->begin(), this->end());
    return local_type(*this);
  }
};

/**
 * Specialization of \c dash::view_traits for IteratorRange.
 */
template <
  typename IteratorT,
  typename SentinelT >
struct view_traits<dash::IteratorRange<IteratorT, SentinelT> > {
private:
  typedef IteratorRange<IteratorT, SentinelT> RangeT;
public:
  typedef IteratorRangeOrigin<IteratorT, SentinelT>            domain_type;
  typedef IteratorRangeOrigin<IteratorT, SentinelT>            origin_type;

  typedef typename IteratorT::pattern_type                    pattern_type;

  typedef RangeT                                                image_type;
  typedef RangeT                                               global_type;
  typedef typename RangeT::local_type                           local_type;
  typedef typename RangeT::index_type                           index_type;
  typedef typename RangeT::size_type                             size_type;

  typedef dash::IndexSetSub<domain_type, 0>                 index_set_type;

  /// Whether the view type is a projection (has less dimensions than the
  /// view's domain type).
  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  /// Whether the view is the origin domain.
  typedef std::integral_constant<bool, false>                is_origin;
  /// Whether the range is local view.
  typedef std::integral_constant<
            bool, std::is_pointer<IteratorT>::value>         is_local;

  typedef std::integral_constant<dim_t, 1>                   rank;
};

/**
 * Adapter template for range concept, wraps `begin` and `end` iterators
 * in range type.
 */
template <
  typename Iterator,
  typename Sentinel >
class IteratorRange
: public ViewModBase<
           IteratorRange<IteratorRangeOrigin<Iterator, Sentinel>>,
           IteratorRangeOrigin<Iterator, Sentinel>,
           1 >
{
  typedef IteratorRange<Iterator, Sentinel>                         self_t;
  typedef ViewModBase<
           IteratorRange<IteratorRangeOrigin<Iterator, Sentinel>>,
           IteratorRangeOrigin<Iterator, Sentinel>,
           1 >                                                      base_t;
public:
  typedef Iterator                                                iterator;
  typedef Sentinel                                                sentinel;

  typedef IteratorRangeOrigin<iterator, sentinel>              domain_type;
  typedef IteratorRangeOrigin<iterator, sentinel>              origin_type;
  typedef self_t                                                image_type;

  typedef self_t                                               global_type;
  typedef ViewLocalMod<self_t>                                  local_type;

  typedef dash::default_index_t                                 index_type;
  typedef dash::default_size_t                                   size_type;

  typedef typename iterator::value_type                         value_type;
  typedef typename iterator::pattern_type                     pattern_type;

  typedef dash::IndexSetSub<domain_type, 0>                 index_set_type;

  typedef std::integral_constant<dim_t, 1>                            rank;

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

private:
  index_set_type  _index_set;

public:
  constexpr IteratorRange(const iterator & begin, const sentinel & end)
  : base_t(domain_type(begin, end))
  , _index_set(this->domain(), begin.pos(), end.pos())
  { }

  constexpr IteratorRange(iterator && begin, sentinel && end)
  : base_t(domain_type(std::move(begin), std::move(end)))
  , _index_set(this->domain(), begin.pos(), end.pos())
  { }

  constexpr IteratorRange()                     = delete;
  constexpr IteratorRange(const self_t & other) = default;
  constexpr IteratorRange(self_t && other)      = default;
  self_t & operator=(const self_t & other)      = default;
  self_t & operator=(self_t && other)           = default;

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr local_type local() const {
    return local_type(*this);
  }
};

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
