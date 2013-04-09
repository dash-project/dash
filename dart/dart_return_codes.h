/*
 * dart_return_codes.h
 *
 * TODO: the name of this file is a little bit misleading...
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#ifndef DART_RETURN_CODES_H_
#define DART_RETURN_CODES_H_

#define DART_OK          0    /* no error */

/* TODO: expand the list of error codes as needed.
 it might be a good idea to use negative integers as
 error codes
 */
#define DART_ERR_OTHER   -999
#define DART_ERR_INVAL   -998

#define DART_TEAM_ALL      0

// TODO: the following may be implementation-specific constants set by the build-process or something like that.
// This might not be the right place...

// max. number of members in a group
#define MAXSIZE_GROUP 100

// maximum number of supported teams
#define MAXNUM_TEAMS 64

#endif /* DART_RETURN_CODES_H_ */
