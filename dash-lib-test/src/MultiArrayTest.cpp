#include "MultiArrayTest.h"
#include "dash/RAIIBarrier.h"

#define TEAM_SIZE 3

using namespace std;
using namespace dash;

static void test_constructor();
static void test_indexing();

int MultiArrayTest::integration_test_method(int argc, char** argv)
{
	dart_init(&argc, &argv);
	if (string(argv[3]) == "test_constructor")
	{
		test_constructor();
	}
	else if (string(argv[3]) == "test_indexing")
	{
		test_indexing();
	}
	dart_exit(0);
	return 0;
}

static void test_constructor()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	{
		MultiArray<int, 1> marr1(DART_TEAM_ALL, 3);
		TLOG("marr1: %llu", marr1.getNumElems());
		MultiArray<int, 3> marr2(DART_TEAM_ALL, 3, 5, 7);
		TLOG("marr2: %llu", marr2.getNumElems());
	}
	dart_team_detach_mempool(DART_TEAM_ALL);
}

TEST_F(MultiArrayTest, integration_test_test_constructor)
{
	int result = -1;
	string log = Util::start_integration_test("MultiArrayTest",
			"test_constructor", &result, TEAM_SIZE);
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # marr1: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # marr2: 105(.|\n)*")));
}

static void test_indexing()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	{
		MultiArray<int, 3> m2x3x4(DART_TEAM_ALL, 2, 3, 4);
		if (dart_myid() == 0)
		{
			typedef typename ::dash::array<int>::iterator DARRAY_IT;
			::dash::array<int>* arr = m2x3x4.getArray();
			unsigned int i = 0;
			for (DARRAY_IT it = arr->begin(); it != arr->end(); it++)
				*it = i++;

			TLOG("m2x3x4: 0 0 3: %d", (int )m2x3x4(0, 0, 3));
			// 3
			TLOG("m2x3x4: 0 2 1: %d", (int )m2x3x4(0, 2, 1));
			// 9
			TLOG("m2x3x4: 1 1 2: %d", (int )m2x3x4(1, 1, 2));
			// 18
		}

		dart_barrier(DART_TEAM_ALL);

		MultiArray<int, 2> m3x5(DART_TEAM_ALL, 3, 5);
		if (dart_myid() == 0)
		{
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 5; j++)
					m3x5(i, j) = i + j;
		}

		dart_barrier(DART_TEAM_ALL);

		if (dart_myid() == 1)
		{
			for (int i = 0; i < 3; i++)
			{
				ostringstream sstr;
				for (int j = 0; j < 5; j++)
					sstr << setw(3) << m3x5(i, j) << " ";
				TLOG("m3x5 -- %d: %s", i, sstr.str().c_str());
			}
		}
	}
	dart_team_detach_mempool(DART_TEAM_ALL);
}

TEST_F(MultiArrayTest, integration_test_test_indexing)
{
	int result = -1;
	string log = Util::start_integration_test("MultiArrayTest", "test_indexing",
			&result, TEAM_SIZE);

	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 0 # m2x3x4: 0 0 3: 3(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 0 # m2x3x4: 0 2 1: 9(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex("(.|\n)*# 0 # m2x3x4: 1 1 2: 18(.|\n)*")));

	EXPECT_TRUE(
			regex_match(log, regex( "(.|\n)*# 1 # m3x5 -- 0:   0   1   2   3   4(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex( "(.|\n)*# 1 # m3x5 -- 1:   1   2   3   4   5(.|\n)*")));
	EXPECT_TRUE(
			regex_match(log, regex( "(.|\n)*# 1 # m3x5 -- 2:   2   3   4   5   6(.|\n)*")));
}

