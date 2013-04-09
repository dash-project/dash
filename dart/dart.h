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


/* 
   --- DART return codes ---
 */ 
#define DART_OK          0    /* no error */

/* TODO: expand the list of error codes as needed. 
   it might be a good idea to use negative integers as
   error codes
 */
#define DART_ERR_OTHER   -999


/*
   --- DART startup/shutdown ---
 */
 
int dart_init(int *argc, char ***argv);
void dart_exit(int exitcode);


/* 
   --- DART unit, group, and team management ---
 */

/* 
   DART groups are objects with local meaning only.
   they are essentially objects representing sets of units
   out of which later teams can be formed. The operations to 
   manipulate groups are local (and cheap). The operations to create
   teams are collective and can be expensive. 
   The 
 */

/* DART groups are represented by an opague type */
typedef struct dart_group_struct dart_group_t;

int dart_group_init(dart_group_t *group);
void dart_group_delete(dart_group_t *group);

int dart_group_union(dart_group_t *g1, dart_group_t *g2, dart_group_t *g3 );
int dart_group_intersect(dart_group_t *g1, dart_group_t *g2, dart_group_t *g3 );
/* more set functions etc. if needed */

int dart_group_ismember(dart_group_t *g, int unitids);

/* DART teams and units are represented by 
   integer IDs */

#define DART_TEAM_ALL      0
#define DART_TEAM_NULL       // ??? 

/* get the group associated with a team */
int dart_team_get_group(int teamid, dart_group_t *group);

/* create a team from the specified group */
/* this is a collective call, all members of team have to call the */
/* function with an equivalent specification of group */
// Q: is the team id globally unique??
int dart_team_create(int team, dart_group_t *group); 

int dart_team_destroy(int team);

/* return the unit id of the calling process */
int dart_team_myid(int teamid);
int dart_team_size(int teamid);

/* my id and size for default global team */
int dart_myid();
int dart_size();



/*
   --- DART global pointers ---

 There are multiple options for representing the global 
 pointer that come to mind:
 
 1) struct with pre-defined members (say, unit id 
    and local address)
 2) an opaque object that leaves the details to a specific
    implementation and is manipulated only through pointers
 3) a fixed size integer data type (say 64 bit or 128 bit),
    manipulated through c macros that packs all the 
    relevant information
 
 There are pros and cons to each option...
 
 Another question is that of offsets vs. addresses: Either 
 a local virtual address is directly included and one in which 
 the pointer holds something like a segment ID and an offset
 within that segment. 
 
 If we want to support virtual addresses then 64 bits is not enough
 to represent the pointer. If we only support segment offsets, 64
 bit could be sufficient
 
 Yet another question is what kind of operations are supported on
 global pointers. For example UPC global pointers keep "phase" 
 information that allows pointer arithmetic (the phase is needed for 
 knowing when you have to move to the next node). 
 
 PROPOSAL: Don't include phase information with pointers on the DART
 level, but don't preclude support the same concept on the DASH level.
  
*/

/*
 PROPOSAL: use 128 bit global pointers with the following layout:


 0         1         2         3         4         5         6
 0123456789012345678901234567890123456789012345678901234567890123
 |------<32 bit unit id>--------|-<segment id>--|--flags/resv---|
 |-----------<either a virtual address or an offset>------------|

*/

typedef struct 
{
  int      unitid;
  short    segid;
  unsigned short flags;
  union {
    unsigned long long offset;
    void *addr;
  };
} gptr_t;

/* return the unit-id of the pointer (that would be the 
   id within the default global team */
#define DART_UNITOF(ptr)

/* return the local virtual address of the pointer 
  if the global pointer is local and NULL otherwise */
#define DART_ADDRESSOF(ptr) 

/* return the segment id of the pointer */
#define DART_SEGMENTOF(ptr) 


/* test if nullpointer */
#define DART_ISNULL(ptr)

/* some initializer for pointers */
#define DART_NULLPTR 


/*
   --- DART memory allocation ---
 */

/* 
 dart_alloc() allocates nbytes of memory in the global address space
 of the calling unit and returns a global pointer to it.  This is not
 a collective function but a local function.  Probably trivial to
 implement on GASNet but difficult on MPI?
 */

gptr_t dart_alloc(size_t nbytes);

/* 
 dart_team_alloc_aligned() is a collective function on the specified
 team. Each team member calls the function and must request the same
 amount of memory (nbytes). The return value of this function on each
 unit in the team is a global pointer pointing to the beginning of the
 allocation. The returned memory allocation is symmetric and aligned
 allowing for an easy determination of global poitners to anywhere in
 the allocated memory block
 */

/* Question: can a unit that was not part of the allocation team
   later access the memory? */
gptr_t dart_alloc_aligned(int teamid, size_t nbytes);

/* collective call to free the allocated memory */
void dart_free(int teamid, gptr_t ptr);



/*
   --- DART collective communication/synchronization ---
 
 It will be useful to include some set of collective communication and
 synchronization calls in the runtime such that for example when
 developing or using the DASH template library we don't have to step
 outside the DASH to do simple things such as barriers.xx  */

/* 
   barrier of all members of the team, usual semantics as in MPI 
   we could think about including a split-phase barrier as well... 
 */
int dart_barrier(int team);

/* broadcast of data from one team member to all others 
   semantics are like in MPI, but the broadcast works on raw 
   bytes and not with datatypes 
 */
int dart_bcast(void* buf, size_t nbytes, int root, int team ); 

/* 
  gather and scatter with similar semantics as in MPI; 
  size specifies the message size between each pair of 
  communicating processes 
 */
int dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, int root, int team);
int dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, int root, int team);

/*
   --- DART pairwise synchronization ---
 */
 
/* DART locks: See redmine issue refs #6 */

typedef struct dart_opaque_lock_t* dart_lock;

// creates lock in local-shared address space -> do we need this -> how to pass locks between processes
int dart_lock_init(dart_lock* lock); 

// team_id may be DART_TEAM_ALL. Create lock at team member 0?
int dart_lock_team_init(int team_id, dart_lock* lock); 

// lock becomes DART_LOCK_NULL. may be called by any team member that initialized the lock?!
int dart_lock_free(dart_lock* lock); 

// blocking call
int dart_lock_acquire(dart_lock lock); 

// returns DART_LOCK_ACQUIRE_SUCCESS, DART_LOCK_ACQUIRE_FAILURE, or something like that
int dart_lock_try_acquire(dart_lock lock); 
int dart_lock_release(dart_lock lock);


/*
   --- DART onesided communication ---
 */

typedef struct {} dart_handle_t;

/* blocking versions of one-sided communication operations */
void dart_get(void *dest, gptr_t ptr, size_t nbytes);
void dart_put(gptr_t ptr, void *src, size_t nbytes);

/* non-blocking versions returning a handle */
dart_handle_t dart_get_nb(void *dest, gptr_t ptr, size_t nbytes);
dart_handle_t dart_put_nb(gptr_t ptr, void *src, size_t nbytes);

/* wait and test for the completion of a single handle */
int dart_wait(dart_handle_t handle);
int dart_test(dart_handle_t handle);

/* wait and test for the completion of multiple handles */
int dart_waitall(dart_handle_t *handle);
int dart_testall(dart_handle_t *handle);

/* TODO: 
  - Do we need bulk version of the above (like in Gasnet)
  - Do we need a way to specify the data to trasmit in 
    a more complex way - strides, offsets, etc. (like in Global Arrays)
 */
 




#endif /* DART_H_INCLUDED */


