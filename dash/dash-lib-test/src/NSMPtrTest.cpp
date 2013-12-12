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
#include "dash/DartDataAccess.h"

#define TEAM_SIZE 4

using namespace std;
using namespace dash;
typedef NSMRef<int> IRef;
typedef NSMPtr<int> IPtr;

void test_std_sort();
void test_dart_data_access();

NSMPtr<int> allocInt(int teamid, int numLocalInts)
{
	size_t local_size = numLocalInts * sizeof(int);
	gptr_t ptr = dart_alloc_aligned(teamid, local_size);
	return NSMPtr<int>(teamid, ptr, local_size);
}

int NSMPtrTest::integration_test_method(int argc, char** argv)
{
	dart_init(&argc, &argv);
	if (string(argv[3]) == "std_sort")
	{
		test_std_sort();
	}
	else if (string(argv[3]) == "dart_data_access")
	{
		test_dart_data_access();
	}

	dart_exit(0);
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

void test_dart_data_access()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	const int num_ints_local = 4;
	gptr_t ptr = dart_alloc_aligned(DART_TEAM_ALL,
			num_ints_local * sizeof(int));
	DartDataAccess<int> acc(DART_TEAM_ALL, ptr, num_ints_local * sizeof(int));

	if (dart_myid() == 1)
	{
		auto printArr = [=]() -> std::string
		{
			ostringstream oss;
			gptr_t tmp = ptr;
			for (int i = 0; i < TEAM_SIZE * num_ints_local; i++)
			{
				int val;
				dart_get(&val, tmp, sizeof(int));
				oss << val << ",";
				tmp = dart_gptr_inc_by(tmp, sizeof(int));
			}
			return oss.str();
		};

		TLOG("before: %s", printArr().c_str());

		for (int i = 0; i < TEAM_SIZE * num_ints_local; i++)
		{
			acc.put_value(i);
			acc.increment();
		}

		DartDataAccess<int> acc2(DART_TEAM_ALL, ptr,
				num_ints_local * sizeof(int), num_ints_local * TEAM_SIZE - 1);
		ostringstream oss;
		for (int i = 0; i < TEAM_SIZE * num_ints_local; i++)
		{
			int out;
			acc2.get_value(&out);
			oss << out << ",";
			acc2.decrement();
		}
		TLOG("reverse: %s", oss.str().c_str());
		acc2.increment(17);
		TLOG("acc == acc2? %d", acc.equals(acc2));
		acc2.decrement(10);
		TLOG("acc.diff(acc2)? %lld", acc.difference(acc2));
		TLOG("acc2.diff(acc)? %lld", acc2.difference(acc));
	}

}

TEST_F(NSMPtrTest, integration_test_dart_data_access)
{
	using namespace dash;
	int result = -1;
	string log = Util::start_integration_test("NSMPtrTest", "dart_data_access",
			&result, TEAM_SIZE);
	cout << log;
}
