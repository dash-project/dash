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
#include <iterator>
#include <sstream>
#include <iostream>

#include <iostream>


namespace dash {

template<
  typename ContainerType>
class GlobDynamicContiguousMem
{
private:
  typedef GlobDynamicContiguousMem<ContainerType>
    self_t;

public:
  typedef ContainerType                                      container_type;
  typedef typename ContainerType::value_type                 value_type;
  typedef typename ContainerType::difference_type            index_type;
  typedef LocalBucketIter<value_type, index_type>            local_iterator;
  typedef typename ContainerType::size_type                  size_type;
  typedef typename local_iterator::bucket_type               bucket_type;
  // TODO: use std::array instead of list -> change LocalBucketIter
  typedef typename std::list<bucket_type>                    bucket_list;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   *
   * \concept{DashDynamicMemorySpaceConcept}
   * \concept{DashMemorySpaceConcept}
   */
  GlobDynamicContiguousMem(
    size_type   n_local_elem = 0,
    Team      & team         = dash::Team::All())
  : _container(new container_type()),
    _unattached_container(new container_type()),
    _buckets(),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid())
  {
    _container->reserve(n_local_elem);
    bucket_type cont_bucket { 
      0, 
      _container->data(), 
      DART_GPTR_NULL,
      false
    };
    bucket_type unattached_cont_bucket {
      0,
      _unattached_container->data(),
      DART_GPTR_NULL,
      false
    };
    _buckets.push_back(cont_bucket);
    _buckets.push_back(unattached_cont_bucket);
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

  void commit()
  {
    // merge public and local containers together
    _container->insert(_container->end(), 
        _unattached_container->begin(), 
        _unattached_container->end());
    _unattached_container->clear();
    // update memory location & size of _container
    auto it = _buckets.begin();
    it->lptr = _container->data();
    it->size = _container->size();
    // update memory location & size of _unattached_container
    ++it;
    it->lptr = _unattached_container->data();
    it->size = 0;

    update_lbegin();
    update_lend();

    // attach new container to global memory space
    dart_gptr_t gptr = DART_GPTR_NULL;
    dart_storage_t ds = dart_storage<value_type>(_container->size());
    dart_team_memregister(
        _team->dart_id(), ds.nelem, ds.dtype, _container->data(), &gptr);
    it = _buckets.begin();
    it->gptr = gptr;
  }

  local_iterator lbegin() {
    return _lbegin;
  }

  local_iterator lend() {
    return _lend;
  }

  void push_back(value_type val) {
    // bucket of _container
    auto it = _buckets.begin();
    // use _unattached container, if _container is full
    // we don't want a realloc of _container because this changes the memory
    // location, which invalidates global pointers of other units
    if(_container->capacity() == _container->size()) {
      // bucket of _unattached_container
      ++it;
      _unattached_container->push_back(val);
      // adding data might change the memory location
      it->lptr = _unattached_container->data();
    } else {
      _container->push_back(val);
    }
    ++(it->size);
    update_lbegin();
    update_lend();
  }

   /**
   * Global pointer referencing an element position in a unit's bucket.
   */
  dart_gptr_t dart_gptr_at(team_unit_t unit, index_type bucket_phase) {
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
             _container->size() + _unattached_container->size(),
             // bucket at position in iteration space,
             // offset in bucket
             _buckets.end(), 0);
    _lend = unit_lend;
  }

private:

  container_type *           _container;
  container_type *           _unattached_container = nullptr;
  bucket_list                _buckets;
  Team *                     _team;
  dart_team_t                _teamid;
  size_type                  _nunits = 0;
  team_unit_t                _myid{DART_UNDEFINED_UNIT_ID};
  local_iterator             _lbegin;
  local_iterator             _lend;

};

}

#endif // DASH__GLOB_DYNAMIC_SEQUENTIAL_MEM_H_

