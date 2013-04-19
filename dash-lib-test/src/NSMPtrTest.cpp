/*
 * NSMPtrTest.cpp
 *
 *  Created on: Mar 4, 2013
 *      Author: maierm
 */

#include "NSMPtrTest.h"
#include <algorithm>
#include <sstream>
#include "test_logger.h"
#include "Util.h"
#include <regex>
#include "dart/dart.h"

#define TEAM_SIZE 4
#define MAX_DATA_ACCESSORS 100

using namespace std;
using namespace dash;
typedef NSMRef<int> IRef;
typedef NSMPtr<int> IPtr;

static dash::NonSequentialMemory* m_nsm;
static dash::DartDataAccessor* m_accessors[MAX_DATA_ACCESSORS];
static int m_num_accessors;

void test_std_sort();

NSMPtr<int> allocInt(int teamid, int numLocalInts)
{
	size_t local_size = numLocalInts * sizeof(int);
	gptr_t ptr = dart_alloc_aligned(teamid, local_size);
	int team_size = dart_team_size(teamid);
	for (int i = 0; i < team_size; i++)
	{
		gptr_t da_ptr = dart_gptr_inc_by(ptr, local_size * i);
		m_accessors[m_num_accessors] = new DartDataAccessor(da_ptr);
		m_nsm->add_segment(
				MemorySegment(m_accessors[m_num_accessors], local_size));
		m_num_accessors++;
	}
	NonSequentialMemoryAccessor<int> beginAcc =
			NonSequentialMemoryAccessor<int>::begin(m_nsm);
	return NSMPtr<int>(beginAcc);
}

int NSMPtrTest::integration_test_method(int argc, char** argv)
{
	dart_init(&argc, &argv);
	m_nsm = new dash::NonSequentialMemory();
	m_num_accessors = 0;
	for (int i = 0; i < MAX_DATA_ACCESSORS; i++)
		m_accessors[i] = NULL;

	if (string(argv[3]) == "std_sort")
	{
		test_std_sort();
	}

	dart_exit(0);
	for (int i = 0; i < m_num_accessors; i++)
		delete m_accessors[i];
	delete m_nsm;
	return 0;
}

void test_std_sort()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	const int num_ints_local = 4;
	const int num_ints_global = num_ints_local * TEAM_SIZE;
	IPtr begin = allocInt(DART_TEAM_ALL, num_ints_local);
	IPtr end = begin + num_ints_global;

	auto ascfun = [] (int i1, int i2) -> bool
	{	return i1 < i2;};
	auto descfun = [] (int i1, int i2) -> bool
	{	return i1 > i2;};

	if (dart_myid() == 0)
	{
		int i = 0;
		std::for_each(begin, end, [&i] (IRef ref)
		{	ref = i++;});
		std::sort(begin, end, descfun);
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_myid() == 1)
	{
		ostringstream oss;
		std::for_each(begin, end, [&oss] (IRef ref)
		{	oss << ref << " ";});
		TLOG("result: %s", oss.str().c_str());

		auto lookFor =
				[=] (int i, bool asc = true)
				{
					TLOG("looking for a %d ...", i);
					bool found = std::binary_search (begin, end, i, (asc)? ascfun : descfun);
					TLOG("%s %d %s", (asc)? "asc" : "desc", i, (found)? "found" : "not found");
				};

		lookFor(9);
		lookFor(9, false);
		lookFor(99, false);
	}
}

TEST_F(NSMPtrTest, integration_test_std_sort)
{
	using namespace dash;
	int result = -1;
	string log = Util::start_integration_test("NSMPtrTest", "std_sort", &result,
			TEAM_SIZE);
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*1 # result: 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 (.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*1 # asc 9 not found(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*1 # desc 9 found(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*1 # desc 99 not found(.|\n)*")));
	cout << log;
}
