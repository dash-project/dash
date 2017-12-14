#ifndef DASH__MEMORY__GLOB_HEAP_CONTIGUOUS_MEM_H_
#define DASH__MEMORY__GLOB_HEAP_CONTIGUOUS_MEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/GlobSharedRef.h>
#include <dash/Team.h>

#include <dash/memory/GlobHeapLocalPtr.h>
#include <dash/memory/internal/GlobHeapMemTypes.h>
#include <dash/memory/GlobHeapContiguousPtr.h>
#include <dash/memory/GlobHeapCombinedMem.h>

#include <dash/internal/Logging.h>

#include <list>
#include <vector>

namespace dash {

namespace internal {

template<typename ContainerType>
struct container_data {

  typedef ContainerType                                    container_type;
  typedef typename ContainerType::value_type               value_type;
  typedef typename ContainerType::difference_type          index_type;
  typedef typename 
    GlobHeapLocalPtr<value_type, index_type>::bucket_type  bucket_type;

  container_data(bucket_type & bkt)
    : bucket(bkt)
  { }

  container_type  container;
  bucket_type &   bucket;

};

}

//TODO: adapt to GlobMem concept
/**
 * Global memory space for multiple, dynaimcally allocated contiguous memory 
 * regions.
 */
template<typename ContainerType>
class GlobHeapContiguousMem {

private:
  typedef GlobHeapContiguousMem<ContainerType>         self_t;

public:
  typedef ContainerType                                container_type;
  typedef internal::container_data<container_type>     data_type;
  typedef std::vector<data_type>                       container_list_type;
  typedef typename ContainerType::value_type           value_type;
  typedef typename ContainerType::difference_type      index_type;
  typedef GlobHeapLocalPtr<value_type, index_type>     local_iterator;
  typedef GlobPtr<value_type, self_t>                  global_iterator;
  typedef typename ContainerType::size_type            size_type;
  typedef typename local_iterator::bucket_type         bucket_type;
  // must be List because of GlobHeapLocalPtr
  typedef typename std::list<bucket_type>              bucket_list_type;
  typedef typename std::list<bucket_type *>            bucket_ptr_list;
  typedef typename bucket_ptr_list::difference_type    bucket_index_type;
  typedef typename bucket_list_type::difference_type   local_bucket_index_type;
  typedef local_iterator                               local_pointer;
  typedef local_iterator                               const_local_pointer;
  typedef std::vector<std::vector<size_type>>          bucket_cumul_sizes_map;
  
  template<typename T_, class GMem_>
  friend class dash::GlobPtr;
  friend class GlobHeapCombinedMem<self_t>;

public:

  /**
   * Constructor.
   */
  GlobHeapContiguousMem(Team & team = dash::Team::All())
    : _buckets(),
      _global_buckets(),
      _team(&team),
      _teamid(team.dart_id()),
      _nunits(team.size()),
      _myid(team.myid()),
      _bucket_cumul_sizes(team.size())
  { }

