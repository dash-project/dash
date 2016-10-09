#ifndef DASH__ALLOCATOR__COLLECTIVE_PERSISTENT_ALLOCATOR_H__INCLUDED
#define DASH__ALLOCATOR__COLLECTIVE_PERSISTENT_ALLOCATOR_H__INCLUDED

#ifdef DASH_ENABLE_PMEM

#include <sys/stat.h>
#include <unistd.h>

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/Team.h>

#include <dash/internal/Logging.h>
#include <dash/util/Random.h>

#include <vector>
#include <algorithm>
#include <utility>

namespace dash {
namespace allocator {

/**
 * Encapsulates a memory allocation and deallocation strategy of global
 * memory regions distributed across local memory of units in a specified
 * team.
 *
 * Satisfied STL concepts:
 *
 * - Allocator
 * - CopyAssignable
 *
 * \concept{DashAllocatorConcept}
 */
template<typename ElementType>
class CollectivePersistentAllocator {
  template <class T, class U>
  friend bool operator==(
    const CollectivePersistentAllocator<T> & lhs,
    const CollectivePersistentAllocator<U> & rhs);

  template <class T, class U>
  friend bool operator!=(
    const CollectivePersistentAllocator<T> & lhs,
    const CollectivePersistentAllocator<U> & rhs);

private:
  typedef CollectivePersistentAllocator<ElementType> self_t;

  /// Type definitions required for std::allocator concept:
public:
  using value_type                             = ElementType;
  using size_type                              = dash::default_size_t;
  using propagate_on_container_move_assignment = std::true_type;

  static size_t const objectID_MAXLEN = 8;

  /// Type definitions required for dash::allocator concept:
public:
  typedef dash::gptrdiff_t        difference_type;
  typedef dart_gptr_t                     pointer;
  typedef dart_gptr_t                void_pointer;
  typedef dart_gptr_t               const_pointer;
  typedef dart_gptr_t          const_void_pointer;


private:
  using local_pointer = value_type *;

  struct pmem_bucket_info {
    size_t          nbytes = 0;
    dart_gptr_t gptr = DART_GPTR_NULL;
  };

  using pmem_bucket_info_t = struct pmem_bucket_info;
  using pmem_bucket_item_t = std::pair<local_pointer, pmem_bucket_info_t>;

public:
  /**
   * Constructor.
   * Creates a new instance of \c dash::CollectivePersistentAllocator for a given team.
   */
  explicit CollectivePersistentAllocator(
    Team & team = dash::Team::Null()) noexcept
    : _team_id(team.dart_id()),
      _nunits(team.size())
  {
    DASH_LOG_TRACE("CollectivePersistentAllocator.CollectivePersistentAllocator(team) >",
                   _nunits);
  }

  CollectivePersistentAllocator(
    Team & team,
    std::string const & poolId) noexcept
    : _team_id(team.dart_id()),
      _nunits(team.size()),
      _poolID(poolId)
  {
    DASH_LOG_TRACE("CollectivePersistentAllocator.CollectivePersistentAllocator(team, poolId) >");
  }

  /**
   * Move-constructor.
   * Takes ownership of the moved instance's allocation.
   */
  CollectivePersistentAllocator(self_t && other) noexcept
    : _team_id(other._team_id),
      _nunits(other._nunits),
      _allocated(other._allocated),
      _poolID(other._poolID),
      _pmem_pool(other._pmem_pool)
  {
    other._allocated.clear();
    other._pmem_pool = nullptr;
    other._team_id = DART_TEAM_NULL;

    DASH_LOG_TRACE("CollectivePersistentAllocator.CollectivePersistentAllocator(&&) >");
  }

  /**
   * Default constructor, deleted.
   */
  CollectivePersistentAllocator() noexcept
    = delete;

