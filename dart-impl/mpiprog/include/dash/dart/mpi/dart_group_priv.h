/** @file dart_group_priv.h
 *  @date 25 Mar 2014
 *  @brief Definition of dart_group_struct.
 */
#ifndef DART_ADAPT_GROUP_PRIV_H_INCLUDED
#define DART_ADAPT_GROUP_PRIV_H_INCLUDED

#include <mpi.h>

/** @brief Dart group type.
 */

struct dart_group_struct {
  MPI_Group mpi_group;
};

#endif /* DART_ADAPT_GROUP_PRIV_H_INCLUDED */
