
#include <string>
#include <functional>

#include <dash/omp/Critical.h>
#include <dash/omp/Mutex.h>

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
