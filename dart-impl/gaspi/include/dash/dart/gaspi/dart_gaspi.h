#ifndef DART_GASPI_H
#define DART_GASPI_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <GASPI.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/gaspi/dart_seg_stack.h>
#include <dash/dart/gaspi/gaspi_utils.h>

#define DART_MAX_TEAM_NUMBER (256)
#define DART_INTERFACE_ON
#define DART_GASPI_BUFFER_SIZE 1 << 16 //2^(sizeof(gaspi_rank_t) * 8) * sizeof(int32_t)

extern gaspi_rank_t dart_gaspi_rank_num;
extern gaspi_rank_t dart_gaspi_rank;
extern const gaspi_segment_id_t dart_gaspi_buffer_id;
extern const gaspi_segment_id_t dart_onesided_seg;

#define PUT_COMPLETION_VALUE 255
extern const gaspi_segment_id_t put_completion_src_seg;
extern const gaspi_segment_id_t put_completion_dst_seg;
extern char put_completion_dst_storage;

extern gaspi_pointer_t dart_gaspi_buffer_ptr;
extern const gaspi_segment_id_t dart_mempool_seg_localalloc;
extern const gaspi_segment_id_t dart_coll_seg;
extern bool dart_fallback_seg_is_allocated;

extern seg_stack_t pool_gaspi_seg_ids;
extern seg_stack_t pool_dart_seg_ids;

#define DART_CHECK_DATA_TYPE(dart_datatype_1, dart_datatype_2)                                   \
  do{                                                                                            \
    if(dart_datatype_1 != dart_datatype_2) {                                                     \
      DART_LOG_ERROR("Types for dst and src have to be same. No type conversion is performed!"); \
      return DART_ERR_INVAL;                                                                     \
    }                                                                                            \
  }while(0)

#define DART_CHECK_ERROR_GOTO_TEMPL(expected, error_code, ret_type, label, func...)          \
  do {                                                                                       \
    const ret_type retval = func;                                                            \
    if (retval != expected) {                                                                \
      printf("ERROR in %s : %s on line %i return value %i\n", #func,                   \
                   __FILE__, __LINE__, retval);                                              \
      goto label;                                                                            \
    }                                                                                        \
  }while (0)

#define DART_CHECK_ERROR_GOTO(label, func...) \
    DART_CHECK_ERROR_GOTO_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, label, func)

#define DART_CHECK_GASPI_ERROR_GOTO(label, func...) \
    DART_CHECK_ERROR_GOTO_TEMPL(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, label, func)

#define DART_CHECK_ERROR_TEMPL(expected, error_code, ret_type, func...)          \
  do {                                                                           \
    const ret_type retval = func;                                                \
    if (retval != expected) {                                                    \
      printf("ERROR in %s : %s on line %i return value %i\n", #func,       \
                   __FILE__, __LINE__, retval);                                  \
      return error_code;                                                         \
    }                                                                            \
  }while (0)

#define DART_CHECK_ERROR(func...) DART_CHECK_ERROR_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, func)

#define DART_CHECK_GASPI_ERROR(func...) DART_CHECK_ERROR_TEMPL(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, func)

#define DART_CHECK_ERROR_RET(ret, func...)                                       \
    do {                                                                         \
      ret = func;                                                                \
      if (ret != DART_OK) {                                                \
        printf("ERROR in %s : %s on line %i return value %i\n", #func,     \
                     __FILE__, __LINE__, ret);                                   \
      }                                                                          \
    }while (0)

#define DART_CHECK_GASPI_ERROR_RET(ret, func...)                                       \
    do {                                                                         \
      ret = func;                                                                \
      if (ret != GASPI_SUCCESS) {                                                \
        printf("ERROR in %s : %s on line %i return value %i\n", #func,     \
                     __FILE__, __LINE__, ret);                                   \
      }                                                                          \
    }while (0)

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_GASPI_H */
