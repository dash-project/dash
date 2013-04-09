/*
 * MempoolTest.h
 *
 *  Created on: Mar 8, 2013
 *      Author: maierm
 */

#ifndef MEMPOOLTEST_H_
#define MEMPOOLTEST_H_

#include "gtest/gtest.h"
#include <malloc.h>
extern "C" {
#include "dart/dart_mempool.h"
#include "dart-shmem-base/dart_mempool_private.h"
}

class MempoolTest: public testing::Test {

public:

protected:

	virtual void SetUp() {
		m_size = 200;
		m_mempos = malloc(m_size);
		m_pool = dart_mempool_create(m_mempos, m_size);

		m_test_list = NULL;
		dart_list_entry entry;
		entry.pos = ((void*)1);
		entry.size = 10u;
		m_test_list = dart_push_front(m_test_list, entry);

		entry.pos = ((void*)2);
		entry.size = 20u;
		m_test_list = dart_push_front(m_test_list, entry);

		entry.pos = ((void*)3);
		entry.size = 30u;
		m_test_list = dart_push_front(m_test_list, entry);

	}

	virtual void TearDown() {
		free(m_mempos);
	}

	static void* add_to_pvoid(void* p, size_t size)
	{
		return ((char*)p) + size; // TODO
	}

	static int pvoid_lt(void* p1, void* p2)
	{
		return ((char*)p1) < ((char*)p2); // TODO
	}

	static int pvoid_eq(void* p1, void* p2)
	{
		return ((char*)p1) == ((char*)p2); // TODO
	}

	size_t m_size;
	void* m_mempos;
	dart_mempool m_pool;
	dart_mempool_list m_test_list;
};

#endif /* MEMPOOLTEST_H_ */
