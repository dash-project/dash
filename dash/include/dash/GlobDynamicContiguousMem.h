#ifndef DASH__GLOB_DYNAMIC_SEQUENTIAL_MEM_H_
#define DASH__GLOB_DYNAMIC_SEQUENTIAL_MEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/GlobSharedRef.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>
#include <dash/Array.h>

#include <dash/algorithm/MinMax.h>
#include <dash/algorithm/Copy.h>

#include <dash/allocator/LocalBucketIter.h>
#include <dash/allocator/internal/GlobDynamicMemTypes.h>

#include <dash/internal/Logging.h>

#include <list>
#include <vector>
#include <unordered_map>
#include <iterator>
#include <sstream>
#include <iostream>

#include <iostream>


namespace dash {

namespace internal {

template <typename ContainerType>
struct container_data {

  typedef ContainerType                                   container_type;
  typedef typename ContainerType::value_type              value_type;
  typedef typename ContainerType::difference_type         index_type;
  typedef typename ContainerType::size_type               size_type;
  typedef LocalBucketIter<value_type, index_type>         local_iterator;
  typedef typename local_iterator::bucket_type            bucket_type;
  typedef typename std::list<bucket_type>                 bucket_list;

  container_data(size_type n_local_elem) 
  : container(new container_type()),
    unattached_container(new container_type())
  { 
    container->reserve(n_local_elem);
    bucket_type cont_bucket { 
      0, 
      container->data(), 
      DART_GPTR_NULL,
      false
    };
    bucket_type unattached_cont_bucket {
      0,
      unattached_container->data(),
      DART_GPTR_NULL,
      false
    };
    buckets.push_back(cont_bucket);
    buckets.push_back(unattached_cont_bucket);

    update_lbegin();
    update_lend();
  }

    /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   *
   */
  void update_lbegin() noexcept
  {
    local_iterator unit_lbegin(
             // iteration space
             buckets.begin(), buckets.end(),
             // position in iteration space
             0,
             // bucket at position in iteration space,
             // offset in bucket
             buckets.begin(), 0);
    lbegin = unit_lbegin;
  }

  /**
   * Update internal native pointer of the final address of the local memory#include <dash/allocator/LocalBucketIter.h>
   * of a unit.
   */
  void update_lend() noexcept
  {
    local_iterator unit_lend(
             // iteration space
             buckets.begin(), buckets.end(),
             // position in iteration space
             container->size() + unattached_container->size(),
             // bucket at position in iteration space,
             // offset in bucket
             buckets.end(), 0);
    lend = unit_lend;
  }

  ContainerType *    container;
  ContainerType *    unattached_container;
  bucket_list        buckets;
  bucket_type *      container_bucket;
  bucket_type *      unattached_container_bucket;
  local_iterator     lbegin;
  local_iterator     lend;

};

}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: Must be compatible (= same interface) to GlobDynamicMem so it
//       can be replaced in e.g.
//
//       dash::Grap<T, ... , dash::GlobDynamicMem>
//       dash::Grap<T, ... , dash::GlobDynamicContiguousMem>
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
template<
  typename ContainerType>
class GlobDynamicContiguousMem
{
private:
  typedef GlobDynamicContiguousMem<ContainerType>         self_t;


public:
  typedef ContainerType                                   container_type;
  typedef internal::container_data<ContainerType>         data_type;
  typedef std::list<data_type>                            container_list_type;
  typedef typename container_list_type::iterator          container_list_iter;
  typedef typename ContainerType::value_type              value_type;
  typedef typename ContainerType::difference_type         index_type;
  typedef LocalBucketIter<value_type, index_type>         local_iterator;
  typedef GlobBucketIter<value_type, self_t>              global_iterator;
  typedef typename ContainerType::size_type               size_type;
  typedef typename local_iterator::bucket_type            bucket_type;
  typedef typename std::list<bucket_type>                 bucket_list;
  typedef local_iterator                                  local_pointer;
  typedef local_iterator                                  const_local_pointer;

private:
  
  typedef std::vector<std::vector<size_type> >
    bucket_cumul_sizes_map;

  template<typename T_, class GMem_, class Ptr_, class Ref_>
  friend class dash::GlobBucketIter;

public:
  /**
   * Constructor
   */
  GlobDynamicContiguousMem(Team & team = dash::Team::All())
  : _buckets(),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid()),
    _bucket_cumul_sizes(team.size())
  {
    for(auto it = _bucket_cumul_sizes.begin(); 
        it != _bucket_cumul_sizes.end(); ++it) {
      it->resize(1);
    }
  }

