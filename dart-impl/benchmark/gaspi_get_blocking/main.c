#include <GASPI.h>
#include <stdio.h>
#include <bench.h>

void bench_blocking_get(size_t transfer_val_count, int repeat_count)
{
    gaspi_segment_id_t seg_id = 0;
    gaspi_rank_t nProc, iProc;
    const gaspi_offset_t offset = transfer_val_count * sizeof(int);

    gaspi_proc_rank(&iProc);
    gaspi_proc_num(&nProc);

    double time_mem_s = get_wtime();
    gaspi_segment_create(seg_id, 2 * transfer_val_count * sizeof(int), GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_UNINITIALIZED);
    double time_mem_e = get_wtime();
    gaspi_printf("mem time %lf\n", time_mem_e - time_mem_s);

    gaspi_pointer_t seg_ptr = NULL;
    gaspi_segment_ptr(seg_id, &seg_ptr);

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) seg_ptr)[i] = iProc + i;
    }

    gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);

    void * recv_ptr = (void *) ((char *) seg_ptr + offset);

    double get_sum = 0.0;
    int cnt = repeat_count;
    while(repeat_count-- > 0)
    {
        int sum = 0;
        for(gaspi_rank_t rank = 0; rank < nProc ; ++rank)
        {
            if(rank == iProc)
                continue;

            const double s = get_wtime();
            gaspi_read(seg_id, offset, rank, seg_id, 0UL, transfer_val_count * sizeof(int), 0, GASPI_BLOCK);
            gaspi_wait(0, GASPI_BLOCK);
            const double e = get_wtime();
            get_sum += (e - s);

            for(size_t i = 0; i < transfer_val_count ; ++i)
            {
                sum += ((int *) recv_ptr)[i];
            }
        }
    }

    gaspi_printf("get %lf\n", get_sum / ((double) (nProc - 1) * cnt));
    gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
    gaspi_segment_delete(seg_id);
}

int main (int argc, char ** argv)
{
    gaspi_rank_t rank;
    const unsigned long count = strtoul(argv[1], NULL, 0);
    const double time_all_start = get_wtime();

    gaspi_proc_init(GASPI_BLOCK);

    const double time_init_end = get_wtime();

    gaspi_proc_rank(&rank);
    const double time_get_start = get_wtime();
    bench_blocking_get(count, 80);
    const double time_get_end = get_wtime();

    gaspi_proc_term(GASPI_BLOCK);
    if(0 == rank)
    {
        double time_all_end = get_wtime();
        // Getting ownership of file descriptor
        FILE * out = get_file_handle(argv[2]);
        fprintf(out, "all, get_blocking, init\n");
        fprintf(out, "%lf, %lf, %lf\n", time_all_end - time_all_start,
                time_get_end - time_get_start,
                time_init_end - time_all_start);

        fclose(out);

    }
    return 0;
}
