#ifndef DASH__MAP__UNORDERED_MAP_LOCAL_REF_H__INCLUDED
#define DASH__MAP__UNORDERED_MAP_LOCAL_REF_H__INCLUDED

namespace dash {

#ifdef DOXYGEN


/**
 * Local view specifier of a dynamic map container with support for
 * workload balancing.
 *
 * \concept{DashUnorderedMapConcept}
 *
 * \ingroup{dash::UnorderedMap}
 */
template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
class UnorderedMapLocalRef;

#else // ifdef DOXYGEN

// Forward-declaration.
template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
class UnorderedMap;

template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
class UnorderedMapLocalRef
{
private:
  typedef UnorderedMapLocalRef<Key, Mapped, Hash, Pred, Alloc>
    self_t;

  typedef UnorderedMap<Key, Mapped, Hash, Pred, Alloc>
    map_type;

public:
  typedef Key                                                       key_type;
  typedef Mapped                                                 mapped_type;
  typedef Hash                                                        hasher;
  typedef Pred                                                     key_equal;
  typedef Alloc                                               allocator_type;

  typedef typename map_type::index_type                           index_type;
  typedef typename map_type::difference_type                 difference_type;
  typedef typename map_type::size_type                             size_type;
  typedef typename map_type::value_type                           value_type;

  typedef self_t                                                  local_type;

  typedef typename map_type::glob_mem_type                     glob_mem_type;

  typedef typename map_type::local_node_pointer                 node_pointer;
  typedef typename map_type::local_node_pointer           local_node_pointer;

  typedef typename map_type::local_iterator                         iterator;
  typedef typename map_type::const_local_iterator             const_iterator;
  typedef typename map_type::reverse_local_iterator
    reverse_iterator;
  typedef typename map_type::const_reverse_local_iterator
    const_reverse_iterator;

  typedef typename map_type::local_iterator                   local_iterator;
  typedef typename map_type::const_local_iterator       const_local_iterator;
  typedef typename map_type::reverse_local_iterator
    reverse_local_iterator;
  typedef typename map_type::const_reverse_local_iterator
    const_reverse_local_iterator;

  typedef typename map_type::local_reference                       reference;
  typedef typename map_type::const_local_reference           const_reference;
  typedef typename map_type::local_reference                 local_reference;
  typedef typename map_type::const_local_reference     const_local_reference;

  typedef typename map_type::mapped_type_reference
    mapped_type_reference;
  typedef typename map_type::const_mapped_type_reference
    const_mapped_type_reference;

private:
  map_type * _map  = nullptr;

public:
  UnorderedMapLocalRef(
    // Pointer to instance of \c dash::UnorderedMap referenced by the view
    // specifier.
    map_type * map)
  : _map(map)
  { }

  UnorderedMapLocalRef() = default;

  //////////////////////////////////////////////////////////////////////////
  // Distributed container
  //////////////////////////////////////////////////////////////////////////

  inline dash::Team & team() const noexcept
  {
    return _map->team();
  }

  inline const glob_mem_type & globmem() const
  {
    return _map->globmem();
  }

  //////////////////////////////////////////////////////////////////////////
  // Iterators
  //////////////////////////////////////////////////////////////////////////

  inline iterator & begin() noexcept
  {
    return _map->lbegin();
  }

  inline const_iterator & begin() const noexcept
  {
    return _map->lbegin();
  }

  inline const_iterator & cbegin() const noexcept
  {
    return _map->clbegin();
  }

  inline iterator & end() noexcept
  {
    return _map->lend();
  }

  inline const_iterator & end() const noexcept
  {
    return _map->lend();
  }

  inline const_iterator & cend() const noexcept
  {
    return _map->clend();
  }

  //////////////////////////////////////////////////////////////////////////
  // Capacity
  //////////////////////////////////////////////////////////////////////////

  constexpr size_type max_size() const noexcept
  {
    return _map->max_size();
  }

  inline size_type size() const noexcept
  {
    return _map->lsize();
  }

