#include <stdio.h>
#include "dart_types.h"
#include "dart_globmem.h"

#include "adapt/dart_adapt_synchronization.h"
#include "adapt/dart_adapt_initialization.h"
#include "adapt/dart_adapt_globmem.h"
#include "adapt/dart_adapt_team_group.h"
#include "adapt/dart_adapt_communication.h"
dart_ret_t dart_gptr_getaddr (dart_gptr_t gptr, void* addr)
{
           return	dart_adapt_gptr_getaddr (gptr, addr);
}

dart_ret_t dart_gptr_setaddr (dart_gptr_t gptr, void* addr)
{
	       return dart_adapt_gptr_setaddr (gptr, addr);
}

dart_ret_t dart_gptr_setunit (dart_gptr_t gptr, dart_unit_t unit_id)
{
	       return dart_adapt_gptr_setunit (gptr, unit_id);
}

dart_ret_t dart_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
	       return dart_adapt_memalloc (nbytes, gptr);
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{	
	       return dart_adapt_memfree (gptr);
}

dart_ret_t dart_team_memalloc_aligned (dart_team_t teamid, size_t nbytes, dart_gptr_t *gptr)
{
 	       return dart_adapt_team_memalloc_aligned (teamid, nbytes, gptr);
}

dart_ret_t dart_team_memfree (dart_team_t teamid, dart_gptr_t gptr)
{		
	       return dart_adapt_team_memfree (teamid, gptr);
}



