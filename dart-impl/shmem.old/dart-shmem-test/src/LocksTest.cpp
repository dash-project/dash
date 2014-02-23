/*
 * LocksTest.cpp
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#include "LocksTest.h"
#include "test_logger.h"
#include "Util.h"
#include <regex>
#include <unistd.h>
#include <string>
using namespace std;

static void test_lock_waits();
static void test_try_lock();

int LocksTest::integration_test_method(int argc, char** argv)
{
	dart_init(&argc, &argv);

	if (string(argv[3]) == "lock_waits")
	{
		test_lock_waits();
	}
	else if (string(argv[3]) == "try_lock")
	{
		test_try_lock();
	}

	dart_exit(0);
	return 0;
}

static void test_try_lock()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	dart_barrier(DART_TEAM_ALL);

	dart_lock lock;
	dart_lock_team_init(DART_TEAM_ALL, &lock);
	int result = dart_lock_try_acquire(lock);

	if (result == DART_OK)
	{
		TLOG("received %s", "DART_OK");
		sleep(1);
		dart_lock_release(lock);
	}
	else if (result == DART_LOCK_ALREADY_AQUIRED)
	{
		TLOG("received %s", "DART_LOCK_ALREADY_AQUIRED");
	}
	dart_barrier(DART_TEAM_ALL);
	dart_lock_free(&lock);
}

TEST_F(LocksTest, integration_test_try_lock)
{
	int res = -1;
	string log = Util::start_integration_test("LocksTest", "try_lock", &res);
	EXPECT_EQ(0, res);
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# received DART_OK(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# received DART_LOCK_ALREADY_AQUIRED(.|\n)*")));
}

static void test_lock_waits()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	gptr_t gptr = dart_alloc_aligned(DART_TEAM_ALL, sizeof(int));
	std::string s = Util::gptr_to_string(gptr);
	TLOG("received gptr: %s", s.c_str());

	int i = -1;
	if (dart_team_myid(DART_TEAM_ALL) == 0)
	{
		i = 40;
		dart_put(gptr, &i, sizeof(int));
	}
	dart_barrier(DART_TEAM_ALL);

	dart_lock lock;
	dart_lock_team_init(DART_TEAM_ALL, &lock);
	dart_lock_acquire(lock);

	dart_get(&i, gptr, sizeof(int));
	TLOG("initial i: %d", i);
	sleep(1);
	i++;
	dart_put(gptr, &i, sizeof(int));

	dart_lock_release(lock);
	dart_barrier(DART_TEAM_ALL);
	dart_lock_free(&lock);

	dart_get(&i, gptr, sizeof(int));
	TLOG("after 2 increments, i: %d", i);
}

TEST_F(LocksTest, integration_test_lock_waits)
{
	int res = -1;
	string log = Util::start_integration_test("LocksTest", "lock_waits", &res);
	EXPECT_EQ(0, res);

	regex tofind("(.|\n)*after 2 increments, i: 42(.|\n)*");
	EXPECT_TRUE(regex_match(log, tofind));
}
