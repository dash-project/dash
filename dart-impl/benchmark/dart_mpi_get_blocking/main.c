#include <dash/dart/if/dart.h>
#include <stdio.h>
#include <bench.h>

double bench_blocking_get(size_t transfer_val_count, int repeat_count)
{
    dart_unit_t myid;
    size_t size;
    const int offset = transfer_val_count * sizeof(int);

    dart_gptr_t g;

    dart_myid(&myid);
    dart_size(&size);

    dart_team_memalloc_aligned(DART_TEAM_ALL, 2 * transfer_val_count * sizeof(int), &g);

    void * g_ptr = NULL;

    dart_gptr_setunit(&g, myid);
    dart_gptr_getaddr(g, &g_ptr);

    for(size_t i = 0 ; i < transfer_val_count ; ++i)
    {
        ((int *) g_ptr)[i] = myid + i;
    }

    dart_barrier(DART_TEAM_ALL);

    dart_gptr_t gptr_dest = g, gptr_src = g;

    dart_gptr_incaddr(&gptr_dest, offset);
    //~ dart_gptr_setunit(&gptr_src, next_unit);
    void * recv = NULL;
    dart_gptr_getaddr(gptr_dest, &recv);

    double get_sum = 0.0;
    int cnt = repeat_count;
    while(repeat_count-- > 0)
    {
        int sum = 0;
        for(dart_unit_t unit = 0; unit < size; ++unit)
        {
            if(unit == myid) continue;

            dart_gptr_setunit(&gptr_src, unit);
            const double s = get_wtime();
            dart_get_blocking(recv, gptr_src, transfer_val_count * sizeof(int));
            const double e = get_wtime();
            get_sum += (e - s);

            for(size_t i = 0; i < transfer_val_count ; ++i)
            {
                sum += ((int *) recv)[i];
                if(((int *) recv)[i] != (i + unit))
                {
                    printf("Wrong value\n");
                }
            }
        }

    }
    dart_barrier(DART_TEAM_ALL);
    dart_team_memfree(DART_TEAM_ALL, g);

    return get_sum / ((double) (size - 1) * cnt);
}

int main (int argc, char ** argv)
{
    dart_unit_t myid;
    const unsigned long count = strtoul(argv[1], NULL, 0);

    const double time_all_start = get_wtime();
    dart_init(&argc, &argv);
    const double time_init_end = get_wtime();

    dart_myid(&myid);
    const double time_get_start = get_wtime();
    double transfer_time = bench_blocking_get(count, 80);
    const double time_get_end = get_wtime();

    dart_exit();
    if(myid == 0)
    {
        double time_all_end = get_wtime();
        // Getting ownership of file descriptor
        FILE * out = get_file_handle(argv[2]);

        fprintf(out, "get_operation, all, get_blocking, init\n");
        fprintf(out, "%lf, %lf, %lf, %lf\n",
                transfer_time,
                time_all_end - time_all_start,
                time_get_end - time_get_start,
                time_init_end - time_all_start);

        fclose(out);

    }
    return 0;
}
