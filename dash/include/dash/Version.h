#ifndef DASH__VERSION_H__INCLUDED
#define DASH__VERSION_H__INCLUDED

#include <dash/internal/Macro.h>

#define DASH_VERSION_MAJOR 0
#define DASH_VERSION_MINOR 4
#define DASH_VERSION_PATCH 0

#define DASH_VERSION_STRING            \
  dash__toxstr(DASH_VERSION_MAJOR) "." \
  dash__toxstr(DASH_VERSION_MINOR) "." \
  dash__toxstr(DASH_VERSION_PATCH)     \


#endif // DASH__VERSION_H__INCLUDED
