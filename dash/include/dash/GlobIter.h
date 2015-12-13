#ifndef DASH__GLOB_ITER_H_
#define DASH__GLOB_ITER_H_

#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>
#include <functional>

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

  typedef       PatternType                     pattern_type;
  typedef       IndexType                         index_type;

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
  /// Unit id of the active unit
  dart_unit_t            _myid;
  /// Pointer to first element in local memory
  ElementType          * _lbegin     = nullptr;

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
    _max_idx(0),
    _myid(dash::myid()),
    _lbegin(nullptr) {
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
    _max_idx(pat.size() - 1),
    _myid(dash::myid()),
    _lbegin(_globmem->lbegin()) {
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
    _max_idx(viewspec.size() - 1),
    _myid(dash::myid()),
    _lbegin(_globmem->lbegin()) {
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
    typedef typename pattern_type::local_index_t
      local_pos_t;
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
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
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
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(_idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(_idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
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
    gptrdiff_t g_index) const {
    DASH_LOG_TRACE_VAR("GlobIter.[]", g_index);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(g_index);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(g_index);
      local_pos        = _pattern->local_index(glob_coords);
    }
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
  inline bool is_local() const {
    return _pattern->is_local(_idx);
  }

  /**
   * Convert global iterator to native pointer.
   */
  ElementType * local() {
    DASH_LOG_TRACE_VAR("GlobIter.local()=", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(_idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(_idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobIter.local=", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.local=", local_pos.index);
    if (_myid != local_pos.unit) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + local_pos.index);
  }

  /**
   * Convert global iterator to native pointer.
   */
  const ElementType * local() const {
    DASH_LOG_TRACE_VAR("GlobIter.local()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos;
    if (_viewspec == nullptr) {
      // No viewspec mapping required:
      local_pos        = _pattern->local(_idx);
    } else {
      // Viewspec projection required:
      auto glob_coords = coords(_idx);
      local_pos        = _pattern->local_index(glob_coords);
    }
    DASH_LOG_TRACE_VAR("GlobIter.local", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.local", local_pos.index);
    if (_myid != local_pos.unit) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + local_pos.index);
  }

  /**
   * Global offset of the iterator within overall element range.
   */
  inline gptrdiff_t pos() const {
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

  inline bool operator<(const self_t & other) const {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::less<index_type>(),
                   std::less<pointer>());
  }

  inline bool operator<=(const self_t & other) const {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::less_equal<index_type>(),
                   std::less_equal<pointer>());
  }

  inline bool operator>(const self_t & other) const {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::greater<index_type>(),
                   std::greater<pointer>());
  }

  inline bool operator>=(const self_t & other) const {
    // NOTE:
    // This function call is significantly slower than the explicit
    // implementation in operator== and operator!=.
    return compare(other,
                   std::greater_equal<index_type>(),
                   std::greater_equal<pointer>());
  }

  inline bool operator==(const self_t & other) const {
    // NOTE: See comments in method compare().
    if (_viewspec == other._viewspec) {
      // Same viewspec pointer
      return _idx == other._idx;
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) && 
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return _idx == other._idx;
    }
    const pointer lhs_dart_gptr(dart_gptr());
    const pointer rhs_dart_gptr(other.dart_gptr());
    return (lhs_dart_gptr == rhs_dart_gptr);
  }

  inline bool operator!=(const self_t & other) const {
    // NOTE: See comments in method compare().
    if (_viewspec == other._viewspec) {
      // Same viewspec pointer
      return _idx != other._idx;
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) && 
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return _idx != other._idx;
    }
    const pointer lhs_dart_gptr(dart_gptr());
    const pointer rhs_dart_gptr(other.dart_gptr());
    return (lhs_dart_gptr != rhs_dart_gptr);
  }

  inline const PatternType & pattern() const {
    return *_pattern;
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template<
    class GlobIndexCmpFun,
    class GlobPtrCmpFun >
  bool compare(
    const self_t & other,
    const GlobIndexCmpFun & gidx_cmp,
    const GlobPtrCmpFun   & gptr_cmp) const {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if (this == &other) {
      return true;
    }
#endif
    // NOTE:
    // Do not check _idx first, as it would never match for comparison with an
    // end iterator.
    if (_viewspec == other._viewspec) {
      // Same viewspec pointer
      return gidx_cmp(_idx, other._idx);
    }
    if ((_viewspec != nullptr && other._viewspec != nullptr) && 
        (*_viewspec) == *(other._viewspec)) {
      // Viewspec instances are equal
      return gidx_cmp(_idx, other._idx);
    }
    // View projection at lhs and/or rhs set.
    // Convert both to GlobPtr (i.e. apply view projection) and compare.
    //
    // NOTE:
    // This conversion is quite expensive but will never be necessary
    // if both iterators have been created from the same range.
    // Example:
    //   a.block(1).begin() == a.block(1).end().
    // does not require viewspace projection while
    //   a.block(1).begin() == a.end()
    // does. The latter case should be avoided for this reason.
    const pointer lhs_dart_gptr(dart_gptr());
    const pointer rhs_dart_gptr(other.dart_gptr());
    return gptr_cmp(lhs_dart_gptr, rhs_dart_gptr);
  }

  /**
   * Convert a global offset within the global iterator's range to
   * corresponding global coordinates with respect to viewspec projection.
   *
   * NOTE:
   * This method could be specialized for NumDimensions = 1 for performance
   * tuning.
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
