#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>


#define PAL_DASH
#include "../../include/dash/omp/PAL.h"


typedef int                 int32;
typedef unsigned int       uint32;
typedef long long           int64;
typedef unsigned long long uint64;


extern "C" {
    #include "wtime.h"
    #include "macrodef.h"
    #include "adc.h"
    #include "rbt.h"
    #include "jobcntl.h"
    #include "extbuild.h"
    #include "npbparams.h"
}




int dc_main (int argc, char * argv[]);
int32 DC(ADC_VIEW_PARS *adcpp);
int Verify(long long int checksum, ADC_VIEW_PARS *adcpp);
void c_print_results(const char *name, const char clss, int n1, int n2, int n3, int niter, double t, double mops, const char *optype, int verification, long long int checksum, unsigned int np, const char *npbversion, const char   *compiletime, const char *cc, const char *clink, const char *c_lib, const char *c_inc, const char *cflags, const char *clinkflags);
void check_line(char *line, char *label, char *val);



int main(int argc, char *argv[]) {
  int res;
  PAL_INIT
  res = dc_main(argc, argv);
  PAL_FINALIZE
  return res;
}






int dc_main(int argc, char * argv[]) 
{
  ADC_VIEW_PARS *adcpp; // DASH_SHARED
  ADC_PAR *parp;        // DASH_PRIVATE_ONLY_MASTER
  int32 retCode;        // DASH_PRIVATE

  PAL_SEQUENTIAL_BEGIN

  // DASH_NOTE: only master, just printing
  fprintf(stderr,"\n\n NAS Parallel Benchmarks (NPB3.2-OMP) - DC Benchmark\n\n" );
  if(argc != 3){
    fprintf(stderr,"No Paramter file. Using compiled defaults\n");
  }
  if(argc < 2){
    fprintf(stderr,"Usage: <program name> <amount of memory>\n");
    fprintf(stderr,"       <file of parameters>\n");
    fprintf(stderr,"Example: bin/dc.S 1000000 DC/ADC.par\n");
    fprintf(stderr,"The last argument, (a parameter file) can be skipped\n");
    exit(1);
  }

  PAL_SEQUENTIAL_END

  // DASH_NOTE: find way to share values
  if (  !(parp = (ADC_PAR*) malloc(sizeof(ADC_PAR)))
      ||!(adcpp = (ADC_VIEW_PARS*) malloc(sizeof(ADC_VIEW_PARS)))) {
    PutErrMsg("main: malloc failed")
    exit(1);
  }

  // DASH_NOTE: should be okay if only master does this and presets the values globally
  initADCpar(parp);
  parp->clss=CLASS;
  if (argc <= 2){
    parp->dim = attrnum;
    parp->tuplenum = input_tuples;    
  } else if ((argc == 3) && (!ParseParFile(argv[2], parp))) {
    PutErrMsg("main.ParseParFile failed")
    exit(1);
  }

  PAL_SEQUENTIAL_BEGIN

  // DASH_NOTE: only master, just printing
  ShowADCPar(parp); 

  if (!GenerateADC(parp)) {
    PutErrMsg("main.GenerateAdc failed")
    exit(1);
  }

  PAL_SEQUENTIAL_END

  // DASH_NOTE: master should set this globally so all threads have the same values
  adcpp->ndid = parp->ndid;  
  adcpp->clss = parp->clss;
  adcpp->nd = parp->dim;
  adcpp->nm = parp->mnum;
  adcpp->nTasks = 1;

  if (argc == 2) {
    adcpp->memoryLimit = atoi(argv[1]);
  } else {
    adcpp->memoryLimit = parp->tuplenum*(40+4*parp->dim); //size of rb-tree with tuplenum nodes
    fprintf(stderr,"Estimated rb-tree size=%ld \n", adcpp->memoryLimit);
  }
  adcpp->nInputRecs = parp->tuplenum;
  strcpy(adcpp->adcName, parp->filename);
  strcpy(adcpp->adcInpFileName, parp->filename);

  //PAL_SEQUENTIAL_END
  PAL_BARRIER

  retCode = DC(adcpp);

  if (retCode) {
     PutErrMsg("main.DC failed")
     fprintf(stderr, "main.ParRun failed: retcode = %d\n", retCode);
     exit(1);
  }

  //PAL_SEQUENTIAL_BEGIN

  // DASH_NOTE: maybe not needed after dash recoding
  if (parp)  { free(parp);   parp = 0; }
  if (adcpp) { free(adcpp); adcpp = 0; }

  //PAL_SEQUENTIAL_END

  return 0;
}

