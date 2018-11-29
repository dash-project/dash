#ifndef DART_BASE_STACK_H_
#define DART_BASE_STACK_H_

#include <stdbool.h>
#include <stdatomic.h>

#include <dash/dart/if/dart_types.h>

/**
 * Implementation of an intrusive stack, i.e.,
 * a stack whose elements contain the required
 * links to avoid additional objects containing
 * pointers to data elements.
 *
 * To provide a generic interface, your type that
 * should be stored in the stack has to contain the
 * links itself. This is done by including the
 * dart_stack_elem_t in your struct, as shown below:
 *
 * struct datatype {
 *   dart_stack_elem_t se;
 *   int counter;
 *   // more datatypes to come
 * };
 *
 * Note that the first sizeof(void*) bytes of struct datatype
 * can be freely used as long as the element is not
 * on the stack but will be overwritten on a call to
 * dart__base__stack_push, i.e., as in
 *
 * struct datatype *data = malloc(sizeof(struct datatype));
 * data.counter = 0;
 * // this call will overwrite the first sizeof(void*)
 * // bytes of data
 * dart__base__stack_push(stack, &(data->se));
 *
 * Note that dart__base__stack_push does not copy any data
 * so objects on the stack have to be used carefully here.
 *
 * A pop from the stack requires a cast:
 * struct datatype *sd = (struct datatype*)dart__base__stack_pop(stack);
 *
 * However, given that dart_stack_elem_t is the first element, this
 * can be done safely.
 *
 */

#define USE_ATOMIC128_CAS

/* Forward declaration */
struct dart_stack;
typedef struct dart_stack dart_stack_t;

typedef struct dart_stack_elem {
  struct dart_stack_elem *next;
} dart_stack_elem_t;

#ifdef USE_ATOMIC128_CAS

#include <stdalign.h>

struct dart_stack_head {
  alignas(16)
  uintptr_t          aba;
  dart_stack_elem_t *node;
};
#define DART_STACK_HEAD_INITIALIZER {.aba = 0, .node = NULL}
#else

struct dart_stack_head {
  dart_stack_elem_t *node;
  dart_mutex_t       mtx;
};
#define DART_STACK_HEAD_INITIALIZER {.node = NULL, .mtx = DART_MUTEX_INITIALIZER}
#endif

struct dart_stack {
  struct dart_stack_head head;
};


#define DART_CONTAINER_OF(__ptr, __type, __member) \
    ((__type *)((char *)(1 ? (__ptr) : &((__type *)0)->__member) - offsetof(__type, __member)))

/**
 * Define a member that is later used to add the object to the stack
 */
#define DART_STACK_MEMBER_DEF dart_stack_elem_t __se
#define DART_STACK_MEMBER_GET(__ptr) ((__ptr)->__se)
#define DART_STACK_CONTAINER_GET(__ptr, __type) DART_CONTAINER_OF(__ptr, __type, __se)

#define DART_STACK_INITIALIZER {.head = DART_STACK_HEAD_INITIALIZER}


static inline
dart_ret_t
dart__base__stack_init(dart_stack_t *st)
{
  static struct dart_stack_head init = DART_STACK_HEAD_INITIALIZER;
  st->head = init;
  return DART_OK;
}

#ifdef USE_ATOMIC128_CAS
static inline
dart_stack_elem_t *
pop_cas(struct dart_stack_head *sh)
{
  struct dart_stack_head next, orig = atomic_load(sh);
  do {
    if (orig.node == NULL) {
      break;
    }
    next.aba  = orig.aba + 1;
    next.node = orig.node->next;
  } while (!atomic_compare_exchange_weak(sh, &orig, next));

  return orig.node;
}

static inline
void
push_cas(struct dart_stack_head *sh, dart_stack_elem_t *elem)
{
  struct dart_stack_head next, orig = atomic_load(sh);

  do {
    elem->next = orig.node;
    next.aba   = orig.aba + 1;
    next.node  = elem;
  } while (!atomic_compare_exchange_weak(sh, &orig, next));
}

#else

#define PUSH(_head, _elem) \
  do {                                \
    _elem->next = _head;           \
    _head = _elem;                    \
  } while (0)

#define POP(_head, _elem) \
  do {                               \
    _elem = _head;                   \
    if (_elem != NULL) {             \
      _head = _elem->next;        \
      _elem->next = NULL;         \
    }                                \
  } while (0)

static inline
dart_stack_elem_t *
pop_cas(struct dart_stack_head *sh)
{
  dart__base__mutex_lock(&sh->mtx);
  dart_stack_elem_t *elem;
  POP(sh->node, elem);
  dart__base__mutex_unlock(&sh->mtx);
  return elem;
}

static inline
void
push_cas(struct dart_stack_head *sh, dart_stack_elem_t *elem)
{
  dart__base__mutex_lock(&sh->mtx);
  PUSH(sh->node, elem);
  dart__base__mutex_unlock(&sh->mtx);
}


#endif // USE_ATOMIC128_CAS

static inline
dart_ret_t
dart__base__stack_push(dart_stack_t *st, dart_stack_elem_t *elem)
{
  if (elem != NULL) {
    push_cas(&st->head, elem);
  }

  return DART_OK;
}

static inline
dart_stack_elem_t *
dart__base__stack_pop(dart_stack_t *st)
{
  dart_stack_elem_t *elem;
  elem = pop_cas(&st->head);
  return elem;
}

static inline
bool
dart__base__stack_empty(dart_stack_t *st)
{
  return atomic_load(&st->head.node) == NULL;
}

static inline
dart_ret_t
dart__base__stack_finalize(dart_stack_t *st)
{
  while(true) {
    dart_stack_elem_t *elem = pop_cas(&st->head);
    if (elem == NULL) {
      break;
    }
    free(elem);
  }

  return DART_OK;
}

#endif /* DART_BASE_STACK_H_ */
