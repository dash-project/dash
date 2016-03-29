#ifndef DASH__GLOB_DYNAMIC_MEM_H_
#define DASH__GLOB_DYNAMIC_MEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>

#include <dash/internal/Logging.h>

namespace dash {

/**
 * Global memory region with dynamic size.
 */
template<
  /// Type of values allocated in the global memory space
  typename ElementType,
  /// Type of allocator implementation used to allocate and deallocate
  /// global memory
  class    AllocatorType =
             dash::allocator::LocalAllocator<ElementType> >
class GlobDynamicMem
{
private:
  typedef GlobDynamicMem<ElementType, AllocatorType>
    self_t;

public:
  typedef AllocatorType                                      allocator_type;
  typedef ElementType                                            value_type;
  typedef typename AllocatorType::size_type                       size_type;
  typedef typename AllocatorType::difference_type           difference_type;
  typedef typename AllocatorType::difference_type                index_type;
  typedef typename AllocatorType::pointer                       raw_pointer;
  typedef typename AllocatorType::void_pointer                 void_pointer;
  typedef typename AllocatorType::const_void_pointer     const_void_pointer;
  typedef GlobPtr<ElementType>                                      pointer;
  typedef GlobPtr<const ElementType>                          const_pointer;
  typedef GlobRef<ElementType>                                    reference;
  typedef GlobRef<const ElementType>                        const_reference;

  typedef ElementType &                                     local_reference;
  typedef const ElementType &                         const_local_reference;
  // TODO: define specific local pointer type
  typedef ElementType *                                       local_pointer;
  typedef const ElementType *                           const_local_pointer;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   */
  GlobDynamicMem(
    /// Number of local elements to allocate in global memory space
    size_type   n_local_elem,
    /// Team containing all units operating on the global memory region
    Team      & team = dash::Team::Null())
  : _allocator(team),
    _teamid(team.dart_id()),
    _nlelem(n_local_elem)
  {
    DASH_LOG_TRACE("GlobDynamicMem(nunits,nelem)", team.size(), _nlelem);

    _begptr = _allocator.allocate(_nlelem);
    DASH_ASSERT_NE(DART_GPTR_NULL, _begptr, "allocation failed");

    if (_teamid == DART_TEAM_NULL) {
      _nunits = 1;
    } else {
      DASH_ASSERT_RETURNS(
        dart_team_size(_teamid, &_nunits),
        DART_OK);
    }
    _lbegin = lbegin(dash::myid());
    _lend   = lend(dash::myid());
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobDynamicMem()
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.~GlobDynamicMem()", _begptr);
    _allocator.deallocate(_begptr);
    DASH_LOG_TRACE("GlobDynamicMem.~GlobDynamicMem >");
  }

  /**
   * Copy constructor.
   */
  GlobDynamicMem(const self_t & other)
    = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs)
    = default;

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & rhs) const
  {
    return (_begptr == rhs._begptr &&
            _teamid == rhs._teamid &&
            _nunits == rhs._nunits &&
            _nlelem == rhs._nlelem &&
            _lbegin == rhs._lbegin &&
            _lend   == rhs._lend);
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & rhs) const
  {
    return !(*this == rhs);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_pointer begin() const
  {
    return const_pointer(_begptr);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  pointer begin()
  {
    return pointer(_begptr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  const_local_pointer lbegin(
    dart_unit_t unit_id) const
  {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin const()", unit_id);
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin const >", addr);
    return const_local_pointer(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  local_pointer lbegin(
    dart_unit_t unit_id)
  {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin()", unit_id);
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin",
                       GlobPtr<ElementType>((dart_gptr_t)gptr));
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin >", addr);
    return local_pointer(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_pointer lbegin() const
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline local_pointer lbegin()
  {
    return _lbegin;
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  const_local_pointer lend(
    dart_unit_t unit_id) const
  {
    void *addr;
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, _nlelem * sizeof(ElementType)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    return const_local_pointer(addr);
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  local_pointer lend(
    dart_unit_t unit_id)
  {
    void *addr;
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, _nlelem * sizeof(ElementType)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    return local_pointer(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_pointer lend() const
  {
    return _lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline local_pointer lend()
  {
    return _lend;
  }

  /**
   * Write value to global memory at given offset.
   *
   * \see  dash::put_value
   */
  template<typename ValueType = ElementType>
  void put_value(
    const ValueType & newval,
    index_type        global_index)
  {
    DASH_LOG_TRACE("GlobDynamicMem.put_value(newval, gidx = %d)",
                   global_index);
    dart_gptr_t gptr = _begptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(
        &gptr,
        global_index * sizeof(ValueType)),
      DART_OK);
    dash::put_value(newval, GlobPtr<ValueType>(gptr));
  }

  /**
   * Read value from global memory at given offset.
   *
   * \see  dash::get_value
   */
  template<typename ValueType = ElementType>
  void get_value(
    ValueType  * ptr,
    index_type   global_index)
  {
    DASH_LOG_TRACE("GlobDynamicMem.get_value(newval, gidx = %d)",
                   global_index);
    dart_gptr_t gptr = _begptr;
    dart_gptr_incaddr(&gptr, global_index * sizeof(ValueType));
    dash::get_value(ptr, GlobPtr<ValueType>(gptr));
  }

  /**
   * Synchronize all units associated with this global memory instance.
   */
  void barrier()
  {
    DASH_ASSERT_RETURNS(
      dart_barrier(_teamid),
      DART_OK);
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced
   * global memory on all units.
   */
  void flush()
  {
    dart_flush(_begptr);
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced
   * global memory on all units.
   */
  void flush_all()
  {
    dart_flush_all(_begptr);
  }

  void flush_local()
  {
    dart_flush_local(_begptr);
  }

  void flush_local_all()
  {
    dart_flush_local_all(_begptr);
  }

  /**
   * Resolve the global pointer from an element position in a unit's
   * local memory.
   */
  template<typename IndexT>
  dart_gptr_t index_to_gptr(
    /// The unit id
    dart_unit_t unit,
    /// The unit's local address offset
    IndexT      local_index) const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.index_to_gptr(unit,l_idx)",
                   unit, local_index);
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = _begptr;
    // Resolve global unit id
    dart_unit_t lunit, gunit;
    DASH_LOG_DEBUG("GlobDynamicMem.index_to_gptr (=g_begp)  ", gptr);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", gptr.unitid);
    // Resolve local unit id from global unit id in global pointer:
    dart_team_unit_g2l(_teamid, gptr.unitid, &lunit);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", lunit);
    lunit = (lunit + unit) % _nunits;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", lunit);
    if (_teamid != dash::Team::All().dart_id()) {
      // Unit is member of a split team, resolve global unit id:
      dart_team_unit_l2g(_teamid, lunit, &gunit);
    } else {
      // Unit is member of top level team, no conversion to global unit id
      // necessary:
      gunit = lunit;
    }
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", gunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, gunit);
    // Apply local offset to global pointer:
    dart_gptr_incaddr(&gptr, local_index * sizeof(ElementType));
    DASH_LOG_DEBUG("GlobDynamicMem.index_to_gptr (+g_unit) >", gptr);
    return gptr;
  }

private:
  allocator_type          _allocator;
  dart_gptr_t             _begptr;
  dart_team_t             _teamid;
  size_type               _nunits;
  size_type               _nlelem;
  local_pointer           _lbegin;
  local_pointer           _lend;

}; // class GlobDynamicMem

} // namespace dash

#endif // DASH__GLOB_DYNAMIC_MEM_H_
