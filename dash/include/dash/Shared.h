#ifndef DASH__SHARED_H_
#define DASH__SHARED_H_

#include <memory>

#include <dash/dart/if/dart_types.h>

#include <dash/GlobRef.h>
#include <dash/memory/MemorySpace.h>

#include <dash/iterator/GlobIter.h>

namespace dash {

/**
 * Shared access to a value in global memory across a team.
 *
 * \tparam  ElementType  The type of the shared value.
 */
template <typename ElementType>
class Shared {
private:
  typedef Shared<ElementType> self_t;
  typedef ElementType         element_t;

public:
  typedef size_t size_type;
  typedef size_t difference_type;

  typedef GlobRef<element_t>       reference;
  typedef GlobRef<const element_t> const_reference;

  // Note: For dash::Shared<dash::Atomic<T>>, reference type is
  //       dash::GlobRef<dash::Atomic<T>> with value_type T.
  //
  //       See definition of dash::GlobRef in dash/atomic/GlobAtomicRef.h.
  typedef typename reference::value_type value_type;

private:
  using GlobMem_t     = dash::GlobLocalMemoryPool<dash::HostSpace>;
  using unique_gptr_t = decltype(dash::allocate_unique<element_t>(
      static_cast<GlobMem_t*>(nullptr), std::size_t{}));
  using pointer_t     = typename unique_gptr_t::pointer;

  template <typename T_>
  friend void swap(Shared<T_>& a, Shared<T_>& b);

  static constexpr size_t nels   = 1;
  static constexpr size_t nbytes = sizeof(element_t);

private:
  dash::Team*   m_team{nullptr};
  team_unit_t   m_owner{DART_UNDEFINED_UNIT_ID};
  GlobMem_t     m_memory_resource{nullptr};
  unique_gptr_t m_data{nullptr};
  pointer_t     m_glob_pointer;

public:
  /**
   * Constructor, allocates shared value at single unit in specified team.
   */
  Shared(
      /// Unit id of the shared value's owner.
      team_unit_t owner = team_unit_t(0),
      /// Team containing all units accessing the element in shared memory
      Team& team = dash::Team::All())
    : Shared(value_type{}, owner, team)
  {
    DASH_LOG_TRACE(
        "Shared.Shared(team,owner) >", "finished delegating constructor");
  }

  /**
   * Constructor, allocates shared value at single unit in specified team.
   * The element is initialized with the given value.
   */
  Shared(
      /// The value to initialize the element with
      const value_type& val,
      /// Unit id of the shared value's owner.
      team_unit_t owner = team_unit_t(0),
      /// Team containing all units accessing the element in shared memory
      Team& team = dash::Team::All())
    : m_team(&team)
    , m_owner(owner)
    , m_memory_resource(nbytes, team)
  {
    DASH_LOG_DEBUG_VAR("Shared.Shared(value,team,owner)()", owner);
    if (dash::is_initialized()) {
      DASH_ASSERT_RETURNS(init(val), true);
    }
  }

  /**
   * Destructor, frees shared memory.
   */
  ~Shared() = default;

  /**
   * Copy-constructor: DELETED
   */
  Shared(const self_t& other) = delete;

  /**
   * Move-constructor. Transfers ownership from other instance.
   */
  Shared(self_t&& other) noexcept

  {
    // redirect to move assignment
    *this = std::move(other);
    DASH_LOG_DEBUG("Shared.Shared(Shared&&) >");
  }

  /**
   * Assignment operator: DELETED
   */
  self_t& operator=(const self_t& other) = delete;

  /**
   * Move-assignment operator.
   */
  self_t& operator=(self_t&& other) noexcept
  {
    m_team    = other.m_team;
    m_owner   = other.m_owner;
    //releases own resources..
    m_memory_resource = std::move(other.m_memory_resource);

    auto tmp__ = unique_gptr_t{other.m_data.release(),
                   typename unique_gptr_t::deleter_type{&m_memory_resource, nels}};
    m_data =
        std::move(tmp__);

    m_glob_pointer = other.m_glob_pointer;

    DASH_LOG_DEBUG("Shared.Shared(Shared&&) >");
    return *this;
  }

