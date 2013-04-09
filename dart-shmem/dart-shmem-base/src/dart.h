/*
 * dart.h - DASH RUNTIME (DART) INTERFACE 
 */

#ifndef DART_H_INCLUDED
#define DART_H_INCLUDED

#include <stdint.h>
#include "dart_return_codes.h"

/*
 --- DART version number and build date ---
 */
extern unsigned int _dart_version;

#define DART_VERSION_NUMBER(maj_,min_,rev_)	\
  (((maj_<<24) | ((min_<<16) | (rev_))))
#define DART_VERSION_MAJOR     ((_dart_version>>24) &   0xFF)
#define DART_VERSION_MINOR     ((_dart_version>>16) &   0xFF)
#define DART_VERSION_REVISION  ((_dart_version)     & 0xFFFF)

#define DART_BUILD_DATE    (__DATE__ " " __TIME__)
#define DART_VERSION       DART_VERSION_NUMBER(0,0,1)

#include "dart_init.h"

#include "dart_group.h"

#include "dart_teams.h"

#include "dart_gptr.h"

#include "dart_malloc.h"

#include "dart_locks.h"

#include "dart_onesided.h"

#endif /* DART_H_INCLUDED */

