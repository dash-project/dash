#ifndef DASH__OMP_CRITICAL_H__INCLUDED
#define DASH__OMP_CRITICAL_H__INCLUDED

#include <dash/omp/Mutex.h>

#define OMP_CRIT_DEFAULT_NAME \
  "__DASH_OMP_DEFAULT_CRITICAL_7858C868A30702BCA93480C31F"

namespace dash {
namespace omp {

/**
 * Implementation of a critical section. The lambda expressen passed
 * is only executed by one thread at a time.
 *
 * \param std::function<void(void)> A block of code that is only executed by
 *                                  one thread at a time
 */
void critical(const std::string& name,
	      const std::function<void(void)>& f)
{
  Mutex& m = Mutex::by_name(name);
  m.lock();
  f();
  m.unlock();
}


void critical(const std::function<void(void)>& f)
{
  critical(OMP_CRIT_DEFAULT_NAME, f);
}
  


}  // namespace omp
}  // namespace dash

#endif // DASH__FUTURE_H__INCLUDED
