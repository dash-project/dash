#ifndef DASH__ALGORITHM__EQUAL_H__
#define DASH__ALGORITHM__EQUAL_H__

#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {

/**
 * \ingroup     DashAlgorithms
 */
template<
    typename ElementType,
    class PatternType>
GlobIter<ElementType, PatternType> equal(
    /// Iterator to the initial position in the sequence
    GlobIter<ElementType, PatternType>   first_1,
    /// Iterator to the final position in the sequence
    GlobIter<ElementType, PatternType>   last_1,
    GlobIter<ElementType, PatternType>   first_2)
{
  typedef dash::default_index_t index_t;

  auto myid          = dash::myid();
  /// Global iterators to local range:
  auto index_range   = dash::local_range(first_1, last_1);
  auto l_first_1       = index_range.begin;
  auto l_last_1        = index_range.end;

  auto l_result      = std::equal(l_first_1, l_last_1, first_2);
  
  dash::Array<index_t> l_results(dash::size());

  l_results.local[0] = l_result;
  
  bool return_result = true ;
  
  

  dash::barrier();

  // All local offsets stored in l_results
  
  if (myid == 0) {
      
      for (int u = 0; u < dash::size(); u++) {
          
          return_result &= l_results.local[u];
          
      }
      
      
  }

  return return_result;
}

template<
    typename ElementType,
    class PatternType,
    class BinaryPredicate>
GlobIter<ElementType, PatternType> equal(
    /// Iterator to the initial position in the sequence
    GlobIter<ElementType, PatternType>   first_1,
    /// Iterator to the final position in the sequence
    GlobIter<ElementType, PatternType>   last_1,
    GlobIter<ElementType, PatternType>   first_2,
    BinaryPredicate p )
    
{
    typedef dash::default_index_t index_t;
    auto myid          = dash::myid();
    /// Global iterators to local range:
    auto index_range   = dash::local_range(first_1, last_1);
    auto l_first_1       = index_range.begin;
    auto l_last_1        = index_range.end;

    auto l_result      = std::equal(l_first_1, l_last_1, first_2, p);
  
    dash::Array<index_t> l_results(dash::size());

    l_results.local[0] = l_result;
  
    bool return_result = true ;
    
    dash::barrier();
    
    // All local offsets stored in l_results
    
    if (myid == 0) {
        
        for (int u = 0; u < dash::size(); u++) {
            
            return_result &= l_results.local[u];
        }
    }

  return return_result;
}
        
        

} // namespace dash

#endif // DASH__ALGORITHM__EQUAL_H__
