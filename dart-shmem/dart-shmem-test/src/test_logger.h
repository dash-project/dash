/*
 * test_logger.h
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#ifndef TEST_LOGGER_H_
#define TEST_LOGGER_H_

extern "C"
{
#include "dart/dart_logger.h"
}

#define TLOG(format, ...) do{fprintf(stderr, "\e[1;45mTEST \e[0m # %d # " format "\n", _glob_myid, __VA_ARGS__); }while(0)

#endif /* TEST_LOGGER_H_ */
