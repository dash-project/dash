/*
 * TeamTest.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: maierm
 */

#include "TeamTest.h"
#include "test_logger.h"
#include "Util.h"
#include <regex>
#include <unistd.h>
#include <string>
using namespace std;

void test_onesided_aligned(int t012, int t345, int t01, int t45);
void test_onesided_not_aligned(int t012, int t345, int t01, int t45);

static void create_a_few_teams(int* t012, int* t345, int* t01, int* t45)
{
	size_t size_of_group = dart_group_size_of();
	dart_group_t* g012 = (dart_group_t*)malloc(size_of_group);
	dart_group_t* g345 = (dart_group_t*)malloc(size_of_group);
	dart_group_init(g012);
	dart_group_init(g345);

	dart_group_addmember(g012, 0);
	dart_group_addmember(g012, 1);
	dart_group_addmember(g012, 2);
	dart_group_addmember(g345, 3);
	dart_group_addmember(g345, 4);
	dart_group_addmember(g345, 5);

	*t012 = dart_team_create(DART_TEAM_ALL, g012);
	*t345 = dart_team_create(DART_TEAM_ALL, g345);

	if(dart_group_ismember(g012, dart_myid()))
	{
		dart_group_t* g01 = (dart_group_t*)malloc(size_of_group);
		dart_group_init(g01);
		dart_group_addmember(g01, 0);
		dart_group_addmember(g01, 1);
		*t01 = dart_team_create(*t012, g01);
		TLOG("created team t01 with id %d", *t01);
		dart_group_fini(g01);
		free(g01);
	}

	if(dart_group_ismember(g345, dart_myid()))
	{
		dart_group_t* g45 = (dart_group_t*)malloc(size_of_group);
		dart_group_init(g45);
		dart_group_addmember(g45, 4);
		dart_group_addmember(g45, 5);
		*t45 = dart_team_create(*t345, g45);
		TLOG("created team t45 with id %d", *t45);
		dart_group_fini(g45);
		free(g45);
	}

	dart_group_fini(g012);
	free(g012);
	dart_group_fini(g345);
	free(g345);
}


int TeamTest::integration_test_method(int argc, char** argv)
{
	dart_init(&argc, &argv);

	int t012, t345, t01, t45;
	create_a_few_teams(&t012, &t345, &t01, &t45);

	if(string("size") == argv[3])
	{
		TLOG("size t012: %d", dart_team_size(t012));
		TLOG("size t345: %d", dart_team_size(t345));
		TLOG("size t01: %d", dart_team_size(t01));
		TLOG("size t45: %d", dart_team_size(t45));
	}
	else if(string("myid") == argv[3])
	{
		TLOG("myid t012: %d", dart_team_myid(t012));
		TLOG("myid t345: %d", dart_team_myid(t345));
		TLOG("myid t01: %d", dart_team_myid(t01));
		TLOG("myid t45: %d", dart_team_myid(t45));
	}
	else if(string("multicast012") == argv[3])
	{
		if(dart_myid() >= 0 && dart_myid() < 3)
		{
			int i = (dart_myid() == 0)? 84 : -1;
			dart_bcast(&i, sizeof(int), 0, t012);
			TLOG("received: %d", i);
		}
	}
	else if(string("multicast345") == argv[3])
	{
		if(dart_myid() >= 3 && dart_myid() < 6)
		{
			int i[] = {-1, -2};
			if(dart_myid() == 4)
			{
				i[0] = 99;
				i[1] = 98;
			}
			dart_bcast(i, 2*sizeof(int), 1, t345);
			TLOG("received: %d,%d", i[0], i[1]);
		}
	}
	else if(string("multicast01") == argv[3])
	{
		if(dart_myid() >= 0 && dart_myid() < 2)
		{
			int i = (dart_myid() == 0)? 77 : -1;
			dart_bcast(&i, sizeof(int), 0, t01);
			TLOG("received: %d", i);
		}
	}
	else if(string("multicast45") == argv[3])
	{
		if(dart_team_myid(t45) >= 0)
		{
			int i = (dart_team_myid(t45) == 1)? 66 : -1;
			dart_bcast(&i, sizeof(int), 1, t45);
			TLOG("received: %d", i);
		}
	}
	else if(string("multicast_01_45") == argv[3])
	{
		if(dart_team_myid(t01) >= 0)
		{
			int i = (dart_team_myid(t01) == 0)? 55 : -1;
			dart_bcast(&i, sizeof(int), 0, t01);
			TLOG("received: %d", i);
		}
		if(dart_team_myid(t45) >= 0)
		{
			int i = (dart_team_myid(t45) == 1)? 66 : -1;
			dart_bcast(&i, sizeof(int), 1, t45);
			TLOG("received: %d", i);
		}
	}
	else if(string("onesided_aligned") == argv[3])
	{
		test_onesided_aligned(t012, t345, t01, t45);
	}
	else if(string("onesided_not_aligned") == argv[3])
	{
		test_onesided_not_aligned(t012, t345, t01, t45);
	}

	dart_exit(0);
	return 0;
}

