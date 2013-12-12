/*
 * ArrayTest.cpp
 *
 *  Created on: May 23, 2013
 *      Author: maierm
 */

#include "ArrayTest.h"
#include "dart/dart.h"
#include "dash/RAIIBarrier.h"

#define TEAM_SIZE 2

using namespace std;
using namespace dash;

static void test_constructor();
static void test_iterators();
static void test_element_access();
static void test_fill_and_swap();
static void test_concerted_fill_and_swap();
static void test_concerted_comparison();

int ArrayTest::integration_test_method(int argc, char** argv)
{
	dart_init(&argc, &argv);
	if (string(argv[3]) == "test_constructor")
	{
		test_constructor();
	}
	else if (string(argv[3]) == "test_iterators")
	{
		test_iterators();
	}
	else if (string(argv[3]) == "test_element_access")
	{
		test_element_access();
	}
	else if (string(argv[3]) == "test_fill_and_swap")
	{
		test_fill_and_swap();
	}
	else if (string(argv[3]) == "test_concerted_fill_and_swap")
	{
		test_concerted_fill_and_swap();
	}
	else if (string(argv[3]) == "test_concerted_comparison")
	{
		test_concerted_comparison();
	}

	dart_exit(0);
	return 0;
}

static void test_constructor()
{
	// resolve ambiguous calls -> we prefer dash::arrays :-)
	using dash::array;
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	array<int> arr(10, DART_TEAM_ALL);
}

TEST_F(ArrayTest, integration_test_test_constructor)
{
	int result = -1;
	string log = Util::start_integration_test("ArrayTest", "test_constructor",
			&result, TEAM_SIZE);
	EXPECT_FALSE( regex_match(log, regex("(.|\n)*ERROR(.|\n)*")));
}

static void test_iterators()
{
	// resolve ambiguous calls -> we prefer dash::arrays :-)
	using dash::array;
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	array<int> arr(10, DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		int i = 5;
		for (array<int>::iterator it = arr.begin(); it != arr.end(); it++)
			*it = i--;
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr: %s", arr.to_string().c_str());
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		int i = 5;
		for (array<int>::reverse_iterator it = arr.rbegin(); it != arr.rend();
				it++)
			*it = i--;
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("rarr: %s", arr.to_string().c_str());
	}
}

TEST_F(ArrayTest, integration_test_test_iterators)
{
	int result = -1;
	string log = Util::start_integration_test("ArrayTest", "test_iterators",
			&result, TEAM_SIZE);
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr: dash::array 5,4,3,2,1,0,-1,-2,-3,-4,end dash::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # rarr: dash::array -4,-3,-2,-1,0,1,2,3,4,5,end dash::array(.|\n)*")));
	cout << log;
}

static void test_element_access()
{
	// resolve ambiguous calls -> we prefer dash::arrays :-)
	using dash::array;
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	array<int> arr(10, DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		for (int i = 0; i < 10; i++)
			arr[i] = i * 2;
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		ostringstream oss;
		oss << "arr at: ";
		for (int i = 0; i < 10; i++)
			oss << arr.at(i) << ",";
		TLOG("%s", oss.str().c_str());
		TLOG("front %d", (int)arr.front());
		TLOG("back %d", (int)arr.back());
	}

}

TEST_F(ArrayTest, integration_test_test_element_access)
{
	int result = -1;
	string log = Util::start_integration_test("ArrayTest",
			"test_element_access", &result, TEAM_SIZE);
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr at: 0,2,4,6,8,10,12,14,16,18,(.|\n)*")));
	EXPECT_TRUE( regex_match(log, regex("(.|\n)*# 1 # front 0(.|\n)*")));
	EXPECT_TRUE( regex_match(log, regex("(.|\n)*# 1 # back 18(.|\n)*")));
}

static void test_fill_and_swap()
{
	// resolve ambiguous calls -> we prefer dash::arrays :-)
	using dash::array;
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	array<int> arr(10, DART_TEAM_ALL);
	array<int> arr2(10, DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		arr.fill(42);
		arr2.fill(84);
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr filled: %s", arr.to_string().c_str());
		TLOG("arr2 filled: %s", arr2.to_string().c_str());
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		arr.swap(arr2);
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr swapped: %s", arr.to_string().c_str());
		TLOG("arr2 swapped: %s", arr2.to_string().c_str());
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		swap(arr, arr2);
	}

	dart_barrier(DART_TEAM_ALL);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr swapped2: %s", arr.to_string().c_str());
		TLOG("arr2 swapped2: %s", arr2.to_string().c_str());
	}
}

