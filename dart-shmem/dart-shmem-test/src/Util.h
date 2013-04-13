/*
 * Util.h
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>
extern "C"
{
#include "dart/dart_gptr.h"
#include "dart/dart_group.h"
}

class Util
{
public:
	Util();
	virtual ~Util();

	/**
	 * @return log output of started processes
	 */
	static std::string start_integration_test(const std::string& test_class,
			const std::string& test_method, int* start_res, int num_procs = 2);

	static std::string args_to_string(int argc, char* argv[]);

	static std::string gptr_to_string(gptr_t ptr);

	static std::string group_to_string(dart_group_t* g);
};

#endif /* UTIL_H_ */
