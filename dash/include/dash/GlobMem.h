#ifndef DASH__GLOBMEM_H_
#define DASH__GLOBMEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>

#include <dash/internal/Logging.h>

namespace dash {

/**
 * Global memory region with static size.
 */
template<
  /// Type of values allocated in the global memory space
  typename ElementType,
  /// Type of allocator implementation used to allocate and deallocate
  /// global memory
  class    AllocatorType =
             dash::allocator::CollectiveAllocator<ElementType> >
class GlobMem
{
private:
  typedef GlobMem<ElementType, AllocatorType>
    self_t;

public:
  typedef AllocatorType                                    allocator_type;
  typedef ElementType                                          value_type;
  typedef typename allocator_type::size_type                    size_type;
  typedef typename allocator_type::difference_type        difference_type;
  typedef typename allocator_type::difference_type             index_type;
  typedef typename allocator_type::pointer                        pointer;
  typedef typename allocator_type::void_pointer              void_pointer;
  typedef typename allocator_type::const_pointer            const_pointer;
  typedef typename allocator_type::const_void_pointer  const_void_pointer;
  typedef ElementType *                                     local_pointer;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   */
  GlobMem(
    /// Number of local elements to allocate in global memory space
    size_type   n_local_elem,
    /// Team containing all units operating on the global memory region
    Team      & team = dash::Team::Null())
  : _allocator(team),
    _teamid(team.dart_id()),
    _nlelem(n_local_elem)
  {
    DASH_LOG_TRACE("GlobMem(nunits,nelem)", team.size(), _nlelem);

    _begptr = _allocator.allocate(_nlelem);
    DASH_ASSERT_NE(DART_GPTR_NULL, _begptr, "allocation failed");

    if (_teamid == DART_TEAM_NULL) {
      _nunits = 1;
    } else {
      DASH_ASSERT_RETURNS(
        dart_team_size(_teamid, (size_t *) &_nunits),
        DART_OK);
    }
    _lbegin = lbegin(dash::myid());
    _lend   = lend(dash::myid());
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobMem()
  {
    DASH_LOG_TRACE_VAR("GlobMem.~GlobMem()", _begptr);
    _allocator.deallocate(_begptr);
    DASH_LOG_TRACE("GlobMem.~GlobMem >");
  }

  /**
   * Copy constructor.
   */
  GlobMem(const self_t & other)
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
  const GlobPtr<ElementType> begin() const
  {
    return GlobPtr<ElementType>(_begptr);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  GlobPtr<ElementType> begin()
  {
    return GlobPtr<ElementType>(_begptr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  const ElementType * lbegin(
    dart_unit_t unit_id) const
  {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobMem.lbegin const()", unit_id);
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobMem.lbegin const >", addr);
    return static_cast<const ElementType *>(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  ElementType * lbegin(
    dart_unit_t unit_id)
  {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobMem.lbegin()", unit_id);
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_LOG_TRACE_VAR("GlobMem.lbegin",
                       GlobPtr<ElementType>((dart_gptr_t)gptr));
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobMem.lbegin >", addr);
    return static_cast<ElementType *>(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  inline const ElementType * lbegin() const
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  inline ElementType * lbegin()
  {
    return _lbegin;
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  const ElementType * lend(
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
    return static_cast<const ElementType *>(addr);
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  ElementType * lend(
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
    return static_cast<ElementType *>(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  inline const ElementType * lend() const
  {
    return _lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  inline ElementType * lend()
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
    DASH_LOG_TRACE("GlobMem.put_value(newval, gidx = %d)", global_index);
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
    DASH_LOG_TRACE("GlobMem.get_value(newval, gidx = %d)", global_index);
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
  template<typename IndexType>
  dart_gptr_t index_to_gptr(
    /// The unit id
    dart_unit_t unit,
    /// The unit's local address offset
    IndexType   local_index) const
  {
    DASH_LOG_DEBUG("GlobMem.index_to_gptr(unit,l_idx)", unit, local_index);
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = _begptr;
    // Resolve global unit id
    dart_unit_t lunit, gunit;
    DASH_LOG_DEBUG("GlobMem.index_to_gptr (=g_begp)  ", gptr);
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", gptr.unitid);
    // Resolve local unit id from global unit id in global pointer:
    dart_team_unit_g2l(_teamid, gptr.unitid, &lunit);
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", lunit);
    lunit = (lunit + unit) % _nunits;
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", lunit);
    if (_teamid != dash::Team::All().dart_id()) {
      // Unit is member of a split team, resolve global unit id:
      dart_team_unit_l2g(_teamid, lunit, &gunit);
    } else {
      // Unit is member of top level team, no conversion to global unit id
      // necessary:
      gunit = lunit;
    }
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", gunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, gunit);
    // Apply local offset to global pointer:
    dart_gptr_incaddr(&gptr, local_index * sizeof(ElementType));
    DASH_LOG_DEBUG("GlobMem.index_to_gptr (+g_unit) >", gptr);
    return gptr;
  }

private:
  allocator_type          _allocator;
  dart_gptr_t             _begptr;
  dart_team_t             _teamid;
  size_type               _nunits;
  size_type               _nlelem;
  ElementType           * _lbegin    = nullptr;
  ElementType           * _lend      = nullptr;
};

template<typename T>
GlobPtr<T> memalloc(size_t nelem)
{
  dart_gptr_t gptr;
  size_t lsize = sizeof(T) * nelem;

  dart_memalloc(lsize, &gptr);
  return GlobPtr<T>(gptr);
}

} // namespace dash

#endif // DASH__GLOBMEM_H_
