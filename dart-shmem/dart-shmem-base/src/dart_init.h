/*
 * dart_init.h
 *
 *  Created on: Apr 5, 2013
 *      Author: maierm
 */

#ifndef DART_INIT_H_
#define DART_INIT_H_

/**
 * parses command line, starts n <executable> processes and waits for them.
 * Command line format: execname <n> <executable> <args> (see usage)
 * @return 0, on success
 */
int dart_start(int argc, char* argv[]);

int dart_init(int *argc, char ***argv);

void dart_exit(int exitcode);

void dart_usage(char *s);

#endif /* DART_INIT_H_ */
