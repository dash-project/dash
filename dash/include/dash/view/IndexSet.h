#ifndef DASH__VIEW__INDEX_SET_H__INCLUDED
#define DASH__VIEW__INDEX_SET_H__INCLUDED

namespace dash {

template <
  class IndexSetType,
  class ViewType >
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

  constexpr iterator begin() const {
    return iterator(*static_cast<const IndexSetType *>(this), 0);
  }

  constexpr iterator end() const {
    return iterator(*static_cast<const IndexSetType *>(this),
                    static_cast<const IndexSetType *>(this)->size());
  }

};

template <class ViewType>
class IndexSetSub
: IndexSetBase<IndexSetSub<ViewType>, ViewType>
{
  typedef IndexSetSub<ViewType>                                 self_t;
  typedef IndexSetBase<self_t, ViewType>                        base_t;
public:
  typedef typename ViewType::index_type                     index_type;

  IndexSetSub(
    const ViewType   & view,
    index_type         begin,
    index_type         end)
  : _domain(view), _domain_begin_idx(begin), _domain_end_idx(end)
  { }

  constexpr index_type operator[](index_type image_index) const {
    return _domain_begin_idx + image_index;
  }

  constexpr index_type size() const {
    return _domain_end_idx - _domain_begin_idx;
  }

private:
  const ViewType & _domain;
  index_type       _domain_begin_idx;
  index_type       _domain_end_idx;
};

} // namespace dash

#endif // DASH__VIEW__INDEX_SET_H__INCLUDED
