
#include <mpi.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/mutex.h>

#define DART_OP_HASH_SIZE 127

static struct dart_operation_struct * hashtab[DART_OP_HASH_SIZE];
static dart_mutex_t hash_mtx = DART_MUTEX_INITIALIZER;
static void register_op(struct dart_operation_struct *op);
static struct dart_operation_struct * get_op(MPI_Datatype mpi_type);
static void deregister_op(struct dart_operation_struct *op);

typedef void (*dart__mpi_min_max_reduce_t)(void*, void*, int*, MPI_Datatype*);

#define DART_NAME_MINMAX_OP(__name) dart__mpi_min_max_reduce_##__name

#define DART_DECLARE_MINMAX_OP(__name) \
DART_INLINE void DART_NAME_MINMAX_OP(__name)(            \
  void *lhs_, void *rhs_, int *len_, MPI_Datatype *dptr_)

#define DART_DEFINE_MINMAX_OP(__name, __type)                              \
DART_DECLARE_MINMAX_OP(__name)                                             \
{                                                                          \
  (void)(dptr_);                                                           \
  const __type *lhs = (__type *)lhs_;                                      \
  int           len = *len_;                                               \
  __type       *rhs = (__type *)rhs_;                                      \
  DART_ASSERT_MSG(                                                         \
      (len % 2) == 0, "DART_OP_MINMAX requires multiple of two elements"); \
  for (int i = 0; i < len; i += 2, rhs += 2, lhs += 2) {                   \
    if (rhs[DART_OP_MINMAX_MIN] > lhs[DART_OP_MINMAX_MIN])                 \
      rhs[DART_OP_MINMAX_MIN] = lhs[DART_OP_MINMAX_MIN];                   \
    if (rhs[DART_OP_MINMAX_MAX] < lhs[DART_OP_MINMAX_MAX])                 \
      rhs[DART_OP_MINMAX_MAX] = lhs[DART_OP_MINMAX_MAX];                   \
  }                                                                        \
}

DART_DEFINE_MINMAX_OP(byte,             char)
DART_DEFINE_MINMAX_OP(short,            short int)
DART_DEFINE_MINMAX_OP(int,              int)
DART_DEFINE_MINMAX_OP(unsigned,         unsigned int)
DART_DEFINE_MINMAX_OP(long,             long)
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

MPI_Op dart__mpi__op_minmax(dart_operation_t op, dart_datatype_t type)
{
  DART_ASSERT_MSG(op == DART_OP_MINMAX, "Unknown custom operation!");
  dart_datatype_t basetype = dart__mpi__datatype_base(type);
  return dart__mpi_minmax_reduce_ops[basetype];
}

dart_ret_t dart__mpi__op_init()
{
  memset(hashtab, 0, sizeof(hashtab));
  MPI_Op_create(&DART_NAME_MINMAX_OP(byte), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_BYTE]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(short), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_SHORT]);
  MPI_Op_create(&DART_NAME_MINMAX_OP(int), true,
                &dart__mpi_minmax_reduce_ops[DART_TYPE_INT]);
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
  DART_ASSERT(op < DART_OP_LAST);
  return dart_op_names[op];
}


dart_ret_t dart__mpi__op_fini()
{
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_BYTE]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_SHORT]);
  MPI_Op_free(&dart__mpi_minmax_reduce_ops[DART_TYPE_INT]);
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

static void dart__mpi__op_invoke_custom(
  void *lhs_, void *rhs_, int *len_, MPI_Datatype *dptr_)
{
  struct dart_operation_struct *dart_op = get_op(*dptr_);
  DART_LOG_TRACE("Invoking custom operation %p (op=%p, ud=%p)",
                 dart_op, dart_op->op, dart_op->user_data);
  // forward to the user operator
  dart_op->op(lhs_, rhs_, *len_, dart_op->user_data);
}

