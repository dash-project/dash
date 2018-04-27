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
template<
  typename ElementType,
  class    PatternType,
  class    GlobMemType   = GlobStaticMem<
                             typename std::decay<ElementType>::type
                           >,
  class    PointerType   = typename GlobMemType::pointer,
  class    ReferenceType = GlobRef<ElementType> >
class GlobIter
// : public std::iterator<
//            std::random_access_iterator_tag,
//            ElementType,
//            typename PatternType::index_type,
//            PointerType,
//            ReferenceType >
{
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
  typedef typename std::random_access_iterator_tag iterator_category;

  typedef typename PatternType::index_type           difference_type;

  typedef          ElementType                            value_type;

  typedef          ReferenceType                           reference;
  typedef typename dash::const_value_cast<ReferenceType>::type
                                                     const_reference;

  typedef          PointerType                               pointer;
  typedef typename dash::const_value_cast<PointerType>::type
                                                       const_pointer;

  typedef typename GlobMemType::local_pointer          local_pointer;
  typedef typename GlobMemType::local_pointer             local_type;

  typedef          PatternType                          pattern_type;
  typedef typename PatternType::index_type                index_type;

private:
  typedef GlobIter<
            const ElementType,
            PatternType,
            GlobMemType,
            const_pointer,
            const_reference >
    self_const_t;

public:
  typedef std::integral_constant<bool, false>            has_view;

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
  /// Unit id of the active unit
  team_unit_t            _myid            = DART_UNDEFINED_TEAM_UNIT_ID;
  /// Pointer to first element in local memory
  local_pointer          _lbegin          = nullptr;

public:
  /**
   * Default constructor.
   */
  constexpr GlobIter()
  : _globmem(nullptr),
    _pattern(nullptr),
    _idx(0),
    _max_idx(0),
    _myid(dash::Team::All().myid()),
    _lbegin(nullptr)
  { }

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
    _max_idx(pat.size() - 1),
    _myid(pat.team().myid()),
    _lbegin(_globmem->lbegin())
  { }

  /**
   * Copy constructor.
   */
  constexpr GlobIter(const self_t & other) = default;

  /**
   * Templated copy constructor.
   */
  template <
    class    T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  constexpr GlobIter(
    const GlobIter<T_, P_, GM_, Ptr_, Ref_> & other)
  : _globmem(other._globmem)
  , _pattern(other._pattern)
  , _idx    (other._idx)
  , _max_idx(other._max_idx)
  , _myid   (other._myid)
  , _lbegin (other._lbegin)
  { }

  /**
   * Move constructor.
   */
  constexpr GlobIter(self_t && other) = default;

  /**
   * Templated move constructor.
   */
  template <
    class    T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  constexpr GlobIter(
    GlobIter<T_, P_, GM_, Ptr_, Ref_> && other)
  : _globmem(other._globmem)
  , _pattern(other._pattern)
  , _idx    (other._idx)
  , _max_idx(other._max_idx)
  , _myid   (other._myid)
  , _lbegin (other._lbegin)
  { }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * Templated assignment operator.
   */
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  self_t & operator=(
    const GlobIter<T_, P_, GM_, Ptr_, Ref_ > & other)
  {
    _globmem = other._globmem;
    _pattern = other._pattern;
    _idx     = other._idx;
    _max_idx = other._max_idx;
    _myid    = other._myid;
    _lbegin  = other._lbegin;
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other) = default;

  /**
   * Templated move-assignment operator.
   */
  template <
    typename T_,
    class    P_,
    class    GM_,
    class    Ptr_,
    class    Ref_ >
  self_t & operator=(
    GlobIter<T_, P_, GM_, Ptr_, Ref_ > && other)
  {
    _globmem = other._globmem;
    _pattern = other._pattern;
    _idx     = other._idx;
    _max_idx = other._max_idx;
    _myid    = other._myid;
    _lbegin  = other._lbegin;
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
    DASH_LOG_TRACE_VAR("GlobIter.const_pointer()", _idx);
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
    }
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobIter.const_pointer >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.const_pointer >", local_pos.index);
    // Create global pointer from unit and local offset:
    const_pointer gptr(
      _globmem->at(team_unit_t(local_pos.unit), local_pos.index)
    );
    return gptr + offset;
  }

  /**
   * <fuchsto> TODO: Conversion from iterator to pointer looks dubios
   *
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  explicit operator pointer() {
    DASH_LOG_TRACE_VAR("GlobIter.pointer()", _idx);
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
    }
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(idx);
    DASH_LOG_TRACE_VAR("GlobIter.pointer >", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobIter.pointer >", local_pos.index);
    // Create global pointer from unit and local offset:
    pointer gptr(
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
#if 0
    const_pointer gptr(
#else
    // TODO: Intermediate, should be `const_pointer`.
    //       Actual issue is `const_pointer` defined as local pointer
    //       type, should be global pointer for global iterator like
    //       `GlobIter`:
    dash::GlobPtr<ElementType> gptr(
#endif
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
  inline reference operator*()
  {
    return this->operator[](_idx);
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  inline const_reference operator*() const
  {
    return this->operator[](_idx);
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  reference operator[](
    /// The global position of the element
    index_type g_index)
  {
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(g_index);
    // Global reference to element at given position:
    DASH_LOG_TRACE("GlobIter.[]",
                   "(index:", g_index, ") ->",
                   "(unit:", local_pos.unit, " index:", local_pos.index, ")");
    return reference(
             _globmem->at(local_pos.unit,
                          local_pos.index));
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  const_reference operator[](
    /// The global position of the element
    index_type g_index) const
  {
    typedef typename pattern_type::local_index_t
      local_pos_t;
    // Global index to local index and unit:
    local_pos_t local_pos = _pattern->local(g_index);
    // Global reference to element at given position:
    DASH_LOG_TRACE("GlobIter.[]",
                   "(index:", g_index, ") ->",
                   "(unit:", local_pos.unit, " index:", local_pos.index, ")");
    return const_reference(
             _globmem->at(local_pos.unit,
                          local_pos.index));
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  constexpr bool is_local() const
  {
    return (_myid == lpos().unit);
  }

  /**
   * Convert global iterator to native pointer.
   */
  local_pointer local() const
  {
    /*
     *
     * TODO: Evaluate alternative:
     *         auto l_idx_this = _container.pattern().local(this->pos());
     *         return (l_idx_this.unit == _myid
     *                 ? _lbegin + l_idx_this
     *                 : nullptr
     *                );
     */
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
  dash::GlobPtr<const ElementType, GlobStaticMem> ptr(*it._globmem,
                                                      it.dart_gptr());
  ss << "dash::GlobIter<" << typeid(ElementType).name() << ">("
     << "idx:"  << it._idx << ", "
     << "gptr:" << ptr << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#include <dash/iterator/GlobViewIter.h>

#endif // DASH__GLOB_ITER_H__INCLUDED
