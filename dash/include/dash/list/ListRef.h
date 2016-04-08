#ifndef DASH__LIST__LIST_REF_H__INCLUDED
#define DASH__LIST__LIST_REF_H__INCLUDED

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

// forward declaration
template<
  typename ElementType,
  class    AllocatorType,
  class    PatternType >
class LocalListRef;

/**
 * Proxy type referencing a \c dash::List.
 *
 * \concept{DashContainerConcept}
 * \concept{DashListConcept}
 */
template<
  typename ElementType,
  class    AllocatorType,
  class    PatternType>
class ListRef
{
private:
  static const dim_t NumDimensions = 1;

/// Type definitions required for DASH list concept:
public:
  typedef typename PatternType::index_type                        index_type;
  typedef ElementType                                             value_type;
  typedef PatternType                                           pattern_type;
  typedef ListRef<ElementType, AllocatorType, PatternType>         view_type;
  typedef LocalListRef<value_type, AllocatorType, PatternType>    local_type;
  typedef AllocatorType                                       allocator_type;

/// Public types as required by STL list concept:
public:
  typedef typename std::make_unsigned<index_type>::type            size_type;
  typedef typename std::make_unsigned<index_type>::type      difference_type;

  typedef       GlobIter<value_type, PatternType>                   iterator;
  typedef const GlobIter<value_type, PatternType>             const_iterator;
  typedef       std::reverse_iterator<      iterator>       reverse_iterator;
  typedef       std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                          const_reference;

  typedef       GlobIter<value_type, PatternType>                    pointer;
  typedef const GlobIter<value_type, PatternType>              const_pointer;

private:
  typedef ListRef<ElementType, AllocatorType, PatternType>
    self_t;
  typedef List<ElementType, AllocatorType, PatternType>
    List_t;
  typedef ViewSpec<NumDimensions, index_type>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

public:
  ListRef(
    /// Pointer to list instance referenced by this view.
    List_t         * list,
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
    if (_list->_lcapacity > _list->_lsize) {
      // no reallocation required
    } else{
      // local capacity must be increased, reallocate
    }
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
    if (_list->_lcapacity > _list->_lsize) {
      // no reallocation required
    } else{
      // local capacity must be increased, reallocate
    }
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

  inline Team              & team();

  inline size_type           size()             const noexcept;
  inline size_type           local_size()       const noexcept;
  inline size_type           local_capacity()   const noexcept;
  inline size_type           extent(dim_t dim)  const noexcept;
  inline Extents_t           extents()          const noexcept;
  inline bool                empty()            const noexcept;

  inline void                barrier()          const;

  inline const_pointer       data()             const noexcept;
  inline iterator            begin()                  noexcept;
  inline const_iterator      begin()            const noexcept;
  inline iterator            end()                    noexcept;
  inline const_iterator      end()              const noexcept;
  /// Pointer to first element in local range.
  inline ElementType       * lbegin()           const noexcept;
  /// Pointer past final element in local range.
  inline ElementType       * lend()             const noexcept;

  /**
   * The pattern used to distribute list elements to units.
   */
  inline const PatternType & pattern() const
  {
    return _list->pattern();
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline PatternType & pattern()
  {
    return _list->pattern();
  }

private:
  /// Pointer to list instance referenced by this view.
  List_t    * _list;
  /// The view's offset and extent within the referenced list.
  ViewSpec_t  _viewspec;

}; // class ListRef

} // namespace dash

#endif // DASH__LIST__LIST_REF_H__INCLUDED
