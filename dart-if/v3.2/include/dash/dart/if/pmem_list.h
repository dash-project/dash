#ifndef DART__PMEM_LIST_H__INCLUDED
#define DART__PMEM_LIST_H__INCLUDED

#include <libpmemobj.h>
/*
 *  * Singly-linked List definitions.
 *   */
#define DART_PMEM_SLIST_HEAD(name, type)\
  struct name {\
      TOID(type) pe_first;\
  }

#define DART_PMEM_SLIST_ENTRY(type)\
  struct {\
      TOID(type) pe_next;\
  }

/*
 *  * Singly-linked List access methods.
 *   */
#define DART_PMEM_SLIST_EMPTY(head)  (TOID_IS_NULL((head)->pe_first))
#define DART_PMEM_SLIST_FIRST(head)  ((head)->pe_first)
#define DART_PMEM_SLIST_NEXT(elm, field) (D_RO(elm)->field.pe_next)

/*
 *  * Singly-linked List functions.
 *   */
#define DART_PMEM_SLIST_INIT(head) do {\
    TX_ADD_DIRECT(&(head)->pe_first);\
    TOID_ASSIGN((head)->pe_first, OID_NULL);\
} while (0)

#define DART_PMEM_SLIST_INSERT_HEAD(head, elm, field) do {\
    TOID_TYPEOF(elm) *elm_ptr = D_RW(elm);\
    TX_ADD_DIRECT(&elm_ptr->field.pe_next);\
    elm_ptr->field.pe_next = (head)->pe_first;\
    TX_SET_DIRECT(head, pe_first, elm);\
} while (0)

#define DART_PMEM_SLIST_INSERT_AFTER(slistelm, elm, field) do {\
    TOID_TYPEOF(slistelm) *slistelm_ptr = D_RW(slistelm);\
    TOID_TYPEOF(elm) *elm_ptr = D_RW(elm);\
    TX_ADD_DIRECT(&elm_ptr->field.pe_next);\
    elm_ptr->field.pe_next = slistelm_ptr->field.pe_next;\
    TX_ADD_DIRECT(&slistelm_ptr->field.pe_next);\
    slistelm_ptr->field.pe_next = elm;\
} while (0)

#define DART_PMEM_SLIST_REMOVE_HEAD(head, field) do {\
    TX_ADD_DIRECT(&(head)->pe_first);\
    (head)->pe_first = D_RO((head)->pe_first)->field.pe_next;\
} while (0)

#define DART_PMEM_SLIST_REMOVE(head, elm, field) do {\
  if (TOID_EQUALS((head)->pe_first, elm)) {\
    DART_PMEM_SLIST_REMOVE_HEAD(head, field);\
  } else {\
    TOID_TYPEOF(elm) *curelm_ptr = D_RW((head)->pe_first);\
    while (!TOID_EQUALS(curelm_ptr->field.pe_next, elm))\
      curelm_ptr = D_RW(curelm_ptr->field.pe_next);\
    TX_ADD_DIRECT(&curelm_ptr->field.pe_next);\
    curelm_ptr->field.pe_next = D_RO(elm)->field.pe_next;\
  }\
} while (0)

#define DART_PMEM_SLIST_REMOVE_FREE(head, elm, field) do {\
    DART_PMEM_SLIST_REMOVE(head, elm, field);\
    TX_FREE(elm);\
} while (0)

#define DART_PMEM_SLIST_FOREACH(var, head, field)\
    for ((var) = DART_PMEM_SLIST_FIRST(head);\
      !TOID_IS_NULL(var);\
      var = DART_PMEM_SLIST_NEXT(var, field))

#endif
