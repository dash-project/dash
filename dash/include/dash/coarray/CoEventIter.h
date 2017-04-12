#ifndef DASH__COARRAY__COEVENTITER_H
#define DASH__COARRAY__COEVENTITER_H

#include <iterator>

#include <dash/Team.h>
#include <dash/coarray/CoEventRef.h>

namespace dash {
namespace coarray {

class CoEventIter {
private:
  using self_t = CoEventIter;
public:
  using difference_type   = std::ptrdiff_t;
  using value_type        = CoEventRef;
  using pointer           = CoEventRef *;
  using reference         = CoEventRef &;
  using iterator_category = std::random_access_iterator_tag;

public:
  
  explicit CoEventIter(
    const int & pos = 0,
    Team & team = dash::Team::Null())
  : _team(team),
    _pos() {}
  
  inline Team & team() {
    return _team;
  }
  inline value_type operator[] (int pos) const {
    return value_type(_pos + pos, _team);
  }
  
  inline value_type operator* () const {
    return CoEventRef(_pos, _team);
  }
  /*
   * Comparison operators
   */
  inline bool operator <(const self_t & other) const noexcept {
    return _pos < other._pos;
  }
  inline bool operator >(const self_t & other) const noexcept {
    return _pos > other._pos;
  }
  inline bool operator <=(const self_t & other) const noexcept {
    return _pos <= other._pos;
  }
  inline bool operator >=(const self_t & other) const noexcept {
    return _pos >= other._pos;
  }
  inline bool operator ==(const self_t & other) const noexcept {
    return (_pos == other._pos) && (_team == other._team);
  }
  inline bool operator !=(const self_t & other) const noexcept {
    return !(*this == other);
  }  
  /*
   * Arith. operators
   */
  inline self_t & operator +=(int i) noexcept {
    _pos += i;
    return *this;
  }
  inline self_t & operator -=(int i) noexcept {
    _pos -= i;
    return *this;
  }
  inline self_t & operator ++() noexcept {
    ++_pos;
    return *this;
  }
  inline self_t operator ++(int) noexcept {
    int oldpos = _pos++;
    return CoEventIter(oldpos);
  }
  inline self_t & operator --() noexcept{
    --_pos;
    return *this;
  }
  inline self_t operator --(int) noexcept {
    int oldpos = _pos--;
    return CoEventIter(oldpos);
  }
  inline self_t operator +(int i) const noexcept {
    return self_t(_pos + i);
  }
  inline self_t operator -(int i) const noexcept {
    return self_t(_pos - i);
  }
  
private:
  Team & _team = dash::Team::Null();
  int    _pos  = 0;
};

} // namespace coarray
} // namespace dash


#endif /* DASH__COARRAY__COEVENTITER_H */

