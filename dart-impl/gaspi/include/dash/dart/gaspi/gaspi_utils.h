#ifndef GASPI_UTILS_H
#define GASPI_UTILS_H

#include <dash/dart/if/dart_types.h>
#include <stdlib.h>
#include <GASPI.h>

gaspi_return_t create_segment(const gaspi_size_t size,
                              gaspi_segment_id_t *seg_id);

gaspi_return_t delete_all_segments();

gaspi_return_t check_queue_size(gaspi_queue_id_t queue);

gaspi_return_t wait_for_queue_entries(gaspi_queue_id_t *queue,
                                      gaspi_number_t wanted_entries);

gaspi_return_t blocking_waitsome(gaspi_notification_id_t id_begin,
                                 gaspi_notification_id_t id_count,
                                 gaspi_notification_id_t *id_available,
                                 gaspi_notification_t *notify_val,
                                 gaspi_segment_id_t seg);

gaspi_return_t flush_queues(gaspi_queue_id_t queue_begin, gaspi_queue_id_t queue_count);

int gaspi_utils_compute_comms(int *parent, int **children, int me, int root, gaspi_rank_t size);

#define DART_CHECK_ERROR_TEMPL(expected, error_code, ret_type, func...)          \
  do {                                                                           \
    const ret_type retval = func;                                                \
    if (retval != expected) {                                                    \
      gaspi_printf("ERROR in %s : %s on line %i return value %i\n", #func,       \
                   __FILE__, __LINE__, retval);                                  \
      return error_code;                                                         \
    }                                                                            \
  }while (0)

#define DART_CHECK_ERROR(func...) DART_CHECK_ERROR_TEMPL(DART_OK, DART_ERR_OTHER, dart_ret_t, func)

#define DART_CHECK_GASPI_ERROR(func...) DART_CHECK_ERROR_TEMPL(GASPI_SUCCESS, GASPI_ERROR, gaspi_return_t, func)

#define DART_CHECK_ERROR_RET(ret, func...)                                       \
    do {                                                                         \
      ret = func;                                                                \
      if (ret != GASPI_SUCCESS) {                                                \
        gaspi_printf("ERROR in %s : %s on line %i return value %i\n", #func,     \
                     __FILE__, __LINE__, ret);                                   \
      }                                                                          \
    }while (0)

#endif // GASPI_UTILS_H
