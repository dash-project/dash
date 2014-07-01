#ifndef INIT_H_INCLUDED
#define INIT_H_INCLUDED

#include "dart.h"

//
// thin wrappers around the corresponding
// DART functions
//

namespace dash
{
  void init(int *argc, char ***argv);
  void finalize();

  int myid();
  int size();
};


#endif /* INIT_H_INCLUDED */
