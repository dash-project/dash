#ifndef DASH__OMP_CRITICAL_H__INCLUDED
#define DASH__OMP_CRITICAL_H__INCLUDED

#include <string>
#include <functional>

namespace dash {
namespace omp {

#define OMP_CRIT_DEFAULT_NAME \
  "__DASH_OMP_DEFAULT_CRITICAL_7858C868A30702BCA93480C31F"

void critical(const std::string& name,
	      const std::function<void(void)>& f);

void critical(const std::function<void(void)>& f);

} // namespace omp
} // namespace dash

#endif // DASH__OMP_CRITICAL_H__INCLUDED
