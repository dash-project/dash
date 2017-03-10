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
class GlobDynamicSequentialMem
{
private:
  typedef GlobDynamicSequentialMem<ContainerType>
    self_t;

public:
  typedef ContainerType                                      container_type;
  typedef typename ContainerType::value_type                 value_type;
  typedef typename ContainerType::iterator                   local_iterator;
  typedef typename ContainerType::size_type                  size_type;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   *
   * \concept{DashDynamicMemorySpaceConcept}
   * \concept{DashMemorySpaceConcept}
   */
  GlobDynamicSequentialMem(
    size_type   n_local_elem = 0,
    Team      & team         = dash::Team::All())
  : _container(new container_type()),
    _public_container(_container),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid())
  {
    grow(n_local_elem);
    commit();
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobDynamicSequentialMem() { }

  GlobDynamicSequentialMem() = delete;

  /**
   * Copy constructor.
   */
  GlobDynamicSequentialMem(const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs) = default;

  local_iterator grow(size_type num_elements) {
    if(num_elements > 0) {
      _container->reserve(_container->capacity() + num_elements);
      _lbegin = _container->begin();
      _lend = _container->end();
    }
    // TODO: return end of container or start of newly added capacity?
    return _lend;
  }

  void shrink(size_type num_elements) {
    if(num_elements > 0) {
      _container->reserve(_container->capacity() - num_elements);
    }
  }

  void commit()
  {
  }

  local_iterator lbegin() {
    return _lbegin;
  }

  local_iterator lend() {
    return _lend;
  }

  void push_back(value_type val) {
    // if realloc would get triggered, copy container and keep old data so 
    // other units can still access the memory region
    if(_container->capacity() == _container->size()) {
      // TODO: new capacity? if capacity is not increased, push-back will
      // trigger a realloc and we can use the increase algorithm of the 
      // container, but this implicates another memory copy.
      container_type new_container(_container->capacity() * 2);
      new_container = *_container;
      if(_public_container != _container) {
        delete _public_container;
        _public_container = _container;
      }
       _container = &new_container;
       _lbegin = _container->begin();
    }
    _container->push_back(val);
    _lend = _container->end();
  }

private:

  container_type *           _container;
  container_type *           _public_container;
  Team *                     _team;
  dart_team_t                _teamid;
  size_type                  _nunits = 0;
  team_unit_t                _myid{DART_UNDEFINED_UNIT_ID};
  local_iterator             _lbegin;
  local_iterator             _lend;

};

}

#endif // DASH__GLOB_DYNAMIC_SEQUENTIAL_MEM_H_

