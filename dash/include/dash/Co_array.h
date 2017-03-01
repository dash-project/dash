#ifndef COARRAY_H_INCLUDED
#define COARRAY_H_INCLUDED

#include <type_traits>

#include <dash/GlobRef.h>
#include <dash/iterator/GlobIter.h>

#include <dash/pattern/BlockPattern.h>
#include <dash/util/ArrayExpr.h>

#include <dash/Array.h>
#include <dash/Matrix.h>

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

/**
 * fortran style co_array
 */
template<
  typename T,
  typename IndexType = dash::default_index_t>
class Co_array {
private:
  
  // storage type dispatching
  template<
    typename __element_type,
    typename __index_type,
    typename __pattern_type,
    int      __rank>
  struct _get_storage_type { 
    typedef Matrix<__element_type, __rank+1, __index_type, __pattern_type> type;
  };
  
  template<
    typename __element_type,
    typename __index_type,
    typename __pattern_type>
  struct _get_storage_type<__element_type, __index_type, __pattern_type, 0> { 
    typedef Array<__element_type, __index_type, __pattern_type> type;
  };

  template<typename __T, typename __S, int __rank>
  struct __get_type_extens_as_array {
    using array_t = std::array<__S,__rank>;
    static constexpr array_t value = dash::ce::append(
                      __get_type_extens_as_array<__T, __S, __rank-1>::value,
                      static_cast<__S>(std::extent<__T, __rank>::value));
  };

  template<typename __T, typename __S>
  struct __get_type_extens_as_array<__T, __S, 0> {
    using array_t = std::array<__S,0>;
    static constexpr array_t value = std::array<__S, 0>();
  };

private:
  static constexpr int _rank = std::rank<T>::value;
  
  using _index_type     = typename std::make_unsigned<IndexType>::type;
  using _pattern_type   = BlockPattern<_rank+1, ROW_MAJOR, _index_type>;
  using _element_type   = typename std::remove_all_extents<T>::type;
  using _storage_type   = typename _get_storage_type<_element_type,
                                                     _index_type,
                                                     _pattern_type,
                                                     _rank>::type;

public:
  // Types
  using value_type             = _element_type;
  using difference_type        = IndexType;
  using index_type             = IndexType;
  using size_type              = _index_type;
  using iterator               = GlobIter<_element_type, _pattern_type>; 
  using const_iterator         = GlobIter<const _element_type, _pattern_type>; 
  using reverse_iterator       = GlobIter<_element_type, _pattern_type>; 
  using const_reverse_iterator = GlobIter<const _element_type, _pattern_type>; 
  using reference              = GlobRef<_element_type>;
  using const_reference        = GlobRef<_element_type>;
  using local_pointer          = _element_type *;
  using const_local_pointer    = const _element_type *;
  //using view_type              = Co_arrayRef<>;
  //using local_type             = LocalCo_arrayRef<>;
  using pattern_type           = _pattern_type;
  
private:
  constexpr SizeSpec<_rank+1, size_type> _make_size_spec() const {
    return SizeSpec<_rank+1, size_type>(
            dash::ce::append(
              std::array<size_type, 1> {static_cast<size_type>(dash::size())},
              __get_type_extens_as_array<T, size_type, _rank>::value));
  }
  
public:
  constexpr Co_array():
    _storage(_pattern_type(_make_size_spec())) { }

private:
  /// storage backend
  _storage_type _storage;

};

} // namespace dash

#endif /* COARRAY_H_INCLUDED */
