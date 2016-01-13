#ifndef DASH__OMP_MASTERSINGLE_H__INCLUDED
#define DASH__OMP_MASTERSINGLE_H__INCLUDED

#include <functional>

#include <dash/omp/Mutex.h>

namespace dash {
namespace omp {

/**
 * Execute a lamda expression only if the executing thread is the
 * master thread
 *
 * \param std::function<void(void)> A block of code that is only
 *                                  executed by the master thread
 */
void master(const std::function<void(void)>&f)
{
  if( dash::myid()==0 ) {
    f();
  }
}


class SingleImpl
{
private:
  Team&              _team;
  Mutex              _mutex;
  dash::Shared<bool> _flag; // indicates whether the f has been executed
  
public:
  SingleImpl(Team& team = Team::All()) :
    _team(team), 
    _flag(team), 
    _mutex(team)
  {
    _flag.set(false);
    _team.barrier();   // wait until the team has initialized the flag
  }
  
  void exec_wait(const std::function<void(void)>& f)
  {
    exec(f, true);
  }
  
  void exec_nowait(const std::function<void(void)>& f)
  {
    exec(f, false);
  }
  
  void exec(const std::function<void(void)>& f,
	    bool wait = false)
  {
    if (_mutex.try_lock()) {
      if (_flag.get() == false) {
	_flag.set(true);
	f();
      }
      _mutex.unlock();
    }
    if (wait) _team.barrier(); 
  }
  
  void clear() {
    _flag.set(false);
    _team.barrier();
  }
};


/**
 * Execute a block of code by the first process that arrives at this
 * statement. There is an implicit barrier at the end of the construct.
 */
void single(const std::function<void(void)>& f,
	    Team& team = Team::All())
{
  SingleImpl s(team);
  s.exec(f, true);
}

/**
 * Execute a block of code by the first process that arrives at this
 * statement.  There is no implicit barrier at the end of the
 * construct.
 */
void single_nowait(const std::function<void(void)>& f,
		   Team& team = Team::All())
{
  SingleImpl s(team);
  s.exec(f, false);
}


}  // namespace omp
}  // namespace dash

#endif // DASH__OMP_MASTERSINGLE_H__INCLUDED
