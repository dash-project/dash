
#include <unordered_map>
#include <dash/omp/Mutex.h>

namespace dash {
namespace omp {

std::unordered_map<std::string, Mutex> Mutex::_mutexes;

} // namespace omp
} // namespace dash
