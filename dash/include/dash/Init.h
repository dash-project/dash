#ifndef DASH__INIT_H_
#define DASH__INIT_H_

#include <dash/dart/if/dart.h>
#include <dash/Types.h>

namespace dash
{
  void   init(int *argc, char ***argv);
  void   init_thread(int *argc, char ***argv, int *concurrency);
  void   finalize();
  bool   is_initialized();
  global_unit_t    myid();
  ssize_t size();
  void   barrier();
}

#endif // DASH__INIT_H_
