/*
 * dart.h - DASH RUNTIME (DART) INTERFACE 
 */

#ifndef DART_H_INCLUDED
#define DART_H_INCLUDED

/* xxx some way to specify a version for the runtime*/
#define DART_VERSION xxx 

/* DART return codes */
#define DART_OK          0    // no error 
#define DART_ERR_XXX     1    // suitable list of error codes
#define DART_ERR_OTHER   99


/* 
 * --- DASH/DART Terminology ---
 */

// xxx define what local, global, private, shared means
// xxx partition
// xxx affinity -- fraglich ob sinnvoll, wenn ja was bedeutet es genau
// affinity schächerer Begriff? mehr oder weniger affinität, hierarchisch...
// xxx define unit/process define team


/*
 * --- DART initialization and finalization ---
 */

int dart_init(int *argc, char ***argv);
void dart_exit(int exitcode);
void dart_abort()???;

/*
  similar to MPI init/finalize 
  dart_init prepares the communication environment, 
  dart_finalize shuts the communication environment down
  init and finalize are both collective calls 
  MPI mandates that an MPI application has to have exactly 
  one pair of init and finalize, DART should mandate the same
  
  list of calls that may be called outside the init/finalize pair:
  -
  
  list of calls that may be called inside init/finalize but outside 
  attach/detach:
  -
  
*/

/* 
 * DART global address space - fraglich
 */
int dart_attach(some spec for the size of the global address space)));
int dart_detach()???;

/* should we support multiple calls to attach/detach? */
   

/*
 * DART unit and team management 
 */

int dart_myid();    // return the Unit ID of the caller
// or dart_myuid() or dart my_unit(), 

int dart_myteam();   // return the ID of the current team;
int dart_teamsize(); // return the number of Units in the current team

int dart_teamsize(int team); // return the size of the specified team
// 0 is the team ID of the default team (all units that exist,
// equivalent to MPI_Comm_world)

// xxx define datatypes for unit and team IDs? 
// dart_uid_t dart_tid_t; dart_unitid_t; 


// xxx functions to manipulate a team hierarchy 
// create sub-teams arranged in a hierarchy,
// navigate the hierarchy up and down


/*
 * --- DART memory allocation ---
 */

// define a global pointer type that can point anywhere in the 
// global address space, composed of unit id and virtual address or offset
typedef /* struct something xxx */ gptr_t; // global ptr type

//
// some functionality for manipulating the global pointers
// since these might be called frequently it's best to implement them
// as macros so the compiler can optimize things away and we don't
// pay function call overheads
 
// returns the unit id of a global pointer
DART_UNITOF();    

// returns the virtual address of a global pointer if local 0 otherwise
DART_ADDRESSOF(); 

// 1 if poitner is local, 0 otherwise
DART_PTR_IS_LOCAL();

// test if null pointer
DART_PTR_IS_NULL();


// memory allocation and de-allocation

gptr_t dart_alloc(size_t nbytes);
// like upc_alloc: allocate nbytes in the shared memory part of the 
// local unit, return a global poiner to it
// not a collective call 

gptr_t dart_team_alloc(int team, size_t nbytes) -

// collective call: all members of the specified team have to call the function
// xxx: what does nbytes specify exactly? total amount or local amount?
// let's try this with different parameters on each unit
// xxx: what does this call return? 

gptr_t dart_team_symmetric_alloc(int team, size_t nbytes) -
// oder als parameter fuer obige funktion

free varianten

/*
 * --- DART collective communication ---
 */

// It will be useful to include some set of collective communication
// and synchronization calls in the runtime such that for example when
// developing or using the DASH template library we don't have to step
// outside the DASH to do simple things such as barriers.

int dart_barrier(int team);
// collective and synchronizing call on all members
// of the specified team

int dart_bcast(int root, int team, void* data, int size); 

// einfache fälle von gather, scatter!

// Paarweise synchronisation von Prozessen?!

typedef /* ... */ dart_lock_t; // shared per default
 // content: global pointer, status

int init_lock( 
destroy_lock...

dash_acquire_lock( dart_lock_t lock );
// blockierende funktion, 
// 

// --> look at UPC locks, might be exactly what we need


/*
 * --- DART onesided communication ---
 */

void dart_get(void *dest, gptr_t ptr, size_t nbytes);
void dart_put(gptr_t ptr, void *src, size_t nbytes);

dart_handle_t dart_get_nb(void *dest, gptr_t ptr, size_t nbytes);
dart_handle_t dart_put_nb(gptr_t ptr, void *src, size_t nbytes);

wait on handle -- like GASNEt
test for completion of handle  -- like GASNET

//bulk needed (like GASNET)?
// more complex specification of data to transmit? -- strides, vectors, ... like global arrays






#endif /* DART_H_INCLUDED */

