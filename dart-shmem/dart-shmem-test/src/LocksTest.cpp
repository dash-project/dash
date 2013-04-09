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

int LocksTest::integration_test_method(int argc, char** argv)
{
	TLOG("starting integration test... Arguments: %s",
			Util::args_to_string(argc, argv).c_str());
	dart_init(&argc, &argv);

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

	dart_exit(0);
	return 0;
}

TEST_F(LocksTest, integration_test_lock_waits)
{
	int res = -1;
	string log = Util::start_integration_test("LocksTest", "lock_waits", &res);
	EXPECT_EQ(0, res);

	regex tofind("(.|\n)*after 2 increments, i: 42(.|\n)*");
	EXPECT_TRUE(regex_match(log, tofind));
}
