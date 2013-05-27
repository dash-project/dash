#include "gtest/gtest.h"
#include <iostream>
#include "NSMPtrTest.h"
#include "ArrayTest.h"

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
		if (string("NSMPtrTest") == argv[2])
		{
			NSMPtrTest::integration_test_method(argc, argv);
		}
		else if (string("ArrayTest") == argv[2])
		{
			ArrayTest::integration_test_method(argc, argv);
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
