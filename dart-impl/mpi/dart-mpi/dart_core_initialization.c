#include <stdio.h>
#include "dart_initialization.h"

#include "adapt/dart_adapt_synchronization.h"
#include "adapt/dart_adapt_initialization.h"
#include "adapt/dart_adapt_globmem.h"
#include "adapt/dart_adapt_team_group.h"
#include "adapt/dart_adapt_communication.h"

dart_ret_t dart_init (int* argc, char*** argv)
{
           return dart_adapt_init (argc, argv);	
}

dart_ret_t dart_exit ()
{
           return  dart_adapt_exit ();     
}


