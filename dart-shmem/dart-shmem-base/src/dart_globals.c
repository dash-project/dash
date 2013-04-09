/*
 * dart_globals.c
 *
 * put global variables in their own compilation unit to omit circular linkage dependencies
 * (Optional: use special linker option (groups))
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

int _glob_myid = -1;

