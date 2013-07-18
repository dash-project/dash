
#include "dart_onesided.h"

/* 
   let my_existing_handle stand for an existing 'handle' type in some
   communication infrastructure, like MPI_Request in MPI
*/
struct my_existing_handle
{
  int foo; 
  double bar;
};

struct dart_handle_struct
{
  struct my_existing_handle h;
};

