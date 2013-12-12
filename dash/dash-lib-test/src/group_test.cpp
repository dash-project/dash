/*
 * group_test.cpp
 *
 *  Created on: Jul 9, 2013
 *      Author: maierm
 */

#include "group_test.h"
#include <vector>

using namespace dash;
using namespace std;
typedef group_builder gb;

TEST_F(group_test, test_iter_range)
{
	auto g1 = gb::range(5, 9);
	vector<unit> expected;
	expected.push_back(unit(5));
	expected.push_back(unit(6));
	expected.push_back(unit(7));
	expected.push_back(unit(8));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_explicit)
{
	auto g1 = gb::list(
	{ 5, 9, 4, 7 });
	vector<unit> expected;
	expected.push_back(unit(4));
	expected.push_back(unit(5));
	expected.push_back(unit(7));
	expected.push_back(unit(9));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_filtered_1)
{
	auto g1 = gb::range(0, 10)->filter([](unit u)
	{	return u % 2 == 0;});
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(2));
	expected.push_back(unit(4));
	expected.push_back(unit(6));
	expected.push_back(unit(8));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_filtered_2)
{
	auto g1 = gb::range(0, 10)->filter([](unit u)
	{	return u % 2 == 1;});
	vector<unit> expected;
	expected.push_back(unit(1));
	expected.push_back(unit(3));
	expected.push_back(unit(5));
	expected.push_back(unit(7));
	expected.push_back(unit(9));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_filtered_3)
{
	auto g1 = gb::range(0, 10)->filter([](unit u)
	{	return u % 2 == 666;});
	vector<unit> expected;
	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_filtered_4)
{
	auto g1 = gb::range(0, 10)->filter([](unit u)
	{	return true;});
	vector<unit> expected;
	for (unsigned int i = 0; i < 10; i++)
		expected.push_back(unit(i));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_1)
{
	auto g1 = gb::range(0, 10) & gb::range(5, 45);
	vector<unit> expected;
	for (unsigned int i = 5; i < 10; i++)
		expected.push_back(unit(i));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_2)
{
	auto g1 = gb::range(0, 0) & gb::range(1, 1);
	vector<unit> expected;
	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_3)
{
	auto g1 = gb::range(0, 0) & gb::range(1, 12);
	vector<unit> expected;
	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_4)
{
	auto g1 = gb::range(0, 10) & gb::range(1, 1);
	vector<unit> expected;
	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_5)
{
	auto g1 = gb::list(
	{ 0, 5, 7, 10, 13, 14 }) & gb::list(
	{ 0, 6, 10, 11, 12, 14, 17 });
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(10));
	expected.push_back(unit(14));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_6)
{
	auto g1 = gb::list(
	{ 0, 5, 7, 10, 13, 14 }) & gb::list(
	{ 3, 6, 9, 11, 12, 15, 17 });
	vector<unit> expected;

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_intersect_7)
{
	auto g1 = gb::list(
	{ 0, 5, 7, 10, 13, 14 }) & gb::list(
	{ 0, 5, 7, 10, 13, 14 });
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(5));
	expected.push_back(unit(7));
	expected.push_back(unit(10));
	expected.push_back(unit(13));
	expected.push_back(unit(14));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_difference_1)
{
	auto g1 = gb::list(
	{ 0, 5, 7, 10, 13, 14 }) - gb::list(
	{ 3, 5, 10, 11, 12 });
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(7));
	expected.push_back(unit(13));
	expected.push_back(unit(14));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_difference_2)
{
	auto g1 = gb::list(
	{ 0, 5, 7, 10, 13, 14 }) - gb::list(
	{ });
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(5));
	expected.push_back(unit(7));
	expected.push_back(unit(10));
	expected.push_back(unit(13));
	expected.push_back(unit(14));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_difference_3)
{
	auto g1 = gb::list(
	{ 0, 5, 7, 10, 13, 14 }) - gb::list(
	{ 0, 5, 7, 10, 13, 14 });
	vector<unit> expected;

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_union_1)
{
	auto g1 = gb::list(
	{ 0, 6, 8, 10, 11 }) | gb::list(
	{ 0, 5, 8, 9, 11, 20 });
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(5));
	expected.push_back(unit(6));
	expected.push_back(unit(8));
	expected.push_back(unit(9));
	expected.push_back(unit(10));
	expected.push_back(unit(11));
	expected.push_back(unit(20));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_union_2)
{
	auto g1 = gb::list(
	{ 0, 6, 8, 10, 11 }) | gb::list(
	{ });
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(6));
	expected.push_back(unit(8));
	expected.push_back(unit(10));
	expected.push_back(unit(11));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_iter_combined_union_3)
{
	auto g1 = gb::list(
	{ 0, 6, 8, 10, 11 }) | gb::range(10, 13);
	vector<unit> expected;
	expected.push_back(unit(0));
	expected.push_back(unit(6));
	expected.push_back(unit(8));
	expected.push_back(unit(10));
	expected.push_back(unit(11));
	expected.push_back(unit(12));

	EXPECT_EQ(expected, g1->get_value());
}

TEST_F(group_test, test_operators)
{
	auto g1 = gb::list(
	{ 0, 6, 8, 10, 11 });
	auto g2 = gb::range(10, 14);
	auto g3 = gb::range(0, 8)->filter([](unit u)
	{	return u % 2 == 1;});
	auto g4 = ((g1 & g2) | g3) - gb::list(
	{ 3, 7 });

	vector<unit> expected;
	expected.push_back(unit(1));
	expected.push_back(unit(5));
	expected.push_back(unit(10));
	expected.push_back(unit(11));

	EXPECT_EQ(expected, g4->get_value());
}
