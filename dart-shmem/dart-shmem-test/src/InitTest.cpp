/*
 * InitTest.cpp
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#include "InitTest.h"
#include "test_logger.h"
#include "Util.h"
#include <regex>

using namespace std;

int InitTest::integration_test_method(int argc, char** argv)
{
	TLOG("starting integration test... Arguments: %s", Util::args_to_string(argc, argv).c_str());

	int init_res = dart_init(&argc, &argv);
	TLOG("init_result: %d", init_res);
	TLOG("args after init: %s", Util::args_to_string(argc, argv).c_str());

	dart_exit(0);
	return 0;
}

TEST_F(InitTest, integration_test_create_2_processes)
{
	int res = -1;
	string log = Util::start_integration_test("InitTest",
			"create_2_processes", &res);
	EXPECT_EQ(0, res);

	regex tofind(
			"(.|\n)*starting integration test\\.\\.\\. Arguments: (.*);integration-test;"
					"InitTest;create_2_processes;--dart-id=0;--dart-size=2(.|\n)*");
	regex tofind2(
			"(.|\n)*starting integration test\\.\\.\\. Arguments: (.*);integration-test;"
					"InitTest;create_2_processes;--dart-id=1;--dart-size=2(.|\n)*");
	EXPECT_TRUE(regex_match(log, tofind));
	EXPECT_TRUE(regex_match(log, tofind2));

	// check init result
	regex initResult0("(.|\n)* # 0 # init_result: 0(.|\n)*");
	regex initResult1("(.|\n)* # 1 # init_result: 0(.|\n)*");
	EXPECT_TRUE(regex_match(log, initResult0));
	EXPECT_TRUE(regex_match(log, initResult1));

	// check init result
	regex argsResult0("(.|\n)*# 0 # args after init: (.*);integration-test;InitTest;create_2_processes;(.|\n)*");
	regex argsResult1("(.|\n)*# 1 # args after init: (.*);integration-test;InitTest;create_2_processes;(.|\n)*");
	EXPECT_TRUE(regex_match(log, argsResult0));
	EXPECT_TRUE(regex_match(log, argsResult1));
}
