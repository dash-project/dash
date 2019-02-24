#ifndef DASH__COEVENT_H__INCLUDED
#define DASH__COEVENT_H__INCLUDED

#include <thread>

#include <dash/Array.h>
#include <dash/Atomic.h>
#include <dash/GlobPtr.h>
#include <dash/Exception.h>
#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/algorithm/Fill.h>

#include <dash/coarray/CoEventIter.h>
#include <dash/coarray/CoEventRef.h>

#include <dash/memory/MemorySpace.h>

namespace dash {

/**
 * \ingroup DashCoarrayConcept
 *
 *
 * A fortran style coevent.
 *
 * Coevent can be used for point-to-point synchronization. Events can be posted
 * to any image. Waiting on non-local events is not supported.
 *
 * \note Coevents might deadlock if multiple units are pinned to the same
 *       cpu-core. This is due to progress problems in MPI.
 *
 * Example:
 *
 * \code
 * Coevent events;
 *
 * events(2).post();
 * if(this_image() == 2){
 *  events.wait();
 * }
 * \endcode
 */
class Coevent {
private:
  using event_cnt_t   = dash::Atomic<int>;
  using array         = dash::Array<event_cnt_t>;
  using pointer       = typename array::pointer;
  using const_pointer = typename pointer::const_type;

public:
  // Types
  using iterator       = coarray::CoEventIter;
  using const_iterator = coarray::CoEventIter;
  using reference      = coarray::CoEventRef;
  using size_type      = int;

private:
   array _event_counts;
public:

  /**
   * Constructor to setup and initialize an Coevent.
   */
  explicit Coevent(Team & team = dash::Team::All())
    : _team(&team) {
      if(dash::is_initialized()){
        initialize(team);
      }
    }

  iterator begin() noexcept {
    return iterator(static_cast<pointer>(_event_counts.begin()));
  }

  const_iterator begin() const noexcept {
    // TODO FIXME
    //CoeventIter is not const correct, so we need a hack and unpack a nonconst
    //pointer from a const iterator to make the comiler happy.
    //return const_iterator(static_cast<const_pointer>(_event_counts.begin()));
    auto dart_ptr = _event_counts.begin().dart_gptr();
    pointer nonconst_ptr{dart_ptr};
    return const_iterator(nonconst_ptr);
  }

  iterator end() {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    return iterator(static_cast<pointer>(_event_counts.end()));
  }

  const_iterator end() const {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    // TODO FIXME
    //CoevenIter is not const correct, so we need a hack and unpack a nonconst
    //pointer from a const iterator to make the comiler happy.
    //return const_iterator(static_cast<const_pointer>(_event_counts.end()));
    auto dart_ptr = _event_counts.end().dart_gptr();
    pointer nonconst_ptr{dart_ptr};
    return const_iterator(nonconst_ptr);
  }

  size_type size() const {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    return _team->size();
  }

  /**
   * wait for a given number of incoming events.
   * This function is thread-safe
   */
  inline void wait(int count = 1) {
    auto gref = _event_counts.at(_team->myid().id);
    int current;
    do {
#ifdef DASH_DEBUG
      // avoid spamming the logs while busy waiting
      DASH_LOG_DEBUG("waiting for event at gptr",
                     static_cast<pointer>(_event_counts.begin()
                                         +_team->myid().id));
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
      current = gref.get();
    } while (current < count);
    // decrement the counter
    gref.sub(count);
  }

  inline int test() {
    DASH_LOG_DEBUG("test for events on this unit");
    return _event_counts.at(static_cast<int>(_team->myid())).load();
  }

  /**
   * initializes the Coevent. If it was already initialized in the Ctor,
   * the second initialization is skipped.
   */
  inline void initialize(Team & team = dash::Team::All()) {
    if(!_is_initialized){
      _team = &team;
      _event_counts.allocate(_team->size());
      dash::fill(_event_counts.begin(), _event_counts.end(), 0);
      _event_counts.barrier();
      _is_initialized = true;
    }
  }

  inline Team & team() {
    return *_team;
  }

  /**
   * Operator to select event at given unit.
   */
  inline reference operator()(const int & unit) DASH_ASSERT_NOEXCEPT {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    auto ptr = static_cast<pointer>(_event_counts.begin() + unit);
    return reference(ptr);
  }

  /**
   * Operator to select event at given unit.
   */
  inline reference operator()(const team_unit_t & unit) DASH_ASSERT_NOEXCEPT {
    return this->operator()(static_cast<int>(unit));
  }

private:
  Team * _team;
  bool   _is_initialized = false;
};

} // namespace dash

#endif /* DASH__COEVENT_H__INCLUDED */
