#ifndef DASH__COARRAY__COEVENTITER_H
#define DASH__COARRAY__COEVENTITER_H

#include <iterator>

#include <dash/Team.h>
#include <dash/Atomic.h>
#include <dash/coarray/CoEventRef.h>

namespace dash {
namespace coarray {

class CoEventIter {
private:
  using self_t = CoEventIter;
  using gptr_t = GlobPtr<dash::Atomic<int>>;
public:
  using difference_type   = typename gptr_t::gptrdiff_t;
  using value_type        = CoEventRef;
  using pointer           = CoEventRef *;
  using reference         = CoEventRef &;
  using iterator_category = std::random_access_iterator_tag;

public:

  explicit CoEventIter(
    const gptr_t & pos,
    Team & team = dash::Team::Null())
  : _team(team),
    _gptr(pos) {}

  inline Team & team() {
    return _team;
  }
  inline value_type operator[] (int pos) const {
    return value_type(_gptr + pos, _team);
  }

  inline value_type operator* () const {
    return value_type(_gptr, _team);
  }
  /*
   * Comparison operators
   */
  inline bool operator <(const self_t & other) const noexcept {
    return _gptr < other._gptr;
  }
  inline bool operator >(const self_t & other) const noexcept {
    return _gptr > other._gptr;
  }
  inline bool operator <=(const self_t & other) const noexcept {
    return _gptr <= other._gptr;
  }
  inline bool operator >=(const self_t & other) const noexcept {
    return _gptr >= other._gptr;
  }
  inline bool operator ==(const self_t & other) const noexcept {
    return (_gptr == other._gptr) && (_team == other._team);
  }
  inline bool operator !=(const self_t & other) const noexcept {
    return !(*this == other);
  }
  /*
   * Arith. operators
   */
  inline self_t & operator +=(int i) noexcept {
    _gptr += i;
    return *this;
  }
  inline self_t & operator -=(int i) noexcept {
    _gptr -= i;
    return *this;
  }
  inline self_t & operator ++() noexcept {
    ++_gptr;
    return *this;
  }
  inline self_t operator ++(int) noexcept {
    auto oldptr = _gptr++;
    return self_t(oldptr);
  }
  inline self_t & operator --() noexcept{
    --_gptr;
    return *this;
  }
  inline self_t operator --(int) noexcept {
    auto oldptr = _gptr--;
    return self_t(oldptr);
  }
  inline self_t operator +(int i) const noexcept {
    return self_t(_gptr + i);
  }
  inline self_t operator -(int i) const noexcept {
    return self_t(_gptr - i);
  }

private:
  Team   & _team = dash::Team::Null();
  gptr_t   _gptr;
};

} // namespace coarray
} // namespace dash


#endif /* DASH__COARRAY__COEVENTITER_H */