  inline size_type capacity() const noexcept
  {
    return _map->lcapacity();
  }

  inline bool empty() const noexcept
  {
    return _map->lsize() == 0;
  }

  inline size_type lsize() const noexcept
  {
    return _map->lsize();
  }

  inline size_type lcapacity() const noexcept
  {
    return _map->lcapacity();
  }

  //////////////////////////////////////////////////////////////////////////
  // Element Access
  //////////////////////////////////////////////////////////////////////////

  mapped_type_reference operator[](const key_type & key)
  {
    DASH_LOG_TRACE("UnorderedMapLocalRef.[]()", "key:", key);
    iterator      git_value   = insert(
                                   std::make_pair(key, mapped_type()))
                                .first;
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.[]", git_value);
    dart_gptr_t   gptr_mapped = git_value.dart_gptr();
    value_type  * lptr_value  = git_value.local();
    mapped_type * lptr_mapped = nullptr;

    _lptr_value_to_mapped(lptr_value, gptr_mapped, lptr_mapped);
    // Create global reference to mapped value member in element:
    mapped_type_reference mapped(gptr_mapped,
                                 lptr_mapped);
    DASH_LOG_TRACE("UnorderedMapLocalRef.[] >", mapped);
    return mapped;
  }

  const_mapped_type_reference at(const key_type & key) const
  {
    DASH_LOG_TRACE("UnorderedMapLocalRef.at() const", "key:", key);
    // TODO: Unoptimized, currently calls find(key) twice as operator[](key)
    //       calls insert(key).
    const_iterator git_value = find(key);
    if (git_value == end()) {
      // No equivalent key in map, throw:
      DASH_THROW(
        dash::exception::InvalidArgument,
        "No element in map for key " << key);
    }
    dart_gptr_t   gptr_mapped = git_value.dart_gptr();
    value_type  * lptr_value  = git_value.local();
    mapped_type * lptr_mapped = nullptr;

    _lptr_value_to_mapped(lptr_value, gptr_mapped, lptr_mapped);
    // Create global reference to mapped value member in element:
    const_mapped_type_reference mapped(gptr_mapped,
                                       lptr_mapped);
    DASH_LOG_TRACE("UnorderedMapLocalRef.at >", mapped);
    return mapped;
  }

  mapped_type_reference at(const key_type & key)
  {
    DASH_LOG_TRACE("UnorderedMapLocalRef.at()", "key:", key);
    // TODO: Unoptimized, currently calls find(key) twice as operator[](key)
    //       calls insert(key).
    const_iterator git_value = find(key);
    if (git_value == end()) {
      // No equivalent key in map, throw:
      DASH_THROW(
        dash::exception::InvalidArgument,
        "No element in map for key " << key);
    }
    auto mapped = this->operator[](key);
    DASH_LOG_TRACE("UnorderedMapLocalRef.at >", mapped);
    return mapped;
  }

  //////////////////////////////////////////////////////////////////////////
  // Element Lookup
  //////////////////////////////////////////////////////////////////////////

  size_type count(const key_type & key) const
  {
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.count()", key);
    size_type      nelem = 0;
    const_iterator found = find(key);
    if (found != end()) {
      nelem = 1;
    }
    DASH_LOG_TRACE("UnorderedMapLocalRef.count >", nelem);
    return nelem;
  }

  iterator find(const key_type & key)
  {
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.find()", key);
    auto   & first = begin();
    auto   & last  = end();
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.find()", first);
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.find()", last);
    auto     pred  = key_eq();
    iterator found = std::find_if(
                       first, last,
                       [&](const value_type & v) {
                         DASH_LOG_TRACE("UnorderedMapLocalRef.find.eq",
                                        v.first, "==?", key);
                         return pred(v.first, key);
                       });
    DASH_LOG_TRACE("UnorderedMapLocalRef.find >", found);
    return found;
  }

