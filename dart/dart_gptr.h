
#ifndef DART_GPTR_H_INCLUDED
#define DART_GPTR_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/*
  --- DART global pointers ---
  
  There are multiple options for representing the global
  pointer that come to mind:
  
  1) struct with pre-defined members (say, unit id
    and local address)
  2) an opaque object that leaves the details to a specific
    implementation and is manipulated only through pointers
  3) a fixed size integer data type (say 64 bit or 128 bit),
    manipulated through c macros that packs all the
    relevant information

 There are pros and cons to each option...

 Another question is that of offsets vs. addresses: Either a local
 virtual address is directly included and one in which the pointer
 holds something like a segment ID and an offset within that segment.

 If we want to support virtual addresses then 64 bits is not enough to
 represent the pointer. If we only support segment offsets, 64 bit
 could be sufficient

 Yet another question is what kind of operations are supported on
 global pointers. For example UPC global pointers keep "phase"
 information that allows pointer arithmetic (the phase is needed for
 knowing when you have to move to the next node).

 PROPOSAL: Don't include phase information with pointers on the DART
 level, but don't preclude support the same concept on the DASH level.

  */

  /*
 PROPOSAL: use 128 bit global pointers with the following layout:


 0         1         2         3         4         5         6
 0123456789012345678901234567890123456789012345678901234567890123
 |------<32 bit unit id>--------|-<segment id>--|--flags/resv---|
 |-----------<either a virtual address or an offset>------------|

  */


typedef struct
{
  int32_t  unitid;
  int16_t  segid;
  uint16_t flags;
  union 
  {
    uint64_t offset;
    void* addr;
  };
} dart_gptr_t;



#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_GPTR_H_INCLUDED */

