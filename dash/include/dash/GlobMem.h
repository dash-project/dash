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
 * \defgroup  DashGlobalMemoryConcept  Global Memory Concept
 * Concept of distributed global memory space shared by units in a specified
 * team.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * An abstraction of global memory that provides sequential iteration and
 * random access to local and global elements to units in a specified team.
 * The C++ STL does not specify a counterpart of this concept as it only
 * considers local memory that is implicitly described by the random access
 * pointer interface.
 *
 * The model of global memory represents a single, virtual global address
 * space partitioned into the local memory spaces of its associated units.
 * The global memory concept depends on the allocator concept that specifies
 * allocation of physical memory.
 *
 * Local pointers are usually, but not necessarily represented as raw native
 * pointers as returned by \c malloc.
 *
 * \see DashAllocatorConcept
 *
 * \par Types
 *
 * Type Name            | Description                                            |
 * -------------------- | ------------------------------------------------------ |
 * \c GlobalRAI         | Random access iterator on global address space         |
 * \c LocalRAI          | Random access iterator on a single local address space |
 *
 *
 * \par Methods
 *
 * Return Type          | Method             | Parameters                         | Description                                                                                                |
 * -------------------- | ------------------ | ---------------------------------- | ---------------------------------------------------------------------------------------------------------- |
 * <tt>GlobalRAI</tt>   | <tt>begin</tt>     | &nbsp;                             | Global pointer to the initial address of the global memory space                                           |
 * <tt>GlobalRAI</tt>   | <tt>end</tt>       | &nbsp;                             | Global pointer past the final element in the global memory space                                           |
 * <tt>LocalRAI</tt>    | <tt>lbegin</tt>    | &nbsp;                             | Local pointer to the initial address in the local segment of the global memory space                       |
 * <tt>LocalRAI</tt>    | <tt>lbegin</tt>    | <tt>unit u</tt>                    | Local pointer to the initial address in the local segment at unit \c u of the global memory space          |
 * <tt>LocalRAI</tt>    | <tt>lend</tt>      | &nbsp;                             | Local pointer past the final element in the local segment of the global memory space                       |
 * <tt>LocalRAI</tt>    | <tt>lend</tt>      | <tt>unit u</tt>                    | Local pointer past the final element in the local segment at unit \c u of the global memory space          |
 * <tt>GlobalRAI</tt>   | <tt>at</tt>        | <tt>index gidx</tt>                | Global pointer to the element at canonical global offset \c gidx in the global memory space                |
 * <tt>void</tt>        | <tt>put_value</tt> | <tt>value & v_in, index gidx</tt>  | Stores value specified in parameter \c v_in to address in global memory at canonical global offset \c gidx |
 * <tt>void</tt>        | <tt>get_value</tt> | <tt>value * v_out, index gidx</tt> | Loads value from address in global memory at canonical global offset \c gidx into local address \c v_out   |
 * <tt>void</tt>        | <tt>barrier</tt>   | &nbsp;                             | Blocking synchronization of all units associated with the global memory instance                           |
 *
 * \}
 */


/**
 * Global memory with address space of static size.
 *
 * \concept{DashGlobalMemoryConcept}
 */
