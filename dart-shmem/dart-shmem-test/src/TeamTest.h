/*
 * TeamTest.h
 *
 *  Created on: Apr 12, 2013
 *      Author: maierm
 */

#ifndef TEAMTEST_H_
#define TEAMTEST_H_

#include "gtest/gtest.h"
extern "C" {
#include "dart/dart.h"
}

class TeamTest: public testing::Test {

public:

protected:

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}

public:

	static int integration_test_method(int argc, char** argv);

};

#endif /* TEAMTEST_H_ */
