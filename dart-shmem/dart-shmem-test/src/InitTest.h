/*
 * InitTest.h
 *
 * Test of start and init-methods
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#ifndef INITTEST_H_
#define INITTEST_H_

#include "gtest/gtest.h"
#include <malloc.h>
extern "C" {
#include "dart/dart_init.h"
}

class InitTest: public testing::Test {

public:

protected:

	virtual void SetUp() {
	}

	virtual void TearDown() {
	}

public:

	static int integration_test_method(int argc, char** argv);

};

#endif /* INITTEST_H_ */
