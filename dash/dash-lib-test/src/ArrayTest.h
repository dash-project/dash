/*
 * ArrayTest.h
 *
 *  Created on: May 23, 2013
 *      Author: maierm
 */

#ifndef ARRAYTEST_H_
#define ARRAYTEST_H_

#include "gtest/gtest.h"
#include "dash/array.h"
#include <stdio.h>
#include "Util.h"
#include "dart/dart.h"
#include <regex>
#include "test_logger.h"
#include <sstream>


class ArrayTest: public testing::Test
{
public:

	static int integration_test_method(int argc, char** argv);

protected:

	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}

};

#endif /* ARRAYTEST_H_ */