/*
int32    CloseAdcView(ADC_VIEW_CNTL *adccntl);  
int32    PartitionCube(ADC_VIEW_CNTL *avp);				
ADC_VIEW_CNTL *NewAdcViewCntl(ADC_VIEW_PARS *adcpp, uint32 pnum);
int32    ComputeGivenGroupbys(ADC_VIEW_CNTL *adccntl);
*/

#ifdef ALTIX_24
#include <cpumemsets.h>
int pinit(int lpid){
  cpumemset_t *prset;
  prset = cmsQueryCMS(CMS_CURRENT,(pid_t)0,(void *)0);
  if(!prset){
    perror("cmsQueryCMS");
    fprintf(stderr,"pinit: can't query CMS\n");
    cmsFreeCMS(prset);
    return -1;
  }
  int lcpu=prset->nr_cpus-1-lpid;
  if(lcpu<0){
    fprintf(stderr,"pinit: can't bind %ld-th thread: only %d cpus availble.\n",
                    lpid,prset->nr_cpus);
    return -1;
  }
  int cpuNum = prset->cpus[lcpu];
  if(cpubind(cpuNum)) {
    perror("cpubind");
    fprintf(stderr,"pinit: can't bind cpu %d\n",lcpu);
  }
  cmsFreeCMS(prset);
  return 0;
}
#endif /* ALTIX_24 */





