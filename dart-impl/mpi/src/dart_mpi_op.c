
#include <mpi.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/base/assert.h>

typedef void (*dart__mpi_min_max_reduce_t)(void*, void*, int*, MPI_Datatype*);

#define DART_NAME_MINMAX_OP(__name) dart__mpi_min_max_reduce##__name

#define DART_DECLARE_MINMAX_OP(__name) \
DART_INLINE void DART_NAME_MINMAX_OP(__name)(            \
  void *lhs_, void *rhs_, int *len, MPI_Datatype *dptr)

#define DART_DEFINE_MINMAX_OP(__name, __type)               \
DART_DECLARE_MINMAX_OP(__name){                             \
  __type *lhs = (__type *)lhs_;                             \
  __type *rhs = (__type *)rhs_;                             \
  DART_ASSERT_MSG((*len%2) == 0,                            \
    "DART_OP_MINMAX requires multiple of two elements");    \
  for (int i = 0; i < *len; i += 2, rhs += 2, lhs += 2) {   \
    if (rhs[DART_OP_MINMAX_MIN] > lhs[DART_OP_MINMAX_MIN])  \
      rhs[DART_OP_MINMAX_MIN] = lhs[DART_OP_MINMAX_MIN];    \
    if (rhs[DART_OP_MINMAX_MAX] < lhs[DART_OP_MINMAX_MAX])  \
      rhs[DART_OP_MINMAX_MAX] = lhs[DART_OP_MINMAX_MAX];    \
  }                                                         \
}

DART_DEFINE_MINMAX_OP(byte,             char)
DART_DEFINE_MINMAX_OP(short,            short int)
DART_DEFINE_MINMAX_OP(unsigned,         unsigned int)
DART_DEFINE_MINMAX_OP(long,             int)
DART_DEFINE_MINMAX_OP(unsignedlong,     unsigned long)
DART_DEFINE_MINMAX_OP(longlong,         long long)
DART_DEFINE_MINMAX_OP(unsignedlonglong, unsigned long long)
DART_DEFINE_MINMAX_OP(float,            float)
DART_DEFINE_MINMAX_OP(double,           double)
DART_DEFINE_MINMAX_OP(longdouble,       long double)

static const char *dart_op_names[DART_OP_LAST] = {
  "DART_OP_UNDEFINED",
  "DART_OP_MIN",
  "DART_OP_MAX",
  "DART_OP_MINMAX",
  "DART_OP_SUM",
  "DART_OP_PROD",
  "DART_OP_BAND",
  "DART_OP_LAND",
  "DART_OP_BOR",
  "DART_OP_LOR",
  "DART_OP_BXOR",
  "DART_OP_LXOR",
  "DART_OP_REPLACE",
  "DART_OP_NO_OP"
};

static MPI_Op dart__mpi_minmax_reduce_ops[DART_TYPE_LAST];

MPI_Op dart__mpi__op_custom(dart_operation_t op, dart_datatype_t type)
{
  DART_ASSERT_MSG(op == DART_OP_MINMAX, "Unknown custom operation!");
  dart_datatype_t basetype = dart__mpi__datatype_base(type);
  return dart__mpi_minmax_reduce_ops[basetype];
}

dart_ret_t dart__mpi__op_init()
{
  MPI_Op_create(&DART_NAME_MINMAX_OP(byte), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_BYTE]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(short), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_SHORT]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(unsigned), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_UINT]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(long), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_LONG]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(unsignedlong), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_ULONG]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(longlong), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_LONGLONG]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(unsignedlonglong), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_ULONGLONG]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(float), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_FLOAT]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(double), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_DOUBLE]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(longdouble), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_LONG_DOUBLE]);

  return DART_OK;
}

const char* dart__mpi__op_name(dart_operation_t op)
{
  DART_ASSERT(op >= DART_OP_UNDEFINED && op < DART_OP_LAST);
  return dart_op_names[op];
}


dart_ret_t dart__mpi__op_fini()
{
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_BYTE]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_SHORT]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_UINT]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_LONG]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_ULONG]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_LONGLONG]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_ULONGLONG]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_FLOAT]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_DOUBLE]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_LONG_DOUBLE]);

  return DART_OK;
}
