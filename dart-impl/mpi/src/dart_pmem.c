/**
 * \file dart_pmem.c
 *
 * Implementation of dart persistent memory
 */
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>

#include <mpi.h>

#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_pmem.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

//define this function as extern since some compilers complain about implicit declaration
extern char * strdup(const char * s);

void * alloc_mem(size_t size)
{
  char * baseptr;
  DART_ASSERT_RETURNS(MPI_Alloc_mem(size, MPI_INFO_NULL, &baseptr), MPI_SUCCESS);
  return baseptr;
}

void free_mem(void * ptr)
{
  DART_ASSERT_RETURNS(MPI_Free_mem(ptr), MPI_SUCCESS);
}

dart_ret_t dart__pmem__init(void)
{
  pmemobj_set_funcs(alloc_mem, free_mem, NULL, NULL);
  return DART_OK;
}

int _dart_pmem_list_new(PMEMobjpool * pop,
                        TOID(struct dart_pmem_bucket_list) * list,
                        struct dart_pmem_list_constr_args * args)
{
  int ret = DART_OK;
  TX_BEGIN(pop) {
    *list = POBJ_ROOT(pop, struct dart_pmem_bucket_list);
    if (TOID_IS_NULL(*list)) {
      pmemobj_tx_abort(1);
    }

    DART_PMEM_TAILQ_INIT(&D_RW(*list)->head);
    TX_MEMCPY(D_RW(*list)->name, args->name, strlen(args->name) + 1);
    TX_SET(*list, size, 0);
  }
  TX_ONABORT {
    DART_LOG_ERROR("%s: transaction aborted: %s", __func__, pmemobj_errormsg());
    ret = DART_ERR_OTHER;
  } TX_END

  return ret;
}


static char * _tempname(const char * layout, int myid)
{
  char suffix[3];
  snprintf(suffix, sizeof(suffix), ".%d", myid);

  char * prefix = malloc(strlen(layout) + sizeof(suffix));
  DART_ASSERT(prefix);

  strcpy(prefix, layout);
  strcat(prefix, suffix);

  //char const * dir = '\0';

  //char * ret = tempnam(dir, prefix);
  //free(prefix);
  //return ret;
  return prefix;
}

dart_pmem_oid_t _dart_pmem_bucket_alloc(PMEMobjpool * pop,
                                        TOID(struct dart_pmem_bucket_list) * list,
                                        struct dart_pmem_bucket_alloc_args args)
{
  if (TOID_IS_NULL(*list)) {
    return DART_PMEM_OID_NULL;
  }

  dart_pmem_oid_t ret = DART_PMEM_OID_NULL;

  struct dart_pmem_list_head * head = &D_RW(*list)->head;

  TOID(struct dart_pmem_bucket) node;
  TX_BEGIN(pop) {
    node = TX_NEW(struct dart_pmem_bucket);
    if (TOID_IS_NULL(node)) {
      pmemobj_tx_abort(1);
    }
    D_RW(node)->element_size = args.element_size;
    D_RW(node)->length = args.nelements;
    D_RW(node)->data = pmemobj_tx_alloc(args.element_size * args.nelements,
                                        TYPE_NUM_BYTE);
    if (OID_IS_NULL(D_RO(node)->data)) {
      pmemobj_tx_abort(1);
    }
    DART_PMEM_TAILQ_INSERT_TAIL(head, node, next);

  }
  TX_ONCOMMIT {
    ret.oid = D_RW(node)->data;
    DART_LOG_DEBUG("%s: successfully allocated %zu bytes", __func__, args.nelements * args.element_size);
  }
  TX_ONABORT {
    DART_LOG_ERROR("%s: could not allocation persistent memory: %s", __func__, pmemobj_errormsg());
    //ret = NULL;
  } TX_END

  return ret;
}


#define DART_PMEM_ALL_FLAGS\
  (DART_PMEM_FILE_CREATE | DART_PMEM_FILE_OPEN)

