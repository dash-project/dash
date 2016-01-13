#ifndef DASH__OMP_MUTEX_H__INCLUDED
#define DASH__OMP_MUTEX_H__INCLUDED

#include <unordered_map>

#include <dash/dart/if/dart.h>
#include <dash/Team.h>

namespace dash {
namespace omp {

/**
 * A wrapper for DART mutual exclusion with support for named mutexes.
 */
class Mutex {
private:
  int           _id;
  Team&         _team;
  dart_lock_t*  _lock;

  static std::unordered_map<std::string, Mutex>  _mutexes;
  
public:
  Mutex(Team& team = Team::All()) : _team(team)
  {
    _lock = new dart_lock_t();
    dart_team_lock_init(_team.dart_id(), _lock);
  }
  
  Mutex(const Mutex& other) = delete;
  
  Mutex(Mutex&& other) : _id(other._id),
			 _team(other._team),
			 _lock(other._lock)
  {
    other._lock = nullptr;
  }
  
  ~Mutex() {
    //dart_team_lock_free(_team, _lock);
    delete _lock;
  }
  
  void lock() {
    dart_lock_acquire(*_lock);
  }
  
  bool try_lock() {
    int32_t res;
    dart_lock_try_acquire(*_lock, &res);
    return (res == 1);
  }
  
  void unlock() {
    dart_lock_release(*_lock);
  }
  
  void release() {
    unlock();
  }

  static Mutex& by_name( const std::string& name,
			Team& team=Team::All() )
  {
    auto found = _mutexes.find(name);
    if( found == _mutexes.end() ) {
      return std::get<0>(_mutexes.emplace(name, Mutex(team)))->second;
    } else {
      return found->second;
    }
  }
};


}  // namespace omp
}  // namespace dash

#endif // DASH__FUTURE_H__INCLUDED
