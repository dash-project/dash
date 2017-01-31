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
  typedef IteratorType           derived_t;
 private:
  index_type _pos;

 private:
  derived_t & derived() {
    return static_cast<IteratorType &>(*this);
  }
  constexpr const derived_t & derived() const {
    return static_cast<const derived_t &>(*this);
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

  constexpr explicit IndexIteratorBase(index_type position)
  : _pos(position)
  { }

  constexpr index_type pos() const {
    return _pos;
  }

  constexpr reference operator*() const {
    return derived().dereference(_pos);
  }

  constexpr reference operator->() const {
    return derived().dereference(_pos);
  }

  derived_t & operator++() {
    _pos++;
    return derived();
  }

  derived_t & operator--() {
    _pos--;
    return derived();
  }

  derived_t & operator+=(int i) {
    _pos += i;
    return derived();
  }

  derived_t & operator-=(int i) {
    _pos -= i;
    return derived();
  }

  constexpr derived_t operator+(int i) const {
    return derived_t(derived(), _pos + i);
  }

  constexpr derived_t operator-(int i) const {
    return derived_t(derived(), _pos - i);
  }

  constexpr index_type operator+(const derived_t & rhs) const {
    return _pos + rhs._pos;
  }

  constexpr index_type operator-(const derived_t & rhs) const {
    return _pos - rhs._pos;
  }

  constexpr bool operator==(const derived_t & rhs) const {
    return _pos == rhs._pos;
  }

  constexpr bool operator!=(const derived_t & rhs) const {
    return _pos != rhs._pos;
  }

  constexpr bool operator<(const derived_t & rhs) const {
    return _pos < rhs._pos;
  }

  constexpr bool operator<=(const derived_t & rhs) const {
    return _pos <= rhs._pos;
  }

  constexpr bool operator>(const derived_t & rhs) const {
    return _pos > rhs._pos;
  }

  constexpr bool operator>=(const derived_t & rhs) const {
    return _pos >= rhs._pos;
  }
};

} // namespace detail


} // namespace dash

#endif // DASH__UTIL__INTERNAL__ITERATOR_BASE_H__INCLUDED
