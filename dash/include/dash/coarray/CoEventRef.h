#ifndef DASH__COARRAY__COEVENTREF_H
#define DASH__COARRAY__COEVENTREF_H

#include <dash/Team.h>

namespace dash {
namespace coarray {

class CoEventRef {
private:
  using self_t = CoEventRef;

public:
  explicit CoEventRef(
    const int & pos = 0,
    Team & team = dash::Team::Null())
  : _team(team),
    _pos() {}
  
  inline void post() const {
    // TODO implement this
  }
  
  inline Team & team() {
    return _team;
  }
  inline bool operator ==(const self_t & other) const noexcept {
    return (_pos == other._pos) && (_team == other._team);
  }
  inline bool operator !=(const self_t & other) const noexcept {
    return !(*this == other);
  }  
  
private:
  Team & _team = dash::Team::All();
  int    _pos  = 0;
};

} // namespace coarray
} // namespace dash


#endif /* DASH__COARRAY__COEVENTREF_H */

