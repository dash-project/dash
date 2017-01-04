#ifndef DASH__VIEW__INDEX_SET_H__INCLUDED
#define DASH__VIEW__INDEX_SET_H__INCLUDED

namespace dash {

template <
  class IndexSetType,
  class ViewType >
class IndexSetBase
{
public:
  class index_iterator {

  };

public:
  typedef index_iterator iterator;

};

template <class ViewType>
class IndexSetSub
: IndexSetBase<IndexSetSub<ViewType>, ViewType>
{
  typedef IndexSetSub<ViewType>                                 self_t;
  typedef IndexSetBase<self_t, ViewType>                        base_t;
public:
  typedef typename ViewType::index_type                     index_type;
  typedef typename base_t::iterator                           iterator;

  IndexSetSub(
    ViewType   & view,
    index_type   begin,
    index_type   end)
  : _domain(view), _domain_begin_idx(begin), _domain_end_idx(end)
  { }

  index_type operator[](index_type image_index) {
    return _domain_begin_idx + image_index;
  }

  iterator begin() const;
  iterator end() const;

private:
  ViewType & _domain;
  index_type _domain_begin_idx;
  index_type _domain_end_idx;
};

} // namespace dash

#endif // DASH__VIEW__INDEX_SET_H__INCLUDED