  /**
   * Collective allocation of a shared variable with an initial value.
   *
   * NOTE: This call succeeds only once during the lifetime of a single
   * object.
   *
   * @param the initial value of the globally shared variable.
   * @return true, if allocation and initialization succeeds.
   *         false otherwise.
   */
  bool init(value_type val = value_type{})
  {
    if (!dash::is_initialized()) {
      DASH_THROW(
          dash::exception::RuntimeError, "runtime not properly initialized");
    }

    // If our Shared has already a non-null pointer let us return it. The user
    // has first to deallocate it.
    if (m_glob_pointer) {
      DASH_LOG_ERROR("Shared scalar is already initialized");
      return false;
    }

    // Shared value is only allocated at unit 0:
    if (!m_data && m_team->myid() == m_owner) {
      DASH_LOG_DEBUG(
          "Shared.init(value,team,owner)",
          "allocating shared value in local memory");

      m_data = std::move(dash::allocate_unique<element_t>(&m_memory_resource, nels));
      DASH_ASSERT_MSG(m_data, "null pointer after allocation");

      auto* laddr = static_cast<element_t*>(m_data.get().local());

      // get the underlying allocator for local memory space
      auto local_alloc       = m_memory_resource.get_allocator();
      using allocator_traits = std::allocator_traits<decltype(local_alloc)>;

      // copy construct based on val
      allocator_traits::construct(local_alloc, laddr, element_t{val});
      m_glob_pointer = m_data.get();
    }
    // Broadcast global pointer of shared value at unit 0 to all units:
    dash::dart_storage<pointer_t> ds(nels);

    DASH_ASSERT_RETURNS(
        dart_bcast(&m_glob_pointer, ds.nelem, ds.dtype, m_owner, m_team->dart_id()),
        DART_OK);

    DASH_LOG_DEBUG_VAR("Shared.init(value,team,owner) >", m_glob_pointer);
    DASH_LOG_DEBUG("Shared.init(value,team,owner) >");

    return static_cast<bool>(m_glob_pointer);
  }

  /**
   * Set the value of the shared element.
   */
  void set(const value_type& val)
  {
    DASH_LOG_DEBUG("Shared.set()");
    DASH_LOG_DEBUG_VAR("Shared.set", m_owner);
    DASH_LOG_DEBUG_VAR("Shared.set", m_glob_pointer);
    DASH_ASSERT(static_cast<bool>(m_glob_pointer));
    this->get().set(val);
    DASH_LOG_DEBUG("Shared.set >");
  }

  /**
   * Get a reference on the shared value.
   */
  reference get()
  {
    DASH_LOG_DEBUG("Shared.cget()");
    DASH_LOG_DEBUG_VAR("Shared.cget", m_owner);
    DASH_LOG_DEBUG_VAR("Shared.get", m_glob_pointer);
    DASH_ASSERT(static_cast<bool>(m_glob_pointer));
    return reference(m_glob_pointer.dart_gptr());
  }

  /**
   * Get a const reference on the shared value.
   */
  const_reference get() const
  {
    DASH_LOG_DEBUG("Shared.get()");
    DASH_LOG_DEBUG_VAR("Shared.get", m_owner);
    DASH_LOG_DEBUG_VAR("Shared.get", m_glob_pointer);
    DASH_ASSERT(m_glob_pointer);
    return const_reference(m_glob_pointer.dart_gptr());
  }

  /**
   * Native pointer to the starting address of the local memory of
   * the unit that initialized this dash::Shared instance.
   */
  constexpr value_type const* local() const noexcept
  {
    return (m_team->myid() == m_owner) ? m_glob_pointer.local() : nullptr;
  }

  /**
   * Native pointer to the starting address of the local memory of
   * the unit that initialized this dash::Shared instance.
   */
  value_type* local() noexcept
  {
    return (m_team->myid() == m_owner) ? m_glob_pointer.local() : nullptr;
  }

  /**
   * The unit owning the memory in the global address space.
   */
  dash::team_unit_t owner() const noexcept
  {
    return m_owner;
  }

  /**
   * The \ref dash::Team that created this shared object.
   */
  dash::Team& team() const noexcept
  {
    return *m_team;
  }

  /**
   * Flush global memory of shared value.
   */
  void flush()
  {
    DASH_ASSERT(static_cast<bool>(m_glob_pointer));
    DASH_ASSERT_RETURNS(
        dart_flush(static_cast<dart_gptr_t>(m_glob_pointer)), DART_OK);
  }

  /**
   * Flush global memory of shared value and synchronize its associated
   * units.
   */
  void barrier()
  {
    flush();
    DASH_ASSERT(m_team != nullptr);
    m_team->barrier();
  }

  /**
   * Get underlying DART global pointer of the shared variable.
   */
  inline dart_gptr_t dart_gptr() const noexcept
  {
    return m_glob_pointer;
  }
};

template <typename T>
void swap(dash::Shared<T>& a, dash::Shared<T>& b)
{
  using unique_ptr = typename Shared<T>::unique_gptr_t;
  using deleter    = typename unique_ptr::deleter_type;

  ::std::swap(a.m_team, b.m_team);
  ::std::swap(a.m_owner, b.m_owner);
  ::std::swap(a.m_memory_resource, b.m_memory_resource);
  ::std::swap(a.m_glob_pointer, b.m_glob_pointer);

  auto tmp = a.m_data.release();
  a.m_data = std::move(
      unique_ptr{b.m_data.release(), deleter{&a.m_memory_resource, Shared<T>::nels}});
  b.m_data = std::move(
      unique_ptr{std::move(tmp), deleter{&b.m_memory_resource, Shared<T>::nels}});
}

}  // namespace dash

#endif  // DASH__SHARED_H_
