/*
 * MultiArrayTest.h
 *
 *  Created on: Jun 28, 2013
 *      Author: maierm
 */

#ifndef MULTIARRAYTEST_H_
#define MULTIARRAYTEST_H_

#include "gtest/gtest.h"
#include "dash/MultiArray.h"
#include "Util.h"
#include "dart/dart.h"
#include <regex>
#include "test_logger.h"
#include <sstream>

class MultiArrayTest: public testing::Test
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

#endif /* MULTIARRAYTEST_H_ */