  container_list_iter add_container(size_type n_elements) {
    auto c_data = data_type(n_elements);
    // TODO: use pointers instead of copies -> no maintenance of 2 object 
    //       copies
    auto it = _buckets.insert(_buckets.end(), 
        c_data.buckets.begin(), c_data.buckets.end());
    c_data.container_bucket = &(*it);
    ++it;
    c_data.unattached_container_bucket = &(*it);
    // insert won't invalidate iterators for std::list, so we can use them
    // to access the container

    return _container_list.insert(_container_list.end(), c_data);
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobDynamicContiguousMem() { }

  GlobDynamicContiguousMem() = delete;

  /**
   * Copy constructor.
   */
  GlobDynamicContiguousMem(const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs) = default;

  void commit() {
    for(auto c_data : _container_list) {
      // merge public and local containers
      c_data.container->insert(c_data.container->end(),
          c_data.unattached_container->begin(),
          c_data.unattached_container->end());
      c_data.unattached_container->clear();
      // update memory location & size of _container
      auto it = c_data.buckets.begin();
      it->lptr = c_data.container->data();
      it->size = c_data.container->size();
      c_data.container_bucket->lptr = c_data.container->data();
      c_data.container_bucket->size = c_data.container->size();
      // update memory location & size of _unattached_container
      ++it;
      it->lptr = c_data.unattached_container->data();
      it->size = 0;
      c_data.unattached_container_bucket->lptr 
        = c_data.unattached_container->data();
      c_data.unattached_container_bucket->size = 0;

      c_data.update_lbegin();
      c_data.update_lend();

      // attach new container to global memory space
      dart_gptr_t gptr = DART_GPTR_NULL;
      dart_storage_t ds = dart_storage<value_type>(c_data.container->size());
      dart_team_memregister(
          _team->dart_id(), 
          ds.nelem, 
          ds.dtype, 
          c_data.container->data(), 
          &gptr
      );
      // no need to update gptr of local bucekt list in c_data
      c_data.container_bucket->gptr = gptr;

      //TODO: Adjust bucket size accumulation, so that multiple containers
      //      are supported
      // distribute bucket sizes between all units
      std::vector<size_type> bucket_sizes(_team->size());
      dart_allgather(&_local_size, bucket_sizes.data(), 
          sizeof(size_type), DART_TYPE_BYTE, _team->dart_id());
      _size = 0;
      for(int i = 0; i < _team->size(); ++i) {
        _bucket_cumul_sizes[i][0] = bucket_sizes[i];
        _size += bucket_sizes[i];
      }
    }
    update_lbegin();
    update_lend();



    _begin = global_iterator(this, 0);
    _end = _begin + _size;
  }

  global_iterator begin() {
    return _begin;
  }

  global_iterator end() {
    return _end;
  }

  local_iterator lbegin() {
    return _lbegin;
  }

  local_iterator lend() {
    return _lend;
  }

  void push_back(container_list_iter pos, value_type val) {
    auto c_data = *pos;
    // bucket of _container
    auto it = c_data.buckets.begin();
    // use _unattached container, if _container is full
    // we don't want a realloc of _container because this changes the memory
    // location, which invalidates global pointers of other units
    if(c_data.container->capacity() == c_data.container->size()) {
      // bucket of _unattached_container
      ++it;
      c_data.unattached_container->push_back(val);
      // adding data might change the memory location
      it->lptr = c_data.unattached_container->data();
      c_data.unattached_container_bucket->lptr 
        = c_data.unattached_container->data();
      ++(c_data.unattached_container_bucket->size);
    } else {
      c_data.container->push_back(val);
      ++(c_data.container_bucket->size);
    }
    ++(it->size);
    ++_local_size;
    c_data.update_lbegin();
    c_data.update_lend();
    update_lbegin();
    update_lend();
    _bucket_cumul_sizes[_myid][0] += 1;
  }

   /**
   * Global pointer referencing an element position in a unit's bucket.
   */
  dart_gptr_t dart_gptr_at(team_unit_t unit, 
      index_type bucket_index, index_type bucket_phase) {
    // we only have 1 global pointer for _container
    auto bucket = *_buckets.begin();
    auto dart_gptr = bucket.gptr;

    if (DART_GPTR_ISNULL(dart_gptr)) {
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
    return dart_gptr;
  }

  size_type size() {
    return _size;
  }

  Team & team() {
    return (_team != nullptr) ? *_team : dash::Team::Null();
  }

private:
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

  /**
   * Update internal native pointer of the final address of the local memory#include <dash/allocator/LocalBucketIter.h>
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

private:

  container_list_type        _container_list;
  container_type *           _container;
  container_type *           _unattached_container = nullptr;
  bucket_list                _buckets;
  Team *                     _team;
  dart_team_t                _teamid;
  size_type                  _nunits = 0;
  team_unit_t                _myid{DART_UNDEFINED_UNIT_ID};
  global_iterator            _begin;
  global_iterator            _end;
  local_iterator             _lbegin;
  local_iterator             _lend;
  bucket_cumul_sizes_map     _bucket_cumul_sizes;
  size_type                  _size = 0;
  size_type                  _local_size = 0;

};

}

#endif // DASH__GLOB_DYNAMIC_SEQUENTIAL_MEM_H_