template<
  /// Type of elements maintained in the global memory space
  typename ElementType,
  /// Type implementing the DASH allocator concept used to allocate and
  /// deallocate physical memory
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
   *
   * \note Must not lead to implicit barrier:
   *       Synchronization depends on underlying allocator.
   *       For example, \c dash::LocalAllocator is used in \c dash::Shared
   *       and only called at owner unit.
   */
  inline GlobMem(
    /// Number of local elements to allocate in global memory space
    size_type   n_local_elem,
    /// Team containing all units operating on the global memory region
    Team      & team = dash::Team::All())
  : _allocator(team),
    _team(team),
    _nlelem(n_local_elem),
    _nunits(team.size())
  {
    DASH_LOG_TRACE("GlobMem(nlocal,team)",
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    if (_nlelem == 0 || _nunits == 0) {
      DASH_LOG_DEBUG("GlobMem(lvals,team)", "nothing to allocate");
      return;
    }
    _begptr = _allocator.allocate(_nlelem);
    DASH_ASSERT_MSG(!DART_GPTR_ISNULL(_begptr), "allocation failed");

    // Use id's of team all
    _lbegin = lbegin(dash::Team::GlobalUnitID());
    _lend   = lend(dash::Team::GlobalUnitID());
    DASH_LOG_TRACE("GlobMem(nlocal,team) >");
  }

  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   *
   * \note Must not lead to implicit barrier:
   *       Synchronization depends on underlying allocator.
   *       For example, \c dash::LocalAllocator is used in \c dash::Shared
   *       and only called at owner unit.
   */
  inline GlobMem(
    /// Local elements to allocate in global memory space
    std::initializer_list<value_type>   local_elements,
    /// Team containing all units operating on the global memory region
    Team                              & team = dash::Team::All())
  : _allocator(team),
    _team(team),
    _nlelem(local_elements.size()),
    _nunits(team.size())
  {
    DASH_LOG_DEBUG("GlobMem(lvals,team)",
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    if (_nlelem == 0 || _nunits == 0) {
      DASH_LOG_DEBUG("GlobMem(lvals,team)", "nothing to allocate");
    } else {
      _begptr = _allocator.allocate(_nlelem);
      DASH_ASSERT_MSG(!DART_GPTR_ISNULL(_begptr), "allocation failed");

      // Use id's of team all
      _lbegin = lbegin(dash::Team::GlobalUnitID());
      _lend   = lend(dash::Team::GlobalUnitID());
      DASH_ASSERT_EQ(std::distance(_lbegin, _lend), local_elements.size(),
                     "Capacity of local memory range differs from number "
                     "of specified local elements");
                     
      // Initialize allocated local elements with specified values:
      auto copy_end = std::copy(local_elements.begin(),
                                local_elements.end(),
                                _lbegin);
      DASH_ASSERT_EQ(_lend, copy_end,
                     "Initialization of specified local values failed");
    }
    if (_nunits > 1) {
      // Wait for initialization of local values at all units.
      // Barrier synchronization is okay here as multiple units are
      // involved in initialization of values in global memory:
      //
      // TODO: Should depend on allocator trait
      //         dash::allocator_traits<Alloc>::is_collective()
      DASH_LOG_DEBUG("GlobMem(lvals,team)", "barrier");
      team.barrier();
    }

    DASH_LOG_DEBUG("GlobMem(lvals,team) >",
                   "_lbegin:", _lbegin, "_lend:", _lend);
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  inline ~GlobMem()
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
  inline bool operator==(const self_t & rhs) const
  {
    return (_begptr == rhs._begptr &&
            _team   == rhs._team   &&
            _nunits == rhs._nunits &&
            _nlelem == rhs._nlelem &&
            _lbegin == rhs._lbegin &&
            _lend   == rhs._lend);
  }

  /**
   * Inequality comparison operator.
   */
  inline bool operator!=(const self_t & rhs) const
  {
    return !(*this == rhs);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  inline const GlobPtr<ElementType> begin() const
  {
    return GlobPtr<ElementType>(_begptr);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  inline GlobPtr<ElementType> begin()
  {
    return GlobPtr<ElementType>(_begptr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   * \param global_unit_id id of unit in \c dash::Team::All()
   */
  const ElementType * lbegin(
    global_unit_t unit_id) const
  {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobMem.lbegin const()", unit_id);
    dart_gptr_t gptr = _begptr;
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
   * \param global_unit_id id of unit in \c dash::Team::All()
   */
  ElementType * lbegin(
    global_unit_t unit_id)
  {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobMem.lbegin()", unit_id);
    dart_gptr_t gptr = _begptr;
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
    global_unit_t unit_id) const
  {
    void *addr;
    dart_gptr_t gptr = _begptr;
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
    global_unit_t unit_id)
  {
    void *addr;
    dart_gptr_t gptr = _begptr;
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
  inline void put_value(
    const ValueType & newval,
    index_type        global_index)
  {
    DASH_LOG_TRACE("GlobMem.put_value(newval, gidx = %d)", global_index);
    dash::put_value(newval, GlobPtr<ValueType>(_begptr) + global_index);
  }

  /**
   * Read value from global memory at given offset.
   *
   * \see  dash::get_value
   */
  template<typename ValueType = ElementType>
  inline void get_value(
    ValueType  * ptr,
    index_type   global_index) const
  {
    DASH_LOG_TRACE("GlobMem.get_value(newval, gidx = %d)", global_index);
    dash::get_value(ptr, GlobPtr<ValueType>(_begptr) + global_index);
  }

  /**
   * Synchronize all units associated with this global memory instance.
   */
  void barrier() const
  {
    _team.barrier();
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
  dash::GlobPtr<value_type> at(
    /// The unit id
    team_unit_t unit,
    /// The unit's local address offset
    IndexType   local_index) const
  {
    DASH_LOG_DEBUG("GlobMem.at(unit,l_idx)", unit, local_index);
    if (_nunits == 0 || DART_GPTR_ISNULL(_begptr)) {
      DASH_LOG_DEBUG("GlobMem.at(unit,l_idx) >",
                     "global memory not allocated");
      return dash::GlobPtr<value_type>(nullptr);
    }
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = _begptr;
    // Resolve global unit id
    global_unit_t gunit{gptr.unitid};
    DASH_LOG_TRACE_VAR("GlobMem.at (=g_begptr)", gptr);
    DASH_LOG_TRACE_VAR("GlobMem.at", gptr.unitid);
    // Resolve local unit id from global unit id in global pointer:
//    dart_team_unit_g2l(_teamid, gptr.unitid, &lunit);
    team_unit_t lunit = _team.relative_id(gunit);
    DASH_LOG_TRACE_VAR("GlobMem.at", lunit);
    lunit = (lunit + unit) % _nunits;
    DASH_LOG_TRACE_VAR("GlobMem.at", lunit);
    if (!_team.is_all()) {
      // Unit is member of a split team, resolve global unit id:
      gunit = _team.global_id(lunit);
    } else {
      // Unit is member of top level team, no conversion to global unit id
      // necessary:
      gunit = global_unit_t(lunit);
    }
    DASH_LOG_TRACE_VAR("GlobMem.at", gunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, gunit);
    // Apply local offset to global pointer:
    dash::GlobPtr<value_type> res_gptr(gptr);
    res_gptr += local_index;
    DASH_LOG_DEBUG("GlobMem.at (+g_unit) >", res_gptr);
    return res_gptr;
  }

private:
  allocator_type          _allocator;
  dart_gptr_t             _begptr     = DART_GPTR_NULL;
  dash::Team&             _team;
  size_type               _nlelem     = 0;
  size_type               _nunits     = 0;
  ElementType           * _lbegin     = nullptr;
  ElementType           * _lend       = nullptr;
};

template<typename T>
GlobPtr<T> memalloc(size_t nelem)
{
  dart_gptr_t gptr;
  dart_storage_t ds = dart_storage<T>(nelem);
  dart_memalloc(ds.nelem, ds.dtype, &gptr);
  return GlobPtr<T>(gptr);
}

} // namespace dash

#endif // DASH__GLOBMEM_H_
