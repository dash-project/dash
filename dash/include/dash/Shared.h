#ifndef DASH__SHARED_H_
#define DASH__SHARED_H_

#include <memory>
#include <dash/std/memory.h>

#include <dash/dart/if/dart_types.h>

#include <dash/memory/GlobStaticMem.h>
#include <dash/GlobRef.h>
#include <dash/Allocator.h>

#include <dash/iterator/GlobIter.h>



namespace dash {

/**
 * Shared access to a value in global memory across a team.
 *
 * \tparam  ElementType  The type of the shared value.
 */
template<typename ElementType>
class Shared {
private:
  typedef Shared<ElementType>   self_t;
  typedef ElementType           element_t;

public:
  typedef size_t                                      size_type;
  typedef size_t                                difference_type;

  typedef GlobRef<      element_t>                    reference;
  typedef GlobRef<const element_t>              const_reference;

  // Note: For dash::Shared<dash::Atomic<T>>, reference type is
  //       dash::GlobRef<dash::Atomic<T>> with value_type T.
  //
  //       See definition of dash::GlobRef in dash/atomic/GlobAtomicRef.h.
  typedef typename reference::value_type             value_type;

private:
  typedef dash::GlobStaticMem<
            value_type,
            dash::allocator::LocalAllocator<value_type> >
          GlobMem_t;

  template<typename T_>
  friend void swap(Shared<T_> & a, Shared<T_> & b);

private:
  dash::Team*                _team{nullptr};
  team_unit_t                _owner{DART_UNDEFINED_UNIT_ID};
  std::unique_ptr<GlobMem_t> _globmem{nullptr};
  dart_gptr_t                _dart_gptr{DART_GPTR_NULL};

public:
  /**
   * Constructor, allocates shared value at single unit in specified team.
   */
  Shared(
    /// Unit id of the shared value's owner.
    team_unit_t   owner = team_unit_t(0),
    /// Team containing all units accessing the element in shared memory
    Team     &    team  = dash::Team::All())
    : Shared(value_type{}, owner, team)
  {
    DASH_LOG_TRACE("Shared.Shared(team,owner) >",
                   "finished delegating constructor");
  }

  /**
   * Constructor, allocates shared value at single unit in specified team.
   * The element is initialized with the given value.
   */
  Shared(
    /// The value to initialize the element with
    const value_type & val,
    /// Unit id of the shared value's owner.
    team_unit_t   owner = team_unit_t(0),
    /// Team containing all units accessing the element in shared memory
    Team     &    team  = dash::Team::All())
    : _team(&team),
      _owner(owner),
      _dart_gptr(DART_GPTR_NULL)
  {
    DASH_LOG_DEBUG_VAR("Shared.Shared(value,team,owner)()", owner);
    if (dash::is_initialized()) {
      init(val);
    }
  }

  /**
   * Destructor, frees shared memory.
   */
  ~Shared() = default;

  /**
   * Copy-constructor: DELETED
   */
  Shared(const self_t & other) = delete;

  /**
   * Move-constructor. Transfers ownership from other instance.
   */
  Shared(self_t && other) = default;

  /**
   * Assignment operator: DELETED
   */
  self_t & operator=(const self_t & other) = delete;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other) = default;

  bool init(value_type val = value_type{})
  {
    if (!dash::is_initialized()) {
      DASH_THROW(
          dash::exception::RuntimeError, "runtime not properly initialized");
    }

    if (!(DART_GPTR_ISNULL(_dart_gptr))) {
      DASH_LOG_ERROR("Shared scalar is already initialized");
      return false;
    }

    // Shared value is only allocated at unit 0:
    if (_team->myid() == _owner) {
      DASH_LOG_DEBUG(
          "Shared.init(value,team,owner)",
          "allocating shared value in local memory");
      _globmem          = std::make_unique<GlobMem_t>(1, *_team);
      _dart_gptr        = _globmem->begin().dart_gptr();
      auto       lbegin = _globmem->lbegin();
      auto const lend   = _globmem->lend();

      DASH_LOG_DEBUG_VAR("Shared.init(value,team,owner) >", val);
      std::uninitialized_fill(lbegin, lend, val);
    }
    // Broadcast global pointer of shared value at unit 0 to all units:
    dash::dart_storage<dart_gptr_t> ds(1);
    dart_bcast(&_dart_gptr, ds.nelem, ds.dtype, _owner, _team->dart_id());
    DASH_LOG_DEBUG_VAR("Shared.init(value,team,owner) >", _dart_gptr);

    return !(DART_GPTR_ISNULL(_dart_gptr));
  }

  /**
   * Set the value of the shared element.
   */
  void set(const value_type & val)
  {
    DASH_LOG_DEBUG_VAR("Shared.set()", val);
    DASH_LOG_DEBUG_VAR("Shared.set",   _owner);
    DASH_LOG_DEBUG_VAR("Shared.set",   _dart_gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_dart_gptr));
    this->get().set(val);
    DASH_LOG_DEBUG("Shared.set >");
  }

  /**
   * Get a reference on the shared value.
   */
  reference get()
  {
    DASH_LOG_DEBUG("Shared.cget()");
    DASH_LOG_DEBUG_VAR("Shared.cget", _owner);
    DASH_LOG_DEBUG_VAR("Shared.get", _dart_gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_dart_gptr));
    return reference(_dart_gptr);
  }

  /**
   * Get a const reference on the shared value.
   */
  const_reference get() const
  {
    DASH_LOG_DEBUG("Shared.get()");
    DASH_LOG_DEBUG_VAR("Shared.get", _owner);
    DASH_LOG_DEBUG_VAR("Shared.get", _dart_gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_dart_gptr));
    return const_reference(_dart_gptr);
  }

  /**
   * Flush global memory of shared value.
   */
  void flush()
  {
    DASH_ASSERT(!DART_GPTR_ISNULL(_dart_gptr));
    DASH_ASSERT_RETURNS(
      dart_flush(_dart_gptr),
      DART_OK);
  }

  /**
   * Flush global memory of shared value and synchronize its associated
   * units.
   */
  void barrier()
  {
    flush();
    DASH_ASSERT(_team != nullptr);
    _team->barrier();
  }

  /**
   * Get underlying DART global pointer of the shared variable.
   */
  inline dart_gptr_t dart_gptr() const noexcept
  {
    return _dart_gptr;
  }

};

template<typename T>
void swap(
  dash::Shared<T> & a,
  dash::Shared<T> & b)
{
  ::std::swap(a._team,       b._team);
  ::std::swap(a._owner,      b._owner);
  ::std::swap(a._globmem,    b._globmem);
  ::std::swap(a._dart_gptr,  b._dart_gptr);
}

} // namespace dash

#endif // DASH__SHARED_H_
