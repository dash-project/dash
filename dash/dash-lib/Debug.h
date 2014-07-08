
#ifndef DEBUG_H_INCLUDED 
#define DEBUG_H_INCLUDED 

#define GPTR_SPRINTF(buf_, gptr_)				\
  sprintf(buf_, "(unit=%d,seg=%d,flags=%d,addr=%p)",		\
	  gptr_.unitid, gptr_.segid, gptr_.flags,		\
	  gptr_.addr_or_offs.addr);


#endif /* DEBUG_H_INCLUDED */
