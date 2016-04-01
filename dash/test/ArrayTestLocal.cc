#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "ArrayTestLocal.h"
#include <iostream>

using namespace std;

void foo(dash::Array<DGNode>& nodes)
{
  cout << "size of nodes is " << nodes.size() << endl;
}

TEST_F(ArrayTestLocal, LocalArrayTest)
{
  size_t array_size = _dash_size;
  // Create array instances using varying constructor options
  LOG_MESSAGE("Array size: %d", array_size);
  // Initialize arrays
  LOG_MESSAGE("Initialize arr1");
  dash::Array<DGNode> arr1(array_size);
  // Check array sizes
  ASSERT_EQ(array_size, arr1.size());
  // Fill arrays with incrementing values
  if(_dash_id == 0) {
    LOG_MESSAGE("Assigning array values");
    char name[MAX_LEN];
    for(size_t i = 0; i < array_size; ++i) {
      sprintf(name, "Process %ld", i);
      DGNode node(name);
      node.address = i;
      if (_dash_id == _dash_size - 1) {
        for (size_t i = 0; i < 4; ++i) {
          node.out[i] = 0;
          node.in[i] = i;
        }
      } else {
        node.out[0] = _dash_size - 1;
      }
      arr1[i] = node;
    }
  }

  // Units waiting for value initialization
  arr1.barrier();

  foo(arr1);

  //DGNode &nd = arr1.local[0];
  //RandomFeatures((char*) "GraphTest", fielddim, nd);
  //printf("Before Loop in ProcessNodes --> nd.size: %d, featlen: %d, address item 0: %d, outDegree item 0: %d, inDegree item 0: %d, name item 0: %s\n", arr1.local.size(), nd.feat.len, nd.address, nd.outDegree, nd.inDegree, nd.name);
}

double  randlc(double *X, double *A)
{
  static int        KS=0;
  static double R23, R46, T23, T46;
  double    T1, T2, T3, T4;
  double    A1;
  double    A2;
  double    X1;
  double    X2;
  double    Z;
  int         i, j;

  if (KS == 0)
  {
    R23 = 1.0;
    R46 = 1.0;
    T23 = 1.0;
    T46 = 1.0;

    for (i=1; i<=23; i++)
    {
      R23 = 0.50 * R23;
      T23 = 2.0 * T23;
    }
    for (i=1; i<=46; i++)
    {
      R46 = 0.50 * R46;
      T46 = 2.0 * T46;
    }
    KS = 1;
  }

  /*  Break A into two parts such that A = 2^23 * A1 + A2 and set X = N.  */

  T1 = R23 **A;
  j  = T1;
  A1 = j;
  A2 = *A - T23 * A1;

  /*  Break X into two parts such that X = 2^23 * X1 + X2, compute
   *      Z = A1 * X2 + A2 * X1  (mod 2^23), and then
   *          X = 2^23 * Z + A2 * X2  (mod 2^46).                            */

  T1 = R23 **X;
  j  = T1;
  X1 = j;
  X2 = *X - T23 * X1;
  T1 = A1 * X2 + A2 * X1;

  j  = R23 * T1;
  T2 = j;
  Z = T1 - T23 * T2;
  T3 = T23 * Z + A2 * X2;
  j  = R46 * T3;
  T4 = j;
  *X = T3 - T46 * T4;
  return(R46 **X);
}

int GetFNumDPar(int* mean, int* stdev) {
  *mean=NUM_SAMPLES;
  *stdev=STD_DEVIATION;
  return 0;
}

int ipowMod(int a,long long int n,int md) {
  int seed=1,q=a,r=1;
  if(n<0) {
    fprintf(stderr,"ipowMod: exponent must be nonnegative exp=%lld\n",n);
    n=-n; /* temp fix */
    /*    return 1; */
  }
  if(md<=0) {
    fprintf(stderr,"ipowMod: module must be positive mod=%d",md);
    return 1;
  }
  if(n==0) return 1;
  while(n>1) {
    int n2 = n/2;
    if (n2*2==n) {
      seed = (q*q)%md;
      q=seed;
      n = n2;
    } else {
      seed = (r*q)%md;
      r=seed;
      n = n-1;
    }
  }
  seed = (r*q)%md;
  return seed;
}

int GetFeatureNum(char *mbname,int id) {
  double tran=314159265.0;
  double A=2*id+1;
  double denom=randlc(&tran,&A);
  char cval='S';
  int mean=NUM_SAMPLES,stdev=128;
  int rtfs=0,len=0;
  GetFNumDPar(&mean,&stdev);
  rtfs=ipowMod((int)(1/denom)*(int)cval,(long long int) (2*id+1),2*stdev);
  if(rtfs<0) rtfs=-rtfs;
  len=mean-stdev+rtfs;
  return len;
}
void RandomFeatures(char *bmname,int fdim, DGNode& nd) {
  int len=GetFeatureNum(bmname,nd.id)*fdim;
  nd.feat.len=len;
  //cout << "Feature length:  " << nd.feat.len << ", calculated length: " << len << ", Address: " << nd.address << endl;
  int nxg=2,nyg=2,nzg=2,nfg=5;
  int nx=421,ny=419,nz=1427,nf=3527;
  long long int expon=(len*(nd.id+1))%3141592;
  int seedx=ipowMod(nxg,expon,nx),
      seedy=ipowMod(nyg,expon,ny),
      seedz=ipowMod(nzg,expon,nz),
      seedf=ipowMod(nfg,expon,nf);
  int i=0;

  for(i=0; i<len; i+=fdim) {
    seedx=(seedx*nxg)%nx;
    seedy=(seedy*nyg)%ny;
    seedz=(seedz*nzg)%nz;
    seedf=(seedf*nfg)%nf;
    nd.feat.val[i]=seedx;
    nd.feat.val[i+1]=seedy;
    nd.feat.val[i+2]=seedz;
    nd.feat.val[i+3]=seedf;
  }

  //cout << "Feature length:  " << nd->feat.len << ", calculated length: " << len  << ", Address: " << nd->address << endl;
}
