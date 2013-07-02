#include "MultiArrayTest.h"
#include "dash/RAIIBarrier.h"
#include <vector>

#define TEAM_SIZE 3

using namespace std;
using namespace dash;

static void test_constructor();
static void test_indexing();
static void test_indexing_block();

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
	else if (string(argv[3]) == "test_indexing_block")
	{
		test_indexing_block();
	}
	dart_exit(0);
	return 0;
}

static void test_constructor()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	{
		MultiArray<BlockDist, int, 1> marr1(BlockDist(1), DART_TEAM_ALL, 3);
		TLOG("marr1: %llu", marr1.getNumElems());
		MultiArray<BlockDist, int, 3> marr2(BlockDist(3 * 5 * 7 / TEAM_SIZE),
				DART_TEAM_ALL, 3, 5, 7);
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
		MultiArray<BlockDist, int, 3> m2x3x4(BlockDist(2 * 3 * 4 / TEAM_SIZE),
				DART_TEAM_ALL, 2, 3, 4);
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

		MultiArray<BlockDist, int, 2> m3x5(BlockDist(3 * 5 / TEAM_SIZE),
				DART_TEAM_ALL, 3, 5);
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

TEST_F(MultiArrayTest, test_BlockDist_1)
{
	// BlockDist [3] with 27 elems and 3 processes:
	// P0	P1	P2
	// -----------
	// 0	3	6
	// 1	4	7
	// 2	5	8
	// -----------
	// 9	12	15
	// 10	13	16
	// 11	14	17
	// -----------
	// 18	21	24
	// 19	22	25
	// 20	23	26
	// -----------

	vector<unsigned int> expected;
	unsigned int i = 0;
	for (; i < 27; i++)
		expected.push_back(0);
	i = 0;

	expected[0] = i++;
	expected[1] = i++;
	expected[2] = i++;
	expected[9] = i++;
	expected[10] = i++;
	expected[11] = i++;
	expected[18] = i++;
	expected[19] = i++;
	expected[20] = i++;

	expected[3] = i++;
	expected[4] = i++;
	expected[5] = i++;
	expected[12] = i++;
	expected[13] = i++;
	expected[14] = i++;
	expected[21] = i++;
	expected[22] = i++;
	expected[23] = i++;

	expected[6] = i++;
	expected[7] = i++;
	expected[8] = i++;
	expected[15] = i++;
	expected[16] = i++;
	expected[17] = i++;
	expected[24] = i++;
	expected[25] = i++;
	expected[26] = i++;

	BlockDist dist(3);
	dist.set_num_array_elems(27);
	dist.set_num_processes(3);

	for (i = 0; i < 27; i++)
	{
		EXPECT_EQ(expected[i], dist.actual_index(i));
	}

}

TEST_F(MultiArrayTest, test_BlockDist_2)
{
	// BlockDist [4] with 16 elems and 2 processes:
	// P0	P1
	// -----------
	// 0	4
	// 1	5
	// 2	6
	// 3	7
	// -----------
	// 8	12
	// 9	13
	// 10	14
	// 11	15

	vector<unsigned int> expected;
	unsigned int i = 0;
	for (; i < 16; i++)
		expected.push_back(0);
	i = 0;

	expected[0] = i++;
	expected[1] = i++;
	expected[2] = i++;
	expected[3] = i++;
	expected[8] = i++;
	expected[9] = i++;
	expected[10] = i++;
	expected[11] = i++;

	expected[4] = i++;
	expected[5] = i++;
	expected[6] = i++;
	expected[7] = i++;
	expected[12] = i++;
	expected[13] = i++;
	expected[14] = i++;
	expected[15] = i++;

	BlockDist dist(4);
	dist.set_num_array_elems(16);
	dist.set_num_processes(2);

	for (i = 0; i < 16; i++)
	{
		EXPECT_EQ(expected[i], dist.actual_index(i));
	}

}

/**
 * This test does not work. We expect full blocks (such that elem 10 is elem 12, etc.), i.e. elems % (num_procs * num_blocks) == 0
 */
//TEST_F(MultiArrayTest, test_BlockDist_3)
//{
//	// BlockDist [3] with 12 elems and 3 processes:
//	// P0	P1	P2
//	// -----------
//	// 0	3	6
//	// 1	4	7
//	// 2	5	8
//	// -----------
//	// 9	10	11
//
//	vector<unsigned int> expected;
//	unsigned int i = 0;
//	for (; i < 12; i++)
//		expected.push_back(0);
//	i = 0;
//
//	expected[0] = i++;
//	expected[1] = i++;
//	expected[2] = i++;
//	expected[9] = i++;
//
//	expected[3] = i++;
//	expected[4] = i++;
//	expected[5] = i++;
//	expected[10] = i++;
//
//	expected[6] = i++;
//	expected[7] = i++;
//	expected[8] = i++;
//	expected[11] = i++;
//
//	BlockDist dist(3);
//	dist.set_num_array_elems(12);
//	dist.set_num_processes(3);
//
//	for (i = 0; i < 12; i++)
//	{
//		EXPECT_EQ(expected[i], dist.actual_index(i));
//	}
//
//}
static void test_indexing_block()
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	{
		// P1		P2		P3
		// 0,0,0	0,0,2	0,1,1
		// 0,0,1	0,1,0	0,1,2
		// ----------------------
		// 1,0,0	1,0,2	1,1,1
		// 1,0,1	1,1,0	1,1,2

		MultiArray<BlockDist, int, 3> m2x2x3(BlockDist(2), DART_TEAM_ALL, 2, 2,
				3);
		if (dart_myid() == 0)
		{
			typedef typename ::dash::array<int>::iterator DARRAY_IT;
			::dash::array<int>* arr = m2x2x3.getArray();
			unsigned int i = 0;
			for (DARRAY_IT it = arr->begin(); it != arr->end(); it++)
				*it = i++;
		}

		dart_barrier(DART_TEAM_ALL);

		if (dart_myid() == 1)
		{
			for (int i = 0; i < 2; i++)
				for (int j = 0; j < 2; j++)
					for (int k = 0; k < 3; k++)
						TLOG("m2x2x3: %d %d %d: %d",
								i, j, k, (int)m2x2x3(i, j, k));
		}
	}
	dart_team_detach_mempool(DART_TEAM_ALL);
}