  /**
   * Adds a new bucket into memory space.
   */
  bucket_index_type add_container(size_type n_elements) {
    // TODO: set capacity
    increment_bucket_sizes();
   
    // create bucket data and add to bucket list
    bucket_type cont_bucket { 
      0, 
      nullptr, 
      DART_GPTR_NULL,
      false
    };
    bucket_type unattached_cont_bucket {
      0,
      nullptr,
      DART_GPTR_NULL,
      false
    };
    _buckets.push_back(cont_bucket);
    // for global iteration, only _container's bucket is needed
    _global_buckets.push_back(&(_buckets.back()));

    _unattached_containers.emplace_back(_buckets.back());
    auto & unattached_container = _unattached_containers.back().container;
    unattached_container.reserve(n_elements);

    _buckets.push_back(unattached_cont_bucket);
    return _global_buckets.size() - 1;
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobHeapContiguousMem() { }

  /**
   * Default constructor. Explicitly deleted.
   */
  GlobHeapContiguousMem() = delete;

  /**
   * Copy constructor.
   */
  GlobHeapContiguousMem(const self_t & other) = default;

  /**
   * Move constructor.
   */
  GlobHeapContiguousMem(self_t && other) = default;

  /**
   * Copy-assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * Move-ssignment operator.
   */
  self_t & operator=(self_t && other) = default;

  /**
   * Commit local changes in memory to global memory space.
   * 
   * Collective operation.
   */
  void commit() {
    // Gather information about the max amount of containers a single unit
    // currently holds
    std::vector<size_type> bucket_count(_team->size());
    size_type my_bucket_count = _unattached_containers.size();
    DASH_ASSERT_RETURNS(
        dart_allgather(&my_bucket_count, bucket_count.data(), 
          sizeof(size_type), DART_TYPE_BYTE, _team->dart_id()),
        DART_OK
    );
    auto max_buckets = std::max_element(bucket_count.begin(),
                                           bucket_count.end());

    container_type * new_container = new container_type();
    new_container->reserve(_local_size);
    int count = 0;
    int elements = 0;
    int bucket_num = 0;
    size_type bucket_cumul = 0;
    auto unattached_container_it = _unattached_containers.begin();
    bucket_type * last_bucket;
    int elements_before = 0;
    for(auto & bucket : _buckets) {
      elements_before = elements;
      // move data to new range
      if(bucket.lptr != nullptr) {
        // TODO: optimize memory usage (delete already moved elements)
        new_container->insert(new_container->end(), bucket.lptr, 
            bucket.lptr + bucket.size);
        elements += bucket.size;
      }
      // bucket list alternates between attached and unattached buckets.
      // if bucket is already attached:
      if(count % 2 == 0) {
        if(bucket.size > 0) {
          bucket.lptr = new_container->data() + elements_before;
        }
        last_bucket = &bucket;
      } else {
        // if bucket is unattached:
        if(bucket.size > 0) {
          if(last_bucket->size == 0) {
            last_bucket->lptr = new_container->data() + elements_before;
          }
          last_bucket->size += bucket.size;
          bucket.size = 0;
          bucket.lptr = nullptr;
          unattached_container_it->container.clear();
          ++unattached_container_it;
        }
        // update cumulated bucket sizes
        bucket_cumul += last_bucket->size;
        _bucket_cumul_sizes[_myid][bucket_num] = bucket_cumul;
        ++bucket_num;
      }
      ++count;
    }
    // 2 local buckets per global bucket
    count /= 2;
    for(int i = count; i < *max_buckets; ++i) {
      add_container(0);
      if(count > 0) {
        _bucket_cumul_sizes[_myid][count] = 
          _bucket_cumul_sizes[_myid][count - 1]; 
      } else {
        _bucket_cumul_sizes[_myid][count] = 0; 
      }
    }

    //  detach old container location from global memory space, if it has
    //  been attached before
    if(_dart_gptr != DART_GPTR_NULL) {
      DASH_ASSERT_RETURNS(
        dart_team_memderegister(_dart_gptr),
        DART_OK
      );
    }

    _container.reset(new_container);

    // attach new container location to global memory space
    dash::dart_storage<value_type> ds(_container->size());
    DASH_ASSERT_RETURNS(
      dart_team_memregister(
        _team->dart_id(), 
        ds.nelem, 
        ds.dtype, 
        _container->data(), 
        &_dart_gptr),
      DART_OK
    );

    // distribute bucket sizes between all units
    auto bucket_amount = _bucket_cumul_sizes[_myid].size();
    std::vector<size_type> bucket_sizes(bucket_amount * _team->size());
    std::vector<size_type> local_buckets(_bucket_cumul_sizes[_myid]);
    DASH_ASSERT_RETURNS(
      dart_allgather(local_buckets.data(), bucket_sizes.data(), 
        sizeof(size_type) * local_buckets.size(), DART_TYPE_BYTE, _team->dart_id()),
      DART_OK
    );
    _size = 0;
    auto begin = bucket_sizes.begin();
    for(int i = 0; i < _team->size(); ++i) {
      auto end = begin + bucket_amount;
      std::copy(begin, end, _bucket_cumul_sizes[i].begin());
      begin = end;
      _size += *(end - 1);
    }

    // update local iterators
    update_lbegin();
    update_lend();

    // update global iterators
    _begin = global_iterator(this, 0);
    _end = global_iterator(this, _size);
  }

  /**
   * Iterator to the beginning of the memory space.
   */
  global_iterator begin() const {
    return _begin;
  }

  /**
   * Iterator to the end of the memory space.
   */
  global_iterator end() const {
    return _end;
  }

  /**
   * Iterator to the beginning of the memory space's local portion.
   */
  local_iterator lbegin() const {
    // TODO: use iterator of _container, if _unattached_containers do not 
    //       contain any data
    return _lbegin;
  }

  /**
   * Iterator to the end of the memory space's local portion.
   */
  local_iterator lend() const {
    return _lend;
  }

  /**
   * Insert value at the end of the given bucket.
   */
  local_iterator push_back(bucket_index_type index, value_type & val) {
    // we don't want a realloc of _container because this changes the memory
    // location, which invalidates global pointers of other units
    auto & unatt = _unattached_containers[index];
    unatt.container.push_back(val);
    unatt.bucket.size = unatt.container.size();
    unatt.bucket.lptr = unatt.container.data();
    ++_local_size;

    update_lbegin();
    update_lend();
    return _lend - 1;
  }

  /**
   * Returns the global size of a given bucket.
   */
  size_type container_size(team_unit_t unit, size_type index) const {
    if(index <= 0) {
      return _bucket_cumul_sizes[unit][index];
    }
    return _bucket_cumul_sizes[unit][index] - 
      _bucket_cumul_sizes[unit][index - 1];
  }

  /**
   * Returns the amount of elements in global memory space.
   */
  size_type size() const {
    return _size;
  }

  /**
   * Returns the team containing all units associated with this memory space.
   */
  Team & team() const {
    return *_team;
  }

   // NOTE: method copied from GlobHeapMem.h
   /**
   * Global pointer referencing an element position in a unit's bucket.
   */
  dart_gptr_t dart_gptr_at(
    /// Unit id mapped to address in global memory space.
    team_unit_t unit,
    /// Index of bucket containing the referenced address.
    bucket_index_type  bucket_index,
    /// Offset of the referenced address in the bucket's memory space.
    index_type  bucket_phase) const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at(u,bi,bp)",
                   unit, bucket_index, bucket_phase);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    auto dart_gptr = _dart_gptr;
    if (DART_GPTR_ISNULL(dart_gptr)) {
      DASH_LOG_TRACE("GlobDynamicMem.dart_gptr_at",
                     "bucket.gptr is DART_GPTR_NULL");
      dart_gptr = DART_GPTR_NULL;
    } else {
      size_type bucket_start;
      if(bucket_index > 0) {
        bucket_start = _bucket_cumul_sizes[unit][bucket_index - 1];
      } else {
        bucket_start = 0;
      }
      // Move dart_gptr to unit and local offset:
      DASH_ASSERT_RETURNS(
        dart_gptr_setunit(&dart_gptr, unit),
        DART_OK);
      DASH_ASSERT_RETURNS(
        dart_gptr_incaddr(
          &dart_gptr,
          (bucket_start + bucket_phase) * sizeof(value_type)),
        DART_OK);
    }
    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at >", dart_gptr);
    return dart_gptr;
  }

private:

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   *
   */
  void update_lbegin() noexcept
  {
    // cannot use lightweight constructor here, because the first bucket might 
    // be empty
    local_iterator unit_lbegin(_buckets.begin(), _buckets.end(), 0);
    _lbegin = unit_lbegin;
  }

