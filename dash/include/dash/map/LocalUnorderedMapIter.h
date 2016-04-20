#ifndef DASH__MAP__LOCAL_UNORDERED_MAP_ITER_H__INCLUDED
#define DASH__MAP__LOCAL_UNORDERED_MAP_ITER_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>

#include <dash/map/LocalUnorderedMapIter.h>

#include <dash/internal/Logging.h>

#include <type_traits>
#include <list>
#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>

namespace dash {

// Forward-declaration
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
class LocalUnorderedMapIter
: public std::iterator<
           std::random_access_iterator_tag,
           std::pair<const Key, Mapped>,
           dash::default_index_t,
           std::pair<const Key, Mapped> *,
           std::pair<const Key, Mapped> &>
{
  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  friend std::ostream & dash::operator<<(
    std::ostream & os,
    const dash::LocalUnorderedMapIter<K_, M_, H_, P_, A_> & it);

private:
  typedef LocalUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    self_t;

  typedef UnorderedMap<Key, Mapped, Hash, Pred, Alloc>
    map_t;

public:
  typedef typename map_t::value_type                              value_type;
#if 0
  typedef typename map_t::index_type                              index_type;
  typedef typename map_t::size_type                                size_type;
#else
  typedef dash::default_index_t                                   index_type;
  typedef dash::default_size_t                                     size_type;
#endif

  typedef       value_type *                                         pointer;
  typedef const value_type *                                   const_pointer;
  typedef       value_type &                                       reference;
  typedef const value_type &                                 const_reference;

  typedef struct {
    dart_unit_t unit;
    index_type  index;
  } local_index;

public:
  /**
   * Default constructor.
   */
  LocalUnorderedMapIter()
  : LocalUnorderedMapIter(nullptr)
  { }

  /**
   * Constructor, creates iterator at specified global position.
   */
  LocalUnorderedMapIter(
    map_t       * map,
    index_type    local_position)
  : _map(map),
    _idx(local_position),
    _max_idx(_map->size() - 1),
    _myid(dash::myid())
  {
    DASH_LOG_TRACE("LocalUnorderedMapIter(map,lpos)()");
    DASH_LOG_TRACE_VAR("LocalUnorderedMapIter(map,lpos)", _idx);
    DASH_LOG_TRACE_VAR("LocalUnorderedMapIter(map,lpos)", _max_idx);
    DASH_LOG_TRACE("LocalUnorderedMapIter(map,lpos) >");
  }

  /**
   * Copy constructor.
   */
  LocalUnorderedMapIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Null-pointer constructor.
   */
  LocalUnorderedMapIter(std::nullptr_t)
  : _map(nullptr),
    _idx(-1),
    _max_idx(-1),
    _myid(DART_UNDEFINED_UNIT_ID),
    _is_nullptr(true)
  {
    DASH_LOG_TRACE("LocalUnorderedMapIter(nullptr)");
  }

  /**
   * Null-pointer assignment operator.
   */
  self_t & operator=(std::nullptr_t) noexcept
  {
    _is_nullptr = true;
    return *this;
  }

  inline bool operator==(std::nullptr_t) const noexcept
  {
    return _is_nullptr;
  }

  inline bool operator!=(std::nullptr_t) const noexcept
  {
    return !_is_nullptr;
  }

  /**
   * Random access operator.
   */
  reference operator[](index_type offset)
  {
    auto res = *this;
    res += offset;
    return *this;
  }

  /**
   * Address resolution to native pointer.
   *
   * \return  A global reference to the element at the iterator's position
   */
  pointer operator&() const
  {
    typedef typename map_t::local_node_iterator local_iter_t;
    if (_is_nullptr) {
      return nullptr;
    }
    // TODO: check for correctness, _idx refers to local iteration space, not
    //       local memory space.
    local_iter_t l_it = _map->globmem().lbegin();
    return pointer(l_it + _idx);
  }

  /**
   * Type conversion operator to native pointer.
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator pointer() const
  {
    typedef typename map_t::local_node_iterator local_iter_t;
    if (_is_nullptr) {
      return nullptr;
    }
    // TODO: check for correctness, _idx refers to local iteration space, not
    //       local memory space.
    local_iter_t l_it = _map->globmem().lbegin();
    return pointer(l_it + static_cast<index_type>(_idx));
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const
  {
    typedef typename map_t::local_node_iterator local_iter_t;
    // TODO: check for correctness, _idx refers to local iteration space, not
    //       local memory space.
    local_iter_t l_it = _map->globmem().lbegin();
    return *pointer(l_it + static_cast<index_type>(_idx));
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("LocalUnorderedMapIter.dart_gptr()", _idx);
    dart_gptr_t dart_gptr = _map->globmem().at(_myid, _idx)
                            .dart_gptr();
    DASH_LOG_TRACE_VAR("LocalUnorderedMapIter.dart_gptr >", dart_gptr);
    return dart_gptr;
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  constexpr bool is_local() const noexcept
  {
    return true;
  }

  /**
   * Unit and local offset at the iterator's position.
   */
  inline local_index lpos() const noexcept
  {
    local_index local_pos;
    local_pos.unit  = _myid;
    local_pos.index = _idx;
    return local_pos;
  }

  /**
   * Position of the iterator in global index space.
   */
  inline index_type pos() const noexcept
  {
    return _idx;
  }

  /**
   * Prefix increment operator.
   */
  inline self_t & operator++()
  {
    increment(1);
    return *this;
  }

  /**
   * Prefix decrement operator.
   */
  inline self_t & operator--()
  {
    decrement(1);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  inline self_t operator++(int)
  {
    auto result = *this;
    increment(1);
    return result;
  }

  /**
   * Postfix decrement operator.
   */
  inline self_t operator--(int)
  {
    auto result = *this;
    decrement(1);
    return result;
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator==(
    const LocalUnorderedMapIter<K_, M_, H_, P_, A_> & other) const
  {
    return (this == std::addressof(other) || _idx == other._idx);
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator!=(
    const LocalUnorderedMapIter<K_, M_, H_, P_, A_> & other) const
  {
    return !(*this == other);
  }

  self_t & operator+=(index_type offset)
  {
    increment(offset);
    return *this;
  }

  self_t & operator-=(index_type offset)
  {
    decrement(offset);
    return *this;
  }

  self_t operator+(index_type offset) const
  {
    auto res = *this;
    res += offset;
    return res;
  }

  self_t operator-(index_type offset) const
  {
    auto res = *this;
    res -= offset;
    return res;
  }

  inline index_type operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  inline index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator<(
    const LocalUnorderedMapIter<K_, M_, H_, P_, A_> & other) const
  {
    return (_idx < other._idx);
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator<=(
    const LocalUnorderedMapIter<K_, M_, H_, P_, A_> & other) const
  {
    return (_idx <= other._idx);
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator>(
    const LocalUnorderedMapIter<K_, M_, H_, P_, A_> & other) const
  {
    return (_idx > other._idx);
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator>=(
    const LocalUnorderedMapIter<K_, M_, H_, P_, A_> & other) const
  {
    return (_idx >= other._idx);
  }

private:
  /**
   * Advance pointer by specified position offset.
   */
  void increment(index_type offset)
  {
    DASH_LOG_TRACE("LocalUnorderedMapIter.increment()",
                   "unit:",   _myid,
                   "lidx:",   _idx,
                   "offset:", offset);
    _idx += offset;
    DASH_LOG_TRACE("LocalUnorderedMapIter.increment >");
  }

  /**
   * Decrement pointer by specified position offset.
   */
  void decrement(index_type offset)
  {
    DASH_LOG_TRACE("LocalUnorderedMapIter.decrement()",
                   "unit:",   _myid,
                   "lidx:",   _idx,
                   "offset:", -offset);
    _idx -= offset;
    DASH_LOG_TRACE("LocalUnorderedMapIter.decrement >");
  }

private:
  /// Pointer to referenced map instance.
  map_t                  * _map           = nullptr;
  /// Current position of the iterator in local canonical index space.
  index_type               _idx           = -1;
  /// Maximum position allowed for this iterator.
  index_type               _max_idx       = -1;
  /// Unit id of the active unit.
  dart_unit_t              _myid          = DART_UNDEFINED_UNIT_ID;
  /// Whether the iterator represents a null pointer.
  bool                     _is_nullptr    = false;

}; // class LocalUnorderedMapIter

template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
std::ostream & operator<<(
  std::ostream & os,
  const dash::LocalUnorderedMapIter<
          Key, Mapped, Hash, Pred, Alloc> & it)
{
  std::ostringstream ss;
  ss << "dash::LocalUnorderedMapIter<"
     << typeid(Key).name()    << ","
     << typeid(Mapped).name() << ">"
     << "("
     << "unit:" << it._myid   << ", "
     << "lidx:" << it._idx
     << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__MAP__LOCAL_UNORDERED_MAP_ITER_H__INCLUDED
