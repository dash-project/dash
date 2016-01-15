#ifndef EXTBUILD_H_INCLUDED
#define EXTBUILD_H_INCLUDED

typedef int                 int32;
typedef unsigned int       uint32;
typedef long long           int64;
typedef unsigned long long uint64;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "adc.h"
#include "rbt.h"
#include "jobcntl.h"
#include "macrodef.h"

int32 ReadWholeInputData(ADC_VIEW_CNTL *avp, FILE *inpf);
int32 ComputeMemoryFittedView (ADC_VIEW_CNTL *avp);
int32 SharedSortAggregate(ADC_VIEW_CNTL *avp);
int32 PrefixedAggregate(ADC_VIEW_CNTL *avp, FILE *iof);
int32 RunFormation (ADC_VIEW_CNTL *avp, FILE *inpf);
int32 MultiWayMerge(ADC_VIEW_CNTL *avp);
void SeekAndReadNextSubChunk(uint32 multiChunkBuffer[], uint32 k,FILE *inFile,uint32 chunkRecSize, uint64 inFileOffs,uint32 subChunkNum);
void ReadSubChunk(uint32 chunkRecSize, uint32 *multiChunkBuffer, uint32 mwBufRecSizeInInt, uint32 iChunk, uint32 regSubChunkSize, CHUNKS *chunks, FILE *fileOfChunks);
void SelectToView(uint32 * ib, uint32 *ix, uint32 *viewBuf, uint32 nd, uint32 nm, uint32 nv);
FILE * AdcFileOpen(const char *fileName, const char *mode);
void AdcFileName(char *adcFileName, const char *adcName, const char *fileName, uint32 taskNumber);
ADC_VIEW_CNTL * NewAdcViewCntl(ADC_VIEW_PARS *adcpp, uint32 pnum);
void InitAdcViewCntl(ADC_VIEW_CNTL *adccntl, uint32 nSelectedDims, uint32 *selection, uint32 fromParent);
int32 CloseAdcView(ADC_VIEW_CNTL *adccntl);
void AdcCntlLog(ADC_VIEW_CNTL *adccntlp);
int32 ViewSizesVerification(ADC_VIEW_CNTL *adccntlp);
int32 ComputeGivenGroupbys(ADC_VIEW_CNTL *adccntlp);

#endif
