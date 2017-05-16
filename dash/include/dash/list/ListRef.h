#ifndef DASH__LIST__LIST_REF_H__INCLUDED
#define DASH__LIST__LIST_REF_H__INCLUDED

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

#include <dash/iterator/GlobIter.h>

#include <dash/list/internal/ListTypes.h>


namespace dash {

// forward declaration
template<
  typename ElementType,
  class    AllocatorType >
class List;

// forward declaration
template<
  typename ElementType,
  class    AllocatorType >
class LocalListRef;

/**
 * Proxy type referencing a \c dash::List.
 *
 * \concept{DashListConcept}
 */
template<
  typename ElementType,
  class    AllocatorType >
class ListRef
{
private:
  static const dim_t NumDimensions = 1;

/// Type definitions required for DASH list concept:
public:
  typedef dash::default_index_t                                   index_type;
  typedef ElementType                                             value_type;
  typedef ListRef<ElementType, AllocatorType>                      view_type;
  typedef LocalListRef<value_type, AllocatorType>                 local_type;
  typedef AllocatorType                                       allocator_type;

private:
  typedef ListRef<ElementType, AllocatorType>
    self_t;
  typedef List<ElementType, AllocatorType>
    list_type;
  typedef ViewSpec<NumDimensions, index_type>
    ViewSpec_t;
  typedef internal::ListNode<ElementType>
    ListNode_t;
  typedef typename allocator_type::template rebind<ListNode_t>::other
    node_allocator_type;
  typedef dash::GlobHeapMem<ElementType, node_allocator_type>
    glob_mem_type;

/// Public types as required by STL list concept:
public:
  typedef typename std::make_unsigned<index_type>::type            size_type;

  typedef typename list_type::iterator                              iterator;
  typedef typename list_type::const_iterator                  const_iterator;
  typedef typename list_type::reverse_iterator              reverse_iterator;
  typedef typename list_type::const_reverse_iterator  const_reverse_iterator;
  typedef typename list_type::reference                            reference;
  typedef typename list_type::const_reference                const_reference;
  typedef typename list_type::pointer                                pointer;
  typedef typename list_type::const_pointer                    const_pointer;
  typedef typename glob_mem_type::local_pointer                local_pointer;
  typedef typename glob_mem_type::const_local_pointer    const_local_pointer;

public:
  ListRef(
    /// Pointer to list instance referenced by this view.
    list_type        * list,
    /// The view's offset and extent within the referenced list.
    const ViewSpec_t & viewspec)
  : _list(list),
    _viewspec(viewspec)
  { }

public:
  /**
   * Inserts a new element at the end of the list, after its current
   * last element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_back(const value_type & value)
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.push_back is not implemented");
  }

  /**
   * Removes and destroys the last element in the list, reducing the
   * container size by one.
   */
  void pop_back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.pop_back is not implemented");
  }

  /**
   * Accesses the last element in the list.
   */
  reference back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.back is not implemented");
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
               "dash::ListRef.push_front is not implemented");
  }

  /**
   * Removes and destroys the first element in the list, reducing the
   * container size by one.
   */
  void pop_front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.pop_front is not implemented");
  }

  /**
   * Accesses the first element in the list.
   */
  reference front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.front is not implemented");
  }

  inline Team              & team()             const noexcept;

  inline size_type           size()             const noexcept;
  inline size_type           local_size()       const noexcept;
  inline size_type           local_capacity()   const noexcept;
  inline bool                empty()            const noexcept;

  inline void                barrier()          const;

  inline const_pointer       data()             const noexcept;
  inline iterator            begin()                  noexcept;
  inline const_iterator      begin()            const noexcept;
  inline iterator            end()                    noexcept;
  inline const_iterator      end()              const noexcept;
  /// Pointer to first element in local range.
  inline local_pointer       lbegin()           const noexcept;
  /// Pointer past final element in local range.
  inline local_pointer       lend()             const noexcept;

private:
  /// Pointer to list instance referenced by this view.
  list_type  * _list;
  /// The view's offset and extent within the referenced list.
  ViewSpec_t   _viewspec;

}; // class ListRef

} // namespace dash

#endif // DASH__LIST__LIST_REF_H__INCLUDED
