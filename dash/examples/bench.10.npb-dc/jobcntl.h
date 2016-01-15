#ifndef JOBCNTL_H_INCLUDED
#define JOBCNTL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "adc.h"

uint32 NumberOfOnes(uint64 s);
void SetOneBit(uint64 *s, int32 pos);
void SetOneBit32(uint32 *s, uint32 pos);
uint32 Mlo32(uint32 x);
int32 mro32(uint32 x);
uint32 setLeadingOnes32(uint32 n);
int32 DeleteOneFile(const char * file_name);
void WriteOne32Tuple(char * t, uint32 s, uint32 l, FILE * logf);
uint32 NumOfCombsFromNbyK( uint32 n, uint32 k );
void JobPoolUpdate(ADC_VIEW_CNTL *avp);
int32 GetParent(ADC_VIEW_CNTL *avp, uint32 binRepTuple);
uint32 GetSmallestParent(ADC_VIEW_CNTL *avp, uint32 binRepTuple);
int32 GetPrefixedParent(ADC_VIEW_CNTL *avp, uint32 binRepTuple);
void JobPoolInit(JOB_POOL *jpp, uint32 n, uint32 nd);
void WriteOne64Tuple(char * t, uint64 s, uint32 l, FILE * logf);
uint32 NumberOfOnes(uint64 s);
void GetRegTupleFromBin64(uint64 binRepTuple, uint32 *selTuple, uint32 numDims, uint32 *numOfUnits);
void getRegTupleFromBin32(uint32 binRepTuple, uint32 *selTuple,uint32 numDims, uint32 *numOfUnits);
void GetRegTupleFromParent(uint64 bin64RepTuple, uint32 bin32RepTuple, uint32 *selTuple, uint32 nd);
void CreateBinTuple(uint64 *binRepTuple, uint32 *selTuple, uint32 numDims);
void d32v( char * t, uint32 *v, uint32 n);
void WriteOne64Tuple(char * t, uint64 s, uint32 l, FILE * logf);
int32 Comp8gbuf(const void *a, const void *b);
void restore(TUPLE_VIEWSIZE x[], uint32 f, uint32 l );
void vszsort( TUPLE_VIEWSIZE x[], uint32 n);
uint32 countTupleOnes(uint64 binRepTuple, uint32 numDims);
void restoreo( TUPLE_ONES x[], uint32 f, uint32 l );
void onessort( TUPLE_ONES x[], uint32 n);
uint32 MultiFileProcJobs(TUPLE_VIEWSIZE *tuplesAndSizes, uint32 nViews, ADC_VIEW_CNTL *avp );
int32 PartitionCube(ADC_VIEW_CNTL *avp);



#endif
