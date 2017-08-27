#ifndef DASH__GLOB_UNIT_MEM_H__INCLUDED
#define DASH__GLOB_UNIT_MEM_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>

#include <dash/internal/Logging.h>

namespace dash {

/**
 * Global memory at a single unit with address space of static size.
 *
 * \concept{DashMemorySpaceConcept}
 */
template<
  /// Type of elements maintained in the global memory space
  typename ElementType,
  /// Type implementing the DASH allocator concept used to allocate and
  /// deallocate physical memory
  class    AllocatorType =
             dash::allocator::LocalAllocator<ElementType> >
class GlobUnitMem
{
private:
  typedef GlobUnitMem<ElementType, AllocatorType>
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
  /// Whether memory region is owned (true) or only maintained by this
  /// memory space.
  bool                    _owns_mem   = true;
  dash::Team            * _team       = nullptr;
  dart_team_t             _teamid     = DART_UNDEFINED_TEAM_ID;
  size_type               _nunits     = 0;
  team_unit_t             _myid       { DART_UNDEFINED_UNIT_ID };
  size_type               _nlelem     = 0;
  local_pointer           _lbegin     = nullptr;
  local_pointer           _lend       = nullptr;

public:
  /**
   * Constructor, creates instance of GlobUnitMem with pre-allocated
   * memory space.
   */
  GlobUnitMem(
    dart_gptr_t gbegin,
    /// Number of local elements to allocate in global memory space
    size_type   n_local_elem,
    /// Team containing all units operating on the global memory region
    Team      & team = dash::Team::All())
  : _allocator(team),
    _begptr(gbegin),
    _owns_mem(false),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid()),
    _nlelem(n_local_elem)
  {
    DASH_LOG_TRACE("GlobUnitMem(gbegin,nlocal,team)",
                   "preallocated at:",        _begptr,
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    update_lbegin();
    update_lend();
    DASH_LOG_TRACE("GlobUnitMem(gbegin,nlocal,team) >");
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
  explicit GlobUnitMem(
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
    DASH_LOG_TRACE("GlobUnitMem(nlocal,team)",
                   "number of local values:", _nlelem,
                   "team size:",              team.size());
    _begptr = _allocator.allocate(_nlelem);
    DASH_ASSERT_MSG(!DART_GPTR_ISNULL(_begptr), "allocation failed");

    // Use id's of team all
    update_lbegin();
    update_lend();
    DASH_LOG_TRACE("GlobUnitMem(nlocal,team) >");
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
  explicit GlobUnitMem(
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
    DASH_LOG_DEBUG("GlobUnitMem(lvals,team)",
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
      DASH_LOG_DEBUG("GlobUnitMem(lvals,team)", "barrier");
      barrier();
    }

    DASH_LOG_DEBUG("GlobUnitMem(lvals,team) >",
                   "_lbegin:", _lbegin, "_lend:", _lend);
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobUnitMem()
  {
    DASH_LOG_TRACE_VAR("GlobUnitMem.~GlobUnitMem()", _begptr);
    if (_owns_mem) {
      _allocator.deallocate(_begptr);
    }
    DASH_LOG_TRACE("GlobUnitMem.~GlobUnitMem >");
  }

  /**
   * Copy constructor.
   */
  GlobUnitMem(const self_t & other)      = delete;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs) = delete;

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
    return _nlelem;
  }

  constexpr size_type local_size(dash::team_unit_t) const noexcept
  {
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
   * the unit that initialized this GlobUnitMem instance.
   */
  constexpr const_local_pointer lbegin() const noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobUnitMem instance.
   */
  inline local_pointer lbegin() noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobUnitMem instance.
   */
  constexpr const_local_pointer lend() const noexcept
  {
    return _lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobUnitMem instance.
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
    DASH_LOG_TRACE("GlobUnitMem.put_value(newval, gidx = %d)", global_index);
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
    DASH_LOG_TRACE("GlobUnitMem.get_value(newval, gidx = %d)", global_index);
    dash::get_value(ptr,
                    GlobPtr<ValueType, self_t>(
                      *this, _begptr
                    ) + global_index);
  }

  /**
   * Synchronize all units associated with this global memory instance.
   */
  void barrier() const noexcept
  {
    DASH_ASSERT_RETURNS(
      dart_barrier(_teamid),
      DART_OK);
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced
   * global memory on all units.
   */
  void flush() noexcept
  {
    dart_flush(_begptr);
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced
   * global memory on all units.
   */
  void flush_all() noexcept
  {
    dart_flush_all(_begptr);
  }

  void flush_local() noexcept
  {
    dart_flush_local(_begptr);
  }

  void flush_local_all() noexcept
  {
    dart_flush_local_all(_begptr);
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
    DASH_LOG_DEBUG("GlobUnitMem.at(unit,l_idx)", unit, local_index);
    if (_nunits == 0 || DART_GPTR_ISNULL(_begptr)) {
      DASH_LOG_ERROR("GlobUnitMem.at(unit,l_idx) >",
                     "global memory not allocated");
      return pointer(nullptr);
    }
    if (unit.id != _begptr.unitid) {
      DASH_LOG_ERROR("GlobUnitMem.at(unit,l_idx) >",
                     "address in global unit memory requested for", unit,
                     "but only allocated at unit", _begptr.unitid);
      return pointer(nullptr);
    }
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = _begptr;
    // Apply local offset to global pointer:
    pointer res_gptr(*this, gptr);
    res_gptr += local_index;
    DASH_LOG_DEBUG("GlobUnitMem.at (+g_unit) >", res_gptr);
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
    DASH_LOG_TRACE_VAR("GlobUnitMem.update_lbegin",
                       pointer(*this, gptr));
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, _myid),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobUnitMem.update_lbegin >", addr);
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
 * Specialization of \c dash::distance for \c dash::GlobPtr as definition
 * of pointer distance in global unit memory space.
 *
 * Equivalent to \c (gend - gbegin).
 *
 * \return  Number of elements in the range between the first and second
 *          global pointer
 *
 * \concept{DashMemorySpaceConcept}
 */
template <typename T1, typename T2>
dash::gptrdiff_t distance(
  // First global pointer in range
  const GlobPtr<T1, dash::GlobUnitMem<T1>> & gbeg,
  // Final global pointer in range
  const GlobPtr<T2, dash::GlobUnitMem<T2>> & gend) {
  using value_type = typename std::decay<decltype(gbeg)>::type::value_type;
  return ( gend.dart_gptr().addr_or_offs.offset -
           gbeg.dart_gptr().addr_or_offs.offset
         ) / sizeof(value_type);
}

/**
 * Allocate elements in the active unit's shared global memory space.
 *
 * \returns  Global pointer to the beginning of the allocated memory region.
 *
 * \concept{DashMemorySpaceConcept}
 */
template<
  typename T,
  class    MemSpaceT = dash::GlobUnitMem<T> >
GlobPtr<T, MemSpaceT> memalloc(size_t nelem)
{
  dart_gptr_t gptr;
  dash::dart_storage<T> ds(nelem);
  if (dart_memalloc(ds.nelem, ds.dtype, &gptr) != DART_OK) {
    return GlobPtr<T, MemSpaceT>(nullptr);
  }
  return GlobPtr<T, MemSpaceT>(MemSpaceT(gptr, nelem), gptr);
}

} // namespace dash

#endif // DASH__GLOB_UNIT_MEM_H__INCLUDED
