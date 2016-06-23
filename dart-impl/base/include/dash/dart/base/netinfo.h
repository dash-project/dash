/**
 * \file dash/dart/base/netinfo.h
 */
#ifndef DART__BASE__NETINFO_H__
#define DART__BASE__NETINFO_H__

#include <dash/dart/if/dart_types.h>

/**
 * Resolves the current unit's network locality information.
 *
 * Network locality information is currently obtained from platform-dependent 
 * topology extraction tool i.e. topology file created on CRAY using xtprocadmin 
 * utility. In future, it can be extended to include the support of netloc library, 
 * however netloc currently only supports InfiniBand networks and OpenFlow-managed 
 * Ethernet networks.
 *
 * If a locality property cannot be resolved, the respective entry is set to
 * \c -1 or an empty string.
 */
dart_ret_t dart_netinfo(
  dart_netinfo_t * netinfo);

#endif /* DART__BASE__NETINFO_H__ */