  /**
   * Copy constructor.
   *
   * \see DashAllocatorConcept
   */
  CollectivePersistentAllocator(const self_t & other) noexcept
    : _team_id(other._team_id),
      _nunits(other._nunits),
      _poolID(other._poolID)
  {
    DASH_ASSERT(other._allocated.empty());
    //TODO: What to do in case of copy? Maybe we have to copy all buckets from
    //old allocator to the new?
    DASH_LOG_TRACE("CollectivePersistentAllocator.CollectivePersistentAllocator(&) >");
  }

  /**
   * Copy-constructor.
   * Does not take ownership of the copied instance's allocation.
   */
  template<class U>
  CollectivePersistentAllocator(const CollectivePersistentAllocator<U> & other)
  noexcept
    : _team_id(other._team_id),
      _nunits(other._nunits),
      _poolID(other._poolID)
  {
    DASH_ASSERT(other._allocated.empty());
    //TODO: What to do in case of copy? Maybe we have to copy all buckets from
    //old allocator to the new?
  }

  /**
   * Destructor.
   * Frees all global memory regions allocated by this allocator instance.
   */
  ~CollectivePersistentAllocator() noexcept
  {
    if (_pmem_pool != nullptr) {
      DASH_ASSERT_RETURNS(dart__pmem__pool_close(&_pmem_pool), DART_OK);
    }

    clear();
  }

  /**
   * Assignment operator.
   *
   * \see DashAllocatorConcept
   */
  self_t & operator=(const self_t & other) noexcept
  {
    // noop
    return *this;
  }

  /**
   * Move-assignment operator.
   */
  self_t & operator=(const self_t && other) noexcept
  {
    // Take ownership of other instance's allocation vector:
    clear();
    std::swap(_allocated, other._allocated);
    std::swap(_poolID, other._poolID);
    std::swap(_nunits, other._nunits);
    std::swap(_team_id, other._team_id);
    std::swap(_pmem_pool, other._pmem_pool);
    DASH_LOG_DEBUG("CollectivePersistentAllocator.=(&&) >");
    return *this;
  }

