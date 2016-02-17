#ifndef DART__MPI__DART_MPI_UTIL_H__
#define DART__MPI__DART_MPI_UTIL_H__

#define DART__MPI__ERROR_STR(mpi_error) ( \
      (mpi_error == MPI_SUCCESS)     ? "MPI_SUCCESS" \
    : (mpi_error == MPI_ERR_ARG)     ? "MPI_ERR_ARG" \
    : (mpi_error == MPI_ERR_COMM)    ? "MPI_ERR_COMM" \
    : (mpi_error == MPI_ERR_INFO)    ? "MPI_ERR_INFO" \
    : (mpi_error == MPI_ERR_SIZE)    ? "MPI_ERR_SIZE" \
    : (mpi_error == MPI_ERR_OTHER)   ? "MPI_ERR_OTHER" \
    : (mpi_error == MPI_ERR_PENDING) ? "MPI_ERR_PENDING" \
    : "other" \
    )

#endif /* DART__MPI__DART_MPI_UTIL_H__ */
