#ifndef DASH__MEMORY__GLOB_HEAP_COMBINED_MEM_H
#define DASH__MEMORY__GLOB_HEAP_COMBINED_MEM_H

#include <dash/Team.h>
#include <dash/memory/GlobHeapCombinedPtr.h>

namespace dash {

template<typename GlobMemType>
class GlobHeapCombinedMem {

public:

  typedef GlobHeapCombinedMem<GlobMemType>             self_t;
  typedef GlobMemType                                  glob_mem_type;
  typedef typename glob_mem_type::index_type           index_type;
  typedef typename glob_mem_type::size_type            size_type;
  typedef typename glob_mem_type::value_type           value_type;
  typedef std::vector<std::vector<size_type>>          bucket_cumul_sizes_map;
  typedef std::list<glob_mem_type *>                   glob_mem_list;
  typedef GlobPtr<value_type, self_t>                  global_iterator;
  typedef typename glob_mem_type::local_iterator       local_iterator;
  typedef typename local_iterator::bucket_type         bucket_type;
  typedef typename std::vector<bucket_type>            bucket_list;
  typedef local_iterator                               local_pointer;
  typedef local_iterator                               const_local_pointer;

  template<typename T_, class GMem_>
  friend class GlobPtr;

public:

  /**
   * Constructor.
   */
  GlobHeapCombinedMem(Team & team = dash::Team::All())
    : _bucket_cumul_sizes(team.size()),
      _team(&team),
      _buckets()
  { }

  /**
   * Default constructor. Explicitly deleted.
   */
  GlobHeapCombinedMem() = delete;

  /**
   * Copy constructor.
   */
  GlobHeapCombinedMem(const self_t & other) = default;

  /**
   * Move constructor.
   */
  GlobHeapCombinedMem(self_t && other) = default;

  /**
   * Copy-assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other) = default;

  /**
   * Adds a GlobMem object to the unifier.
   */
  void add_globmem(glob_mem_type & glob_mem) {
    // only GlobMem objects with the same Team can be used
    if(*_team == glob_mem.team()) {
      _glob_mem_list.push_back(&glob_mem);
      //update_bucket_sizes();
    }
  }

  /**
   * Updates memory spaces of underlying GlobMem objects.
   * 
   * Non-collective operation.
   */
  void commit() {
    update_bucket_sizes();
    // TODO: The bucket list should normally update on every element insertion,
    //       to always be consistent with the current local memory space
    update_bucket_list();
    update_size();
    
    _begin = global_iterator(this, 0);
    _end = global_iterator(this, _size);
  }

  dart_gptr_t dart_gptr_at(team_unit_t unit, 
      index_type bucket_index, index_type bucket_phase) const {
    auto gmem_index = bucket_index % _glob_mem_list.size();
    auto gmem_it = _glob_mem_list.begin();
    std::advance(gmem_it, gmem_index);
    auto gmem = *gmem_it;
    // translate combined bucket index to index for particular GlobMem object
    int gmem_bucket_index = bucket_index / _glob_mem_list.size();
    return gmem->dart_gptr_at(unit, gmem_bucket_index, bucket_phase);
  }

  /**
   * Iterator to the beginning of the memory space.
   */
  global_iterator begin() const {
    return _begin;
  }

  /**
   * Iterator past the end of the memory space.
   */
  global_iterator end() const {
    return _end;
  }

  /**
   * Iterator to the beginning of the mamory space's local portion.
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
   * Returns the amount of elements currently available in global memory space.
   */
  size_type size() const {
    return _size;
  }

  /**
   * Returns the team containing all units associated with this memory space.
   */
  Team & team() const {
    return (_team != nullptr) ? *_team : dash::Team::Null();
  }

  /**
   * Returns the size of a the memory space belonging to a certain bucket.
   */
  size_type container_size(team_unit_t unit, size_type index) const {
    size_type bucket_size = _bucket_cumul_sizes[unit][index + 
      _glob_mem_list.size() - 1];
    if(index > 0) {
      bucket_size -= _bucket_cumul_sizes[unit][index - 1];
    }
    return bucket_size;
  }

  size_type lsize() {
    return _local_size;
  }