TEST_F(ArrayTest, integration_test_test_fill_and_swap)
{
	int result = -1;
	string log = Util::start_integration_test("ArrayTest", "test_fill_and_swap",
			&result, TEAM_SIZE);

	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr filled: dash::array 42,42,42,42,42,42,42,42,42,42,end dash::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr2 filled: dash::array 84,84,84,84,84,84,84,84,84,84,end dash::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr2 swapped: dash::array 42,42,42,42,42,42,42,42,42,42,end dash::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr swapped: dash::array 84,84,84,84,84,84,84,84,84,84,end dash::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr swapped2: dash::array 42,42,42,42,42,42,42,42,42,42,end dash::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr2 swapped2: dash::array 84,84,84,84,84,84,84,84,84,84,end dash::array(.|\n)*")));
	cout << log;
}

static void test_concerted_fill_and_swap()
{
	// resolve ambiguous calls -> we prefer dash::concerted::arrays :-)
	using dash::concerted::array;
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	array<int> arr(15, DART_TEAM_ALL);
	array<int> arr2(15, DART_TEAM_ALL);

	arr.fill(dart_team_myid(arr.team_id()));
	arr2.fill(dart_team_myid(arr2.team_id()) + 42);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr filled: %s", arr.solo_to_string().c_str());
		TLOG("arr2 filled: %s", arr2.solo_to_string().c_str());
	}

	dart_barrier(arr.team_id());
	arr.swap(arr2);

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr swapped: %s", arr.solo_to_string().c_str());
		TLOG("arr2 swapped: %s", arr2.solo_to_string().c_str());
	}

	{
		RAIIBarrier barr(arr.team_id(), true);
		for (auto i : arr)
			i = i + 7;
	}

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr after for: %s", arr.solo_to_string().c_str());
	}

	{
		RAIIBarrier barr(arr.team_id(), true);
		for (unsigned int i = 0; i < arr.size(); i++)
			arr[i] = arr[i] + 6;
	}

	if (dart_team_myid(DART_TEAM_ALL) == 1)
	{
		TLOG("arr after second for: %s", arr.solo_to_string().c_str());
	}

}

TEST_F(ArrayTest, integration_test_test_concerted_fill_and_swap)
{
	int result = -1;
	string log = Util::start_integration_test("ArrayTest",
			"test_concerted_fill_and_swap", &result, 3);

	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr filled: dash::concerted::array 0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,end dash::concerted::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr2 filled: dash::concerted::array 42,42,42,42,42,43,43,43,43,43,44,44,44,44,44,end dash::concerted::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr swapped: dash::concerted::array 42,42,42,42,42,43,43,43,43,43,44,44,44,44,44,end dash::concerted::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr2 swapped: dash::concerted::array 0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,end dash::concerted::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr after for: dash::concerted::array 49,49,49,49,49,50,50,50,50,50,51,51,51,51,51,end dash::concerted::array(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 1 # arr after second for: dash::concerted::array 55,55,55,55,55,56,56,56,56,56,57,57,57,57,57,end dash::concerted::array(.|\n)*")));
}

static void test_concerted_comparison()
{
	// resolve ambiguous calls -> we prefer dash::concerted::arrays :-)
	using dash::concerted::array;
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	array<int> arr(15, DART_TEAM_ALL);
	array<int> arr2(15, DART_TEAM_ALL);

	arr.fill(3);
	arr2.fill(3);
	TLOG("1: arr == arr2: %d", arr == arr2);

	{
		RAIIBarrier rb(arr.team_id(), true);
		if (dart_myid() == 0)
		{
			arr[3] = 2;
		}
	}

	TLOG("2: arr == arr2: %d", arr == arr2);
	{
		RAIIBarrier rb(arr.team_id(), true);
		if (dart_myid() == 0)
		{
			arr[3] = 3;
			arr[8] = 27;
		}
	}

	TLOG("3: arr == arr2: %d", arr == arr2);

	{
		RAIIBarrier rb(arr.team_id(), true);
		if (dart_myid() == 0)
		{
			arr[8] = 3;
			arr[10] = 90;
		}
	}

	TLOG("4: arr == arr2: %d", arr == arr2);
	TLOG("4: arr != arr2: %d", arr != arr2);
}

TEST_F(ArrayTest, integration_test_test_concerted_comparison)
{
	const int NUM_PROCS = 3;
	int result = -1;
	string log = Util::start_integration_test("ArrayTest",
			"test_concerted_comparison", &result, NUM_PROCS);

	auto check = [&log](int testnumber, int unit, string op, bool result)
	{
		const char* tmpl = "(.|\n)*# %d # %d: arr %s arr2: %d(.|\n)*";
		char toCheck[256];
		sprintf(toCheck, tmpl, unit, testnumber, op.c_str(), result);
		EXPECT_TRUE(
				regex_match(log, regex(toCheck)));
	};

	for (int i = 0; i < NUM_PROCS; i++)
		check(1, i, "==", true);
	for (int i = 0; i < NUM_PROCS; i++)
		check(2, i, "==", false);
	for (int i = 0; i < NUM_PROCS; i++)
		check(3, i, "==", false);
	for (int i = 0; i < NUM_PROCS; i++)
		check(4, i, "==", false);
	for (int i = 0; i < NUM_PROCS; i++)
		check(4, i, "!=", true);
}
