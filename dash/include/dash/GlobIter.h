#ifndef DASH__GLOB_ITER_H_
#define DASH__GLOB_ITER_H_

#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>
#include <iostream>

namespace dash {

typedef long long gptrdiff_t;

template<
  typename ElementType,
  class PatternType   = Pattern<1>,
  class PointerType   = GlobPtr<ElementType>,
  class ReferenceType = GlobRef<ElementType> >
class GlobIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           gptrdiff_t,
           PointerType,
           ReferenceType > {
private:
  typedef GlobIter<ElementType, PatternType, PointerType, ReferenceType>
    self_t;
  
  typedef typename PatternType::viewspec_type
    ViewSpecType;
  typedef typename PatternType::index_type
    IndexType;

public:
  typedef       ReferenceType                      reference;
  typedef const ReferenceType                const_reference;
  typedef       PointerType                          pointer;
  typedef const PointerType                    const_pointer;

private:
  static const dim_t      NumDimensions = PatternType::ndim();
  static const MemArrange Arrangement   = PatternType::memory_order();

protected:
  GlobMem<ElementType> * _globmem;
  const PatternType    * _pattern;
  const ViewSpecType   * _viewspec;
  /// Current position of the iterator.
  size_t                 _idx        = 0;
  /// Maximum position allowed for this iterator.
  size_t                 _max_idx    = 0;

  // For ostream output
  template <
    typename T_,
    class P_,
    class Ptr_,
    class Ref_ >
  friend std::ostream & operator<<(
      std::ostream & os,
      const GlobIter<T_, P_, Ptr_, Ref_> & it);

public:
  /**
   * Default constructor.
   */
  GlobIter()
  : _globmem(nullptr),
    _pattern(nullptr),
    _viewspec(nullptr),
    _idx(0),
    _max_idx(0) {
    DASH_LOG_TRACE_VAR("GlobIter()", _idx);
    DASH_LOG_TRACE_VAR("GlobIter()", _max_idx);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern.
   */
  GlobIter(
    GlobMem<ElementType> * gmem,
	  const PatternType    & pat,
	  size_t                 idx = 0)
  : _globmem(gmem), 
    _pattern(&pat),
    _viewspec(nullptr),
    _idx(idx),
    _max_idx(pat.size() - 1) {
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,idx)", _idx);
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,idx)", _max_idx);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern and view spec.
   */
  GlobIter(
    GlobMem<ElementType> * gmem,
	  const PatternType    & pat,
    const ViewSpecType   & viewspec,
	  size_t                 idx = 0)
  : _globmem(gmem), 
    _pattern(&pat),
    _viewspec(&viewspec),
    _idx(idx),
    _max_idx(viewspec.size() - 1) {
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,vs,idx)", _idx);
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,vs,idx)", _max_idx);
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,vs,idx)", _viewspec->offsets());
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,vs,idx)", _viewspec->extents());
  }

  /**
   * Copy constructor.
   */
  GlobIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator PointerType() const {
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr()", _idx);
    size_t idx     = _idx;
    size_t offset  = 0;
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr()", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx    = _max_idx;
      offset = _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr", idx);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr", offset);
    // Global cartesian coords of current iterator position:
    std::array<IndexType, NumDimensions> glob_coords = coords(idx);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr", glob_coords);
    // Convert global coords to unit and local offset:
    auto local_pos   = _pattern->local_index(glob_coords);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr >", local_pos.index);
    // Create global pointer from unit and local offset:
    PointerType gptr(
      _globmem->index_to_gptr(local_pos.unit, local_pos.index)
    );
    return gptr + offset;
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's 
   *          position
   */
  dart_gptr_t dart_gptr() const {
    return ((GlobPtr<ElementType>)(*this)).dart_gptr();
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  ReferenceType operator*() const {
    DASH_LOG_TRACE_VAR("GlobIter.*", _idx);
    // Global index to local index and unit:
    auto glob_coords = coords(_idx);
    auto local_pos   = _pattern->local_index(glob_coords);
    DASH_LOG_TRACE_VAR("GlobIter.*", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.*", local_pos.index);
    // Global pointer to element at given position:
    PointerType gptr(
      _globmem->index_to_gptr(local_pos.unit, local_pos.index)
    );
    // Global reference to element at given position:
    return ReferenceType(gptr);
  }  

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  ReferenceType operator[](
    /// The global position of the element
    gptrdiff_t global_index) const {
    DASH_LOG_TRACE_VAR("GlobIter.[]", global_index);
    // Global index to local index and unit:
    auto glob_coords = coords(global_index);
    DASH_LOG_TRACE_VAR("GlobIter.[]", glob_coords);
    auto local_pos   = _pattern->local_index(glob_coords);
    DASH_LOG_TRACE_VAR("GlobIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.[]", local_pos.index);
    // Global pointer to element at given position:
    PointerType gptr(
      _globmem->index_to_gptr(local_pos.unit, local_pos.index)
    );
    // Global reference to element at given position:
    return ReferenceType(gptr);
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  bool is_local() const {
    return _pattern->is_local(_idx);
  }

  /**
   * Global offset of the iterator within overall element range.
   */
  gptrdiff_t pos() const {
    return _idx;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  const GlobMem<ElementType> & globmem() const {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  GlobMem<ElementType> & globmem() {
    return *_globmem;
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++() {
    ++_idx;
    return *this;
  }
  
  /**
   * Postfix increment operator.
   */
  self_t operator++(int) {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--() {
    --_idx;
    return *this;
  }
  
  /**
   * Postfix decrement operator.
   */
  self_t operator--(int) {
    self_t result = *this;
    --_idx;
    return result;
  }
  
  self_t & operator+=(gptrdiff_t n) {
    _idx += n;
    return *this;
  }
  
  self_t & operator-=(gptrdiff_t n) {
    _idx -= n;
    return *this;
  }

  self_t operator+(gptrdiff_t n) const {
    if (_viewspec == nullptr) {
      self_t res(
        _globmem,
        *_pattern,
        _idx + static_cast<size_t>(n));
      return res;
    }
    self_t res(
      _globmem,
      *_pattern,
      *_viewspec,
      _idx + static_cast<size_t>(n));
    return res;
  }

  self_t operator-(gptrdiff_t n) const {
    if (_viewspec == nullptr) {
      self_t res(
        _globmem,
        *_pattern,
        _idx - static_cast<size_t>(n));
      return res;
    }
    self_t res(
      _globmem,
      *_pattern,
      *_viewspec,
      _idx - static_cast<size_t>(n));
    return res;
  }

  gptrdiff_t operator+(
    const self_t & other) const {
    return _idx + other._idx;
  }

  gptrdiff_t operator-(
    const self_t & other) const {
    return _idx - other._idx;
  }

  bool operator<(const self_t & other) const {
    return _idx < other._idx;
  }

  bool operator<=(const self_t & other) const {
    return _idx <= other._idx;
  }

  bool operator>(const self_t & other) const {
    return _idx > other._idx;
  }

  bool operator>=(const self_t & other) const {
    return _idx >= other._idx;
  }

  bool operator==(const self_t & other) const {
    if (_idx == other._idx) {
      // Same global index
      if (_viewspec  == other._viewspec) {
        // Same viewspec instance
        return true;
      }
      if ((_viewspec != nullptr && other._viewspec != nullptr) && 
          (*_viewspec) == *(other._viewspec)) {
        // Same global index and same viewspec
        return true;
      }
    }
    // View projection at lhs and/or rhs set.
    // Convert both to GlobPtr (i.e. apply view projection) and compare:
    auto lhs_dart_gptr = GlobPtr<ElementType>(dart_gptr());
    auto rhs_dart_gptr = GlobPtr<ElementType>(other.dart_gptr());
    return lhs_dart_gptr == rhs_dart_gptr;
           
  }

  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  const PatternType & pattern() const {
    return *_pattern;
  }

private:
  /**
   * Convert a global offset within the global iterator's range to
   * corresponding global coordinates with respect to viewspec projection.
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType glob_index) const {
    // Global cartesian coords of current iterator position:
    std::array<IndexType, NumDimensions> glob_coords;
    if (_viewspec != nullptr) {
      DASH_LOG_TRACE_VAR("GlobIter.GlobPtr v", _viewspec->extents());
      DASH_LOG_TRACE_VAR("GlobIter.GlobPtr v", _viewspec->offsets());
      DASH_LOG_TRACE_VAR("GlobIter.GlobPtr v", _viewspec->rank());
      // Create cartesian index space from extents of view projection:
      CartesianIndexSpace<NumDimensions, Arrangement, IndexType> index_space(
        _viewspec->extents());
      // Initialize global coords with view coords (coords of iterator
      // position in view index space):
      glob_coords = index_space.coords(glob_index);
      DASH_LOG_TRACE_VAR("GlobIter.GlobPtr v", glob_coords);
      // Apply offset of view projection to view coords:
      for (dim_t d = 0; d < NumDimensions; ++d) {
        auto dim_offset = _viewspec->offsets()[d];
        DASH_LOG_TRACE_VAR("GlobIter.GlobPtr+o", dim_offset);
        glob_coords[d] += dim_offset;
        DASH_LOG_TRACE_VAR("GlobIter.GlobPtr+o", glob_coords);
      }
    } else {
      glob_coords = _pattern->memory_layout().coords(glob_index);
    }
    return glob_coords;
  }

};

/**
 * Resolve the number of elements between two global iterators.
 *
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *   g_last == g_first + (l_last - l_first)
 * Example: 
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * When iterating in local memory range [0,5[ of unit 0, the position of the
 * global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<typename ElementType>
gptrdiff_t distance(
  /// Global iterator to the initial position in the global sequence
  const GlobIter<ElementType> & first,
  /// Global iterator to the final position in the global sequence
  const GlobIter<ElementType> & last) {
  return last - first;
}

/**
 * Resolve the number of elements between two global pointers.
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *   g_last == g_first + (l_last - l_first)
 * Example: 
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * When iterating in local memory range [0,5[ of unit 0, the position of the
 * global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<typename ElementType>
gptrdiff_t distance(
  /// Global pointer to the initial position in the global sequence
  dart_gptr_t first,
  /// Global pointer to the final position in the global sequence
  dart_gptr_t last) {
  GlobPtr<ElementType> & gptr_first(dart_gptr_t(first));
  GlobPtr<ElementType> & gptr_last(dart_gptr_t(last));
  return gptr_last - gptr_first;
}

} // namespace dash

template <
  typename ElementType_,
  class Pattern_,
  class Pointer_,
  class Reference_ >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobIter<ElementType_, Pattern_, Pointer_, Reference_> & it)
{
  dash::GlobPtr<ElementType_> ptr(it); 
  os << "dash::GlobIter<ElementType, PatternType>: ";
  os << "idx=" << it._idx << std::endl;
  os << "--> " << ptr;
  return operator<<(os, ptr);
}

#endif // DASH__GLOB_ITER_H_
