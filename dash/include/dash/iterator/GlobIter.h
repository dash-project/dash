#ifndef DASH__GLOB_ITER_H_
#define DASH__GLOB_ITER_H_

#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>

#include <functional>
#include <sstream>

namespace dash {

#ifndef DOXYGEN

// Forward-declaration
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType,
  class    PointerType,
  class    ReferenceType >
class GlobStencilIter;
// Forward-declaration
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType,
  class    PointerType,
  class    ReferenceType >
class GlobViewIter;

#endif // DOXYGEN

/**
 * \defgroup  DashGlobalIteratorConcept  Global Iterator Concept
 * Concept for iterators in global index space.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Methods
 * Return Type             | Method                 | Parameters     | Description                                             |
 * ----------------------- | ---------------------- | -------------- | ------------------------------------------------------- |
 * <tt>dart_gptr_t</tt>    | <tt>dart_gptr</tt>     | &nbsp;         | DART global pointer on the iterator's current position. |
 *
 * \}
 */


/**
 * Iterator on Partitioned Global Address Space.
 *
 * \concept{DashGlobalIteratorConcept}
 */
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType   = GlobMem<ElementType>,
  class    PointerType   = GlobPtr<ElementType, PatternType>,
  class    ReferenceType = GlobRef<ElementType> >
class GlobIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           typename PatternType::index_type,
           PointerType,
           ReferenceType >
{
private:
  typedef GlobIter<
            ElementType,
            PatternType,
            GlobMemType,
            PointerType,
            ReferenceType>
    self_t;

public:
  typedef       ElementType                       value_type;
  typedef       ReferenceType                      reference;
  typedef const ReferenceType                const_reference;
  typedef       PointerType                          pointer;
  typedef const PointerType                    const_pointer;

  typedef typename GlobMemType::local_pointer  local_pointer;

  typedef          PatternType                  pattern_type;
  typedef typename PatternType::index_type        index_type;

public:
  typedef std::integral_constant<bool, false>       has_view;

public:
  // For ostream output
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend std::ostream & operator<<(
           std::ostream & os,
           const GlobIter<T_, P_, GM_, Ptr_, Ref_> & it);

  // For conversion to GlobStencilIter
  template<
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend class GlobStencilIter;

  // For conversion to GlobViewIter
  template<
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend class GlobViewIter;

private:
  static const dim_t      NumDimensions = PatternType::ndim();
  static const MemArrange Arrangement   = PatternType::memory_order();

protected:
  /// Global memory used to dereference iterated values.
  GlobMemType          * _globmem;
  /// Pattern that specifies the iteration order (access pattern).
  const PatternType    * _pattern;
  /// Current position of the iterator in global canonical index space.
  index_type             _idx             = 0;
  /// Maximum position allowed for this iterator.
  index_type             _max_idx         = 0;
  /// Unit id of the active unit
  team_unit_t            _myid;
  /// Pointer to first element in local memory
  local_pointer          _lbegin          = nullptr;

public:
  /**
   * Default constructor.
   */
  GlobIter()
  : _globmem(nullptr),
    _pattern(nullptr),
    _idx(0),
    _max_idx(0),
    _myid(dash::Team::All().myid()),
    _lbegin(nullptr)
  {
    DASH_LOG_TRACE_VAR("GlobIter()", _idx);
    DASH_LOG_TRACE_VAR("GlobIter()", _max_idx);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern.
   */
  GlobIter(
    GlobMemType       * gmem,
	  const PatternType & pat,
	  index_type          position = 0)
  : _globmem(gmem),
    _pattern(&pat),
    _idx(position),
    _max_idx(pat.size() - 1),
    _myid(pat.team().myid()),
    _lbegin(_globmem->lbegin())
  {
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,idx,abs)", _idx);
    DASH_LOG_TRACE_VAR("GlobIter(gmem,pat,idx,abs)", _max_idx);
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
   * The number of dimensions of the iterator's underlying pattern.
   */
  static dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator PointerType() const
  {
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr()", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr", idx);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr", offset);
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.GlobPtr >", local_pos.index);
    // Create global pointer from unit and local offset:
    PointerType gptr(
      _globmem->at(team_unit_t(local_pos.unit), local_pos.index)
    );
    return gptr + offset;
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("GlobIter.dart_gptr()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
      DASH_LOG_TRACE_VAR("GlobIter.dart_gptr", _max_idx);
      DASH_LOG_TRACE_VAR("GlobIter.dart_gptr", idx);
      DASH_LOG_TRACE_VAR("GlobIter.dart_gptr", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE("GlobIter.dart_gptr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    // Global pointer to element at given position:
    dash::GlobPtr<ElementType, PatternType> gptr(
      _globmem->at(
        team_unit_t(local_pos.unit),
        local_pos.index)
    );
    DASH_LOG_TRACE_VAR("GlobIter.dart_gptr >", gptr);
    return (gptr + offset).dart_gptr();
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  ReferenceType operator*() const
  {
    DASH_LOG_TRACE("GlobIter.*", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx = _idx;
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobIter.*", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.*", local_pos.index);
    // Global reference to element at given position:
    return ReferenceType(
             _globmem->at(local_pos.unit,
                          local_pos.index));
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  ReferenceType operator[](
    /// The global position of the element
    index_type g_index) const
  {
    DASH_LOG_TRACE("GlobIter.[]", g_index);
    index_type idx = g_index;
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.[]", local_pos.index);
    // Global reference to element at given position:
    return ReferenceType(
             _globmem->at(local_pos.unit,
                          local_pos.index));
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  inline bool is_local() const
  {
    return (_myid == lpos().unit);
  }

  /**
   * Convert global iterator to native pointer.
   */
  local_pointer local() const
  {
    DASH_LOG_TRACE_VAR("GlobIter.local=()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    DASH_LOG_TRACE_VAR("GlobIter.local=", _max_idx);
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx     = _max_idx;
      offset += _idx - _max_idx;
    }
    DASH_LOG_TRACE_VAR("GlobIter.local=", idx);
    DASH_LOG_TRACE_VAR("GlobIter.local=", offset);
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobIter.local= >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.local= >", local_pos.index);
    if (_myid != local_pos.unit) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + local_pos.index + offset);
  }

  /**
   * Unit and local offset at the iterator's position.
   */
  inline typename pattern_type::local_index_t lpos() const
  {
    DASH_LOG_TRACE_VAR("GlobIter.lpos()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;
    index_type idx    = _idx;
    index_type offset = 0;
    // Convert iterator position (_idx) to local index and unit.
    if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      idx    = _max_idx;
      offset = _idx - _max_idx;
      DASH_LOG_TRACE_VAR("GlobIter.lpos", _max_idx);
      DASH_LOG_TRACE_VAR("GlobIter.lpos", idx);
      DASH_LOG_TRACE_VAR("GlobIter.lpos", offset);
    }
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    local_pos.index += offset;
    DASH_LOG_TRACE("GlobIter.lpos >",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    return local_pos;
  }

  /**
   * Map iterator to global index domain.
   */
  inline self_t global() const
  {
    return *this;
  }

  /**
   * Position of the iterator in global index space.
   */
  inline index_type pos() const
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   */
  inline index_type gpos() const
  {
    return _idx;
  }

  /**
   * Whether the iterator's position is relative to a view.
   *
   * TODO:
   * should be iterator trait:
   *   dash::iterator_traits<GlobIter<..>>::is_relative()::value
   */
  inline constexpr bool is_relative() const noexcept
  {
    return false;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline const GlobMemType & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline GlobMemType & globmem()
  {
    return *_globmem;
  }

  /**
   * Prefix increment operator.
   */
  inline self_t & operator++()
  {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  inline self_t operator++(int)
  {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  inline self_t & operator--()
  {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  inline self_t operator--(int)
  {
    self_t result = *this;
    --_idx;
    return result;
  }

  inline self_t & operator+=(index_type n)
  {
    _idx += n;
    return *this;
  }

  inline self_t & operator-=(index_type n)
  {
    _idx -= n;
    return *this;
  }

  inline self_t operator+(index_type n) const
  {
    self_t res(
      _globmem,
      *_pattern,
      _idx + static_cast<index_type>(n));
    return res;
  }

  inline self_t operator-(index_type n) const
  {
    self_t res(
      _globmem,
      *_pattern,
      _idx - static_cast<index_type>(n));
    return res;
  }

  inline index_type operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  inline index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }

  inline bool operator<(const self_t & other) const
  {
    return (_idx < other._idx);
  }

  inline bool operator<=(const self_t & other) const
  {
    return (_idx <= other._idx);
  }

  inline bool operator>(const self_t & other) const
  {
    return (_idx > other._idx);
  }

  inline bool operator>=(const self_t & other) const
  {
    return (_idx >= other._idx);
  }

  inline bool operator==(const self_t & other) const
  {
    return _idx == other._idx;
  }

  inline bool operator!=(const self_t & other) const
  {
    return _idx != other._idx;
  }

  inline const PatternType & pattern() const
  {
    return *_pattern;
  }

  inline dash::Team & team() const
  {
    return _pattern->team();
  }

}; // class GlobIter

/**
 * Resolve the number of elements between two global iterators.
 *
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *   g_last == g_first + (l_last - l_first)
 * Example:
 *
 * \code
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * \endcode
 *
 * When iterating in local memory range [0,5[ of unit 0, the position of the
 * global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<
  typename ElementType,
  class    Pattern,
  class    GlobMem,
  class    Pointer,
  class    Reference >
auto distance(
  const GlobIter<ElementType, Pattern, GlobMem, Pointer, Reference> &
    first,
  /// Global iterator to the final position in the global sequence
  const GlobIter<ElementType, Pattern, GlobMem, Pointer, Reference> &
    last)
-> typename Pattern::index_type
{
  return last - first;
}

/**
 * Resolve the number of elements between two global pointers.
 * The difference of global pointers is not well-defined if their range
 * spans over more than one block.
 * The corresponding invariant is:
 *
 * \code
 *   g_last == g_first + (l_last - l_first)
 * \endcode
 *
 * \code
 * Example:
 *   unit:            0       1       0
 *   local offset:  | 0 1 2 | 0 1 2 | 3 4 5 | ...
 *   global offset: | 0 1 2   3 4 5   6 7 8   ...
 *   range:          [- - -           - -]
 * \endcode
 *
 * When iterating in local memory range [0,5[ of unit 0, the position of the
 * global iterator to return is 8 != 5
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<typename ElementType>
dash::default_index_t distance(
  /// Global pointer to the initial position in the global sequence
  dart_gptr_t first,
  /// Global pointer to the final position in the global sequence
  dart_gptr_t last)
{
  GlobPtr<ElementType> & gptr_first(first);
  GlobPtr<ElementType> & gptr_last(last);
  return gptr_last - gptr_first;
}

template <
  typename ElementType,
  class    Pattern,
  class    GlobMem,
  class    Pointer,
  class    Reference >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobIter<
          ElementType, Pattern, GlobMem, Pointer, Reference> & it)
{
  std::ostringstream ss;
  dash::GlobPtr<ElementType, Pattern> ptr(it);
  ss << "dash::GlobIter<" << typeid(ElementType).name() << ">("
     << "idx:"  << it._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#include <dash/iterator/GlobViewIter.h>

#endif // DASH__GLOB_ITER_H_
