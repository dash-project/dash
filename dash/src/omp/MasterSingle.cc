
#include <string>
#include <functional>

#include <dash/omp/MasterSingle.h>
#include <dash/omp/Critical.h>
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

/**
 * Execute a block of code by the first process that arrives at this
 * statement. There is an implicit barrier at the end of the construct.
 */
void single(const std::function<void(void)>& f, Team& team)
{
  SingleImpl s(team);
  s.exec(f, true);
}

/**
 * Execute a block of code by the first process that arrives at this
 * statement.  There is no implicit barrier at the end of the
 * construct.
 */
void single_nowait(const std::function<void(void)>& f, Team& team)
{
  SingleImpl s(team);
  s.exec(f, false);
}


}  // namespace omp
}  // namespace dash
