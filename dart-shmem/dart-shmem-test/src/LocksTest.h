/*
 * LocksTest.h
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#ifndef LOCKSTEST_H_
#define LOCKSTEST_H_

#include "gtest/gtest.h"
extern "C" {
#include "dart/dart.h"
}

class LocksTest: public testing::Test {

public:

protected:

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}

public:

	static int integration_test_method(int argc, char** argv);

};

#endif /* LOCKSTEST_H_ */
