#ifndef COARRAY_H_INCLUDED
#define COARRAY_H_INCLUDED

#include <dash/GlobRef.h>
#include <dash/iterator/GlobIter.h>

#include <dash/pattern/BlockPattern.h>

/**
 * \defgroup  DashCoArrayConcept  co_array Concept
 *
 * \ingroup DashContainerConcept
 * \{
 * \par Description
 *
 * A fortran style co_array.
 *
 * DASH co_arrays support delayed allocation (\c dash::co_array::allocate),
 * so global memory of an array instance can be allocated any time after
 * declaring a \c dash::co_array variable.
 *
 * \par Types
 *
 * \TODO: Types
 *
 */

namespace dash {

template<
  typename T,
  typename IndexType = dash::default_index_t>
class Co_array {
private:
  using _pattern_type   = dash::BlockPattern<1>;

public:
  // Types
  using value_type             = T;
  using difference_type        = IndexType;
  using index_type             = IndexType;
  using size_type              = typename std::make_unsigned<IndexType>::type;
  using iterator               = GlobIter<T, _pattern_type>; 
  using const_iterator         = GlobIter<const T, _pattern_type>; 
  using reverse_iterator       = GlobIter<T, _pattern_type>; 
  using const_reverse_iterator = GlobIter<const T, _pattern_type>; 
  using reference              = GlobRef<T>;
  using const_reference        = GlobRef<T>;
  using local_pointer          = T *;
  using const_local_pointer    = const T *;
  //using view_type              = Co_arrayRef<>;
  //using local_type             = LocalCo_arrayRef<>;
  using pattern_type           = _pattern_type;
  
};

} // namespace dash

#endif /* COARRAY_H_INCLUDED */