  size_type size(team_unit_t unit) {
    return _bucket_cumul_sizes[unit][1];
  }

  size_type size() {
    return _size;
  }

private:

  /**
   * Combines bucket sizes of all currently added GlobMem objects.
   * Resulting order for gmem_0 & gmem_1:
   * [unit_0] : [gmem_0 b_0][gmem_1 b_0] ... [gmem_0 b_n][gmem_1 b_n]
   *    :             :           :                :           :
   * [unit_n] : [gmem_0 b_0][gmem_1 b_0] ... [gmem_0 b_n][gmem_1 b_n]
   */
  void update_bucket_sizes() {
    // the index of a bucket directly translates to its corresponding GlobMem
    // object:
    //   example for 2 GlobMem objects:
    //   bucket 0 -> GlobMem 0
    //   bucket 1 -> GlobMem 1
    //   bucket 2 -> GlobMem 0
    //   ...
    if(_glob_mem_list.size() > 0) {
      for(int i = 0; i < _bucket_cumul_sizes.size(); ++i) {
        int max = 0;
        for(auto gmem : _glob_mem_list) {
          if(gmem->_bucket_cumul_sizes[i].size() > max) {
            max = gmem->_bucket_cumul_sizes[i].size();
          }
        }
        _bucket_cumul_sizes[i].clear();
        _bucket_cumul_sizes[i].resize(max * _glob_mem_list.size());
      }
      int offset = 0;
      for(auto gmem : _glob_mem_list) {
        auto & gmem_buckets = gmem->_bucket_cumul_sizes;
        int pos = offset;
        for(int i = 0; i < gmem_buckets.size(); ++i) {
          size_type last = 0;
          for(int j = 0; j + offset < _bucket_cumul_sizes[i].size(); ++j) {
            int gmem_index = j / _glob_mem_list.size();
            // if number of buckets shorter than the maximum, repeat the last
            // bucket size for full accumulation
            if(gmem_index < gmem_buckets[i].size()) {
              last = gmem_buckets[i][gmem_index];
            }
            _bucket_cumul_sizes[i][j + offset] += last;
          }
        }
        ++offset;
      }
    }
  }

  /**
   * Updates the bucket list.
   */
  void update_bucket_list() {
    _buckets.clear();
    std::vector<std::pair<typename bucket_list::iterator, 
      typename bucket_list::iterator>> iters;
    int max = 0;
    if(_glob_mem_list.size() > 0) {
      for(auto gmem : _glob_mem_list) {
        iters.push_back(std::make_pair(gmem->_buckets.begin(), 
              gmem->_buckets.end()));
        if(max < gmem->_buckets.size()) {
          max = gmem->_buckets.size();
        }
      }
      // TODO: Currently onlys working for GlobMemContiguousMem because of
      //       the double buckets
      for(int i = 0; i < max; i += 2) {
        for(auto & it : iters) {
          if(it.first != it.second) {
            _buckets.push_back(*(it.first));
            ++(it.first);
          }
          if(it.first != it.second) {
            _buckets.push_back(*(it.first));
            ++(it.first);
          }
        }
      }
    }
    update_local_size();
    update_lbegin();
    update_lend();
  }

private:

  /**
   * Updates the global size.
   */
  void update_size() {
    _size = 0;
    for(auto gmem : _glob_mem_list) {
      _size += gmem->_size;
    }
  }

  /**
   * Updates the local size.
   */
  void update_local_size() {
    _local_size = 0;
    for(auto gmem : _glob_mem_list) {
      _local_size += gmem->_local_size;
    }
  }

  // NOTE: copied from GlobHeapMem.h
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

  // NOTE: copied from GlobHeapMem.h
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

private:

  bucket_list              _buckets;
  bucket_cumul_sizes_map   _bucket_cumul_sizes;
  glob_mem_list            _glob_mem_list;
  Team *                   _team = nullptr;
  size_type                _size = 0;
  global_iterator          _begin;
  global_iterator          _end;
  local_iterator           _lbegin;
  local_iterator           _lend;
  size_type                _local_size = 0;

};

}

#endif //DASH__MEMORY__GLOB_HEAP_COMBINED_MEM_H

