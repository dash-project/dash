#ifndef COEVENT_H_INCLUDED
#define COEVENT_H_INCLUDED

#include <dash/Exception.h>
#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/coarray/CoEventIter.h>
#include <dash/coarray/CoEventRef.h>

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
public:
  // Types
  using iterator       = coarray::CoEventIter;
  using const_iterator = coarray::CoEventIter;
  using reference      = coarray::CoEventRef;
  using size_type      = int;
  
public:
  
  /**
   * Constructor to setup and initialize an Coevent.
   */
  explicit Coevent(Team & team = dash::Team::All())
    : _team(&team) { }
  
  iterator begin() noexcept {
    return iterator(0);
  }
  
  const_iterator begin() const noexcept {
    return const_iterator(0);
  }
  
  iterator end() {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    return iterator(_team->size());
  }
  
  const_iterator end() const {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    return const_iterator(_team->size());
  }
  
  size_type size() const {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    return _team->size();
  }
  
  /**
   * wait for any incoming event
   */
  inline void wait() const {
    // TODO: Implement this
  }

  /**
   * initializes the Coevent. If it was already initialized in the Ctor,
   * the second initialization is skipped.
   */
  inline void initialize(Team & team) noexcept {
    _team = &team;
    _is_initialized = true;
  }
  
  inline Team & team() {
    return *_team;
  }

  /**
   * Operator to select event at given unit.
   */
  inline reference operator()(const int & unit) noexcept {
    DASH_ASSERT_MSG(dash::is_initialized(), "DASH is not initialized");
    return reference(unit);
  }

  /**
   * Operator to select event at given unit.
   */
  inline reference operator()(const team_unit_t & unit) noexcept {
    return this->operator()(static_cast<int>(unit));
  }
  
private:
  Team * _team;
  bool   _is_initialized = false;
};

} // namespace dash

#endif /* COEVENT_H_INCLUDED */