int32 DC(ADC_VIEW_PARS *adcpp) {  // DASH_SHARED?
  ADC_VIEW_CNTL *adccntlp = NULL; // DASH_PRIVATE
  time_t tm;                      // DASH_PRIVATE
  //double tm0 = 0;                 // DASH_SHARED
  PAL_SHARED_VAR_DECL(tm0, double)
  PAL_SHARED_VAR_SET(tm0, 0.0)
  //double t_total = 0.0;           // DASH_SHARED
  PAL_SHARED_VAR_DECL(t_total, double)
  PAL_SHARED_VAR_SET(t_total, 0.0)
  struct tm *tmptr = NULL;        // DASH_PRIVATE_ONLY_MASTER
  const int LL = 400;             // DASH_PRIVATE_ONLY_MASTER
  char execdate[LL];              // DASH_PRIVATE_ONLY_MASTER

  typedef struct { 
    uint32 verificationFailed;
    uint32 totalViewTuples;
    uint64 totalViewSizesInBytes;
    uint32 totalNumberOfMadeViews;
    uint64 checksum;
    double tm_max;
  } PAR_VIEW_ST;

  //PAR_VIEW_ST *pvstp;             // DASH_SHARED
  PAL_SHARED_VAR_PTR_DECL(pvstp, PAR_VIEW_ST)

  //pvstp = (PAR_VIEW_ST*) malloc(sizeof(PAR_VIEW_ST));
  //pvstp->verificationFailed = 0;
  //pvstp->totalViewTuples = 0;
  //pvstp->totalViewSizesInBytes = 0;
  //pvstp->totalNumberOfMadeViews = 0;
  //pvstp->checksum = 0;

  PAL_SHARED_VAR_PTR_ALLOC(pvstp, PAR_VIEW_ST)
  PAL_SHARED_STRUCT_PTR_MEMBER(verificationFailed, uint32, pvstp, PAR_VIEW_ST) = 0;
  PAL_SHARED_STRUCT_PTR_MEMBER(totalViewTuples, uint32, pvstp, PAR_VIEW_ST) = 0;
  PAL_SHARED_STRUCT_PTR_MEMBER(totalViewSizesInBytes, uint64, pvstp, PAR_VIEW_ST) = 0;
  PAL_SHARED_STRUCT_PTR_MEMBER(totalNumberOfMadeViews, uint32, pvstp, PAR_VIEW_ST) = 0;
  PAL_SHARED_STRUCT_PTR_MEMBER(checksum, uint64, pvstp, PAR_VIEW_ST) = 0;

  adcpp->nTasks = PAL_NUM_THREADS;
  //PAL_SHARED_STRUCT_PTR_MEMBER(nTasks, uint32, adcpp, ADC_VIEW_PARS) = PAL_NUM_THREADS;

  PAL_CRITICAL_INIT(crit1)
  PAL_CRITICAL_INIT(crit2)

  PAL_PARALLEL_BEGIN(shared(pvstp) private(adccntlp,tm))

  PAL_MASTER_BEGIN
  fprintf(stderr, "max_threads=%d\n", PAL_MAX_THREADS);
  fprintf(stderr, "num_threads=%d\n", PAL_NUM_THREADS);
  PAL_MASTER_END
  
  int itsk=0;
  
  PAL_CRITICAL_BEGIN(crit1)
  {
    itsk = PAL_THREAD_NUM;
    #ifdef ALTIX_24
    pinit(itsk);
    #endif
  }
  PAL_CRITICAL_END

  // DASH_NOTE: should not need any sharing except global vars, check
  adccntlp = NewAdcViewCntl(adcpp, itsk);

  if (!adccntlp) { 
    PutErrMsg("ParRun.NewAdcViewCntl: returned NULL")
    adccntlp->verificationFailed=1;
  } else {
    adccntlp->verificationFailed = 0;
    if (adccntlp->retCode!=0) {
      fprintf(stderr, 
              "DC.NewAdcViewCntl: return code = %d\n",  
              adccntlp->retCode); 
    }
  }

  if (!adccntlp->verificationFailed) {
    // DASH_NOTE: should not need any sharing except global vars, check
    if (PartitionCube(adccntlp)) {
      PutErrMsg("DC.PartitionCube failed");
    }
    timer_clear(itsk);
    timer_start(itsk);

    // DASH_NOTE: should not need any sharing except global vars, check
    if (ComputeGivenGroupbys(adccntlp)) {
      PutErrMsg("DC.ComputeGivenGroupbys failed");
    }
    timer_stop(itsk);
    //tm0 = timer_read(itsk);
    PAL_SHARED_VAR_SET(tm0, timer_read(itsk))
  }

  PAL_CRITICAL_BEGIN(crit2)
  {
    //if (pvstp->tm_max<tm0) pvstp->tm_max = tm0;
    if (PAL_SHARED_STRUCT_PTR_MEMBER(tm_max, double, pvstp, PAR_VIEW_ST) < PAL_SHARED_VAR_RVAL(tm0, double))
      PAL_SHARED_STRUCT_PTR_MEMBER(tm_max, double, pvstp, PAR_VIEW_ST) = PAL_SHARED_VAR_RVAL(tm0, double);
    
    //pvstp->verificationFailed += adccntlp->verificationFailed;
    PAL_SHARED_STRUCT_PTR_MEMBER(verificationFailed, uint32, pvstp, PAR_VIEW_ST) += adccntlp->verificationFailed;

    if (!adccntlp->verificationFailed) {
      //pvstp->totalNumberOfMadeViews += adccntlp->numberOfMadeViews;
      PAL_SHARED_STRUCT_PTR_MEMBER(totalNumberOfMadeViews, uint32, pvstp, PAR_VIEW_ST) += adccntlp->numberOfMadeViews;
      //pvstp->totalViewSizesInBytes += adccntlp->totalViewFileSize;
      PAL_SHARED_STRUCT_PTR_MEMBER(totalViewSizesInBytes, uint64, pvstp, PAR_VIEW_ST) += adccntlp->totalViewFileSize;
      //pvstp->totalViewTuples += adccntlp->totalOfViewRows;
      PAL_SHARED_STRUCT_PTR_MEMBER(totalViewTuples, uint32, pvstp, PAR_VIEW_ST) += adccntlp->totalOfViewRows;
      //pvstp->checksum += adccntlp->totchs[0];
      PAL_SHARED_STRUCT_PTR_MEMBER(checksum, uint64, pvstp, PAR_VIEW_ST) += adccntlp->totchs[0];
    }
  }
  PAL_CRITICAL_END

  //t_total = pvstp->tm_max;
  PAL_SHARED_VAR_SET(t_total, PAL_SHARED_STRUCT_PTR_MEMBER(tm_max, double, pvstp, PAR_VIEW_ST))
  // DASH_NOTE: should not need any sharing except global vars, check
  if (CloseAdcView(adccntlp)) {
    PutErrMsg("ParRun.CloseAdcView: is failed");
    adccntlp->verificationFailed = 1;
  }

  PAL_PARALLEL_END
  PAL_SEQUENTIAL_BEGIN
 
  //pvstp->verificationFailed = Verify(pvstp->checksum,adcpp);
  PAL_SHARED_STRUCT_PTR_MEMBER(verificationFailed, uint32, pvstp, PAR_VIEW_ST) = Verify(PAL_SHARED_STRUCT_PTR_MEMBER(checksum, uint64, pvstp, PAR_VIEW_ST), adcpp);
  //if (pvstp->verificationFailed)
  if (PAL_SHARED_STRUCT_PTR_MEMBER(verificationFailed, uint32, pvstp, PAR_VIEW_ST))
    fprintf(stderr, "Verification failed\n");

  time(&tm);
  tmptr = localtime(&tm);
  strftime(execdate, (size_t)LL, "%d %b %Y", tmptr);

  c_print_results((char *)"DC",
                  adcpp->clss,
                  (int)adcpp->nd,
                  (int)adcpp->nm,
                  (int)adcpp->nInputRecs,
                  //pvstp->totalNumberOfMadeViews,
                  PAL_SHARED_STRUCT_PTR_MEMBER(totalNumberOfMadeViews, uint32, pvstp, PAR_VIEW_ST),
                  //t_total,
                  PAL_SHARED_VAR_RVAL(t_total, double),
                  //(double) pvstp->totalViewTuples, 
                  (double) PAL_SHARED_STRUCT_PTR_MEMBER(totalViewTuples, uint32, pvstp, PAR_VIEW_ST),
                  (char *)"Tuples generated", 
                  //(int)(pvstp->verificationFailed),
                  (int) PAL_SHARED_STRUCT_PTR_MEMBER(verificationFailed, uint32, pvstp, PAR_VIEW_ST),
                  //(long long int) pvstp->checksum,
                  (long long int) PAL_SHARED_STRUCT_PTR_MEMBER(checksum, uint64, pvstp, PAR_VIEW_ST),
                  (unsigned int) adcpp->nTasks,
                  //(unsigned int) PAL_SHARED_STRUCT_PTR_MEMBER(nTasks, uint32, adcpp, ADC_VIEW_PARS),
                  (char *) NPBVERSION,
                  (char *) execdate,
                  (char *) CC,
                  (char *) CLINK,
                  (char *) C_LIB,
                  (char *) C_INC,
                  (char *) CFLAGS,
                  (char *) CLINKFLAGS);

  PAL_SEQUENTIAL_END
  PAL_BARRIER

  PAL_SHARED_VAR_PTR_FREE(pvstp)

  return ADC_OK;
}

