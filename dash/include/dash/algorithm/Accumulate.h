#ifndef DASH__ALGORITHM__ACCUMULATE_H__
#define DASH__ALGORITHM__ACCUMULATE_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>

namespace dash {

/**
 * Accumulate values in range \c [first, last) using the given binary
 * reduce function \c op.
 *
 * Collective operation.
 *
 * Note: For equivalent of semantics of \c MPI_Accumulate, see
 * \c dash::transform.
 *
 * Semantics:
 *
 *     acc = init (+) in[0] (+) in[1] (+) ... (+) in[n]
 *
 * \see      dash::transform
 *
 * \ingroup  DashAlgorithms
 */
    
template<
    class GlobInputIt,
    class ValueType,
    class BinaryOperation >
ValueType accumulate(
    GlobInputIt     in_first,
    GlobInputIt     in_last,
    ValueType       init)
    {
        typedef dash::default_index_t index_t;
        
        
        auto myid           = dash::myid();
        
        auto index_range    = dash::local_range(first, last);
        auto l_first        = index_range.begin;
        auto l_last         = index_range.end;
        
        auto l_result =std::accumulate(l_first, l_last, init);

        dash::Array<index_t> l_results(dash::size());
        
        l_results.local[0] = l_result;
        
        dash::barrier();
        auto result        = 0;
        
        if (myid == 0){
            
            for (int i = 0; i < dash::size(); i++) {
                
                result += l_results[i];
            
            }
            
        }
        
        return result;
        
            
        
    
     }
        
        
      
    
template<
  class GlobInputIt,
  class ValueType,
  class BinaryOperation >
ValueType accumulate(
  GlobInputIt     in_first,
  GlobInputIt     in_last,
  ValueType       init,
  BinaryOperation binary_op = dash::plus<ValueType>())
{
    
    typedef dash::default_index_t index_t;
    
    
    auto myid           = dash::myid();
    
    auto index_range    = dash::local_range(first, last);
    auto l_first        = index_range.begin;
    auto l_last         = index_range.end;
    
    auto l_result =std::accumulate(l_first, l_last, init, binary_op);

    dash::Array<index_t> l_results(dash::size());
    
    l_results.local[0] = l_result;
    
    dash::barrier();
    auto result        = 0;
    
    if (myid == 0){
        
        for (int i = 0; i < dash::size(); i++) {
            
            result = binary_op(result, l_results[i]);
        
        }
        
    }
    
    return result;
    
        
  
  
    
}

} // namespace dash

#endif // DASH__ALGORITHM__ACCUMULATE_H__