  const_iterator find(const key_type & key) const
  {
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.find() const", key);
    auto   & first = begin();
    auto   & last  = end();
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.find()", first);
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.find()", last);
    auto     pred  = key_eq();
    const_iterator found = std::find_if(
                             first, last,
                             [&](const value_type & v) {
                               DASH_LOG_TRACE("UnorderedMapLocalRef.find.eq",
                                              v.first, "==?", key);
                               return pred(v.first, key);
                             });
    DASH_LOG_TRACE("UnorderedMapLocalRef.find const >", found);
    return found;
  }

  //////////////////////////////////////////////////////////////////////////
  // Modifiers
  //////////////////////////////////////////////////////////////////////////

  std::pair<iterator, bool> insert(
    /// The element to insert.
    const value_type & value)
  {
    auto && key = value.first;
    DASH_LOG_DEBUG("UnorderedMapLocalRef.insert()", "key:", key);
    auto result = std::make_pair(_map->_lend, false);

    // Look up existing element at given key:
    DASH_LOG_TRACE("UnorderedMapLocalRef.insert", "element key lookup");
    const_iterator found = find(key);
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.insert", found);

    if (found != end()) {
      DASH_LOG_TRACE("UnorderedMapLocalRef.insert", "key found");
      // Existing element found, no insertion:
      result.first  = iterator(_map, found.pos());
      result.second = false;
    } else {
      DASH_LOG_TRACE("UnorderedMapLocalRef.insert", "key not found");
      // Unit mapped to the new element's key by the hash function:
      auto unit = hash_function()(key);
      // Do not store local unit id in member as _map->_myid is initialized
      // after this instance (_map->local).
      auto myid = _map->_myid;
      DASH_LOG_TRACE("UnorderedMapLocalRef.insert", "target unit:", unit);
      DASH_LOG_TRACE("UnorderedMapLocalRef.insert", "local unit:",  myid);
      if (unit != myid) {
        DASH_THROW(
          dash::exception::RuntimeError,
          "attempted local insert of " <<
          "key "     << key  << " which is mapped " <<
          "to unit " << unit << " by hash function");
      }
      auto inserted = _map->_insert_at(unit, value);
      result.first  = inserted.first.local();
      result.second = inserted.second;
      // Updated local end iterator of the referenced map:
      _map->_lend   = _map->_lbegin + _map->lsize();
      DASH_LOG_TRACE("UnorderedMapLocalRef.insert", "updated map.lend:",
                     _map->_lend);
    }
    DASH_LOG_DEBUG("UnorderedMapLocalRef.insert >",
                   (result.second ? "inserted" : "existing"), ":",
                   result.first);
    return result;
  }

  template<class InputIterator>
  void insert(
    // Iterator at first value in the range to insert.
    InputIterator first,
    // Iterator past the last value in the range to insert.
    InputIterator last)
  {
    // TODO: Calling insert() on every single element in the range could cause
    //       multiple calls of globmem.grow(_local_buffer_size).
    //       Could be optimized to allocate additional memory in a single call
    //       of globmem.grow(std::distance(first,last)).
    for (auto it = first; it != last; ++it) {
      insert(*it);
    }
  }

  iterator erase(
    const_iterator it)
  {
    DASH_LOG_DEBUG("UnorderedMapLocalRef.erase()", "iterator:", it);
    erase(*it.first);
    DASH_LOG_DEBUG("UnorderedMapLocalRef.erase >");
  }

  size_type erase(
    /// Key of the container element to remove.
    const key_type & key)
  {
    DASH_LOG_DEBUG("UnorderedMapLocalRef.erase()", "key:", key);
    auto b_idx  = bucket(key);
    auto b_size = bucket_size(b_idx);
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.erase", b_idx);
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.erase", b_size);

    DASH_THROW(
      dash::exception::NotImplemented,
      "dash::UnorderedMapLocalRef.erase is not implemented.");

    DASH_LOG_DEBUG("UnorderedMapLocalRef.erase >");
  }

