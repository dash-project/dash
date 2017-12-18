/*
 * dart_active_messages_priv.h
 *
 *  Created on: Apr 20, 2017
 *      Author: joseph
 */

#ifndef DART_ACTIVE_MESSAGES_PRIV_H_
#define DART_ACTIVE_MESSAGES_PRIV_H_


//#define DART_AMSGQ_SENDRECV
#define DART_AMSGQ_SENDRECV_PT
//#define DART_AMSGQ_LOCKFREE
//#define DART_AMSGQ_SINGLEWIN

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

#endif /* DART_ACTIVE_MESSAGES_PRIV_H_ */
