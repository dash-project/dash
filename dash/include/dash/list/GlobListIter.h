#ifndef DASH__LIST__GLOB_LIST_ITER_H__INCLUDED
#define DASH__LIST__GLOB_LIST_ITER_H__INCLUDED

#include <dash/GlobPtr.h>
#include <dash/GlobRef.h>

#include <dash/list/internal/ListTypes.h>

#include <iterator>


namespace dash {

/**
 * Bi-directional global iterator on elements of a \c dash::List instance.
 *
 * \concept{DashListConcept}
 * \concept{DashGlobalIteratorConcept}
 */
template<
  typename ElementType,
  class    GlobMemType,
  class    PointerType   = GlobPtr<ElementType>,
  class    ReferenceType = GlobRef<ElementType> >
class GlobListIter
: public std::iterator<
           std::bidirectional_iterator_tag,
           ElementType,
           dash::default_index_t,
           PointerType,
           ReferenceType >
{
private:
  typedef GlobListIter<
            ElementType,
            GlobMemType,
            PointerType,
            ReferenceType>
    self_t;

public:
  typedef ElementType                             value_type;
  typedef       ReferenceType                      reference;
  typedef const ReferenceType                const_reference;
  typedef       PointerType                          pointer;
  typedef const PointerType                    const_pointer;

  typedef typename GlobMemType::local_pointer  local_pointer;

  typedef internal::ListNode<value_type>           node_type;

public:
  typedef std::integral_constant<bool, false>       has_view;

public:
  /**
   * Default constructor.
   */
  GlobListIter() = default;

  /**
   * Constructor, creates a global iterator on a \c dash::List instance.
   */
  GlobListIter(
    GlobMemType  * gmem,
    node_type    & node)
  : _globmem(gmem),
    _node(&node),
    _myid(dash::Team::GlobalUnitID())
  {
    DASH_LOG_TRACE("GlobListIter(gmem,node,pat)");
  }

  /**
   * Copy constructor.
   */
  GlobListIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Type conversion operator to \c pointer type.
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator pointer() const
  {
    pointer ptr;
    // TODO
    return ptr;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*()
  {
    return reference(this);
  }

  /**
   * Dereference operator.
   *
   * \return  A global const reference to the element at the iterator's
   *          position.
   */
  const_reference operator*() const
  {
  }

  /**
   * Map iterator to global index domain.
   */
  inline self_t global() const
  {
    return *this;
  }

  /**
   * Whether the iterator's position is relative to a view.
   *
   * TODO:
   * should be iterator trait:
   *   dash::iterator_traits<GlobListIter<..>>::is_relative()::value
   */
  inline constexpr bool is_relative() const noexcept
  {
    return false;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline const GlobMemType & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline GlobMemType & globmem()
  {
    return *_globmem;
  }

  /**
   * Prefix increment operator.
   */
  inline self_t & operator++()
  {
    increment();
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  inline self_t operator++(int)
  {
    self_t result = *this;
    increment();
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  inline self_t & operator--()
  {
    decrement();
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  inline self_t operator--(int)
  {
    self_t result = *this;
    decrement();
    return result;
  }

  /**
   * Equality comparison operator.
   */
  inline bool operator==(const self_t & other) const
  {
    return _node == other._node;
  }

  /**
   * Inequality comparison operator.
   */
  inline bool operator!=(const self_t & other) const
  {
    return _node != other._node;
  }

private:

  void increment()
  {
  }

  void decrement()
  {
  }

private:
  /// Global memory used to dereference iterated values.
  GlobMemType          * _globmem;
  /// The node element referenced at the iterator's position.
  node_type            * _node     = nullptr;
  /// Unit id of the active unit
  dart_unit_t            _myid;

}; // class GlobListIter

} // namespace dash

#endif // DASH__LIST__GLOB_LIST_ITER_H__INCLUDED