dart_ret_t
dart_op_create(
  dart_operator_t    op,
  void             * user_data,
  bool               commute,
  dart_datatype_t    dt,
  bool               dtype_is_tmp,
  dart_operation_t * new_op)
{
  MPI_Op mpi_op;
  if (dart__unlikely(new_op == NULL)) {
    DART_LOG_ERROR("Output pointer new_op may not be NULL!");
    return DART_ERR_INVAL;
  }

  *new_op = DART_OP_UNDEFINED;

  if (!dart__mpi__datatype_iscontiguous(dt)) {
    DART_LOG_ERROR("Custom operators only supported on contiguous datatypes!");
    return DART_ERR_INVAL;
  }
  dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dt);
  MPI_Datatype mpi_type = dts->contiguous.mpi_type;

  MPI_Datatype dup_mpi_type = mpi_type;
  if (!dtype_is_tmp) {
    MPI_Type_dup(mpi_type, &dup_mpi_type);
  }

  MPI_Op_create(dart__mpi__op_invoke_custom, commute, &mpi_op);

  struct dart_operation_struct *dart_op = malloc(sizeof(*dart_op));
  dart_op->mpi_op      = mpi_op;
  dart_op->mpi_type_op = dup_mpi_type;
  dart_op->mpi_type    = mpi_type;
  dart_op->op          = *op;
  dart_op->user_data   = user_data;

  DART_LOG_DEBUG(
    "Created custom operation %p (op=%p, ud=%p)",
    dart_op, dart_op->op, dart_op->user_data);

  register_op(dart_op);
  struct dart_operation_struct **new_op_ptr;
  new_op_ptr = (struct dart_operation_struct **)new_op;
  *new_op_ptr = dart_op;

  return DART_OK;
}

dart_ret_t
dart_op_destroy(dart_operation_t *op)
{
  struct dart_operation_struct **op_ptr = (struct dart_operation_struct **)op;
  struct dart_operation_struct *dart_op = *op_ptr;
  deregister_op(dart_op);
  if (dart_op->mpi_type_op != dart_op->mpi_type) {
    MPI_Type_free(&dart_op->mpi_type_op);
  }
  MPI_Op_free(&dart_op->mpi_op);
  free(dart_op);
  *op = DART_OP_UNDEFINED;

  return DART_OK;
}

/**************************************************************/
/** Operations hash table                                     */
/**************************************************************/

static inline int hash_mpi_dtype(MPI_Datatype mpi_type)
{
  return (((uintptr_t)mpi_type) % DART_OP_HASH_SIZE);
}

static void
register_op(struct dart_operation_struct *op)
{
  int slot = hash_mpi_dtype(op->mpi_type_op);
  dart__base__mutex_lock(&hash_mtx);
  op->next = hashtab[slot];
  hashtab[slot] = op;
  dart__base__mutex_unlock(&hash_mtx);
}

static struct dart_operation_struct * get_op(MPI_Datatype mpi_type)
{
  int slot = hash_mpi_dtype(mpi_type);
  dart__base__mutex_lock(&hash_mtx);
  struct dart_operation_struct *elem = hashtab[slot];

  while (elem != NULL) {
    if (elem->mpi_type_op == mpi_type) {
      break;
    }
    elem = elem->next;
  }

  if (elem == NULL) {
    DART_LOG_ERROR("Unknown MPI datatype for custom operation detected!");
  }
  dart__base__mutex_unlock(&hash_mtx);

  return elem;
}

static void deregister_op(struct dart_operation_struct *op)
{
  int slot = hash_mpi_dtype(op->mpi_type_op);
  dart__base__mutex_lock(&hash_mtx);
  struct dart_operation_struct *elem = hashtab[slot];
  struct dart_operation_struct *prev = NULL;
  do {

    if (elem == op) {
      if (prev != NULL) {
        prev->next = elem->next;
      } else {
        hashtab[slot] = elem->next;
      }
      break;
    }
    prev = elem;
    elem = elem->next;
  } while(elem != NULL);
  dart__base__mutex_unlock(&hash_mtx);
}
