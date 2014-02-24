#include <stdio.h>
#include "dart_initialization.h"



dart_ret_t dart_init (int* argc, char*** argv)
{
           return dart_adapt_init (argc, argv);	
}

dart_ret_t dart_exit ()
{
           return  dart_adapt_exit ();     
}