void test_onesided_not_aligned(int t012, int t345, int t01, int t45)
{
	dart_team_attach_mempool(DART_TEAM_ALL, 4096);
	int my45 = dart_team_myid(t45);
	if (my45 >= 0)
	{
		if (my45 == 0)
		{
			gptr_t p1 = dart_alloc(1 * sizeof(int));
			int i = 666;
			dart_put(p1, &i, sizeof(int));
			dart_bcast(&p1, sizeof(gptr_t), 0, t45);
		}
		else
		{
			gptr_t p2;
			dart_bcast(&p2, sizeof(gptr_t), 0, t45);
			int i = -1;
			dart_get(&i, p2, sizeof(int));
			TLOG("received: %d", i);
		}
	}
	dart_team_detach_mempool(DART_TEAM_ALL);
}

TEST_F(TeamTest, integration_test_onesided_not_aligned)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "onesided_not_aligned", &res, 6);
	EXPECT_EQ(0, res);
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # received: 666(.|\n)*")));
}

void test_onesided_aligned(int t012, int t345, int t01, int t45)
{
	if(dart_team_myid(t012) >= 0)
	{
		dart_team_attach_mempool(t012, 4096);

		gptr_t p1 = dart_alloc_aligned(t012, 2 * sizeof(int));
		gptr_t p2 = dart_alloc_aligned(t012, 2 * sizeof(int));

		int i[] = {-1, -1};
		if(dart_team_myid(t012) == 2)
		{
			i[0] = 4;
			i[1] = 2;
			dart_put(p1, i, 2 * sizeof(int));
			i[0] = 6;
			i[1] = 3;
			dart_put(p2, i, 2 * sizeof(int));
		}
		dart_barrier(t012);
		if(dart_team_myid(t012) == 1)
		{
			dart_get(i, p1, 2 * sizeof(int));
			TLOG("received: %d %d", i[0], i[1]);
		}
		if(dart_team_myid(t012) == 0)
		{
			dart_get(i, p2, 2 * sizeof(int));
			TLOG("received: %d %d", i[0], i[1]);
		}


		dart_free(t012, p1);
		dart_free(t012, p2);
		dart_team_detach_mempool(t012);
	}
}

TEST_F(TeamTest, integration_test_onesided_aligned)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "onesided_aligned", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # received: 4 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # received: 6 3(.|\n)*")));
}

TEST_F(TeamTest, integration_test_multicast_01_45)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "multicast_01_45", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # received: 55(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # received: 55(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # received: 66(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # received: 66(.|\n)*")));
}

TEST_F(TeamTest, integration_test_multicast45)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "multicast45", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # received: 66(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # received: 66(.|\n)*")));
}

TEST_F(TeamTest, integration_test_multicast01)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "multicast01", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # received: 77(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # received: 77(.|\n)*")));
}

TEST_F(TeamTest, integration_test_multicast345)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "multicast345", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # received: 99,98(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # received: 99,98(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # received: 99,98(.|\n)*")));
}

TEST_F(TeamTest, integration_test_multicast012)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "multicast012", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # received: 84(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # received: 84(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # received: 84(.|\n)*")));
}

TEST_F(TeamTest, integration_test_myid)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "myid", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # myid t012: 0(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # myid t012: 1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # myid t012: 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # myid t012: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # myid t012: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # myid t012: -1(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # myid t345: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # myid t345: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # myid t345: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # myid t345: 0(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # myid t345: 1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # myid t345: 2(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # myid t01: 0(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # myid t01: 1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # myid t01: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # myid t01: -998(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # myid t01: -998(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # myid t01: -998(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # myid t45: -998(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # myid t45: -998(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # myid t45: -998(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # myid t45: -1(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # myid t45: 0(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # myid t45: 1(.|\n)*")));
}

TEST_F(TeamTest, integration_test_create_teams_with_equal_id)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "create_teams_with_equal_id", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # created team t01 with id 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # created team t01 with id 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # created team t01 with id 3(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # created team t45 with id 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # created team t45 with id 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # created team t45 with id 3(.|\n)*")));

}

TEST_F(TeamTest, integration_test_size)
{
	int res = -1;
	string log = Util::start_integration_test("TeamTest", "size", &res, 6);
	EXPECT_EQ(0, res);

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # size t012: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # size t012: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # size t012: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # size t012: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # size t012: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # size t012: 3(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # size t345: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # size t345: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # size t345: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # size t345: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # size t345: 3(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # size t345: 3(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # size t01: 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # size t01: 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # size t01: 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # size t01: -(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # size t01: -(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # size t01: -(.|\n)*")));

	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 0 # size t45: -(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 1 # size t45: -(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 2 # size t45: -(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 3 # size t45: 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 4 # size t45: 2(.|\n)*")));
	EXPECT_TRUE(regex_match(log, regex("(.|\n)*# 5 # size t45: 2(.|\n)*")));
}
