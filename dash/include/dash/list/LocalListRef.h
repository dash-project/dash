#ifndef DASH__LIST__LOCAL_LIST_REF_H__INCLUDED
#define DASH__LIST__LOCAL_LIST_REF_H__INCLUDED

#include <iterator>
#include <limits>

#include <dash/Types.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>
#include <dash/DynamicPattern.h>
#include <dash/GlobDynamicMem.h>
#include <dash/Allocator.h>

#include <dash/internal/list/ListTypes.h>


namespace dash {

// forward declaration
template<
  typename ElementType,
  class    AllocatorType,
  class    PatternType >
class List;

/**
 * Proxy type representing a local view on a referenced \c dash::List.
 *
 * \concept{DashContainerConcept}
 * \concept{DashListConcept}
 */
template<
  typename T,
  class    AllocatorType,
  class    PatternType >
class LocalListRef
{
  template <typename T_, typename I_, typename P_>
    friend class LocalListRef;

private:
  static const dim_t NumDimensions = 1;

/// Type definitions required for dash::List concept:
public:
  typedef PatternType                                           pattern_type;
  typedef typename PatternType::index_type                        index_type;
  typedef LocalListRef<T, index_type, PatternType>                 view_type;
  typedef AllocatorType                                       allocator_type;
  typedef dash::GlobDynamicMem<T, AllocatorType>               glob_mem_type;

/// Type definitions required for std::list concept:
public:
  typedef T                                                       value_type;
  typedef typename std::make_unsigned<index_type>::type            size_type;
  typedef typename std::make_unsigned<index_type>::type      difference_type;

  typedef typename glob_mem_type::local_pointer                      pointer;
  typedef typename glob_mem_type::const_local_pointer          const_pointer;

  typedef typename glob_mem_type::local_reference                  reference;
  typedef typename glob_mem_type::const_local_reference      const_reference;

  typedef typename glob_mem_type::local_iterator                    iterator;
  typedef typename glob_mem_type::const_local_iterator        const_iterator;

  typedef std::reverse_iterator<      iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;

private:
  typedef LocalListRef<T, AllocatorType, PatternType>
    self_t;
  typedef List<T, AllocatorType, PatternType>
    List_t;
  typedef ViewSpec<NumDimensions, index_type>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;
  typedef internal::ListNode<value_type>
    ListNode_t;
  typedef typename allocator_type::template rebind<ListNode_t>::other
    node_allocator_type;

public:
  /**
   * Constructor, creates a local access proxy for the given list.
   */
  LocalListRef(
    List_t * list)
  : _list(list)
  { }

  LocalListRef(
    /// Pointer to list instance referenced by this view.
    List<T, index_type, PatternType> * list,
    /// The view's offset and extent within the referenced list.
    const ViewSpec_t                 & viewspec)
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
    // New element node:
    ListNode_t node;
    node.lprev = nullptr;
    node.lnext = nullptr;
    node.gprev = _gprev;
    node.gnext = _gnext;

    iterator it_insert_end;
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
    // New element node:
    ListNode_t node;
    node.lprev = nullptr;
    node.lnext = nullptr;
    node.gprev = _gprev;
    node.gnext = _gnext;
    if (_list->_globmem->local_size() > 0) {
      // Node predecessor:
      node.lprev = _list->_globmem->lend() - 1;
    }
    // Increase local size in pattern:
    _list->pattern().local_resize(_list->_lsize + 1);
    // Acquire local memory for new element:
    _list->_globmem->grow(1);
    // Pointer to allocated node:
    ListNode_t * node_lptr = _list->_globmem->lend() - 1;
    *node_lptr             = node;
    _list->_lsize++;
    _list->_size++;
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
    return end() - begin();
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

  /**
   * View at block at given global block offset.
   */
  self_t block(index_type block_lindex)
  {
    DASH_LOG_TRACE("LocalListRef.block()", block_lindex);
    ViewSpec<1> block_view = pattern().local_block(block_lindex);
    DASH_LOG_TRACE("LocalListRef.block >", block_view);
    return self_t(_list, block_view);
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline const pattern_type & pattern() const
  {
    return _list->pattern();
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline pattern_type & pattern()
  {
    return _list->pattern();
  }

private:
  /// Pointer to list instance referenced by this view.
  List_t * const _list;
  /// The view's offset and extent within the referenced list.
  ViewSpec_t     _viewspec;

  dart_gptr_t    _gprev;
  dart_gptr_t    _gnext;
};

} // namespace dash

#endif // DASH__LIST__LOCAL_LIST_REF_H__INCLUDED
