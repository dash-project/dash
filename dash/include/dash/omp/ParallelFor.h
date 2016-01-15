#ifndef DASH__OMP_PARALLELFOR_H__INCLUDED
#define DASH__OMP_PARALLELFOR_H__INCLUDED

#include <functional>
#include <dash/Types.h>
#include <dash/Enums.h>
#include <dash/Distribution.h>
#include <dash/Pattern.h>
#include <dash/omp/Reduction.h>

namespace dash {
namespace omp {

/**
 * OpenMP FOR loop
 *
 * \tparam IndexType    IndexType is deduced from \c func. Works with positive 
 *                      and negative integer and should work with floating 
 *                      point numbers as long as this can be computed correctly: 
 *                      iterations = (end - begin) / step + 1 
 * \tparam LambdaType   Type of the lambda expression for the the body of the
 *                      for loop: \c func(IndexType) -> void
 *
 * \param  begin        Initial index value
 * \param  end          Last index value
 * \param  step         Increment or decrement in each step
 * \param  dist         Distribution pattern
 * \param  func         Body of the for loop
 * \param  wait         Barrier after loop (default true)
 */
template<typename IndexType,
	 typename LambdaType>
void for_loop(IndexType     begin,
	      IndexType     end,
	      IndexType     step, 
	      Distribution  dist, 
	      LambdaType&&  func,
	      bool          wait = true)
{
  if (step == 0) {
    DASH_THROW(dash::exception::InvalidArgument,
	       "Cannot start OpenMP FOR loop with step 0");
  }
  
  IndexType iterations = (end - begin) / step + 1;
  if (iterations < 0) {
    DASH_THROW(dash::exception::InvalidArgument,
	       "Cannot start OpenMP FOR loop with negative number of iterations");
  }
  
  dash::Pattern<1> pat (iterations, dist);
  for (IndexType i = 0; i < pat.local_size(); ++i) {
    IndexType gidx = pat.global(i);
    func(std::forward<IndexType>(gidx * step + begin));
  }
  
  if (wait) {
    dash::barrier();
  }
}


template<typename IndexType,
	 typename LambdaType>
void for_loop_nowait(IndexType     begin,
		     IndexType     end,
		     IndexType     step, 
		     Distribution  dist, 
		     LambdaType&&  func)
{
  for_loop(begin, end, step, dist, func, false);
}


/**
 * OpenMP FOR loop with reduction on a single variable.
 *
 * \tparam IndexType    IndexType is deduced from \c func. Works with positive 
 *                      and negative integer and should work with floating 
 *                      point numbers as long as this can be computed correctly: 
 *                      iterations = (end - begin) / step + 1 
 * \tparam LambdaType   Type of the lambda expression for the the body of the
 *                      for loop: \c func(IndexType) -> void
 * \tparam ResultType   Type of the reduction variable; deduced from the last
 *                      parameter
 *
 * \param  begin        Initial index value
 * \param  end          Last index value
 * \param  step         Increment or decrement in each step
 * \param  dist         Distribution pattern
 * \param  func         Body of the for loop
 * \param  reduct_var   Reference to variable that may hold an initial value
 *                      before the loop. The variable is set to the neutral
 *                      element of the reduction operation before the loop
 *                      starts. After the loop, the varibale will hold the
 *                      aggregated value (including the initial value if set)
 */
template<
  class ReductionOp,
  typename ResultType,
  typename IndexType,
  typename LambdaType>
void for_reduce(IndexType    begin,
		IndexType    end,
		IndexType    step, 
		Distribution dist,
		ResultType&  reduct_var, 
		LambdaType&& func)
{
  if (step == 0) {
    DASH_THROW(dash::exception::InvalidArgument,
	       "Cannot start OpenMP FOR loop with step 0");
  }
  
  IndexType iterations = (end - begin) / step + 1;
  if (iterations < 0) {
    DASH_THROW(dash::exception::InvalidArgument,
	       "Cannot start OpenMP FOR loop with negative number of iterations");
  }
  
  dash::Pattern<1> pat(iterations, dist);
  dash::omp::Reduction<ResultType, ReductionOp> 
    reduction(reduct_var, Team::All());
  
  for (IndexType i = 0; i < pat.local_size(); ++i) {
    IndexType gidx = pat.global(i);
    func(std::forward<IndexType>(gidx * step + begin));
  }
  
  reduction.reduce(reduct_var);
}



}  // namespace omp
}  // namespace dash

#endif // DASH__PARALLELFOR_H__INCLUDED
