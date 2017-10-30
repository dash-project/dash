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

/**
 * Helper class encapsulating single bucket container.
 */
template <typename GlobMemType>
struct container_data {

  typedef typename GlobMemType::container_type            container_type;
  typedef typename GlobMemType::value_type                value_type;
  typedef typename GlobMemType::index_type                index_type;
  typedef typename GlobMemType::size_type                 size_type;
  typedef typename GlobMemType::local_iterator            local_iterator;
  typedef typename local_iterator::bucket_type            bucket_type;
  typedef typename std::list<bucket_type>                 bucket_list;
  typedef typename 
    GlobMemType::bucket_cumul_sizes_map::difference_type  bucket_sizes_index;

  /**
   * Constructor. Creates containers for globally available elements and 
   * locally available (unattached) elements.
   */
  container_data(size_type n_local_elem) 
  : container(new container_type()),
    unattached_container(new container_type())
  { 
    container->reserve(n_local_elem);
  }

  /**
   * Default constructor. Explicitly deleted.
   */
  container_data() = delete;

  /** container for globally available elements */
  std::shared_ptr<container_type>     container;
  /** container for locally available (unattached) elements */
  std::shared_ptr<container_type>     unattached_container;
  /** pointer to corresponding bucket in GlobHeapContiguousMem object */
  bucket_type *                       container_bucket;
  /** pointer to corresponding bucket in GlobHeapContiguousMem object */
  bucket_type *                       unattached_container_bucket;

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
  typedef GlobHeapContiguousMem<ContainerType>          self_t;

public:
  typedef ContainerType                                 container_type;
  typedef internal::container_data<self_t>              data_type;
  typedef std::list<data_type>                          container_list_type;
  typedef typename 
    container_list_type::difference_type                container_list_index;
  typedef typename ContainerType::value_type            value_type;
  typedef typename ContainerType::difference_type       index_type;
  typedef GlobHeapLocalPtr<value_type, index_type>      local_iterator;
  typedef GlobPtr<value_type, self_t>                   global_iterator;
  typedef typename ContainerType::size_type             size_type;
  typedef typename local_iterator::bucket_type          bucket_type;
  typedef typename std::list<bucket_type>               bucket_list;
  typedef typename std::list<bucket_type *>             bucket_ptr_list;
  typedef local_iterator                                local_pointer;
  typedef local_iterator                                const_local_pointer;
  typedef std::vector<std::vector<size_type>>           bucket_cumul_sizes_map;
  
  template<typename T_, class GMem_>
  friend class dash::GlobPtr;
  friend class GlobHeapCombinedMem<self_t>;

public:

  /**
   * Constructor.
   */
  GlobHeapContiguousMem(Team & team = dash::Team::All())
    : _container_list(new container_list_type()),
      _buckets(),
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
  container_list_index add_container(size_type n_elements) {
    increment_bucket_sizes();
    auto c_data = data_type(n_elements);
   
    // create bucket data and add to bucket list
    bucket_type cont_bucket { 
      0, 
      c_data.container->data(), 
      DART_GPTR_NULL,
      false
    };
    bucket_type unattached_cont_bucket {
      0,
      c_data.unattached_container->data(),
      DART_GPTR_NULL,
      false
    };
    _buckets.push_back(cont_bucket);
    // add a pointer to the corresponding bucket data
    c_data.container_bucket = &(_buckets.back());
    // for global iteration, only _container's bucket is needed
    _global_buckets.push_back(&(_buckets.back()));
    _buckets.push_back(unattached_cont_bucket);
    c_data.unattached_container_bucket = &(_buckets.back());

    _container_list->push_back(c_data);
    return _container_list->size() - 1;
  }

