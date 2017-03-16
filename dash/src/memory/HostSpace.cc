#include <dash/memory/HostSpace.h>


dash::default_memory_space * get_default_memory_space() {
  static dash::HostSpace memory_space_singleton;
  return &memory_space_singleton;
}