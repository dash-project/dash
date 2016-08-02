#ifndef DART_PMEM_H_INCLUDED
#define DART_PMEM_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>


#define DART_INTERFACE_ON

/**
 * \file dart_pmem.h
 *
 * Routines for allocation and reclamation of persistent memory regions
 * in global address space
 */

/**
 * \defgroup  DartPersistentMemory    Persistent memory semantics
 * \ingroup   DartInterface
 */


typedef struct dart_pmem_pool dart_pmem_pool_t;

#define DART_PMEM_FILE_CREATE (1 << 0)
#define DART_PMEM_FILE_OPEN   (1 << 1)

/* ======================================================================== *
 * Open and Close                                                           *
 * ======================================================================== */

dart_ret_t dart__pmem__init(void);
dart_ret_t dart__pmem__finalize(void);

/* ======================================================================== *
 * Open and Close                                                           *
 * ======================================================================== */

dart_pmem_pool_t * dart__pmem__open(
  dart_team_t         team,
  const char        * name,
  int                 flags,
  mode_t              mode);

dart_ret_t dart__pmem__close(
  dart_pmem_pool_t * pool);

/* ======================================================================== *
 * Persistent Memory Allocation                                             *
 * ======================================================================== */

dart_ret_t  dart__pmem__alloc(
  dart_pmem_pool_t    pool,
  size_t              nbytes,
  void    **    addr);

dart_ret_t  dart__pmem__free(
  dart_gptr_t    *    gptr);



//TODO: move to dart base
//
/* ======================================================================== *
 * Implementation Details                                                  *
 * ======================================================================== */

#include <libpmemobj.h>
#include "pmem_list.h"

#define DART_NVM_POOL_NAME (1024)
#define DART_PMEM_MIN_POOL PMEMOBJ_MIN_POOL

struct dart_pmem_pool {
  size_t          poolsize;
  dart_team_t     teamid;
  char const   *  path;
  char const   *  layout;
  PMEMobjpool  *  pop;
};

static struct dart_pmem_pool DART_PMEM_POOL_NULL = {0, DART_TEAM_NULL, NULL, NULL, NULL};

struct dart_pmem_slist_constr_args {
  char const * name;
};

struct dart_pmem_bucket_alloc_args {
  size_t element_size;
  size_t nelements;
};


POBJ_LAYOUT_BEGIN(dart_pmem_bucket_list);

//list_root
POBJ_LAYOUT_ROOT(dart_pmem_bucket_list, struct dart_pmem_bucket_list)
//list element
POBJ_LAYOUT_TOID(dart_pmem_bucket_list, struct dart_pmem_bucket)
//
POBJ_LAYOUT_END(dart_pmem_bucket_list)

#ifndef DART_PMEM_TYPES_OFFSET
#define DART_PMEM_TYPES_OFFSET 2183
#endif

//define type num for void elements
TOID_DECLARE(char, DART_PMEM_TYPES_OFFSET + 0);
#define TYPE_NUM_BYTE (TOID_TYPE_NUM(char))

#ifndef MAX_BUFFLEN
#define MAX_BUFFLEN 30
#endif

struct dart_pmem_bucket_list {
  char                name[MAX_BUFFLEN];
  //Head Node
  DART_PMEM_SLIST_HEAD(dart_pmem_list_head, struct dart_pmem_bucket) head;
};

struct dart_pmem_bucket {
  //Number of bytes for a single element
  size_t    element_size;
  //Number of elements in this bucket
  size_t    length;
  //Persistent Memory Buffer
  PMEMoid    data;
  //Pointer to next node
  DART_PMEM_SLIST_ENTRY(struct dart_pmem_bucket) next;
};

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_PMEM_H_INCLUDED */
