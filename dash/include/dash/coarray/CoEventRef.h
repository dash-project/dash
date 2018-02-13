#ifndef DASH__COARRAY__COEVENTREF_H
#define DASH__COARRAY__COEVENTREF_H

#include <dash/Team.h>
#include <dash/GlobPtr.h>
#include <dash/Atomic.h>

namespace dash {
namespace coarray {

class CoEventRef {
private:
  using self_t      = CoEventRef;
  using event_ctr_t = dash::Atomic<int>;
  using gptr_t      = GlobPtr<event_ctr_t>;

public:
  explicit CoEventRef(
    const gptr_t & gptr,
    Team & team = dash::Team::Null())
  : _team(team),
    _gptr(gptr) {}

  /**
   * post an event to this unit. This function is thread-safe
   */
  inline void post() const {
    DASH_LOG_DEBUG("post event to gptr", _gptr);
    GlobRef<event_ctr_t> gref(_gptr);
    gref.add(1);
    DASH_LOG_DEBUG("event posted");
  }

  /**
   * returns the number of arrived events at this unit
   */
  inline int test() const {
    DASH_LOG_DEBUG("test for events on", _gptr);
    GlobRef<event_ctr_t> gref(_gptr);
    return gref.load();
  }

  inline Team & team() {
    return _team;
  }
  inline bool operator ==(const self_t & other) const noexcept {
    return (_gptr == other._gptr) && (_team == other._team);
  }
  inline bool operator !=(const self_t & other) const noexcept {
    return !(*this == other);
  }

private:
  Team   & _team = dash::Team::All();
  gptr_t   _gptr;
};

} // namespace coarray
} // namespace dash


#endif /* DASH__COARRAY__COEVENTREF_H */

