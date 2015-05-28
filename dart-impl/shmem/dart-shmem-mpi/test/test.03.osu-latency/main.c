#ifdef _ENABLE_CUDA_
    #define BENCHMARK "OSU MPI-CUDA Latency Test"
#else
    #define BENCHMARK "OSU MPI Latency Test"
#endif
/*
 * Copyright (C) 2002-2012 the Network-Based Computing Laboratory
 * (NBCL), The Ohio State University. 
 *
 * Contact: Dr. D. K. Panda (panda@cse.ohio-state.edu)
 *
 * For detailed copyright and licensing information, please refer to the
 * copyright file COPYRIGHT in the top level OMB directory.
 */

#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define MESSAGE_ALIGNMENT 64
#define MAX_MSG_SIZE (1<<10)
#define MYBUFSIZE (MAX_MSG_SIZE + MESSAGE_ALIGNMENT)
#define SKIP_LARGE  10
#define LOOP_LARGE  100
#define LARGE_MESSAGE_SIZE  8192


#ifdef _ENABLE_CUDA_
#include <cuda.h>
#include <cuda_runtime.h>
#endif

char s_buf_original[MYBUFSIZE];
char r_buf_original[MYBUFSIZE];

int skip = 1000;
int loop = 10000;
#ifdef PACKAGE_VERSION
#   define HEADER "# " BENCHMARK " v" PACKAGE_VERSION "\n"
#else
#   define HEADER "# " BENCHMARK "\n"
#endif

#ifndef FIELD_WIDTH
#   define FIELD_WIDTH 20
#endif

#ifndef FLOAT_PRECISION
#   define FLOAT_PRECISION 2
#endif

