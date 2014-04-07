/** @file dart_adapt_communication_priv.h
 *  @date 25 Mar 2014
 *  @brief Definition of dart_handle_struct
 */
#ifndef DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED
#define DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include "dart_communication.h"

/** @brief Dart handle type for non-blocking one-sided operations.
 */
struct dart_handle_struct
{
	MPI_Request request;
};
#endif /* DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED */
