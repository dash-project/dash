
#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#ifndef MAXSIZE_GROUP
#define MAXSIZE_GROUP 256
#endif

#define MYTIMEVAL( tv_ )			\
  ((tv_.tv_sec)+(tv_.tv_usec)*1.0e-6)

#define TIMESTAMP( time_ )				\
  {							\
    static struct timeval tv;				\
    gettimeofday( &tv, NULL );				\
    time_=MYTIMEVAL(tv);				\
  }

#define GPTR_SPRINTF(buf_, gptr_)				\
  sprintf(buf_, "(unit=%d,seg=%d,flags=%d,addr=%p)",		\
	  gptr_.unitid, gptr_.segid, gptr_.flags,		\
	  gptr_.addr_or_offs.addr);


#define CHECK(fncall) do {					     \
    int _retval;                                                     \
    if ((_retval = fncall) != DART_OK) {                             \
      fprintf(stderr, "ERROR %d calling: %s"                         \
	      " at: %s:%i",					     \
              _retval, #fncall, __FILE__, __LINE__);                 \
      fflush(stderr);                                                \
    }                                                                \
  } while(0)


#define GROUP_SPRINTF(_buf, _group)					\
  do {									\
    size_t _size;							\
    char *_str;								\
    int _i, _len;							\
    dart_unit_t _members[MAXSIZE_GROUP];				\
    dart_group_size(_group, &_size);					\
    dart_group_getmembers(_group, _members);				\
    _str=_buf;								\
    _len=sprintf(_str, "size=%d members=", _size);			\
    _str=_str+_len;							\
    for( _i=0; _i<_size; _i++ ) {					\
      _len=sprintf(_str, "%d ", _members[_i]);				\
      _str=_str+_len;							\
    }									\
  }									\
  while(0) 



#endif /* UTILS_H_INCLUDED */
