#ifndef DART_MEMBUCKET_PRIV_H_INCLUDED
#define DART_MEMBUCKET_PRIV_H_INCLUDED

#include <stddef.h>

#include "extern_c.h"
EXTERN_C_BEGIN

typedef struct dart_membucket_list_entry* dart_membucket_list;
typedef struct dart_membucket_list_entry dart_list_entry;

struct dart_membucket_list_entry
{
  void* pos;
  size_t size;
  dart_membucket_list next;
};

struct dart_opaque_membucket
{
  dart_membucket_list free;
  dart_membucket_list allocated;
  void* shm_address;
  int localsize;
  int shm_id;
  size_t size;
};

dart_membucket_list 
dart_remove_list_entry(dart_membucket_list list,
		       dart_membucket_list prev, 
		       dart_membucket_list toRemove);

dart_membucket_list dart_push_front(dart_membucket_list list,
				  dart_list_entry newEntry);

dart_membucket_list dart_list_melt(dart_membucket_list list);

dart_membucket_list dart_insert_sorted(dart_membucket_list list,
				     dart_list_entry newEntry);

void dart_membucket_list_to_string(FILE* f, dart_membucket_list list);

int dart_membucket_list_size(dart_membucket_list list);

EXTERN_C_END

#endif /* DART_MEMBUCKET_PRIV_H_INCLUDED */


