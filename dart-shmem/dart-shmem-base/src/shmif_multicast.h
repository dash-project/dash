/*
 * shmif_multicast.h
 *
 *  Created on: Apr 4, 2013
 *      Author: maierm
 */

#ifndef DART_SHMIF_MULTICAST_H_
#define DART_SHMIF_MULTICAST_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int shmif_multicast_bcast(void* buf, size_t nbytes, int root, int group_id,
		int id_in_group, int group_size);

int shmif_multicast_init_multicast_group(int group_id, int my_id,
		int group_size);

int shmif_multicast_release_multicast_group(int group_id, int my_id,
		int group_size);

#ifdef __cplusplus
}
#endif
#endif /* SHMIF_MULTICAST_H_ */
