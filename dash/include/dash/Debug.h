/* 
 * dash-lib/Debug.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DEBUG_H_INCLUDED 
#define DEBUG_H_INCLUDED 

#ifndef MAXSIZE_GROUP
#define MAXSIZE_GROUP 256
#endif

#define GPTR_SPRINTF(buf_, gptr_)				\
  sprintf(buf_, "(unit=%d,seg=%d,flags=%d,addr=%p)",		\
	  gptr_.unitid, gptr_.segid, gptr_.flags,		\
	  gptr_.addr_or_offs.addr);

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


#endif /* DEBUG_H_INCLUDED */
