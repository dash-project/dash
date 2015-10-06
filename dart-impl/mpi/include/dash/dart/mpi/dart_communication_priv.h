/** @file dart_communication_priv.h
 *  @date 25 Aug 2014
 *  @brief Definition of dart_handle_struct
 */
#ifndef DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED
#define DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_communication.h>

/** @brief Dart handle type for non-blocking one-sided operations.
 */
struct dart_handle_struct
{
	MPI_Request request;
	MPI_Win	    win;
	dart_unit_t dest;
};

MPI_Op dart_mpi_op(dart_operation_t dart_op) {
  switch(dart_op) {
  case DART_OP_MIN:  return MPI_MIN;
  case DART_OP_MAX:  return MPI_MAX;
  case DART_OP_SUM:  return MPI_SUM;
  case DART_OP_PROD: return MPI_PROD;
  case DART_OP_BAND: return MPI_BAND;
  case DART_OP_LAND: return MPI_LAND;
  case DART_OP_BOR:  return MPI_BOR;
  case DART_OP_LOR:  return MPI_LOR;
  case DART_OP_BXOR: return MPI_BXOR;
  case DART_OP_LXOR: return MPI_LXOR;
  }
}

#endif /* DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED */
