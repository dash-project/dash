/*
 * Util.cpp
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#include "Util.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
extern "C"
{
#include "dart/dart_init.h"
}

using namespace std;

std::string Util::gptr_to_string(gptr_t ptr)
{
	ostringstream sstr;
	sstr << "{" << ptr.unitid << ", " << ptr.segid << ", " << ptr.flags << ", "
			<< ptr.offset << "}";
	return sstr.str();
}

std::string Util::args_to_string(int argc, char* argv[])
{
	ostringstream sstr;
	char** tmp = argv;
	while (*tmp != NULL)
	{
		sstr << *tmp << ";";
		tmp++;
	}
	return sstr.str();
}

std::string Util::start_integration_test(const std::string& test_class,
		const std::string& test_method, int* start_res)
{
	int argc = 6;
	const char* argv[argc + 1];
	argv[0] = "./dartrun";
	argv[1] = "2";
	argv[2] = "./test-dart-shmem";
	argv[3] = "integration-test";
	argv[4] = test_class.c_str();
	argv[5] = test_method.c_str();
	argv[argc] = NULL;

	int stderr_copy = dup(STDERR_FILENO);
	string filename = "/tmp/dart-integration-test-";
	filename += test_class;
	filename += "#";
	filename += test_method;
	filename += ".err";
	close (STDERR_FILENO);
	int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_APPEND | O_TRUNC,
			S_IRWXU);

	*start_res = dart_start(argc, (char**) argv);

	dup2(stderr_copy, STDERR_FILENO);
	close(stderr_copy);
	close(fd);

	ifstream output(filename.c_str());
	std::stringstream buffer;
	buffer << output.rdbuf();
	output.close();
	return buffer.str();
}

Util::Util()
{
	// TODO Auto-generated constructor stub

}

Util::~Util()
{
	// TODO Auto-generated destructor stub
}

