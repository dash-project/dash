#ifndef DASH__MUTEX_H__INCLUDED
#define DASH__MUTEX_H__INCLUDED

#include <dash/Team.h>

namespace dash {

/**
 * Behaves similar to \c std::mutex and is used to ensure mutual exclusion
 * within a dash team.
 * 
 * \note This works properly with \c std::lock_guard
 * \note Mutex cannot be placed in DASH containers
 * 
 * \code
 * // just for demonstration, better use atomic operations
 * dash::Mutex mx; // mutex for dash::Team::All();
 * dash::Array<int> arr(10);
 * {
 *    std::lock_guard<dash::Mutex> lg(mx);
 *    int tmp = arr[0];
 *    arr[0] = tmp + 1;
 *    // TODO: this almost certainly requires a flush
 * }
 * dash::barrier();
 * // postcondition: arr[0] == dash::size();
 * \endcode
 */
class Mutex {
private:
  using self_t = Mutex;

public:
  /**
   * DASH Mutex is only valid for a dash team. If no team is passed, team all
   * is used.
   * 
   * This function is not thread-safe
   * @param team team for mutual exclusive accesses
   */
  explicit Mutex(Team & team = dash::Team::All());
  
  Mutex(const Mutex & other)               = delete;
  Mutex(Mutex && other)                    = default;

  self_t & operator=(const self_t & other) = delete;
  self_t & operator=(self_t && other)      = default;
  
  /**
   * Collective destructor to destruct a DART lock.
   * 
   * This function is not thread-safe
   */
  ~Mutex();
  
  /**
   * Block until the lock was acquired.
   */
  void lock();
  
  /**
   * Try to acquire the lock and return immediately.
   * @return True if lock was successfully aquired, False otherwise
   */
  bool try_lock();
  
  /**
   * Release the lock acquired through \c lock() or \c try_lock().
   */
  void unlock();
  
private:
  dart_lock_t   _mutex;
}; // class Mutex

} // namespace dash

#endif // DASH__MUTEX_H__INCLUDED

