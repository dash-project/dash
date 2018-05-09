#ifndef DASH__GLOB_STATIC_HEAP_H__INCLUDED
#define DASH__GLOB_STATIC_HEAP_H__INCLUDED

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
 * For global memory spaces with support for resizing, see
 * \c dash::GlobHeapMem.
 *
 * \see dash::GlobHeapMem
 *
 * \concept{DashMemorySpaceConcept}
 */
template<
  /// Type of elements maintained in the global memory space
  typename ElementType,
  /// Type implementing the DASH allocator concept used to allocate and
  /// deallocate physical memory
  class    AllocatorType =
             dash::allocator::SymmetricAllocator<ElementType> >
class GlobStaticMem
{
private:
  typedef GlobStaticMem<ElementType, AllocatorType>
    self_t;

public:
  typedef AllocatorType                                    allocator_type;
  typedef typename std::decay<ElementType>::type               value_type;

  typedef typename allocator_type::size_type                    size_type;
  typedef typename allocator_type::difference_type        difference_type;
  typedef typename allocator_type::difference_type             index_type;

  typedef GlobPtr<      value_type, self_t>                       pointer;
  typedef GlobPtr<const value_type, self_t>                 const_pointer;
  typedef GlobPtr<      void,       self_t>                  void_pointer;
  typedef GlobPtr<const void,       self_t>            const_void_pointer;

