#ifndef DART__BASE__LOCALITY__INTERNAL__HWLOC_H_INCLUDED
#define DART__BASE__LOCALITY__INTERNAL__HWLOC_H_INCLUDED

#ifdef DART_ENABLE_HWLOC

#include <hwloc.h>
#include <hwloc/helper.h>

#if HWLOC_API_VERSION < 0x00011100
#  define DART__HWLOC_OBJ_NUMANODE HWLOC_OBJ_NODE
#else
#  define DART__HWLOC_OBJ_NUMANODE HWLOC_OBJ_NUMANODE
#endif

static inline dart_locality_scope_t dart__base__hwloc__obj_type_to_dart_scope(
  int hwloc_obj_type)
{
  switch (hwloc_obj_type) {
    case HWLOC_OBJ_MACHINE        : return DART_LOCALITY_SCOPE_NODE;
    case DART__HWLOC_OBJ_NUMANODE : return DART_LOCALITY_SCOPE_NUMA;
    case HWLOC_OBJ_CORE           : return DART_LOCALITY_SCOPE_CORE;
#if HWLOC_API_VERSION > 0x00011000
    case HWLOC_OBJ_PACKAGE        : return DART_LOCALITY_SCOPE_PACKAGE;
#else
    case HWLOC_OBJ_SOCKET         : return DART_LOCALITY_SCOPE_PACKAGE;
#endif
    case HWLOC_OBJ_PU             : return DART_LOCALITY_SCOPE_CPU;
#if HWLOC_API_VERSION < 0x00020000
    case HWLOC_OBJ_CACHE          : return DART_LOCALITY_SCOPE_CACHE;
#else
    case HWLOC_OBJ_L1CACHE        : /* fall-through */
    case HWLOC_OBJ_L2CACHE        : /* fall-through */
    case HWLOC_OBJ_L3CACHE        : return DART_LOCALITY_SCOPE_CACHE;
#endif
    case HWLOC_OBJ_PCI_DEVICE     : return DART_LOCALITY_SCOPE_MODULE;
    default : return DART_LOCALITY_SCOPE_UNDEFINED;
  }
}

#endif /* DART_ENABLE_HWLOC */

#endif /* DART__BASE__LOCALITY__INTERNAL__HWLOC_H_INCLUDED */
