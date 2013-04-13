/*
 * main.c
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#include "gtest/gtest.h"
#include <iostream>
#include <sstream>
#include <string>
#include "InitTest.h"
#include "LocksTest.h"
#include "TeamTest.h"

using namespace std;

static bool is_integration_test(char** argv)
{
	const string it = "integration-test";
	while (*argv != NULL)
	{
		if (string(*argv) == it)
			return true;
		argv++;
	}
	return false;
}

GTEST_API_ int main(int argc, char **argv)
{
	if (is_integration_test(argv))
	{
		if (string("InitTest") == argv[2])
		{
			InitTest::integration_test_method(argc, argv);
		}
		if (string("LocksTest") == argv[2])
		{
			LocksTest::integration_test_method(argc, argv);
		}
		if (string("TeamTest") == argv[2])
		{
			TeamTest::integration_test_method(argc, argv);
		}
		else
		{
			cerr << "no valid test class given\n";
			return 1;
		}
		return 0;
	}

	std::cout << "Starting tests\n";
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