  iterator erase(
    /// Iterator at first element to remove.
    const_iterator first,
    /// Iterator past the last element to remove.
    const_iterator last)
  {
    DASH_LOG_DEBUG("UnorderedMapLocalRef.erase(first,last)");
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.erase()", first);
    DASH_LOG_TRACE_VAR("UnorderedMapLocalRef.erase()", last);
    for (auto it = first; it != last; ++it) {
      erase(*it.first);
    }
    DASH_LOG_DEBUG("UnorderedMapLocalRef.erase(first,last) >");
  }

  //////////////////////////////////////////////////////////////////////////
  // Bucket Interface
  //////////////////////////////////////////////////////////////////////////

  inline size_type bucket(const key_type & key) const
  {
    return _map->bucket(key);
  }

  inline size_type bucket_size(size_type bucket_index) const
  {
    return _map->bucket_size(bucket_index);
  }

  //////////////////////////////////////////////////////////////////////////
  // Observers
  //////////////////////////////////////////////////////////////////////////

  inline key_equal key_eq() const
  {
    return _map->key_eq();
  }

  inline hasher hash_function() const
  {
    return _map->hash_function();
  }

private:
  /**
   * Helper to resolve address of mapped value from map entries.
   *
   * std::pair cannot be used as MPI data type directly.
   * Offset-to-member only works reliably with offsetof in the general case
   * We have to use `offsetof` as there is no instance of value_type
   * available that could be used to calculate the member offset as
   * `l_ptr_value` is possibly undefined.
   *
   * Using `std::declval()` instead (to generate a compile-time
   * pseudo-instance for member resolution) only works if Key and Mapped
   * are default-constructible.
   * 
   * Finally, the distance obtained from
   *
   *   &(lptr_value->second) - lptr_value
   *
   * had different alignment than the address obtained via offsetof in some
   * cases, depending on the combination of MPI runtime and compiler.
   * Apparently some compilers / standard libs have special treatment
   * (padding?) for members of std::pair such that
   *
   *   __builtin_offsetof(type, member)
   *
   * differs from the member-offset provided by the type system.
   * The alternative, using `offsetof` (resolves to `__builtin_offsetof`
   * automatically if needed) and manual pointer increments works, however.
   */
  void _lptr_value_to_mapped(
    // [IN]    native pointer to map entry
    value_type    * lptr_value,
    // [INOUT] corresponding global pointer to mapped value
    dart_gptr_t   & gptr_mapped,
    // [OUT]   corresponding native pointer to mapped value
    mapped_type * & lptr_mapped) const
  {
    // Byte offset of mapped value in element type:
    auto mapped_offs = offsetof(value_type, second);
    DASH_LOG_TRACE("UnorderedMap.lptr_value_to_mapped()",
                   "byte offset of mapped member:", mapped_offs);
    // Increment pointers to element by byte offset of mapped value member:
    if (lptr_value != nullptr) {
        if (std::is_standard_layout<value_type>::value) {
        // Convert to char pointer for byte-wise increment:
        char * b_lptr_mapped = reinterpret_cast<char *>(lptr_value);
        b_lptr_mapped       += mapped_offs;
        // Convert to mapped type pointer:
        lptr_mapped          = reinterpret_cast<mapped_type *>(b_lptr_mapped);
      } else {
        lptr_mapped = &(lptr_value->second);
      }
    }
    if (!DART_GPTR_ISNULL(gptr_mapped)) {
      DASH_ASSERT_RETURNS(
        dart_gptr_incaddr(&gptr_mapped, mapped_offs),
        DART_OK);
    }
    DASH_LOG_TRACE("UnorderedMap.lptr_value_to_mapped >",
                   "gptr to mapped:", gptr_mapped);
    DASH_LOG_TRACE("UnorderedMap.lptr_value_to_mapped >",
                   "lptr to mapped:", lptr_mapped);
  }

}; // class UnorderedMapLocalRef

#endif // ifdef DOXYGEN

} // namespace dash

#endif // DASH__MAP__UNORDERED_MAP_LOCAL_REF_H__INCLUDED
