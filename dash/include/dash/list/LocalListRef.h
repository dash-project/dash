#ifndef DASH__LIST__LOCAL_LIST_REF_H__INCLUDED
#define DASH__LIST__LOCAL_LIST_REF_H__INCLUDED

#include <iterator>
#include <limits>

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>
#include <dash/memory/GlobHeapMem.h>
#include <dash/Allocator.h>

#include <dash/list/internal/ListTypes.h>

#include <dash/iterator/GlobIter.h>

namespace dash {

// forward declaration
template<
  typename ElementType,
  class    AllocatorType >
class List;

/**
 * Proxy type representing a local view on a referenced \c dash::List.
 *
 * \concept{DashListConcept}
 */
template<
  typename T,
  class    LMemSpace >
class LocalListRef
{
  template <typename T_, typename LM_>
  friend class LocalListRef;

  static const dim_t NumDimensions = 1;

private:
  typedef LocalListRef<T, LMemSpace>
    self_t;
  typedef List<T, LMemSpace>
    list_type;
  typedef ViewSpec<NumDimensions, dash::default_index_t>
    ViewSpec_t;

  typedef typename list_type::node_type ListNode_t;

  typedef typename list_type::glob_mem_type
    glob_mem_type;

/// Type definitions required for dash::List concept:
public:
  typedef dash::default_index_t                                   index_type;

/// Type definitions required for std::list concept:
public:
  typedef T                                                       value_type;
  typedef typename std::make_unsigned<index_type>::type            size_type;
  typedef typename std::make_unsigned<index_type>::type      difference_type;

  typedef typename glob_mem_type::local_pointer                      pointer;
  typedef typename glob_mem_type::const_local_pointer          const_pointer;

  typedef typename list_type::local_reference                      reference;
  typedef typename list_type::const_local_reference          const_reference;

  typedef typename list_type::local_iterator                        iterator;
  typedef typename list_type::const_local_iterator            const_iterator;

  typedef std::reverse_iterator<      iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;

public:
  /**
   * Constructor, creates a local access proxy for the given list.
   */
  LocalListRef(
    list_type * list)
  : _list(list)
  { }

  LocalListRef(
    /// Pointer to list instance referenced by this view.
    list_type        * list,
    /// The view's offset and extent within the referenced list.
    const ViewSpec_t & viewspec)
  : _list(list),
    _viewspec(viewspec)
  { }

  /**
   * Pointer to initial local element in the list.
   */
  inline iterator begin() const noexcept
  {
    return _list->_lbegin;
  }

  /**
   * Pointer past final local element in the list.
   */
  inline iterator end() const noexcept
  {
    return _list->_lend;
  }

  inline iterator insert(
    /// Position in the list where the new element is inserted.
    const_iterator     position,
    /// Value to be copied in the inserted element.
    const value_type & value)
  {
    DASH_LOG_TRACE("LocalListRef.insert()");
    // New element node:
    ListNode_t node;
    node.lprev = nullptr;
    node.lnext = nullptr;
    node.gprev = _gprev;
    node.gnext = _gnext;
    iterator it_insert_end = _list->_end;
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.pop_back is not implemented");

    DASH_LOG_TRACE("LocalListRef.insert >");
    return it_insert_end;
  }

  /**
   * Inserts a new element at the end of the list, after its current
   * last element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_back(const value_type & value)
  {
    DASH_LOG_TRACE("LocalListRef.push_back()");
    // Pointer to new node element:
    ListNode_t * node_lptr = nullptr;
    // New element node:
    ListNode_t   node;
    node.value = value;
    node.lprev = nullptr;
    node.lnext = nullptr;
    node.gprev = _gprev;
    node.gnext = _gnext;
    // Local capacity before operation:
    auto l_cap_old  = _list->_globmem->local_size();
    // Number of local elements before operation:
    auto l_size_old = _list->_local_sizes.local[0];
    // Update local size:
    _list->_local_sizes.local[0]++;
    // Number of local elements after operation:
    auto l_size_new = _list->_local_sizes.local[0];

    DASH_LOG_TRACE_VAR("LocalListRef.push_back", l_cap_old);
    DASH_LOG_TRACE_VAR("LocalListRef.push_back", l_size_old);
    if (l_size_new > l_cap_old) {
      DASH_LOG_TRACE("LocalListRef.push_back",
                     "globmem.grow(", _list->_local_buffer_size, ")");
      // Acquire local memory for new node:
      node_lptr = static_cast<ListNode_t *>(
                    _list->_globmem->grow(_list->_local_buffer_size));
      DASH_ASSERT_GT(_list->_globmem->local_size(), l_cap_old,
                     "local capacity not increased after globmem.grow()");
    } else {
      // No allocation required (cast from LocalBucketIter<T> to T *):
      node_lptr = static_cast<ListNode_t *>(
                    _list->_globmem->lbegin() + l_size_old);
    }
    // Local capacity before operation:
    auto l_cap_new = _list->_globmem->local_size();
    DASH_LOG_TRACE("LocalListRef.push_back",
                   "node target address:", node_lptr);
    if (l_size_old > 0) {
      // Set node predecessor (cast from LocalBucketIter<T> to T *):
      node.lprev = static_cast<ListNode_t *>(
                     _list->_globmem->lbegin() + (l_size_old - 1));
      // Set successor of node predecessor to new node:
      DASH_ASSERT(node.lprev->lnext == nullptr);
      node.lprev->lnext = node_lptr;
      DASH_LOG_TRACE_VAR("LocalListRef.push_back", node.lprev->lnext);
    }
    DASH_LOG_TRACE_VAR("LocalListRef.push_back", node.lprev);
    DASH_LOG_TRACE_VAR("LocalListRef.push_back", node.value);
    DASH_LOG_TRACE_VAR("LocalListRef.push_back", node.lnext);
    // Copy new node to target address:
    *node_lptr = node;
    DASH_LOG_TRACE_VAR("LocalListRef.push_back", l_cap_new);
    DASH_LOG_TRACE_VAR("LocalListRef.push_back", l_size_new);
    DASH_LOG_TRACE("LocalListRef.push_back >");
  }

  /**
   * Removes and destroys the last element in the list, reducing the
   * container size by one.
   */
  void pop_back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.pop_back is not implemented");
  }

  /**
   * Accesses the last element in the list.
   */
  reference back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef._back is not implemented");
  }

  /**
   * Inserts a new element at the beginning of the list, before its current
   * first element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_front(const value_type & value)
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListLocalRef.push_front is not implemented");
  }

  /**
   * Removes and destroys the first element in the list, reducing the
   * container size by one.
   */
  void pop_front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.pop_front is not implemented");
  }

  /**
   * Accesses the first element in the list.
   */
  reference front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.front is not implemented");
  }

  /**
   * Number of list elements in local memory.
   */
  inline size_type size() const noexcept
  {
    return _list->lsize();
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True
   */
  constexpr bool is_local(
    /// A global list index
    index_type global_index) const
  {
    return true;
  }

private:
  /// Pointer to list instance referenced by this view.
  list_type * const _list;
  /// The view's offset and extent within the referenced list.
  ViewSpec_t        _viewspec;

  dart_gptr_t       _gprev{};
  dart_gptr_t       _gnext{};
};

} // namespace dash

#endif // DASH__LIST__LOCAL_LIST_REF_H__INCLUDED