int main(int argc, char *argv[])
{
    int myid, numprocs, i;
    int size;
    MPI_Status reqstat;
    char *s_buf, *r_buf;
    int align_size;
    double t_start = 0.0, t_end = 0.0;
#ifdef _ENABLE_CUDA_
    char *str = NULL;
    int dev_id, local_rank, dev_count;
    char *s_buf_rev = NULL;
    char *r_buf_rev = NULL;
    char src, desti;
    char *sender;
    char *receiver;
    cudaError_t  cuerr = cudaSuccess;
    CUresult curesult = CUDA_SUCCESS;
    CUcontext cuContext;
    CUdevice cuDevice;

    if (3 != argc && 1 != argc) {
        fprintf(stdout, "Enter source and destination type.\n"
            "FORMAT: EXE SOURCE DESTINATION, where SOURCE and DESTINATION can be either of D or H\n");
        return EXIT_FAILURE;
    } else if (1 == argc) {
        src = 'H';
        desti = 'H';
    } else {
        src = argv[1][0];
        desti = argv[2][0];
    }
#endif

#ifdef _ENABLE_CUDA_
    dev_id = 0;
    if ((str = getenv("LOCAL_RANK")) != NULL) {
        cudaGetDeviceCount(&dev_count);
        local_rank = atoi(str);
        dev_id = local_rank % dev_count;
    }
    curesult = cuInit(0);
    if (curesult != CUDA_SUCCESS) {
        return EXIT_FAILURE;
    }
    curesult = cuDeviceGet(&cuDevice, dev_id);
    if (curesult != CUDA_SUCCESS) {
        return EXIT_FAILURE;
    }
    curesult = cuCtxCreate(&cuContext, 0, cuDevice);
    if (curesult != CUDA_SUCCESS) {
        return EXIT_FAILURE;
    }
#endif
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    if(numprocs != 2) {
        if(myid == 0) {
            fprintf(stderr, "This test requires exactly two processes\n");
        }

        MPI_Finalize();

        return EXIT_FAILURE;
    }

    align_size = MESSAGE_ALIGNMENT;

/**************Allocating Memory*********************/

#ifdef _ENABLE_CUDA_
    if (src == 'D' && desti == 'D'){
        sender = "Send Buffer on DEVICE (D)";
        cuerr = cudaMalloc((void**) &s_buf, MYBUFSIZE);
        if (cudaSuccess != cuerr){
            fprintf(stderr, "Could not allocate device memory\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        receiver = "Receive Buffer on DEVICE (D)";
        cuerr = cudaMalloc((void**) &r_buf, MYBUFSIZE);
        if (cudaSuccess != cuerr){
            fprintf(stderr, "Could not allocate device memory\n");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

    } else if (src == 'H' && desti == 'D'){
        sender = "Send Buffer on HOST (H)";
        receiver = "Receive Buffer on DEVICE (D)";
        if (0 == myid){
            s_buf =
                (char *) (((unsigned long) s_buf_original + (align_size - 1)) /
                    align_size * align_size);
            r_buf_rev =
                (char *) (((unsigned long) s_buf_original + (align_size - 1)) /
                    align_size * align_size);
        } else if (1 == myid) {
            cuerr = cudaMalloc((void**) &r_buf, MYBUFSIZE);
            if (cudaSuccess != cuerr){
                fprintf(stderr, "Could not allocate device memory\n");
                MPI_Finalize();
                return EXIT_FAILURE;
            }
            cuerr = cudaMalloc((void**) &s_buf_rev, MYBUFSIZE);
            if (cudaSuccess != cuerr){
                fprintf(stderr, "Could not allocate device memory\n");
                MPI_Finalize();
                return EXIT_FAILURE;
            }
        }

    } else if (src == 'D' && desti == 'H'){
        sender = "Send Buffer on DEVICE (D)";
        receiver = "Receive Buffer on HOST (H)";
        if (0 == myid) {
            cuerr = cudaMalloc((void**) &s_buf, MYBUFSIZE);
            if (cudaSuccess != cuerr){
                fprintf(stderr, "Could not allocate device memory\n");
                MPI_Finalize();
                return EXIT_FAILURE;
            }
            cuerr = cudaMalloc((void**) &r_buf_rev, MYBUFSIZE);
            if (cudaSuccess != cuerr){
                fprintf(stderr, "Could not allocate device memory\n");
                MPI_Finalize();
                return EXIT_FAILURE;
            }
        } else if (1 == myid) { 
            r_buf =
                (char *) (((unsigned long) r_buf_original + (align_size - 1)) /
                  align_size * align_size);
            s_buf_rev =
                (char *) (((unsigned long) r_buf_original + (align_size - 1)) /
                  align_size * align_size);
        }
    } else {
        sender = "Send Buffer on HOST (H)";
        receiver = "Receive Buffer on HOST (H)";
#endif
        s_buf =
            (char *) (((unsigned long) s_buf_original + (align_size - 1)) /
              align_size * align_size);

        r_buf =
            (char *) (((unsigned long) r_buf_original + (align_size - 1)) /
              align_size * align_size);
#ifdef _ENABLE_CUDA_
    }
#endif

/**************Memory Allocation Done*********************/

    if(myid == 0) {
        fprintf(stdout, HEADER);
#ifdef _ENABLE_CUDA_
        fprintf(stdout, "# %s and %s\n", sender, receiver);
#endif
        fprintf(stdout, "%-*s%*s\n", 10, "# Size", FIELD_WIDTH, "Latency (us)");
        fflush(stdout);
    }

    for(size = 0; size <= MAX_MSG_SIZE; size = (size ? size * 2 : 1)) {
        /* touch the data */
#ifdef _ENABLE_CUDA_
        if (src != 'D' && desti != 'D') {
#endif
            for(i = 0; i < size; i++) {
                s_buf[i] = 'a';
                r_buf[i] = 'b';
            }
#ifdef _ENABLE_CUDA_
        } else if (src == 'D' && desti == 'D') {
            cudaMemset(s_buf, 0, size);
            cudaMemset(r_buf, 1, size);
        } else if (src == 'D' && desti == 'H') {
            if (0 == myid) {
                cudaMemset(s_buf, 0, size);
                cudaMemset(r_buf_rev, 1, size);
            } else if (1 == myid) {
                for(i = 0; i < size; i++) {
                    r_buf[i] = 'b';
                    s_buf_rev[i] = 'a';
                }
            }
        } else if (src == 'H' && desti == 'D') {
            if (0 == myid) {
                for(i = 0; i < size; i++) {
                    s_buf[i] = 'a';
                    r_buf_rev[i] = 'b';
                }
            } else if (1 == myid) {
                cudaMemset(s_buf_rev, 0, size);
                cudaMemset(r_buf, 1, size);
            }
        }
#endif

        if(size > LARGE_MESSAGE_SIZE) {
            loop = LOOP_LARGE;
            skip = SKIP_LARGE;
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if(myid == 0) {
            for(i = 0; i < loop + skip; i++) {
                if(i == skip) t_start = MPI_Wtime();

                MPI_Send(s_buf, size, MPI_CHAR, 1, 1, MPI_COMM_WORLD);
#ifdef _ENABLE_CUDA_
                if ((src == 'H' && desti == 'D') || ( src == 'D' && desti == 'H')){
                    MPI_Recv(r_buf_rev, size, MPI_CHAR, 1, 1, MPI_COMM_WORLD, &reqstat);
                } else {
#endif
                    MPI_Recv(r_buf, size, MPI_CHAR, 1, 1, MPI_COMM_WORLD, &reqstat);
#ifdef _ENABLE_CUDA_
                }
#endif
            }

            t_end = MPI_Wtime();
        }

        else if(myid == 1) {
            for(i = 0; i < loop + skip; i++) {
                MPI_Recv(r_buf, size, MPI_CHAR, 0, 1, MPI_COMM_WORLD, &reqstat);
#ifdef _ENABLE_CUDA_
                if ((src == 'H' && desti == 'D') || ( src == 'D' && desti == 'H')){
                    MPI_Send(s_buf_rev, size, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
                } else {
#endif
                    MPI_Send(s_buf, size, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
#ifdef _ENABLE_CUDA_
                }
#endif
            }
        }

        if(myid == 0) {
            double latency = (t_end - t_start) * 1e6 / (2.0 * loop);

            fprintf(stdout, "%-*d%*.*f\n", 10, size, FIELD_WIDTH,
                    FLOAT_PRECISION, latency);
            fflush(stdout);
        }
    }

    MPI_Finalize();
#ifdef _ENABLE_CUDA_
    if (src == 'D'){
        cudaFree(s_buf);
    }
    if (desti == 'D'){
        cudaFree(r_buf);
    }   
    curesult = cuCtxDestroy(cuContext);   
    if (curesult != CUDA_SUCCESS) {
        return EXIT_FAILURE;
    }  
#endif
    return EXIT_SUCCESS;
}

/* vi: set sw=4 sts=4 tw=80: */
