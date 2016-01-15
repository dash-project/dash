#ifndef PAL_H_INCLUDED
#define PAL_H_INCLUDED

/**
 * Parallel Abstraction Layer Macros
 * 
 * This header file contains macros which allow to write a parallel program
 * in a more general way by explictly defining all parallel and sequential
 * regions as well as all shared and private variables.
 *
 * Currently there are three modes:
 * - PAL_DASH      parallel using DASH
 * - PAL_OPENMP    parallel using OpenMP
 * - PAL_DEFAULT   default single threaded
 * 
 * Usage: simply #define PAL_DASH (or one of the other modes).
 *
 * Example:
 * 
 * int main(int argc, char *argv[]) {
 *   PAL_INIT
 *   PAL_SHARED_ARR_DECL(arr, int, 100)
 *   PAL_PARALLEL_BEGIN(private: id, shared: arr)
 *     int i;
 *     PAL_FOR_NOWAIT_BEGIN(0, 99, 1, i, int)
 *       PAL_SHARED_ARR_SET(arr, i, i*i)
 *     PAL_FOR_NOWAIT_END
 *   PAL_PARALLEL_END
 *   PAL_SEQUENTIAL_BEGIN
 *     printf("Hello world only printed once\n");
 *   PAL_SEQUENTIAL_END
 *   PAL_FINALIZE
 * }
 *
 *
 * Result using DASH:
 *
 * int main(int argc, char *argv[]) {
 *   dash::init(&argc, &argv);
 *   dash::Array<int> arr(100);
 *   {
 *     dash::barrier();
 *     int i;
 *     dash::omp::for_loop(0, 99, 1, dash::BLOCKED, [&](int i){
 *       arr[i] = i*i;
 *     }, false);
 *     dash::barrier();
 *   }
 *   dash::omp::master([&]{
 *     printf("Hello world only printed once\n");
 *   }, false);
 *   dash::barrier();
 * }
 *
 *
 * Result using OpenMP (almost the same using default):
 *
 * int main(int argc, char *argv[]) {
 *   int arr[100];
 *   #pragma omp parallel
 *   {
 *     int i;
 *     #pragma omp for nowait
 *     for(i = 0, i <= 99, i += 1) {
 *       arr[i] = i*i;
 *     }
 *   }
 *   printf("Hello world only printed once\n");
 * }
 *
 */


#define PAL_STRINGIFY(a) #a
#define PAL_PRAGMA(a) _Pragma(PAL_STRINGIFY(a))


#ifdef PAL_DASH
#undef _OPENMP

#include <libdash.h>

#define PAL_INIT                                         dash::init(&argc, &argv);
#define PAL_FINALIZE                                     dash::finalize();

#define PAL_THREAD_NUM                                   dash::myid()
#define PAL_NUM_THREADS                                  dash::size()
#define PAL_MAX_THREADS                                  dash::size()

#define PAL_OMP_PRAGMA(params)

#define PAL_BARRIER                                      dash::barrier();
#define PAL_PARALLEL_BEGIN(params)                       { dash::barrier();
#define PAL_PARALLEL_END                                 dash::barrier(); }
#define PAL_PARALLEL_REDUCE_BEGIN(rop, rvar, rtype) \
    { dash::barrier(); Reduction<rtype, rop<rtype> >& pal_red(rvar, dash::Team::All());
#define PAL_PARALLEL_REDUCE_END \
    pal_red.reduce(rvar); dash::barrier(); }
#define PAL_SEQUENTIAL_BEGIN                             dash::omp::master([&]{
#define PAL_SEQUENTIAL_END                               }); dash::barrier();
#define PAL_MASTER_BEGIN                                 dash::omp::master([&]{
#define PAL_MASTER_END                                   }); dash::barrier();

#define PAL_CRITICAL_INIT(name)                          dash::omp::mutex_init(#name);
#define PAL_CRITICAL_BEGIN(name)                         dash::omp::critical(#name, [&]{
#define PAL_CRITICAL_END                                 });

