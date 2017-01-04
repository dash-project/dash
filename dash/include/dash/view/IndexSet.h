#ifndef DASH__VIEW__INDEX_SET_H__INCLUDED
#define DASH__VIEW__INDEX_SET_H__INCLUDED

#include <dash/view/ViewTraits.h>
#include <dash/view/Origin.h>


namespace dash {

template <
  class IndexSetType,
  class ViewType,
  class IndexSetDomain >
class IndexSetBase
{
public:
  typedef typename ViewType::index_type                     index_type;

  class index_iterator {
  public:
    index_iterator(
      const IndexSetType & index_set,
      index_type           position)
    : _index_set(index_set), _pos(position)
    { }

    constexpr index_type operator*() const {
      return _index_set[_pos];
    }

  private:
    const IndexSetType & _index_set;
    index_type           _pos;
  };

public:
  typedef index_iterator iterator;

  IndexSetBase(const ViewType & view)
  : _view(view)
  { }

  constexpr iterator begin() const {
    return iterator(*static_cast<const IndexSetType *>(this), 0);
  }

  constexpr iterator end() const {
    return iterator(*static_cast<const IndexSetType *>(this),
                    static_cast<const IndexSetType *>(this)->size());
  }

protected:
  constexpr const ViewType & view() const {
    return _view;
  }
  constexpr const IndexSetDomain & domain() const {
    return _domain;
  }

private:
  const ViewType       & _view;
  const IndexSetDomain & _domain;
};

template <
  class ViewType,
  class IndexSetDomain >
class IndexSetSub
: IndexSetBase<
    IndexSetSub<ViewType, IndexSetDomain>,
    ViewType,
    IndexSetDomain >
{
  typedef IndexSetSub<ViewType, IndexSetDomain>                 self_t;
  typedef IndexSetBase<self_t, ViewType, IndexSetDomain>        base_t;
public:
  typedef typename ViewType::index_type                     index_type;

  IndexSetSub(
    const ViewType   & view,
    index_type         begin,
    index_type         end)
  : base_t(view),
    _domain_begin_idx(begin),
    _domain_end_idx(end)
  { }

  constexpr index_type operator[](index_type image_index) const {
    return _domain_begin_idx + image_index;
  }

  constexpr index_type size() const {
    return _domain_end_idx - _domain_begin_idx;
  }

private:
  index_type _domain_begin_idx;
  index_type _domain_end_idx;
};

template <
  class ViewType,
  class IndexSetDomain >
class IndexSetLocal
: IndexSetBase<
    IndexSetLocal<ViewType, IndexSetDomain>,
    ViewType,
    IndexSetDomain >
{
  typedef IndexSetLocal<ViewType, IndexSetDomain>               self_t;
  typedef IndexSetBase<self_t, ViewType, IndexSetDomain>        base_t;

  typedef typename dash::view_traits<ViewType>::origin_type   origin_t;
  typedef typename origin_t::pattern_type                    pattern_t;
public:
  typedef typename ViewType::index_type                     index_type;

  IndexSetLocal(const ViewType & view)
  : base_t(view)
  { }

private:
  constexpr const pattern_t & pattern() const {
    return dash::origin(this->view()).pattern();
  }

public:

  constexpr index_type operator[](index_type image_index) const {
    return pattern().local_index(
                    // this->domain()[0] +
                       image_index);
  }

  constexpr index_type size() const {
    return static_cast<index_type>(pattern().local_size());
  }
};

} // namespace dash

#endif // DASH__VIEW__INDEX_SET_H__INCLUDED
