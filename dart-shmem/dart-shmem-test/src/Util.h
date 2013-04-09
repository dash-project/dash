/*
 * Util.h
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>

class Util
{
public:
	Util();
	virtual ~Util();

	/**
	 * @return log output of started processes
	 */
	static std::string start_integration_test(const std::string& test_class,
			const std::string& test_method, int* start_res);

	static std::string args_to_string(int argc, char* argv[]);
};

#endif /* UTIL_H_ */