TEST_F(MultiArrayTest, integration_test_test_indexing_block)
{
	int result = -1;
	string log = Util::start_integration_test("MultiArrayTest",
			"test_indexing_block", &result, TEAM_SIZE);

	vector<string> expected;
	expected.push_back("# 1 # m2x2x3: 0 0 0: 0");
	expected.push_back("# 1 # m2x2x3: 0 0 1: 1");
	expected.push_back("# 1 # m2x2x3: 0 0 2: 4");
	expected.push_back("# 1 # m2x2x3: 0 1 0: 5");
	expected.push_back("# 1 # m2x2x3: 0 1 1: 8");
	expected.push_back("# 1 # m2x2x3: 0 1 2: 9");
	expected.push_back("# 1 # m2x2x3: 1 0 0: 2");
	expected.push_back("# 1 # m2x2x3: 1 0 1: 3");
	expected.push_back("# 1 # m2x2x3: 1 0 2: 6");
	expected.push_back("# 1 # m2x2x3: 1 1 0: 7");
	expected.push_back("# 1 # m2x2x3: 1 1 1: 10");
	expected.push_back("# 1 # m2x2x3: 1 1 2: 11");

	for (int i = 0; i < 12; i++)
	{
		string prefix = "(.|\n)*";
		EXPECT_TRUE( regex_match(log, regex(prefix + expected[i] + "(.|\n)*")));
	}
}
