#ifndef DART_TASKING_EXTRAE__H
#define DART_TASKING_EXTRAE__H

#define USE_EXTRAE


#define EVENT_NONE     0
#define EVENT_TASK     1
#define EVENT_IDLE     2

#ifdef USE_EXTRAE
//#include "extrae_user_events.h"
//#include "extrae_types.h"
typedef unsigned extrae_type_t;
typedef unsigned long long extrae_value_t;
void Extrae_init (void) __attribute__((weak));
void Extrae_event (extrae_type_t type, extrae_value_t value) __attribute__((weak));
void Extrae_fini (void) __attribute__((weak));
void Extrae_define_event_type (extrae_type_t *type, char *type_description, unsigned *nvalues, extrae_value_t *values, char **values_description) __attribute__((weak));

static extrae_type_t et = 0;
static extrae_value_t ev[] = {0, 10, 20};
static char *extrae_names[] = {"NONE", "COMPUTE", "IDLE"};

#define EXTRAE_ENTER(_e) if (Extrae_event) Extrae_event(et, ev[_e])
#define EXTRAE_EXIT(_e)  if (Extrae_event) Extrae_event(et, ev[EVENT_NONE])
#else  // USE_EXTRAE
#define EXTRAE_ENTER(_e)
#define EXTRAE_EXIT(_e)
#endif // USE_EXTRAE

#endif // DART_TASKING_EXTRAE__H

