/*
 * MempoolTest.cpp
 *
 *  Created on: Mar 8, 2013
 *      Author: maierm
 */

#include "MempoolTest.h"
#include <iostream>

TEST_F(MempoolTest, test_create) {
	EXPECT_EQ(NULL, m_pool->free->next);
	EXPECT_EQ(m_mempos, m_pool->free->pos);
	EXPECT_EQ(m_size, m_pool->free->size);
	EXPECT_EQ(NULL, m_pool->allocated);
}

TEST_F(MempoolTest, test_alloc_AND_free) {
	void* al1 = dart_mempool_alloc(m_pool, 50);
	void* al2 = dart_mempool_alloc(m_pool, 30);
	void* al3 = dart_mempool_alloc(m_pool, 10);
	dart_mempool_free(m_pool, al2);
	dart_mempool_free(m_pool, al3);
	dart_mempool_free(m_pool, al1);

	EXPECT_EQ(1, dart_mempool_list_size(m_pool->free));
	EXPECT_EQ(0, dart_mempool_list_size(m_pool->allocated));
	EXPECT_EQ(m_mempos, m_pool->free->pos);
	EXPECT_EQ(m_size, m_pool->free->size);
}

TEST_F(MempoolTest, test_alloc) {
	void* aloc1 = dart_mempool_alloc(m_pool, 50);
	EXPECT_EQ(aloc1, m_mempos);
	EXPECT_EQ(NULL, m_pool->free->next);
	EXPECT_EQ(add_to_pvoid(m_mempos, 50), m_pool->free->pos);
	EXPECT_EQ(150u, m_pool->free->size);

	EXPECT_NE(((void*)NULL), m_pool->allocated);
	EXPECT_EQ(NULL, m_pool->allocated->next);
	EXPECT_EQ(m_mempos, m_pool->allocated->pos);
	EXPECT_EQ(50u, m_pool->allocated->size);
}

TEST_F(MempoolTest, test_list_melt_1) {
	dart_mempool_list test_list = NULL;
	dart_list_entry entry;

	for(int i=0; i<5; i++)
	{
		entry.pos = ((void*)(50-i*10));
		entry.size = 10u;
		test_list = dart_push_front(test_list, entry);
	}

	test_list = dart_list_melt(test_list);
	EXPECT_EQ(1, dart_mempool_list_size(test_list));
	EXPECT_EQ(((void*)10), test_list->pos);
	EXPECT_EQ(50u, test_list->size);
}

TEST_F(MempoolTest, test_list_melt_2) {
	dart_mempool_list test_list = NULL;
	dart_list_entry entry;

	// build test list:
	// 10 10
	// 20 10
	// 30 5
	// 40 10
	// 50 10
	for(int i=0; i<2; i++)
	{
		entry.pos = ((void*)(50-i*10));
		entry.size = 10u;
		test_list = dart_push_front(test_list, entry);
	}

	entry.pos = ((void*)30);
	entry.size = 5u;
	test_list = dart_push_front(test_list, entry);

	for(int i=0; i<2; i++)
	{
		entry.pos = ((void*)(20-i*10));
		entry.size = 10u;
		test_list = dart_push_front(test_list, entry);
	}

	test_list = dart_list_melt(test_list);
	EXPECT_EQ(2, dart_mempool_list_size(test_list));
	EXPECT_EQ(((void*)10), test_list->pos);
	EXPECT_EQ(25u, test_list->size);
	EXPECT_EQ(((void*)40), test_list->next->pos);
	EXPECT_EQ(20u, test_list->next->size);
}

TEST_F(MempoolTest, test_dart_remove_list_entry_1) {

	m_test_list = dart_remove_list_entry(m_test_list, NULL, m_test_list);
	EXPECT_EQ(2, dart_mempool_list_size(m_test_list));
	EXPECT_EQ(((void*)2), m_test_list->pos);
	EXPECT_EQ(((void*)1), m_test_list->next->pos);
}

TEST_F(MempoolTest, test_dart_remove_list_entry_2) {

	m_test_list = dart_remove_list_entry(m_test_list, m_test_list,
			m_test_list->next);
	EXPECT_EQ(2, dart_mempool_list_size(m_test_list));
	EXPECT_EQ(((void*)3), m_test_list->pos);
	EXPECT_EQ(((void*)1), m_test_list->next->pos);
}

TEST_F(MempoolTest, test_dart_remove_list_entry_3) {

	m_test_list = dart_remove_list_entry(m_test_list, m_test_list->next,
			m_test_list->next->next);
	EXPECT_EQ(2, dart_mempool_list_size(m_test_list));
	EXPECT_EQ(((void*)3), m_test_list->pos);
	EXPECT_EQ(((void*)2), m_test_list->next->pos);
}

TEST_F(MempoolTest, test_dart_insert_sorted) {
	dart_mempool_list list = NULL;
	dart_list_entry entry;
	entry.size = 1u;

	entry.pos = ((void*) 50);
	list = dart_insert_sorted(list, entry);
	entry.pos = ((void*) 60);
	list = dart_insert_sorted(list, entry);
	entry.pos = ((void*) 30);
	list = dart_insert_sorted(list, entry);
	entry.pos = ((void*) 40);
	list = dart_insert_sorted(list, entry);

	EXPECT_EQ(4, dart_mempool_list_size(list));
	int begin = 30;
	while (list != NULL) {
		entry = *list;
		EXPECT_EQ(((void*)begin), entry.pos);
		EXPECT_EQ(1u, entry.size);
		list = list->next;
		begin += 10;
	}
}

TEST_F(MempoolTest, test_dart_push_front) {
	EXPECT_EQ(3, dart_mempool_list_size(m_test_list));

	dart_list_entry entry = *m_test_list;
	EXPECT_EQ(((void*)3), entry.pos);
	EXPECT_EQ(30u, entry.size);

	m_test_list = m_test_list->next;
	entry = *m_test_list;
	EXPECT_EQ(((void*)2), entry.pos);
	EXPECT_EQ(20u, entry.size);

	m_test_list = m_test_list->next;
	entry = *m_test_list;
	EXPECT_EQ(((void*)1), entry.pos);
	EXPECT_EQ(10u, entry.size);
}