  /**
   * Update internal native pointer of the final address of the local memory
   * of a unit.
   */
  void update_lend() noexcept
  {
    local_iterator unit_lend(_buckets.begin(), _buckets.end(), _local_size,
        _buckets.end(), 0);
    _lend = unit_lend;
  }

  /**
   * Increments the bucket count of each unit by one.
   */
  void increment_bucket_sizes() {
    for(auto it = _bucket_cumul_sizes.begin(); 
        it != _bucket_cumul_sizes.end(); ++it) {
      // gets initiliazed with 0 automatically
      it->resize(it->size() + 1);
    }
  }

private:

  /** List of buckets for GlobHeapLocalPtr */
  bucket_list_type                        _buckets;
  /** list of buckets available for global iteration */
  bucket_ptr_list                         _global_buckets;
  /** Container holding globally visible data */
  std::unique_ptr<container_type>         _container;
  /** List of containers holding locally visible data of each bucket */
  container_list_type                     _unattached_containers;
  /** DART gptr of _container */
  dart_gptr_t                             _dart_gptr = DART_GPTR_NULL;
  /** Team associated with this memory space */
  Team *                                  _team;
  /** ID of the team */
  dart_team_t                             _teamid;
  /** Number of units in the team */
  size_type                               _nunits = 0;
  /** ID of this unit */
  team_unit_t                             _myid{DART_UNDEFINED_UNIT_ID};
  /** Iterator to the beginning ot the memory space */
  global_iterator                         _begin;
  /** Iterator to the end ot the memory space */
  global_iterator                         _end;
  /** Iterator to the beginning of this memory space's local portion */
  local_iterator                          _lbegin;
  /** Iterator to the end of this memory space's local portion */
  local_iterator                          _lend;
  /** Accumulated sizes of the buckets of each unit. See GlobHeapMem */
  bucket_cumul_sizes_map                  _bucket_cumul_sizes;
  /** Global size of the memory space */
  size_type                               _size = 0;
  /** Local size of the memory space */
  size_type                               _local_size = 0;

};

}

#endif // DASH_MEMORY__GLOB_HEAP_CONTIGUOUS_MEM_H_

