#ifndef DART_TASKING_CRAYPAT__H
#define DART_TASKING_CRAYPAT__H

#if defined(CRAYPAT)

#include<pat_api.h>

#define EVENT_NONE     0
#define EVENT_TASK     1
#define EVENT_IDLE     2


static int events[] = {0, 10, 20};
static const char *craypat_names[] = {"NONE", "COMPUTE", "IDLE"};

#define CRAYPAT_ENTER(_ev) PAT_region_begin(events[_ev], craypat_names[_ev])
#define CRAYPAT_EXIT(_ev)  PAT_region_end  (events[_ev])

#else   // CRAYPAT

#define CRAYPAT_ENTER(_ev)
#define CRAYPAT_EXIT(_ev)

#endif // CRAYPAT

#endif // DART_TASKING_CRAYPAT__H