  /**
   * Returns a reference to the requested element.
   */
  value_type & get(container_list_index cont, index_type pos) {
    auto it = _container_list->begin();
    std::advance(it, cont);
    auto c_data = *it;
    if(c_data.container->size() > pos) {
      return c_data.container->operator[](pos);
    }
    pos -= c_data.container->size();
    return c_data.unattached_container->operator[](pos);
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
    std::vector<size_type> container_count(_team->size());
    int my_container_count = _container_list->size();
    DASH_ASSERT_RETURNS(
        dart_allgather(&my_container_count, container_count.data(), 
          sizeof(size_type), DART_TYPE_BYTE, _team->dart_id()),
        DART_OK
    );
    auto max_containers = std::max_element(container_count.begin(),
                                           container_count.end());

    int bucket_num = 0;
    int bucket_cumul = 0;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Important for performance:
    // TODO: put multiple containers into one bucket
    // TODO: update only containers with unattached_container.size() > 0
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // attach buckets for the maximum amount of containers, even if this unit
    // holds less containers
    {
      data_type * c_data;
      auto c_data_it = _container_list->begin();
      auto current_size = _container_list->size();
      for(int i = 0; i < *max_containers; ++i) {
        if(i < current_size) {
          c_data = &(*c_data_it);
          // merge public and local containers
          c_data->container->insert(c_data->container->end(),
              c_data->unattached_container->begin(),
              c_data->unattached_container->end());
          c_data->unattached_container->clear();
          // update memory location & size of _container
          c_data->container_bucket->lptr = c_data->container->data();
          c_data->container_bucket->size = c_data->container->size();
          // update memory location & size of _unattached_container
          c_data->unattached_container_bucket->lptr 
            = c_data->unattached_container->data();
          c_data->unattached_container_bucket->size = 0;
          ++c_data_it;
        } else {
          // if other units add more containers, create an empty container to 
          // store a dart_gptr for the collective allocation
          auto cont_it = _container_list->begin();
          auto cont_index = add_container(0);
          std::advance(cont_it, cont_index);
          c_data = &(*cont_it);
        }

        //  detach old container location from global memory space, if it has
        //  been attached before
        if(c_data->container_bucket->gptr != DART_GPTR_NULL) {
          DASH_ASSERT_RETURNS(
            dart_team_memderegister(c_data->container_bucket->gptr),
            DART_OK
          );
        }

        // attach new container location to global memory space
        dart_gptr_t gptr = DART_GPTR_NULL;
        dart_storage_t ds = dart_storage<value_type>(c_data->container->size());
        DASH_ASSERT_RETURNS(
          dart_team_memregister(
            _team->dart_id(), 
            ds.nelem, 
            ds.dtype, 
            c_data->container->data(), 
            &gptr),
          DART_OK
        );
        // no need to update gptr of local bucket list in c_data
        c_data->container_bucket->gptr = gptr;
        
        // update cumulated bucket sizes
        bucket_cumul += c_data->container->size();
        _bucket_cumul_sizes[_myid][bucket_num] = bucket_cumul;
        ++bucket_num;
      }
      _team->barrier();
    }

    // distribute bucket sizes between all units
    // TODO: use one allgather for all buckets
    // TODO: make it work for unevenly distributed amount of buckets
    auto bucket_count = _bucket_cumul_sizes[_myid].size();
    for(auto c_data : *_container_list) {
      std::vector<size_type> bucket_sizes(bucket_count * _team->size());
      std::vector<size_type> local_buckets(_bucket_cumul_sizes[_myid]);
      DASH_ASSERT_RETURNS(
        dart_allgather(local_buckets.data(), bucket_sizes.data(), 
          sizeof(size_type) * local_buckets.size(), DART_TYPE_BYTE, _team->dart_id()),
        DART_OK
      );
      _size = 0;
      auto begin = bucket_sizes.begin();
      for(int i = 0; i < _team->size(); ++i) {
        auto end = begin + bucket_count - 1;
        std::copy(begin, end + 1, _bucket_cumul_sizes[i].begin());
        begin = end + 1;
        _size += *end;
      }
    }

    update_lbegin();
    update_lend();
    int b_num = 0;

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
  local_iterator push_back(container_list_index cont, value_type & val) {
    auto cont_it = _container_list->begin();
    std::advance(cont_it, cont);
    auto c_data = *cont_it;
    // use _unattached container, if _container is full
    // we don't want a realloc of _container because this changes the memory
    // location, which invalidates global pointers of other units
    if(c_data.container->capacity() == c_data.container->size()) {
      c_data.unattached_container->push_back(val);
      c_data.unattached_container_bucket->lptr 
        = c_data.unattached_container->data();
      ++(c_data.unattached_container_bucket->size);
    } else {
      c_data.container->push_back(val);
      ++(c_data.container_bucket->size);
    }
    ++_local_size;

    update_lbegin();
    update_lend();
    return _lend;
  }

  /**
   * Returns the local size of a given bucket.
   */
  size_type container_local_size(container_list_index index) const {
    auto cont_it = _container_list->begin();
    std::advance(cont_it, index);
    auto c_data = *cont_it;
    return c_data.container->size() + c_data.unattached_container->size();
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
    index_type  bucket_index,
    /// Offset of the referenced address in the bucket's memory space.
    index_type  bucket_phase) const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at(u,bi,bp)",
                   unit, bucket_index, bucket_phase);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    // Get the referenced bucket's dart_gptr:
    auto bucket_it = _global_buckets.begin();
    std::advance(bucket_it, bucket_index);
    auto dart_gptr = (*bucket_it)->gptr;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", (*bucket_it)->attached);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", (*bucket_it)->gptr);
    if (unit == _myid) {
      DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", (*bucket_it)->lptr);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", (*bucket_it)->size);
      DASH_ASSERT_LT(bucket_phase, (*bucket_it)->size,
                     "bucket phase out of bounds");
    }
    if (DART_GPTR_ISNULL(dart_gptr)) {
      DASH_LOG_TRACE("GlobDynamicMem.dart_gptr_at",
                     "bucket.gptr is DART_GPTR_NULL");
      dart_gptr = DART_GPTR_NULL;
    } else {
      // Move dart_gptr to unit and local offset:
      DASH_ASSERT_RETURNS(
        dart_gptr_setunit(&dart_gptr, unit),
        DART_OK);
      DASH_ASSERT_RETURNS(
        dart_gptr_incaddr(
          &dart_gptr,
          bucket_phase * sizeof(value_type)),
        DART_OK);
    }
    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at >", dart_gptr);
    return dart_gptr;
  }

private:

  // NOTE: method copied from GlobHeapMem.h
  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   *
   */
  void update_lbegin() noexcept
  {
    local_iterator unit_lbegin(
             // iteration space
             _buckets.begin(), _buckets.end(),
             // position in iteration space
             0,
             // bucket at position in iteration space,
             // offset in bucket
             _buckets.begin(), 0);
    _lbegin = unit_lbegin;
  }

  // NOTE: method copied from GlobHeapMem.h
  /**
   * Update internal native pointer of the final address of the local memory
   * of a unit.
   */
  void update_lend() noexcept
  {
    local_iterator unit_lend(
             // iteration space
             _buckets.begin(), _buckets.end(),
             // position in iteration space
             _local_size,
             // bucket at position in iteration space,
             // offset in bucket
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

  /** List of all attached container */
  std::shared_ptr<container_list_type>    _container_list;
  /** List of buckets for GlobHeapLocalPtr */
  bucket_list                             _buckets;
  /** list of buckets available for global iteration */
  bucket_ptr_list                         _global_buckets;
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

