/* 
 * dash-lib/Init.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH_INIT_H_
#define DASH_INIT_H_

#include <dash/dart/if/dart.h>

/**
 * Thin wrappers around the corresponding DART functions
 */
namespace dash
{
  static int _myid = -1;

  void   init(int *argc, char ***argv);
  void   finalize();
  int    myid();
  size_t size();
  void   barrier();
}

#endif // DASH_INIT_H_
