#ifndef DASH__GLOB_ITER_H__INCLUDED
#define DASH__GLOB_ITER_H__INCLUDED

#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/GlobPtr.h>

#include <functional>
#include <sstream>

namespace dash {

#ifndef DOXYGEN

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
template <
    typename ElementType,
    class PatternType,
    class GlobMemType,
    // TODO rko: use pointer traits here
    class PointerType =
        typename GlobMemType::void_pointer::template rebind<ElementType>,
    class ReferenceType = GlobRef<ElementType> >
class GlobIter : public std::iterator<
                     std::random_access_iterator_tag,
                     ElementType,
                     typename PatternType::index_type,
                     PointerType,
                     ReferenceType> {
private:
  typedef GlobIter<
            ElementType,
            PatternType,
            GlobMemType,
            PointerType,
            ReferenceType>
    self_t;

  typedef typename std::remove_const<ElementType>::type
    nonconst_value_type;

public:
  typedef          ElementType                         value_type;

  typedef          ReferenceType                        reference;
  typedef typename ReferenceType::const_type      const_reference;

  typedef          PointerType                            pointer;
  typedef typename PointerType::const_type          const_pointer;

  typedef typename pointer::local_type local_type;

  typedef typename pointer::const_local_type const_local_type;

  typedef          PatternType                       pattern_type;
  typedef typename PatternType::index_type             index_type;

private:
  typedef GlobIter<
            const ElementType,
            PatternType,
            GlobMemType,
            const_pointer,
            const_reference >
    self_const_t;

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

  // For conversion to GlobViewIter
  template<
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend class GlobViewIter;

  // For comparison operators
  template<
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  friend class GlobIter;

private:
  static const dim_t      NumDimensions = PatternType::ndim();
  static const MemArrange Arrangement   = PatternType::memory_order();

protected:
  /// Global memory used to dereference iterated values.
  GlobMemType          * _globmem         = nullptr;
  /// Pattern that specifies the iteration order (access pattern).
  const PatternType    * _pattern         = nullptr;
  /// Current position of the iterator in global canonical index space.
  index_type             _idx             = 0;
  /// Maximum position allowed for this iterator.
  index_type             _max_idx         = 0;
public:

  constexpr GlobIter() = default;

  /**
   * Constructor, creates a global iterator on global memory following
   * the element order specified by the given pattern.
   */
  constexpr GlobIter(
    GlobMemType       * gmem,
    const PatternType & pat,
    index_type          position = 0)
  : _globmem(gmem),
    _pattern(&pat),
    _idx(position),
    _max_idx(std::max(index_type(pat.size()) - 1, index_type(0)))
  { }

  /**
   * Copy constructor.
   */
  template <
    class    Ptr_,
    class    Ref_ >
  constexpr GlobIter(
    const GlobIter<nonconst_value_type, PatternType, GlobMemType, Ptr_, Ref_> & other)
  : _globmem(other._globmem)
  , _pattern(other._pattern)
  , _idx    (other._idx)
  , _max_idx(other._max_idx)
  { }

  /**
   * Move constructor.
   */
  template <
    class    Ptr_,
    class    Ref_ >
  constexpr GlobIter(
    GlobIter<nonconst_value_type, PatternType, GlobMemType, Ptr_, Ref_> && other)
  : _globmem(other._globmem)
  , _pattern(other._pattern)
  , _idx    (other._idx)
  , _max_idx(other._max_idx)
  { }

  /**
   * Assignment operator.
   */
  template <
    typename T_,
    class    Ptr_,
    class    Ref_ >
  self_t & operator=(
    const GlobIter<T_, PatternType, GlobMemType, Ptr_, Ref_ > & other)
  {
    _globmem = other._globmem;
    _pattern = other._pattern;
    _idx     = other._idx;
    _max_idx = other._max_idx;
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  template <
    typename T_,
    class    Ptr_,
    class    Ref_ >
  self_t & operator=(
    GlobIter<T_, PatternType, GlobMemType, Ptr_, Ref_ > && other)
  {
    _globmem = other._globmem;
    _pattern = other._pattern;
    _idx     = other._idx;
    _max_idx = other._max_idx;
    // no ownership to transfer
    return *this;
  }

  /**
   * The number of dimensions of the iterator's underlying pattern.
   */
  static constexpr dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * <fuchsto> TODO: Conversion from iterator to pointer looks dubios
   *
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  explicit operator const_pointer() const {
    return const_pointer(this->dart_gptr());
  }

  /**
   * <fuchsto> TODO: Conversion from iterator to pointer looks dubios
   *
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  explicit operator pointer() {
    return pointer(this->dart_gptr());
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    if (_globmem == nullptr) {
      return DART_GPTR_NULL;
    }
    else if (_idx > _max_idx) {
      // Global iterator pointing past the range indexed by the pattern
      // which is the case for .end() iterators.
      return static_cast<dart_gptr_t>(_globmem->end());
    }

    DASH_LOG_TRACE_VAR("GlobIter.dart_gptr()", _idx);
    typedef typename pattern_type::local_index_t
      local_pos_t;

    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(_idx);
    DASH_LOG_TRACE("GlobIter.dart_gptr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    auto const dart_pointer = _get_pointer_at(local_pos);
    DASH_ASSERT_MSG(!DART_GPTR_ISNULL(dart_pointer), "dart pointer must not be null");
    return dart_pointer;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  inline reference operator*()
  {
    return reference{this->dart_gptr()};
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  inline const_reference operator*() const
  {
    return const_reference{this->dart_gptr()};
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  reference operator[](
    /// The global position of the element
    index_type g_index)
  {
    auto p = *this;
    p += g_index;
    return reference(p.dart_gptr());
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  const_reference operator[](
    /// The global position of the element
    index_type g_index) const
  {
    auto p = *this;
    p += g_index;
    return const_reference(p.dart_gptr());
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  constexpr bool is_local() const
  {
    return (_globmem->team().myid() == lpos().unit);
  }

  /**
   * Convert global iterator to native pointer.
   */
  local_type local() const
  {

    auto local_pos = lpos();

    if (local_pos.unit != _pattern->team().myid()) {
      return nullptr;
    }

    auto* lbegin = dash::local_begin(
        static_cast<pointer>(_globmem->begin()), _pattern->team().myid());

    DASH_ASSERT(lbegin);

    return std::next(lbegin, local_pos.index);
  }

  /**
   * Unit and local offset at the iterator's position.
   */
  auto lpos() const
  {
    DASH_LOG_TRACE_VAR("GlobIter.lpos()", _idx);

    index_type idx = _idx;
    index_type offset = 0;

    // Convert iterator position (_idx) to local index and unit.
    if (idx > _max_idx) {
      idx    = _max_idx;
      offset = _idx - _max_idx;
      DASH_ASSERT_EQ(offset, 1, "invalid index");
    }
    // Global index to local index and unit:
    auto local_pos = _pattern->local(idx);
    // Add the offset
    local_pos.index += offset;

    DASH_LOG_TRACE(
        "GlobIter.lpos >",
        "unit:",
        local_pos.unit,
        "local index:",
        local_pos.index);

    return local_pos;
  }

  /**
   * Map iterator to global index domain.
   */
  constexpr const self_t & global() const noexcept {
    return *this;
  }

  /**
   * Map iterator to global index domain.
   */
  self_t & global() {
    return *this;
  }

  /**
   * Position of the iterator in global index space.
   */
  constexpr index_type pos() const noexcept
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   */
  constexpr index_type gpos() const noexcept
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
  constexpr bool is_relative() const noexcept
  {
    return false;
  }


  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   */
  constexpr const GlobMemType & globmem() const noexcept
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
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

  constexpr self_t operator+(index_type n) const noexcept
  {
    return self_t(
      _globmem,
      *_pattern,
      _idx + static_cast<index_type>(n));
  }

  constexpr self_t operator-(index_type n) const noexcept
  {
    return self_t(
      _globmem,
      *_pattern,
      _idx - static_cast<index_type>(n));
  }

  template <class GlobIterT>
  constexpr auto operator+(
    const GlobIterT & other) const noexcept
    -> typename std::enable_if<
         !std::is_integral<GlobIterT>::value,
         index_type
       >::type
  {
    return _idx + other._idx;
  }

  template <class GlobIterT>
  constexpr auto operator-(
    const GlobIterT & other) const noexcept
    -> typename std::enable_if<
         !std::is_integral<GlobIterT>::value,
         index_type
       >::type
  {
    return _idx - other._idx;
  }

  template <class GlobIterT>
  constexpr bool operator<(const GlobIterT & other) const noexcept
  {
    return (_idx < other._idx);
  }

  template <class GlobIterT>
  constexpr bool operator<=(const GlobIterT & other) const noexcept
  {
    return (_idx <= other._idx);
  }

  template <class GlobIterT>
  constexpr bool operator>(const GlobIterT & other) const noexcept
  {
    return (_idx > other._idx);
  }

  template <class GlobIterT>
  constexpr bool operator>=(const GlobIterT & other) const noexcept
  {
    return (_idx >= other._idx);
  }

  template <class GlobIterT>
  constexpr bool operator==(const GlobIterT & other) const noexcept
  {
    return _idx == other._idx;
  }

  template <class GlobIterT>
  constexpr bool operator!=(const GlobIterT & other) const noexcept
  {
    return _idx != other._idx;
  }

  constexpr const PatternType & pattern() const noexcept
  {
    return *_pattern;
  }

  constexpr dash::Team & team() const noexcept
  {
    return _pattern->team();
  }

private:

  dart_gptr_t _get_pointer_at(typename pattern_type::local_index_t pos) const {

    auto dart_pointer = static_cast<dart_gptr_t>(_globmem->begin());

    DASH_ASSERT(pos.index >= 0);

    dart_pointer.unitid = pos.unit;

    dart_pointer.addr_or_offs.offset += pos.index * sizeof(value_type);

    return dart_pointer;
  }

}; // class GlobIter


template <
  typename ElementType,
  class    Pattern,
  class    GlobStaticMem,
  class    Pointer,
  class    Reference >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobIter<
          ElementType, Pattern, GlobStaticMem, Pointer, Reference> & it)
{
  std::ostringstream ss;
  dash::GlobPtr<const ElementType, GlobStaticMem> ptr(it.dart_gptr());
  ss << "dash::GlobIter<" << typeid(ElementType).name() << ">("
     << "idx:"  << it._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__GLOB_ITER_H__INCLUDED
