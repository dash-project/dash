#ifndef DASH__UTIL__INTERNAL__ITERATOR_BASE_H__INCLUDED
#define DASH__UTIL__INTERNAL__ITERATOR_BASE_H__INCLUDED

#include <dash/Types.h>

#include <iterator>


namespace dash {
namespace internal {

template <
  class IteratorType,
  class ValueType    = typename IteratorType::value_type,
  class IndexType    = dash::default_index_t,
  class Pointer      = ValueType *,
  class Reference    = ValueType & >
class IndexIteratorBase {
  typedef IndexIteratorBase<
            IteratorType,
            ValueType,
            IndexType,
            Pointer,
            Reference >          self_t;
  typedef dash::default_index_t  index_type;
  typedef IteratorType           iterator;
private:
  index_type _pos;

private:
  iterator & derived() {
    return static_cast<IteratorType &>(*this);
  }
  constexpr const iterator & derived() const {
    return static_cast<const iterator &>(*this);
  }

public:
  typedef std::random_access_iterator_tag       iterator_category;

  typedef ValueType                                    value_type;
  typedef index_type                              difference_type;
  typedef Pointer                                         pointer;
  typedef const Pointer                             const_pointer;
  typedef Reference                                     reference;
  typedef const Reference                         const_reference;

public:
  constexpr IndexIteratorBase()                = delete;
  constexpr IndexIteratorBase(self_t &&)       = default;
  constexpr IndexIteratorBase(const self_t &)  = default;
  ~IndexIteratorBase()                         = default;
  self_t & operator=(self_t &&)                = default;
  self_t & operator=(const self_t &)           = default;

  constexpr IndexIteratorBase(index_type position)
  : _pos(position)
  { }

  constexpr index_type pos() const {
    return _pos;
  }

  constexpr const_reference operator*() const {
    return derived().dereference(_pos);
  }

  constexpr const_reference operator->() const {
    return derived().dereference(_pos);
  }

  iterator & operator++() {
    _pos++;
    return derived();
  }

  iterator & operator--() {
    _pos--;
    return derived();
  }

  iterator & operator+=(int i) {
    _pos += i;
    return derived();
  }

  iterator & operator-=(int i) {
    _pos -= i;
    return derived();
  }

  constexpr iterator operator+(int i) const {
    return iterator(derived(), _pos + i);
  }

  constexpr iterator operator-(int i) const {
    return iterator(derived(), _pos - i);
  }

  constexpr index_type operator+(const iterator & rhs) const {
    return _pos + rhs._pos;
  }

  constexpr index_type operator-(const iterator & rhs) const {
    return _pos - rhs._pos;
  }

  constexpr bool operator==(const iterator & rhs) const {
    return _pos == rhs._pos;
  }

  constexpr bool operator!=(const iterator & rhs) const {
    return not (derived() == rhs);
  }

  constexpr bool operator<(const iterator & rhs) const {
    return _pos < rhs._pos;
  }

  constexpr bool operator<=(const iterator & rhs) const {
    return _pos <= rhs._pos;
  }

  constexpr bool operator>(const iterator & rhs) const {
    return _pos > rhs._pos;
  }

  constexpr bool operator>=(const iterator & rhs) const {
    return _pos >= rhs._pos;
  }
};

} // namespace detail


} // namespace dash

#endif // DASH__UTIL__INTERNAL__ITERATOR_BASE_H__INCLUDED
