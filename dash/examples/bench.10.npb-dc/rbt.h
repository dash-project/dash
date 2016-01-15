#ifndef RBT_H_INCLUDED
#define RBT_H_INCLUDED

typedef int                 int32;
typedef unsigned int       uint32;
typedef long long           int64;
typedef unsigned long long uint64;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "adc.h"
#include "macrodef.h"

#define MAX_TREE_HEIGHT	64
enum{BLACK,RED};

typedef struct treeNode{
  struct treeNode *left;
  struct treeNode *right;
  uint32 clr;
  int64 nodeMemPool[1];
} treeNode;

typedef struct RBTree{
  treeNode root;	
  treeNode * mp;
  uint32 count;       
  uint32 treeNodeSize;
  uint32 nodeDataSize;
  uint32 memoryLimit; 
  uint32 memaddr;
  uint32 memoryIsFull;
  uint32 freeNodeCounter;
  uint32 nNodesLimit;
  uint32 nd;
  uint32 nm;
  uint32   *drcts;
  treeNode **nodes;
  unsigned char * memPool;
} RBTree;

#define NEW_TREE_NODE(node_ptr,memPool,memaddr,treeNodeSize, \
 freeNodeCounter,memoryIsFull) \
 node_ptr=(struct treeNode*)(memPool+memaddr); \
 memaddr+=treeNodeSize; \
 (freeNodeCounter)--; \
 if( freeNodeCounter == 0 ) { \
     memoryIsFull = 1; \
 }

typedef struct ADC_VIEW_CNTL ADC_VIEW_CNTL;

int32 KeyComp( const uint32 *a, const uint32 *b, uint32 n );
int32 TreeInsert(RBTree *tree, uint32 *attrs);
int32 WriteViewToDisk(ADC_VIEW_CNTL *avp, treeNode *t);
int32 WriteViewToDiskCS(ADC_VIEW_CNTL *avp, treeNode *t,uint64 *ordern);
int32 computeChecksum(ADC_VIEW_CNTL *avp, treeNode *t,uint64 *ordern);
int32 WriteChunkToDisk(uint32 recordSize,FILE *fileOfChunks, treeNode *t, FILE *logFile);
RBTree * CreateEmptyTree(uint32 nd, uint32 nm, uint32 memoryLimit, unsigned char * memPool);
void InitializeTree(RBTree *tree, uint32 nd, uint32 nm);
int32 DestroyTree(RBTree *tree);

#endif
