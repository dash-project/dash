/*
 * dart_active_messages_priv.h
 *
 *  Created on: Apr 20, 2017
 *      Author: joseph
 */

#ifndef DART_ACTIVE_MESSAGES_PRIV_H_
#define DART_ACTIVE_MESSAGES_PRIV_H_

#include <dash/dart/if/dart_active_messages.h>

/**
 * Forward declaration of implementation internal data.
 */
struct dart_amsgq_impl_data;

/**
 * Initialize the active messaging subsystem, mainly to determine the
 * offsets of function pointers between different units.
 * This has to be done only once in a collective global operation.
 *
 * We assume that there is a single offset for all function pointers.
 */
dart_ret_t
dart_amsg_init();


/**
 * Tear-down the active messages subsystem, releasing any held resources.
 */
dart_ret_t
dart_amsgq_fini();



/**
 * Function signatures for message queue implementations.
 */

/**
 * Open a new queue. Called from \ref dart_amsg_openq.
 */
typedef dart_ret_t (*dart_amsg_openq_impl)(
  size_t,
  size_t,
  dart_team_t,
  struct dart_amsgq_impl_data**);


/**
 * Try sending a message. Called from \ref dart_amsg_trysend.
 */
typedef dart_ret_t (*dart_amsg_trysend_impl)(
  dart_team_unit_t,
  struct dart_amsgq_impl_data*,
  const void*,
  size_t);

struct dart_flush_info {
  // the data to be written
  void  *data;
  // the size of the data
  size_t size;
  // the target of the write
  int    target;
  // 0 if the flush failed, >0 if successful
  int    status;
  // internal data structure for the implementation
  uint64_t _internal[2];
};

/**
 * Try sending a message. Called from \ref dart_amsg_trysend.
 */
typedef dart_ret_t (*dart_amsg_trysend_all_impl)(
  struct dart_amsgq_impl_data*,
  struct dart_flush_info*,
  int);

typedef dart_ret_t (*dart_amsg_buffered_send_impl)(
  dart_team_unit_t,
  struct dart_amsgq_impl_data*,
  dart_task_action_t,
  const void*,
  size_t);

typedef dart_ret_t (*dart_amsg_flush_buffer_impl)(struct dart_amsgq_impl_data*);


typedef dart_ret_t (*dart_amsg_process_impl)(struct dart_amsgq_impl_data*);

typedef dart_ret_t (*dart_amsg_process_blocking_impl)(
  struct dart_amsgq_impl_data*,
  dart_team_t);


typedef dart_ret_t (*dart_amsg_closeq_impl)(struct dart_amsgq_impl_data*);

typedef struct {
  dart_amsg_openq_impl            openq;
  dart_amsg_trysend_impl          trysend;
  dart_amsg_trysend_all_impl      trysend_all;
  dart_amsg_process_impl          process;
  dart_amsg_process_blocking_impl process_blocking;
  dart_amsg_closeq_impl           closeq;
} dart_amsgq_impl_t;

struct dart_amsg_header {
  dart_task_action_t fn;
  uint32_t           data_size;
#ifdef DART_ENABLE_LOGGING
  dart_global_unit_t remote;
  uint32_t           msgid;
#endif // DART_ENABLE_LOGGING
};

/**
 * Function called from implementations to process the queue
 */
void
dart__amsgq__process_buffer(
  void   *dbuf,
  size_t  tailpos);


/**
 * Initialization functions for message queue implementations.
 */

dart_ret_t
dart_amsg_sopnop_init(dart_amsgq_impl_t *impl)    DART_INTERNAL;

dart_ret_t
dart_amsg_sopnop2_init(dart_amsgq_impl_t *impl)   DART_INTERNAL;

dart_ret_t
dart_amsg_sopnop3_init(dart_amsgq_impl_t *impl)   DART_INTERNAL;

dart_ret_t
dart_amsg_sopnop4_init(dart_amsgq_impl_t *impl)   DART_INTERNAL;

dart_ret_t
dart_amsg_singlewin_init(dart_amsgq_impl_t *impl) DART_INTERNAL;

dart_ret_t
dart_amsg_dualwin_init(dart_amsgq_impl_t *impl)   DART_INTERNAL;

dart_ret_t
dart_amsg_sendrecv_init(dart_amsgq_impl_t *impl)  DART_INTERNAL;

#endif /* DART_ACTIVE_MESSAGES_PRIV_H_ */