  /**
   * Whether storage allocated by this allocator can be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   *
   * \returns  true if the storage allocated by this allocator can be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator==(const self_t & rhs) const noexcept
  {
    return (_team_id == rhs._team_id) && (_poolID == rhs._poolID);
  }

  /**
   * Whether storage allocated by this allocator cannot be deallocated
   * through the given allocator instance.
   * Establishes reflexive, symmetric, and transitive relationship.
   * Does not throw exceptions.
   * Same as \c !(operator==(rhs)).
   *
   * \returns  true if the storage allocated by this allocator cannot be
   *           deallocated through the given allocator instance.
   *
   * \see DashAllocatorConcept
   */
  bool operator!=(const self_t & rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /**
   * Allocates \c num_local_elem local elements at every unit in global
   * memory space.
   *
   * \see DashAllocatorConcept
   */
  pointer allocate(size_type num_local_elem)
  {
    DASH_LOG_DEBUG("CollectivePersistentAllocator.allocate(nlocal)",
                   "number of local values:", num_local_elem);
    if (num_local_elem <= 0) {
      return DART_GPTR_NULL;
    }

    //open pmem pool
    open_pmem_pool();
    //relocate persistent memory
    relocate_pmem_buckets();
    
    local_pointer lptr = nullptr;
    size_type num_local_bytes = sizeof(ElementType) * num_local_elem;
    dart_gptr_t gptr = DART_GPTR_NULL;
    //static container may only have one persistent bucket
    if (_allocated.size() > 0) {
      DASH_ASSERT(_allocated.size() == 1);
      auto & bucket = _allocated.front();
      if (bucket.second.nbytes != num_local_bytes) {
        DASH_THROW(dash::exception::RuntimeError,
                   "PersistentCollectiveAllocator.allocate(): trying to reallocate static memory of different size");
      } else {
        lptr = bucket.first;
        if (bucket.second.gptr == DART_GPTR_NULL) {
        DASH_ASSERT_RETURNS(
            dart_team_memregister_aligned(_team_id, num_local_bytes, lptr, &gptr),
            DART_OK);
        bucket.second.gptr = gptr;
        } else {
          gptr = bucket.second.gptr;
        }
        DASH_LOG_DEBUG_VAR("CollectivePersistentAllocator.allocate >", gptr);
      }
    } else {
    //allocate persistent memory
      dart_pmem_oid_t oid = dart__pmem__alloc(_pmem_pool, num_local_bytes);

      //convert it to a native address
      DASH_ASSERT_RETURNS(dart__pmem__get_addr(oid,
                           reinterpret_cast<void **>(&lptr)), DART_OK);

      if (lptr == nullptr) {
        DASH_LOG_ERROR("failed to allocate persistent memory of size: ", num_local_bytes);
      } else {
        pmem_bucket_info_t bucket;
        bucket.nbytes = num_local_bytes;
        if (dart_team_memregister_aligned(
              _team_id, num_local_bytes, lptr, &gptr
            ) == DART_OK) {
          DASH_LOG_TRACE("CollectivePersistentAllocator.attach ", num_local_bytes,
                         " bytes >");
          bucket.gptr = gptr;
          _allocated.push_back(std::make_pair(lptr, bucket));
          DASH_LOG_DEBUG_VAR("CollectivePersistentAllocator.allocate >", gptr);
        }
      }
    }

    return gptr;
  }

  /**
   * Returns the associated DART Team ID
   */
  dart_team_t dart_team_id() const noexcept {
    return _team_id;
  }

  /**
   * Deallocates memory in global memory space previously allocated across
   * local memory of all units in the team.
   *
   * \see DashAllocatorConcept
   */
  void deallocate(pointer gptr)
  {
    if (!dash::is_initialized()) {
      // If a DASH container is deleted after dash::finalize(), global
      // memory has already been freed by dart_exit() and must not be
      // deallocated again.
      DASH_LOG_DEBUG("CollectivePersistentAllocator.deallocate >",
                     "DASH not initialized, abort");
      return;
    }

    // Free local memory:
    DASH_LOG_DEBUG("CollectivePersistentAllocator.deallocate",
                   "deallocate local memory");

    detach_bucket_by_gptr(gptr, true);

    DASH_LOG_DEBUG("CollectivePersistentAllocator.deallocate >");
  }

private:
  /**
   * Frees all global memory regions allocated by this allocator instance.
   */
  void clear() noexcept
  {
    DASH_LOG_DEBUG("CollectivePersistentAllocator.clear()");
    for (auto & e : _allocated) {

      if (!DART_GPTR_EQUAL(e.second.gptr, DART_GPTR_NULL)) {
        DASH_LOG_DEBUG("CollectivePersistentAllocator.clear",
                       "detach local persistent memory:",
                       e.second.gptr);

        detach_bucket_by_gptr(e.second.gptr, false);
      }
      // Null-buckets have lptr set to nullptr
      /*
      if (e.first != nullptr) {
        DASH_LOG_DEBUG("CollectivePersistentAllocator.clear", "deallocate local memory:",
                       e.first);
        deallocate_local(e.first);
      }
      */
    }
    _allocated.clear();
    DASH_LOG_DEBUG("CollectivePersistentAllocator.clear >");
  }

  void open_pmem_pool()
  {
    if (_pmem_pool != nullptr) {
      return;
    }

    int flags;
    if (_poolID.empty()) {
      _poolID = dash::util::random_str(objectID_MAXLEN) + ".pmem";
      //Ensure that this file is exclusively created
      //If the path already exists it raises an error
      flags = DART_PMEM_FILE_CREATE | DART_PMEM_FILE_EXCL;
    } else {
      flags = DART_PMEM_FILE_CREATE;
    }

    mode_t mode = S_IRWXU;

    _pmem_pool = dart__pmem__pool_open(_team_id, _poolID.c_str(), flags, mode);

    //relocate_pmem_buckets();

    DASH_ASSERT_MSG(_pmem_pool != nullptr, "_pmem_pool must not be NULL");
  }

  void detach_bucket_by_gptr(dart_gptr_t const & gptr, bool deallocate = false)
  {
    DASH_LOG_DEBUG("CollectivePersistentAllocator.detach_bucket_by_gptr",
                   "deallocate: ", deallocate);
    auto bucket_it =
      std::find_if(_allocated.begin(), _allocated.end(),
    [&gptr] (pmem_bucket_item_t const & i) {
      return i.second.gptr == gptr;
    });

    if (bucket_it == _allocated.end()) {
      DASH_LOG_ERROR("CollectivePersistentAllocator.detach: cannot detach gptr");
      return;
    }

    if (dart_team_memderegister(_team_id, gptr) == DART_OK) {
      //persist all changes in persistent memory
      DASH_ASSERT_RETURNS(
        dart__pmem__persist_addr(_pmem_pool, bucket_it->first,
                                 bucket_it->second.nbytes),
        DART_OK);

      if (deallocate) {
        //bucket_it->first = nullptr;
        //bucket_it->second.gptr = DART_GPTR_NULL;
        //bucket_it->second.nbytes = 0;

        //TODO: free persistent memory region
      }

      _allocated.erase(bucket_it);
    }
    DASH_LOG_DEBUG("CollectivePersistentAllocator.detach_bucket_by_gptr >");
  }

  void relocate_pmem_buckets()
  {
    if (_allocated.size() > 0) {
      return;
    }

    DASH_LOG_TRACE("CollectivePersistentAllocator.relocate_pmem_buckets");

    struct dart_pmem_pool_stat stats;

    DASH_ASSERT(_pmem_pool);
    dart__pmem__pool_stat(_pmem_pool, &stats);

    if (stats.num_objects > 0) {
      DASH_ASSERT(stats.num_objects == 1);
      std::vector<dart_pmem_oid_t> bucket_ptrs{stats.num_objects, DART_PMEM_OID_NULL};

      dart__pmem__fetch_all(_pmem_pool, bucket_ptrs.data());

      auto pool = _pmem_pool;
      auto create_bucket_info = [pool](dart_pmem_oid_t const & poid) {
        local_pointer lptr = nullptr;
        //convert it to a native address
        DASH_ASSERT_RETURNS(
          dart__pmem__get_addr(poid, reinterpret_cast<void **>(&lptr)),
          DART_OK);

        pmem_bucket_info_t bucket;
        //bucket.pmem_addr = oid;
        DASH_ASSERT_RETURNS(dart__pmem__sizeof_oid(pool, poid, &bucket.nbytes),
                            DART_OK);
        DASH_LOG_TRACE("CollectivePersistentAllocator.relocate_pmem_buckets()", "relocated bucket, nbytes", bucket.nbytes);
        return std::make_pair(lptr, bucket);
      };

      std::transform(bucket_ptrs.begin(), bucket_ptrs.end(),
                     std::back_inserter(_allocated), create_bucket_info);
    }
    DASH_LOG_TRACE("CollectivePersistentAllocator.relocate_pmem_buckets >");
  }

private:
  dart_team_t          _team_id   = DART_TEAM_NULL;
  size_t               _nunits    = 0;
  std::vector<pmem_bucket_item_t> _allocated;
  std::string          _poolID = "";
  dart_pmem_pool_t  *  _pmem_pool = nullptr;

}; // class CollectivePersistentAllocator

template <class T, class U>
bool operator==(
  const CollectivePersistentAllocator<T> & lhs,
  const CollectivePersistentAllocator<U> & rhs)
{
  return (sizeof(T)    == sizeof(U) &&
          lhs._team_id == rhs._team_id &&
          lhs._nunits  == rhs._nunits );
}

template <class T, class U>
bool operator!=(
  const CollectivePersistentAllocator<T> & lhs,
  const CollectivePersistentAllocator<U> & rhs)
{
  return !(lhs == rhs);
}

} // namespace allocator
} // namespace dash

#endif //DASH_ENABLE_PMEM
#endif // DASH__ALLOCATOR__COLLECTIVE_PERSISTENT_ALLOCATOR_H__INCLUDED
