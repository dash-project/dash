/* 
 * dash-lib/Init.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef INIT_H_INCLUDED
#define INIT_H_INCLUDED

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

#endif /* INIT_H_INCLUDED */