#define PAL_FOR_WAIT_BEGIN(begin, end, inc, iname, itype) \
    dash::omp::for_loop(begin, end, inc, dash::BLOCKED, [&](itype iname){
#define PAL_FOR_WAIT_END                                 }, true);
#define PAL_FOR_NOWAIT_BEGIN(begin, end, inc, iname, itype) \
    dash::omp::for_loop(begin, end, inc, dash::BLOCKED, [&](itype iname){
#define PAL_FOR_NOWAIT_END                               }, false);
#define PAL_FOR_REDUCE_BEGIN(begin, end, inc, iname, itype, rop, rvar, rtype) \
    dash::omp::for_reduce<rop<rtype> >(begin, end, inc, dash::BLOCKED, rvar, [&](itype iname){
#define PAL_FOR_REDUCE_END                               });

#define PAL_RED_OP_NONE                                  dash::omp::Op::None
#define PAL_RED_OP_PLUS                                  dash::omp::Op::Plus
#define PAL_RED_OP_MINUS                                 dash::omp::Op::Minus
#define PAL_RED_OP_MULT                                  dash::omp::Op::Mult
#define PAL_RED_OP_BIT_AND                               dash::omp::Op::BitAnd
#define PAL_RED_OP_BIT_OR                                dash::omp::Op::BitOr
#define PAL_RED_OP_BIT_XOR                               dash::omp::Op::BitXor
#define PAL_RED_OP_LOGIC_AND                             dash::omp::Op::LogicAnd
#define PAL_RED_OP_LOGIC_OR                              dash::omp::Op::LogicOr
#define PAL_RED_OP_MIN                                   dash::omp::Op::Min
#define PAL_RED_OP_MAX                                   dash::omp::Op::Max

#define PAL_SHARED_VAR_PTR_DECL(name, type)              dash::Shared<type> *name;
#define PAL_SHARED_VAR_PTR_ALLOC(name, type)             name = new dash::Shared<type>();
#define PAL_SHARED_VAR_PTR_FREE(name)                    delete name;
#define PAL_SHARED_VAR_PTR_GET(name, type, dest)         dest = ((type)(*name).get());
#define PAL_SHARED_VAR_PTR_SET(name, value)              (*name).set(value);
#define PAL_SHARED_VAR_PTR_RVAL(name, type)              ((type)(*name).get())

#define PAL_SHARED_VAR_DECL(name, type)                  dash::Shared<type> name;
#define PAL_SHARED_VAR_GET(name, type, dest)             dest = (type)name.get();
#define PAL_SHARED_VAR_SET(name, value)                  name.set(value);
#define PAL_SHARED_VAR_RVAL(name, type)                  (type)name.get()

#define PAL_SHARED_ARR_PTR_DECL(name, type)              dash::Array<type> *name;
#define PAL_SHARED_ARR_PTR_ALLOC(name, type, size) \
      name = new dash::Array<type>(); (*name).allocate(size);
#define PAL_SHARED_ARR_PTR_FREE(name)                    delete name;
#define PAL_SHARED_ARR_PTR_GET(name, index, type, dest)  dest = (type)((*name)[index]);
#define PAL_SHARED_ARR_PTR_SET(name, index, value)       (*name)[index] = value;
#define PAL_SHARED_ARR_PTR_RVAL(name, index, type)       ((type)((*name)[index]))

#define PAL_SHARED_ARR_DECL(name, type, size)            dash::Array<type> name(size);
#define PAL_SHARED_ARR_DEF(name, type, size)             dash::Array<type> name;
#define PAL_SHARED_ARR_ALLOC(name, type, size)           name.allocate(size, dash::BLOCKED);
#define PAL_SHARED_ARR_GET(name, index, type, dest)      dest = (type)name[index];
#define PAL_SHARED_ARR_SET(name, index, value)           name[index] = value;
#define PAL_SHARED_ARR_ADD(name, index, value)           name[index] += value;
#define PAL_SHARED_ARR_RVAL(name, index, type)           (type)name[index]

#define PAL_SHARED_STRUCT_MEMBER(mname, mtype, sname, stype) \
    sname.get().member<mtype>(&stype::mname)
#define PAL_SHARED_STRUCT_PTR_MEMBER(mname, mtype, sname, stype) \
    (*sname).get().member<mtype>(&stype::mname)

#define PAL_SECTIONS_BEGIN \
    { dash::barrier(); dash::omp::Sections<> pal_sec;
#define PAL_SECTIONS_END                                 pal_sec.execute(); }
#define PAL_SECTION_BEGIN                                pal_sec.section([&]{
#define PAL_SECTION_END                                  });

#define PAL_SINGLE_BEGIN \
    { dash::barrier(); dash::omp::single([&]{
#define PAL_SINGLE_END                                   }); }

#endif



#ifdef PAL_OPENMP

#include <omp.h>

#define PAL_INIT
#define PAL_FINALIZE

#define PAL_THREAD_NUM                                   omp_get_thread_num()
#define PAL_NUM_THREADS                                  omp_get_num_threads()
#define PAL_MAX_THREADS                                  omp_get_max_threads()

#define PAL_OMP_PRAGMA(params)                           PAL_PRAGMA(omp params)

#define PAL_BARRIER                                      PAL_PRAGMA(omp barrier)
#define PAL_PARALLEL_BEGIN(params)                       PAL_PRAGMA(omp parallel params) {
#define PAL_PARALLEL_END                                 }
#define PAL_PARALLEL_REDUCE_BEGIN(rop, rvar, rtype) \
    PAL_PRAGMA(omp parallel reduction(rop: rvar)) {
#define PAL_PARALLEL_REDUCE_END                          }
#define PAL_SEQUENTIAL_BEGIN
#define PAL_SEQUENTIAL_END
#define PAL_MASTER_BEGIN                                 PAL_PRAGMA(omp master) {
#define PAL_MASTER_END                                   }

#define PAL_CRITICAL_INIT(name)
#define PAL_CRITICAL_BEGIN(name)                         PAL_PRAGMA(omp critical (name)) {
#define PAL_CRITICAL_END                                 }

#define PAL_FOR_WAIT_BEGIN(begin, end, inc, iname, itype) \
    PAL_PRAGMA(omp for) \
    for (iname = begin; iname <= end; iname += inc) {
#define PAL_FOR_WAIT_END                                 }
#define PAL_FOR_NOWAIT_BEGIN(begin, end, inc, iname, itype) \
    PAL_PRAGMA(omp for nowait) \
    for (iname = begin; iname <= end; iname += inc) {
#define PAL_FOR_NOWAIT_END                               }
#define PAL_FOR_REDUCE_BEGIN(begin, end, inc, iname, itype, rop, rvar, rtype) \
    PAL_PRAGMA(omp for reduction(rop: rvar)) \
    for (iname = begin; iname <= end; iname += inc) {
#define PAL_FOR_REDUCE_END                               }

#define PAL_RED_OP_NONE
#define PAL_RED_OP_PLUS                                  +
#define PAL_RED_OP_MINUS                                 -
#define PAL_RED_OP_MULT                                  *
#define PAL_RED_OP_BIT_AND                               &
#define PAL_RED_OP_BIT_OR                                |
#define PAL_RED_OP_BIT_XOR                               ^
#define PAL_RED_OP_LOGIC_AND                             &&
#define PAL_RED_OP_LOGIC_OR                              ||
#define PAL_RED_OP_MIN                                   min
#define PAL_RED_OP_MAX                                   max

#define PAL_SHARED_VAR_PTR_DECL(name, type)              type *name;
#define PAL_SHARED_VAR_PTR_ALLOC(name, type)             name = (type*)malloc(sizeof(type));
#define PAL_SHARED_VAR_PTR_FREE(name)                    free(name);
#define PAL_SHARED_VAR_PTR_GET(name, type, dest)         dest = *name;
#define PAL_SHARED_VAR_PTR_SET(name, value)              *name = value;
#define PAL_SHARED_VAR_PTR_RVAL(name, type)              (*name)

#define PAL_SHARED_VAR_DECL(name, type)                  type name;
#define PAL_SHARED_VAR_GET(name, type, dest)             dest = name;
#define PAL_SHARED_VAR_SET(name, value)                  name = value;
#define PAL_SHARED_VAR_RVAL(name, type)                  name

#define PAL_SHARED_ARR_PTR_DECL(name, type)              type* name;
#define PAL_SHARED_ARR_PTR_ALLOC(name, type, size)       name = (type*)malloc(size*sizeof(type));
#define PAL_SHARED_ARR_PTR_FREE(name)                    free(name);
#define PAL_SHARED_ARR_PTR_GET(name, index, type, dest)  dest = name[index];
#define PAL_SHARED_ARR_PTR_SET(name, index, value)       name[index] = value;
#define PAL_SHARED_ARR_PTR_RVAL(name, index, type)       name[index]

#define PAL_SHARED_ARR_DECL(name, type, size)            type name[size];
#define PAL_SHARED_ARR_DEF(name, type, size)             type name[size];
#define PAL_SHARED_ARR_ALLOC(name, type, size)
#define PAL_SHARED_ARR_GET(name, index, type, dest)      dest = name[index];
#define PAL_SHARED_ARR_SET(name, index, value)           name[index] = value;
#define PAL_SHARED_ARR_ADD(name, index, value)           name[index] += value;
#define PAL_SHARED_ARR_RVAL(name, index, type)           name[index]

#define PAL_SHARED_STRUCT_MEMBER(mname, mtype, sname, stype) \
  sname.mname
#define PAL_SHARED_STRUCT_PTR_MEMBER(mname, mtype, sname, stype) \
  sname->mname

#define PAL_SECTIONS_BEGIN                               PAL_PRAGMA(omp sections) {
#define PAL_SECTIONS_END                                 }
#define PAL_SECTION_BEGIN                                PAL_PRAGMA(omp section) {
#define PAL_SECTION_END                                  }

#define PAL_SINGLE_BEGIN                                 PAL_PRAGMA(omp single) {
#define PAL_SINGLE_END                                   }

#endif



#ifdef PAL_DEFAULT
#undef _OPENMP

#define PAL_INIT
#define PAL_FINALIZE

#define PAL_THREAD_NUM                                   0
#define PAL_NUM_THREADS                                  1
#define PAL_MAX_THREADS                                  1

#define PAL_OMP_PRAGMA(params)

#define PAL_BARRIER
#define PAL_PARALLEL_BEGIN(params)                       {
#define PAL_PARALLEL_END                                 }
#define PAL_PARALLEL_REDUCE_BEGIN(rop, rvar, rtype)      {
#define PAL_PARALLEL_REDUCE_END                          }
#define PAL_SEQUENTIAL_BEGIN                             {
#define PAL_SEQUENTIAL_END                               }
#define PAL_MASTER_BEGIN                                 {
#define PAL_MASTER_END                                   }

#define PAL_CRITICAL_INIT(name)
#define PAL_CRITICAL_BEGIN(name)                         {
#define PAL_CRITICAL_END                                 }

#define PAL_FOR_WAIT_BEGIN(begin, end, inc, iname, itype) \
    for (iname = begin; iname <= end; iname += inc) {
#define PAL_FOR_WAIT_END                                 }
#define PAL_FOR_NOWAIT_BEGIN(begin, end, inc, iname, itype) \
    for (iname = begin; iname <= end; iname += inc) {
#define PAL_FOR_NOWAIT_END                               }
#define PAL_FOR_REDUCE_BEGIN(begin, end, inc, iname, itype, rop, rvar, rtype) \
    for (iname = begin; iname <= end; iname += inc) {
#define PAL_FOR_REDUCE_END                               }

#define PAL_RED_OP_NONE
#define PAL_RED_OP_PLUS
#define PAL_RED_OP_MINUS
#define PAL_RED_OP_MULT
#define PAL_RED_OP_BIT_AND
#define PAL_RED_OP_BIT_OR
#define PAL_RED_OP_BIT_XOR
#define PAL_RED_OP_LOGIC_AND
#define PAL_RED_OP_LOGIC_OR
#define PAL_RED_OP_MIN
#define PAL_RED_OP_MAX

#define PAL_SHARED_VAR_PTR_DECL(name, type)              type *name;
#define PAL_SHARED_VAR_PTR_ALLOC(name, type)             name = (type*)malloc(sizeof(type));
#define PAL_SHARED_VAR_PTR_FREE(name)                    free(name);
#define PAL_SHARED_VAR_PTR_GET(name, type, dest)         dest = *name;
#define PAL_SHARED_VAR_PTR_SET(name, value)              *name = value;
#define PAL_SHARED_VAR_PTR_RVAL(name, type)              (*name)

#define PAL_SHARED_VAR_DECL(name, type)                  type name;
#define PAL_SHARED_VAR_GET(name, type, dest)             dest = name;
#define PAL_SHARED_VAR_SET(name, value)                  name = value;
#define PAL_SHARED_VAR_RVAL(name, type)                  name

#define PAL_SHARED_ARR_PTR_DECL(name, type)              type* name;
#define PAL_SHARED_ARR_PTR_ALLOC(name, type, size)       name = (type*)malloc(size*sizeof(type));
#define PAL_SHARED_ARR_PTR_FREE(name)                    free(name);
#define PAL_SHARED_ARR_PTR_GET(name, index, type, dest)  dest = name[index];
#define PAL_SHARED_ARR_PTR_SET(name, index, value)       name[index] = value;
#define PAL_SHARED_ARR_PTR_RVAL(name, index, type)       name[index]

#define PAL_SHARED_ARR_DECL(name, type, size)            type name[size];
#define PAL_SHARED_ARR_DEF(name, type, size)             type name[size];
#define PAL_SHARED_ARR_ALLOC(name, type, size)
#define PAL_SHARED_ARR_GET(name, index, type, dest)      dest = name[index];
#define PAL_SHARED_ARR_SET(name, index, value)           name[index] = value;
#define PAL_SHARED_ARR_ADD(name, index, value)           name[index] += value;
#define PAL_SHARED_ARR_RVAL(name, index, type)           name[index]

#define PAL_SHARED_STRUCT_MEMBER(mname, mtype, sname, stype) \
  sname.mname
#define PAL_SHARED_STRUCT_PTR_MEMBER(mname, mtype, sname, stype) \
  sname->mname

#define PAL_SECTIONS_BEGIN                               {
#define PAL_SECTIONS_END                                 }
#define PAL_SECTION_BEGIN                                {
#define PAL_SECTION_END                                  }

#define PAL_SINGLE_BEGIN                                 {
#define PAL_SINGLE_END                                   }

#endif

#endif /* PAL_H_INCLUDED */
