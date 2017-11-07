#ifndef COMUTEX_H_INCLUDED
#define COMUTEX_H_INCLUDED

#include <dash/Exception.h>
#include <dash/Team.h>
#include <dash/Mutex.h>

#include <vector>

namespace dash {

/**
 * \ingroup DashCoarrayConcept
 *
 *
 * A fortran style comutex.
 *
 * Comutex is used to ensure mutual exclusion on a certain image.
 * The interface is similar to \c dash::Coarray but does not allow
 * local accesses. Hence it does not fulfill the DASH Container Concept.
 *
 * \note In its current implementation the Comutex does not scale well as
 *       each unit stores internally one \dash::mutex per unit. Hence, use this
 *       only for small teams.
 *
 * Example:
 *
 * \code
 * Coarray<int> arr;
 * Comutex      comx;
 *
 * {
 *  // lock unit i
 *  std::lock_guard<dash::Mutex> lg(comx(i));
 *  // exclusively access data on unti i
 *  arr(i) = 42;
 * }
 * \endcode
 *
 * \TODO: Use custom mutex to avoid storing the team multiple times
 *        (Each \c dash::Mutex) contains the team itself.
 *
 *
 *
 */
class Comutex {
private:
  using _storage_type = std::vector<dash::Mutex>;
public:
  // Types
  using iterator       = typename _storage_type::iterator;
  using const_iterator = typename _storage_type::const_iterator;
  using reference      = typename _storage_type::reference;
  using size_type      = typename _storage_type::size_type;

public:

  /**
   * Constructor to setup and initialize an Comutex. If dash is not initialized,
   * use \cinitialize() after dash is initialized to initialize the Comutex
   */
  explicit Comutex(Team & team = dash::Team::All()) {
    if(dash::is_initialized()){
      initialize(team);
    }
  }

  iterator begin() noexcept {
    return _mutexes.begin();
  }

  const_iterator begin() const noexcept {
    return _mutexes.begin();
  }

  iterator end() noexcept {
    return _mutexes.end();
  }

  const_iterator end() const noexcept {
    return _mutexes.end();
  }

  size_type size() const noexcept {
    return _mutexes.size();
  }

  /**
   * initializeds the mutexes. If they were already initialized in the Ctor,
   * the second initialization is skipped.
   */
  inline void initialize(Team & team){
    if(!_is_initialized){
      _team = &team;
      _mutexes.reserve(team.size());
      for(decltype(team.size()) i = 0; i<team.size(); ++i){
        _mutexes.emplace_back(team);
      }
    } else {
      DASH_ASSERT_MSG((team == *_team),
                      "Comutex was initialized with a different team");
    }
    _is_initialized = true;
  }

  inline Team & team() {
    return *_team;
  }

  /**
   * Operator to select mutex at given unit.
   */
  inline reference operator()(const int & unit) {
    DASH_ASSERT_MSG(_is_initialized, "Comutex is not initialized");
    return _mutexes.at(unit);
  }

  /**
   * Operator to select mutex at given unit.
   */
  inline reference operator()(const team_unit_t & unit) {
    return this->operator()(static_cast<int>(unit));
  }

private:
  _storage_type  _mutexes;
  Team         * _team;
  bool           _is_initialized = false;
};

} // namespace dash

#endif /* COMUTEX_H_INCLUDED */
