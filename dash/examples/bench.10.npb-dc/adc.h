#ifndef ADC_H_INCLUDED
#define ADC_H_INCLUDED

#include "rbt.h"

typedef int                 int32;
typedef unsigned int       uint32;
typedef long long           int64;
typedef unsigned long long uint64;

#define ADC_OK                        0
#define ADC_WRITE_FAILED              1
#define ADC_INTERNAL_ERROR            2
#define ADC_TREE_DESTROY_FAILURE      3
#define ADC_FILE_OPEN_FAILURE         4
#define ADC_MEMORY_ALLOCATION_FAILURE 5
#define ADC_FILE_DELETE_FAILURE       6
#define ADC_VERIFICATION_FAILED       7
#define ADC_SHMEMORY_FAILURE          8

#define SSA_BUFFER_SIZE     (1024*1024)
#define MAX_NUMBER_OF_TASKS         256

#define MAX_PAR_FILE_LINE_SIZE      512
#define MAX_FILE_FULL_PATH_SIZE     512
#define MAX_ADC_NAME_SIZE            32

#define DIM_FSZ                       4
#define MSR_FSZ                       8

#define MAX_NUM_OF_DIMS              20
#define MAX_NUM_OF_MEAS               4

#define MAX_NUM_OF_CHUNKS          1024      
#define MAX_PARAM_LINE_SIZE        1024

#define OUTPUT_BUFFER_SIZE (MAX_NUM_OF_DIMS + (MSR_FSZ/4)*MAX_NUM_OF_MEAS)
#define MAX_VIEW_REC_SIZE ((DIM_FSZ*MAX_NUM_OF_DIMS)+(MSR_FSZ*MAX_NUM_OF_MEAS))     
#define MAX_VIEW_ROW_SIZE_IN_INTS (MAX_NUM_OF_DIMS + 2*MAX_NUM_OF_MEAS)
#define MLB32  0x80000000

#ifdef WINNT
#define MLB    0x8000000000000000
#else
#define MLB 0x8000000000000000LL
#endif

#define BlockSize 1024

static int measbound=31415;   /* upper limit on a view measre bound */

enum { smallestParent, prefixedParent, sharedSortParent, noneParent };

static const char* adcKeyword[]={
  "attrNum",
  "measuresNum",
  "tuplesNum",
  "INVERSE_ENDIAN",
  "fileName",
  "class",
  NULL
};

typedef struct ADCpar {
  int ndid;
  int dim;
  int mnum;
  long long int tuplenum;
  int inverse_endian;
  const char *filename;
  char clss;
} ADC_PAR, ADCPar;

typedef struct {
    int32 ndid;
   char   clss;
   char          adcName[MAX_FILE_FULL_PATH_SIZE];
   char   adcInpFileName[MAX_FILE_FULL_PATH_SIZE];
   uint32 nd; 
   uint32 nm;
   uint32 nInputRecs;
   uint32 memoryLimit;
   uint32 nTasks;
   /*  FILE *statf; */
} ADC_VIEW_PARS;

typedef struct job_pool { 
   uint32 grpb; 
   uint32 nv;
   uint32 nRows; 
    int64 viewOffset; 
} JOB_POOL;

typedef struct layer {
   uint32 layerIndex;
   uint32 layerQuantityLimit;
   uint32 layerCurrentPopulation;
} LAYER;

typedef struct chunks {
   uint32 curChunkNum;
    int64 chunkOffset;
   uint32 posSubChunk;
   uint32 curSubChunk;
} CHUNKS;

typedef struct tuplevsize {
    uint64 viewsize;
    uint64 tuple;
} TUPLE_VIEWSIZE;

typedef struct tupleones {
    uint32 nOnes;
    uint64 tuple;
} TUPLE_ONES;

typedef struct RBTree RBTree;

typedef struct ADC_VIEW_CNTL {
   char adcName[MAX_FILE_FULL_PATH_SIZE];
   uint32 retCode;
   uint32 verificationFailed;
   uint32 swapIt;
   uint32 nTasks;
   uint32 taskNumber;
    int32 ndid;

   uint32 nTopDims; /* given number of dimension attributes */
   uint32 nm;       /* number of measures */ 
   uint32 nd;       /* number of parent's dimensions */
   uint32 nv;       /* number of child's dimensions */

   uint32 nInputRecs;
   uint32 nViewRows; 
   uint32 totalOfViewRows;
   uint32 nParentViewRows;

    int64 viewOffset;
    int64 accViewFileOffset;

   uint32 inpRecSize;
   uint32 outRecSize;

   uint32 memoryLimit;
 unsigned char * memPool;
   uint32 * inpDataBuffer;

   RBTree *tree;

   uint32 numberOfChunks;
   CHUNKS *chunksParams;

     char       adcLogFileName[MAX_FILE_FULL_PATH_SIZE];
     char          inpFileName[MAX_FILE_FULL_PATH_SIZE];
     char         viewFileName[MAX_FILE_FULL_PATH_SIZE];
     char       chunksFileName[MAX_FILE_FULL_PATH_SIZE];
     char      groupbyFileName[MAX_FILE_FULL_PATH_SIZE];
     char adcViewSizesFileName[MAX_FILE_FULL_PATH_SIZE];
     char    viewSizesFileName[MAX_FILE_FULL_PATH_SIZE];

     FILE *logf;
     FILE *inpf;
     FILE *viewFile;   
     FILE *fileOfChunks;
     FILE *groupbyFile;
     FILE *adcViewSizesFile;
     FILE *viewSizesFile;
   
    int64     mSums[MAX_NUM_OF_MEAS];
   uint32 selection[MAX_NUM_OF_DIMS];
    int64 checksums[MAX_NUM_OF_MEAS]; /* view checksums */
    int64 totchs[MAX_NUM_OF_MEAS];    /* checksums of a group of views */

 JOB_POOL *jpp;
    LAYER *lpp;
   uint32 nViewLimit;
   uint32 groupby;
   uint32 smallestParentLevel;
   uint32 parBinRepTuple;
   uint32 nRowsToRead;
   uint32 fromParent;

   uint64 totalViewFileSize; /* in bytes */
   uint32 numberOfMadeViews;
   uint32 numberOfViewsMadeFromInput;
   uint32 numberOfPrefixedGroupbys;
   uint32 numberOfSharedSortGroupbys;
} ADC_VIEW_CNTL;

typedef struct Factorization{
  long int *mlt;
  long int *exp;
  long int dim;
} Factorization;


void swap4(void *num);
void swap8(void *num);
void initADCpar(ADC_PAR *par);
void ShowFactorization(Factorization *nmbfct);
long int ListFirstPrimes(long int mpr,long int *prlist);
long long int GetLCM(long long int mask, Factorization **fctlist, long int *adcexpons);
void ExtendFactors(long int nmb,long int firstdiv, Factorization *nmbfct,Factorization **fctlist);
void GetFactorization(long int prnum,long int *prlist, Factorization **fctlist);
int CompareSizesByValue(const void* sz0, const void* sz1);
int CompareViewsBySize(const void* vw0, const void* vw1);
int CalculateVeiwSizes(ADC_PAR *par);
int ParseParFile(char* parfname,ADC_PAR *par);
int WriteADCPar(ADC_PAR *par,char* fname);
void ShowADCPar(ADC_PAR *par);
int GetNextTuple(int dcdim, int measnum, long long int* attr, long long int* meas, char clss);
int GenerateADC(ADC_PAR *par);




#endif
