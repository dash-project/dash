/*
 * NSMPtrTest.h
 *
 *  Created on: Mar 4, 2013
 *      Author: maierm
 */

#ifndef NSMPTRTEST_H_
#define NSMPTRTEST_H_

#include "gtest/gtest.h"
#include "dash/NSMPtr.h"
#include <vector>
#include <sstream>

class NSMPtrTest: public testing::Test
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

#endif /* NSMPTRTEST_H_ */