dart_pmem_pool_t * dart__pmem__open(
  dart_team_t   team,
  const char  * name,
  int           flags,
  mode_t        mode)
{
  DART_ASSERT(name);

  if (flags & ~(DART_PMEM_ALL_FLAGS)) {
    DART_LOG_ERROR("invalid flag specified: %d", flags);
    return NULL;
  }

  if (DART_TEAM_NULL == team) {
    DART_LOG_ERROR("invalid team specified: %d", team);
    return NULL;
  }

  if (strlen(name) >= DART_NVM_POOL_NAME ) {
    DART_LOG_ERROR("invalid pool name: %s", name);
    return NULL;
  }

  int myid;
  DART_ASSERT_RETURNS(dart_team_myid(team, &myid), DART_OK);

  PMEMobjpool * pop;

  char * full_path = _tempname(name, myid);
  const size_t poolsize = DART_PMEM_MIN_POOL;

  if ((flags & DART_PMEM_FILE_CREATE) && access(full_path, F_OK) != 0) {


    if ((pop = pmemobj_create(full_path, name, poolsize,
                              mode)) == NULL) {
      DART_LOG_ERROR("dart__pmem__open: failed to create pmem pool (%s)", name);
      return NULL;
    }

    struct dart_pmem_list_constr_args args = {
      .name = name
    };

    TOID(struct dart_pmem_bucket_list) root;
    DART_ASSERT_RETURNS(_dart_pmem_list_new(pop, &root, &args), DART_OK);

    DART_LOG_DEBUG("dart__pmem__open: pool created (%s), poolsize (%zu)",
                   full_path, poolsize);
  } else {
    if ((pop = pmemobj_open(full_path, name)) == NULL) {
      DART_LOG_ERROR("dart__pmem_open: failed to open pmem pool(%s)", name);
      return NULL;
    }

    DART_ASSERT(pmemobj_root_size(pop));
    DART_LOG_DEBUG("dart__pmem__open: pool opened (%s)", full_path);
  }

  dart_pmem_pool_t * poolp = malloc(sizeof(dart_pmem_pool_t));
  poolp->poolsize = poolsize;
  poolp->path = full_path; //must be freed in pmem__pool__close
  poolp->layout = strdup(name);
  poolp->pop = pop;
  poolp->teamid = team;

  DART_LOG_DEBUG("dart__pmem__open >");
  return poolp;
}

dart_pmem_oid_t dart__pmem__alloc(
  dart_pmem_pool_t  const * pool,
  size_t              nbytes)
{
  DART_ASSERT(pool);

  if (NULL == pool->pop) {
    DART_LOG_ERROR("invalid pmem pool");
    return DART_PMEM_OID_NULL;
  }

  size_t root_size = pmemobj_root_size(pool->pop);

  if (root_size < sizeof(struct dart_pmem_bucket_list)) {
    //TODO: apply better consistency check
    DART_LOG_ERROR("improperly initialized pool");
    return DART_PMEM_OID_NULL;
  }

  TOID(struct dart_pmem_bucket_list) list = POBJ_ROOT(pool->pop,
      struct dart_pmem_bucket_list);

  struct dart_pmem_bucket_alloc_args args = {
    .element_size = sizeof(char),
    .nelements = nbytes,
  };


  dart_pmem_oid_t ret = _dart_pmem_bucket_alloc(pool->pop, &list, args);

  return ret;
}

dart_ret_t dart__pmem__getaddr(
  dart_pmem_oid_t oid,
  void ** addr)
{
  *addr = pmemobj_direct(oid.oid);
  return DART_OK;
}

dart_ret_t dart__pmem__persist(
  dart_pmem_pool_t * pool,
  void * addr,
  size_t nbytes)
{
  if (NULL == pool->pop) {
    DART_LOG_ERROR("invalid pmem pool");
    return DART_ERR_INVAL;
  }

  pmemobj_persist(pool->pop, addr, nbytes);
  DART_LOG_DEBUG("%s >", __func__);
  return DART_OK;
}

dart_ret_t dart__pmem__close(
  dart_pmem_pool_t ** pool)
{
  DART_LOG_DEBUG("dart__pmem__close");
  pmemobj_close((*pool)->pop);
  free((char *) (*pool)->path);
  free((char *) (*pool)->layout);
  free(*pool);
  *pool = NULL;
  DART_LOG_DEBUG("dart__pmem__close >");

  return DART_OK;
}