long long checksumS   = 464620213;
long long checksumWlo = 434318;
long long checksumWhi = 1401796;
long long checksumAlo = 178042;
long long checksumAhi = 7141688;
long long checksumBlo = 700453;
long long checksumBhi = 9348365;

int Verify(long long int checksum, ADC_VIEW_PARS *adcpp){
  switch(adcpp->clss){
    case 'S':
      if (checksum == checksumS) return 0;
      break;
    case 'W':
      if (checksum == checksumWlo+1000000*checksumWhi) return 0;
      break;
    case 'A':
      if (checksum == checksumAlo+1000000*checksumAhi) return 0;
      break;
    case 'B':
      if (checksum == checksumBlo+1000000*checksumBhi) return 0;
      break;
    default:
      return -1; /* CLASS U */
  }
  return 1;
}

void c_print_results( const char   *name,
                      const char   clss,
                      int    n1, 
                      int    n2,
                      int    n3,
                      int    niter,
                      double t,
                      double mops,
                      const char   *optype,
                      int    verification,
                      long long int checksum,
                      unsigned int np,
                      const char   *npbversion,
                      const char   *compiletime,
                      const char   *cc,
                      const char   *clink,
                      const char   *c_lib,
                      const char   *c_inc,
                      const char   *cflags,
                      const char   *clinkflags ){
    #ifdef _OPENMP
    int num_threads, max_threads;
    const char *num_threads_set;

    /* figure out number of threads used */
    max_threads = PAL_MAX_THREADS;
    num_threads = PAL_NUM_THREADS;
    #endif

    fprintf(stdout,"\n\n %s Benchmark Completed\n", name ); 
    fprintf(stdout," Class           =                %c\n", clss );
    fprintf(stdout," Dimensions      =              %3d\n", n1);
    fprintf(stdout," Measures        =              %3d\n", n2);
    fprintf(stdout," Input Tuples    =     %12d\n", n3);
    fprintf(stdout," Tuples Generated=     %12.0f\n", mops);
    fprintf(stdout," Number of views =     %12d\n", niter );
    // Always wrong since not called in parallel region...
    //fprintf(stdout," Total threads   =     %12d\n", num_threads);
    #ifdef _OPENMP
    num_threads_set = getenv("OMP_NUM_THREADS");
    if (!num_threads_set) num_threads_set = "unset";
    fprintf(stdout," Request threads =     %12s\n", num_threads_set);
    #endif
    //if (num_threads != max_threads)
    //  fprintf(stdout," Warning: Threads used differs from threads available\n");
    fprintf(stdout," Time in seconds =     %12.2f\n", t );
    fprintf(stdout," Tuples/s        =     %12.2f\n", mops/t );
    fprintf(stdout," Operation type  = %s\n", optype);
    if( verification==0 )
      fprintf(stdout," Verification    =       SUCCESSFUL\n" );
    else if( verification==-1)
      fprintf(stdout," Verification    =    NOT PERFORMED\n" );
    else
      fprintf(stdout," Verification    =     UNSUCCESSFUL\n" );
    fprintf(stdout," Checksum        =%17lld\n", checksum);
    if (np>1) 
      fprintf(stdout," Processes       =     %12d\n", np);
    fprintf(stdout," Version         =     %12s\n", npbversion );
    fprintf(stdout," Execution date  =     %12s\n", compiletime );
    fprintf(stdout,"\n Compile options:\n" );
    fprintf(stdout,"    CC           = %s\n", cc );
    fprintf(stdout,"    CLINK        = %s\n", clink );
    fprintf(stdout,"    C_LIB        = %s\n", c_lib );
    fprintf(stdout,"    C_INC        = %s\n", c_inc );
    fprintf(stdout,"    CFLAGS       = %s\n", cflags );
    fprintf(stdout,"    CLINKFLAGS   = %s\n", clinkflags );
#ifdef SMP
    evalue = getenv("MP_SET_NUMTHREADS");
    fprintf(stdout,"   MULTICPUS = %s\n", evalue );
#endif
    fprintf(stdout,"\n Please send all errors/feedbacks to:\n\n" );
    fprintf(stdout," NPB Development Team\n" );
    fprintf(stdout," npb@nas.nasa.gov\n\n" );
}

#define DEFFILE "../config/make.def"
void check_line(char *line, char *label, char *val){
  char *original_line;
  original_line = line;
  /* compare beginning of line and label */
  while (*label != '\0' && *line == *label) {
    line++; label++; 
  }
  /* if *label is not EOS, we must have had a mismatch */
  if (*label != '\0') return;
  /* if *line is not a space, actual label is longer than test label */
  if (!isspace(*line) && *line != '=') return ; 
  /* skip over white space */
  while (isspace(*line)) line++;
  /* next char should be '=' */
  if (*line != '=') return;
  /* skip over white space */
  while (isspace(*++line));
  /* if EOS, nothing was specified */
  if (*line == '\0') return;
  /* finally we've come to the value */
  strcpy(val, line);
  /* chop off the newline at the end */
  val[strlen(val)-1] = '\0';
  if (val[strlen(val) - 1] == '\\') {
    fprintf(stderr,"\n\
      check_line: Error in file %s. Because by historical reasons\n\
      you can't have any continued\n\
      lines in the file make.def, that is, lines ending\n\
      with the character \"\\\". Although it may be ugly, \n\
      you should be able to reformat without continuation\n\
      lines. The offending line is\n %s\n", DEFFILE, original_line);
    exit(1);
  }
}


