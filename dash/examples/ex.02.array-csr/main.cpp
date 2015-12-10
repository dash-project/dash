/* 
 * \file examples/ex.02.array-copy/main.cpp 
 *
 * author(s): Tobias Fuchs, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <cstddef>

#define DASH_ENABLE_LOGGING

#include <libdash.h>

using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  typedef dash::CSRPattern<> pattern_t;
  typedef size_t             value_t;
  typedef int                index_t;

  dash::init(&argc, &argv);

  char   buf[100];
  pid_t  pid            = getpid();
  auto   myid           = dash::myid();
  size_t num_units      = dash::size();
  size_t max_local_size = 100;

  gethostname(buf, 100);

  DASH_LOG_DEBUG("Host:"   << buf << 
                 " PID:"   << pid << 
                 " Units:" << num_units);

  std::vector<size_t> local_sizes;
  for (auto unit_idx = 0; unit_idx < num_units; ++unit_idx) {
    local_sizes.push_back(((unit_idx + 11) * 23) % max_local_size);
  }
  pattern_t pattern(local_sizes);

  if (myid == 0) {
    DASH_LOG_DEBUG("Pattern size:     " << pattern.size());
    DASH_LOG_DEBUG("Block sizes:      " << local_sizes);
  }
  DASH_LOG_DEBUG("Local size:       " << pattern.local_size());

  dash::Array<int> arr(pattern);

  if (myid == 0) {
    DASH_LOG_DEBUG("Array size:      " << array.size());
  }
  DASH_LOG_DEBUG("Array local size:   " << array.lsize());

  dash::finalize();

  return 0;
}