  typedef       value_type *                                local_pointer;
  typedef const value_type *                          const_local_pointer;

private:
  allocator_type          _allocator;
  dart_gptr_t             _begptr     = DART_GPTR_NULL;
  dash::Team            * _team       = nullptr;
  dart_team_t             _teamid;
  size_type               _nunits     = 0;
  team_unit_t             _myid       { DART_UNDEFINED_UNIT_ID };
  size_type               _nlelem     = 0;
  local_pointer           _lbegin     = nullptr;
  local_pointer           _lend       = nullptr;

public:
  /**
   * Constructor, creates instance of GlobStaticMem with pre-allocated
   * memory space.
   */
  GlobStaticMem(
    dart_gptr_t gbegin,
    /// Number of local elements to allocate in global memory space
    size_type   n_local_elem,
    /// Team containing all units operating on the global memory region
    Team      & team = dash::Team::All())
  : _allocator(team),
    _begptr(gbegin),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid()),
    _nlelem(n_local_elem)
  {
    DASH_LOG_TRACE("GlobStaticMem(gbegin,nlocal,team)",
                   "preallocated at:",        _begptr,
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    update_lbegin();
    update_lend();
    DASH_LOG_TRACE("GlobStaticMem(gbegin,nlocal,team) >");
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
  explicit GlobStaticMem(
    /// Number of local elements to allocate in global memory space
    size_type   n_local_elem,
    /// Team containing all units operating on the global memory region
    Team      & team = dash::Team::All())
  : _allocator(team),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid()),
    _nlelem(n_local_elem)
  {
    DASH_LOG_TRACE("GlobStaticMem(nlocal,team)",
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    _begptr = _allocator.allocate(_nlelem);
    DASH_ASSERT_MSG(!DART_GPTR_ISNULL(_begptr), "allocation failed");

    // Use id's of team all
    update_lbegin();
    update_lend();
    DASH_LOG_TRACE("GlobStaticMem(nlocal,team) >");
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
  explicit GlobStaticMem(
    /// Local elements to allocate in global memory space
    std::initializer_list<value_type>   local_elements,
    /// Team containing all units operating on the global memory region
    Team                              & team = dash::Team::All())
  : _allocator(team),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid()),
    _nlelem(local_elements.size())
  {
    DASH_LOG_DEBUG("GlobStaticMem(lvals,team)",
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    _begptr = _allocator.allocate(_nlelem);
    DASH_ASSERT_MSG(!DART_GPTR_ISNULL(_begptr), "allocation failed");

    // Use id's of team all
    update_lbegin();
    update_lend();
    DASH_ASSERT_EQ(std::distance(_lbegin, _lend), local_elements.size(),
                   "Capacity of local memory range differs from number "
                   "of specified local elements");

    // Initialize allocated local elements with specified values:
    auto copy_end = std::copy(local_elements.begin(),
                              local_elements.end(),
                              _lbegin);
    DASH_ASSERT_EQ(_lend, copy_end,
                   "Initialization of specified local values failed");

    if (_nunits > 1) {
      // Wait for initialization of local values at all units.
      // Barrier synchronization is okay here as multiple units are
      // involved in initialization of values in global memory:
      //
      // TODO: Should depend on allocator trait
      //         dash::allocator_traits<Alloc>::is_collective()
      DASH_LOG_DEBUG("GlobStaticMem(lvals,team)", "barrier");
      barrier();
    }

    DASH_LOG_DEBUG("GlobStaticMem(lvals,team) >",
                   "_lbegin:", _lbegin, "_lend:", _lend);
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobStaticMem()
  {
    DASH_LOG_TRACE_VAR("GlobStaticMem.~GlobStaticMem()", _begptr);
    // check if has been moved away
    if(!DART_GPTR_ISNULL(_begptr)){
      _allocator.deallocate(_begptr);
    }
    DASH_LOG_TRACE("GlobStaticMem.~GlobStaticMem >");
  }

  /**
   * Copy constructor.
   */
  GlobStaticMem(const self_t & other)
    = delete;

  /**
   * Move constructor
   *
   * \TODO make move constructor defaultable by using RAII
   */
  GlobStaticMem(self_t && other)
  : _allocator(std::move(other._allocator)),
    _begptr(other._begptr),
    _team(other._team),
    _teamid(other._teamid),
    _nunits(other._nunits),
    _myid(other._myid),
    _nlelem(other._nlelem),
    _lbegin(other._lbegin),
    _lend(other._lend)
  {
    // avoid deallocation of underlying memory
    // in the dead hull
    other._begptr = DART_GPTR_NULL;
    other._lbegin = nullptr;
    other._lend   = nullptr;
  }

  /**
   * Copy-assignment operator.
   *
   */
  self_t & operator=(const self_t & rhs)
    = delete;

  /**
   * Move-assignment operator.
   *
   * \TODO make move constructor defaultable by using RAII
   */
  self_t & operator=(self_t && other) {
    // deallocate old memory
    if(!DART_GPTR_ISNULL(_begptr)){
      _allocator.deallocate(_begptr);
    }

    _allocator = std::move(other._allocator);
    _begptr    = other._begptr;
    _team      = other._team;
    _teamid    = other._teamid;
    _nunits    = other._nunits;
    _myid      = other._myid;
    _nlelem    = other._nlelem;
    _lbegin    = other._lbegin;
    _lend      = other._lend;

    // avoid deallocation of underlying memory
    // in the dead hull
    other._begptr = DART_GPTR_NULL;
    other._lbegin = nullptr;
    other._lend   = nullptr;

    return *this;
  }

  /**
   * Equality comparison operator.
   */
  inline bool operator==(const self_t & rhs) const noexcept
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
  constexpr bool operator!=(const self_t & rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /**
   * Total number of elements in the global memory space.
   */
  constexpr size_type size() const noexcept
  {
    return _nlelem * _nunits;
  }

  constexpr size_type local_size(dash::team_unit_t) const noexcept
  {
    // TODO this only holds if all units allocate the same number of elements
    return _nlelem;
  }

  constexpr size_type local_size() const noexcept
  {
    return _nlelem;
  }

  /**
   * The team containing all units accessing the global memory space.
   *
   * \return  A reference to the Team containing the units associated with
   *          the global dynamic memory space.
   */
  constexpr dash::Team & team() const noexcept
  {
    return (_team != nullptr) ? *_team : dash::Team::Null();
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  constexpr const_pointer begin() const noexcept
  {
    return const_pointer(*this, _begptr);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  inline pointer begin() noexcept
  {
    return pointer(*this, _begptr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobStaticMem instance.
   */
  constexpr const_local_pointer lbegin() const noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobStaticMem instance.
   */
  inline local_pointer lbegin() noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobStaticMem instance.
   */
  constexpr const_local_pointer lend() const noexcept
  {
    return _lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobStaticMem instance.
   */
  inline local_pointer lend() noexcept
  {
    return _lend;
  }

  /**
   * Write value to global memory at given offset.
   *
   * \see  dash::put_value
   */
  template<typename ValueType = value_type>
  inline void put_value(
    const ValueType & newval,
    index_type        global_index)
  {
    DASH_LOG_TRACE("GlobStaticMem.put_value(val, gidx = %d)", global_index);
    dash::put_value(newval,
                    GlobPtr<ValueType, self_t>(
                      *this, _begptr
                    ) + global_index);
  }

  /**
   * Read value from global memory at given offset.
   *
   * \see  dash::get_value
   */
  template<typename ValueType = value_type>
  inline void get_value(
    ValueType  * ptr,
    index_type   global_index) const
  {
    DASH_LOG_TRACE("GlobStaticMem.get_value(val, gidx = %d)", global_index);
    dash::get_value(ptr,
                    GlobPtr<ValueType, self_t>(
                      *this, _begptr
                    ) + global_index);
  }

  /**
   * Synchronize all units associated with this global memory instance.
   */
  void barrier() const
  {
    DASH_ASSERT_RETURNS(
      dart_barrier(_teamid),
      DART_OK);
  }

  /**
   * Complete all outstanding non-blocking operations to all units.
   */
  void flush() noexcept
  {
    dart_flush_all(_begptr);
  }

  /**
   * Complete all outstanding non-blocking operations to the specified unit.
   */
  void flush(dash::team_unit_t target) noexcept
  {
    dart_gptr_t gptr = _begptr;
    gptr.unitid = target.id;
    dart_flush(gptr);
  }

  /**
   * Locally complete all outstanding non-blocking operations to all units.
   */
  void flush_local() noexcept
  {
    dart_flush_local_all(_begptr);
  }

  /**
   * Locally complete all outstanding non-blocking operations to the specified
   * unit.
   */
  void flush_local(dash::team_unit_t target) noexcept
  {
    dart_gptr_t gptr = _begptr;
    gptr.unitid = target.id;
    dart_flush_local(gptr);
  }


  /**
   * Resolve the global pointer from an element position in a unit's
   * local memory.
   */
  template<typename IndexType>
  pointer at(
    /// The unit id
    team_unit_t unit,
    /// The unit's local address offset
    IndexType   local_index) const
  {
    DASH_LOG_DEBUG("GlobStaticMem.at(unit,l_idx)", unit, local_index);
    if (_nunits == 0 || DART_GPTR_ISNULL(_begptr)) {
      DASH_LOG_DEBUG("GlobStaticMem.at(unit,l_idx) >",
                     "global memory not allocated");
      return pointer(nullptr);
    }
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = _begptr;
    // Resolve global unit id
    DASH_LOG_TRACE_VAR("GlobStaticMem.at (=g_begptr)", gptr);
    DASH_LOG_TRACE_VAR("GlobStaticMem.at", gptr.unitid);
    team_unit_t lunit{gptr.unitid};
    DASH_LOG_TRACE_VAR("GlobStaticMem.at", lunit);
    lunit = (lunit + unit) % _nunits;
    DASH_LOG_TRACE_VAR("GlobStaticMem.at", lunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, lunit);
    // increment locally only
    gptr.addr_or_offs.offset += local_index * sizeof(value_type);
    // Apply local offset to global pointer:
    pointer res_gptr(*this, gptr);
    DASH_LOG_DEBUG("GlobStaticMem.at (+g_unit) >", res_gptr);
    return res_gptr;
  }

private:
  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   * \param team_unit_id id of unit in \c dash::Team::All()
   */
  void update_lbegin()
  {
    void *addr;
    dart_gptr_t gptr = _begptr;
    DASH_LOG_TRACE_VAR("GlobStaticMem.update_lbegin",
                       pointer(*this, gptr));
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, _myid),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobStaticMem.update_lbegin >", addr);
    _lbegin = static_cast<local_pointer>(addr);
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  void update_lend()
  {
    void *addr;
    dart_gptr_t gptr = _begptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, _myid),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, _nlelem * sizeof(value_type)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    _lend = static_cast<local_pointer>(addr);
  }
};

/**
 * Allocate elements in the specified memory space.
 *
 * \returns  Global pointer to the beginning of the allocated memory region.
 *
 * \concept{DashMemorySpaceConcept}
 */
template<
  typename T,
  class    MemSpaceT >
GlobPtr<T, MemSpaceT> memalloc(const MemSpaceT & mspace, size_t nelem)
{
  dart_gptr_t gptr;
  dash::dart_storage<T> ds(nelem);
  if (dart_memalloc(ds.nelem, ds.dtype, &gptr) != DART_OK) {
    return GlobPtr<T, MemSpaceT>(nullptr);
  }
  return GlobPtr<T, MemSpaceT>(mspace, gptr);
}

/**
 * Deallocate segment in global memory space referenced by the specified
 * global pointer.
 *
 * \concept{DashMemorySpaceConcept}
 */
template<class GlobPtrT>
void memfree(GlobPtrT gptr)
{
  dart_memfree(gptr.dart_gptr());
}

} // namespace dash

#include <dash/memory/GlobUnitMem.h>

#endif // DASH__GLOB_STATIC_HEAP_H__INCLUDED
