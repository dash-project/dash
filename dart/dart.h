/*
 * dart.h - DASH RUNTIME (DART) INTERFACE 
 */

#ifndef DART_H_INCLUDED
#define DART_H_INCLUDED

/*
   --- DASH/DART Terminology ---
 
 DASH is a realization of the PGAS (partitioned global address
 space) programming model. Below is an attempt to define some 
 of the terminology used in the project. DART is the name of the 
 DASH runtime.
 
 DASH Units, Teams, and Groups
 -----------------------------
 The individual participants in a DASH program are called units. 
 One can think of a DASH unit like an MPI process or UPC thread.
 The generic term 'unit' is used to have the conceptual freedom to 
 later map a dash unit to a OS process, thread, or any other concept
 that might fit (for example, in the context of GPUs and accelerators). 
 
 Teams are ordered sets of units, identified by an integer ID.
 Each unit has a nonnegative zero-based integer ID in a given team, which
 remains unchanged throughtout the program execution.
 In each application there is a default team that contains all the units
 that comprise the program. Teams are identified by an interger ID.
 
 Groups are sets of units. The difference between groups and teams
 is that groups have local meaning only, while teams are coherent across 
 several units. In other words, group related operations are local, while 
 operations to manipulate teams are collective. 
 
 Local/Global/Private/Shared
 ---------------------------
 1) Local and Global 
 The terms local and global are adjectives to describe the address spaces
 in a DASH program. The local address space of a dash unit is managed 
 by the regular OS mechanisms (malloc, free), and data items in the
 local address space are addressed by regular pointers. The global 
 address space in a DASH program is a virtual abstraction. Each DASH
 unit contributes a part of it's memory to make up it's partition of the
 global address space. Data items in the global memory are addressed by 
 global pointers provided by the DART runtime. 
 
 2) Private and Shared 
 The adjectives private and shared describe the accessibility of 
 data items in DASH. A shared datum is one that can be accessed 
 by more than one unit (by means  of the DART runtime). 
 A private datum is one that is not shared.
 
 3) Partitions, Affinity, Ownership
 ... to be written... 
 idea: we might use the term affinity to express hierarchical locality
 
 4) Aligned and Symmetric
 Aligned and symmetric are terms describing memory allocations. A memory
 allocation is symmetric (with respect to a team), if the same amount 
 of memory (in bytes) is allocated by each member of the team. The 
 memory allocation is said to be aligned (with respect to a team), if 
 the same segment-id can be used in a global pointer to refer to any member's
 portion of the allocated memory. (See section on global pointers below on
 segment ids).
 
 An aligned and symmetric allocation has the nice property that any member of 
 the team is able to locally compute a global pointer to any location 
 in the allocated memory. 

 */ 


/*
   --- DART version number and build date ---
 */
extern unsigned int _dart_version;

#define DART_VERSION_NUMBER(maj_,min_,rev_)	\
  (((maj_<<24) | ((min_<<16) | (rev_))))
#define DART_VERSION_MAJOR     ((_dart_version>>24) &   0xFF)
#define DART_VERSION_MINOR     ((_dart_version>>16) &   0xFF)
#define DART_VERSION_REVISION  ((_dart_version)     & 0xFFFF)

#define DART_BUILD_DATE    (__DATE__ " " __TIME__)
#define DART_VERSION       DART_VERSION_NUMBER(0,0,1)

#include "dart_return_codes.h"

#include "dart_init.h"

#include "dart_group.h"

#include "dart_team.h"

#include "dart_gptr.h"

#include "dart_malloc.h"

#include "dart_communication.h"

#include "dart_locks.h"


#endif /* DART_H_INCLUDED */


