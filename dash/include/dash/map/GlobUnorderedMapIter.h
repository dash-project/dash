#ifndef DASH__MAP__GLOB_UNORDERED_MAP_ITER_H__INCLUDED
#define DASH__MAP__GLOB_UNORDERED_MAP_ITER_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/GlobSharedRef.h>
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
class GlobUnorderedMapIter
: public std::iterator<
           std::bidirectional_iterator_tag,
           std::pair<const Key, Mapped>,
           dash::default_index_t,
           dash::GlobPtr< std::pair<const Key, Mapped> >,
           dash::GlobSharedRef< std::pair<const Key, Mapped> > >
{
  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  friend class GlobUnorderedMapIter;

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  friend std::ostream & dash::operator<<(
    std::ostream & os,
    const dash::GlobUnorderedMapIter<K_, M_, H_, P_, A_> & it);

private:
  typedef GlobUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    self_t;

  typedef UnorderedMap<Key, Mapped, Hash, Pred, Alloc>
    map_t;

public:
  typedef typename map_t::value_type                              value_type;
  typedef typename map_t::index_type                              index_type;
  typedef typename map_t::size_type                                size_type;

  typedef dash::GlobPtr<      value_type>                            pointer;
  typedef dash::GlobPtr<const value_type>                      const_pointer;
  typedef dash::GlobSharedRef<      value_type>                    reference;
  typedef dash::GlobSharedRef<const value_type>              const_reference;

  typedef typename
    std::conditional<
      std::is_const<value_type>::value,
      typename map_t::const_local_pointer,
      typename map_t::local_pointer
    >::type
    local_pointer;

  typedef struct {
    dart_unit_t unit;
    index_type  index;
  } local_index;

public:
  /**
   * Default constructor.
   */
  GlobUnorderedMapIter()
  : GlobUnorderedMapIter(nullptr)
  { }

  /**
   * Constructor, creates iterator at specified global position.
   */
  GlobUnorderedMapIter(
    map_t       * map,
    index_type    position)
  : _map(map),
    _idx(0),
    _max_idx(_map->size() - 1),
    _myid(dash::myid()),
    _idx_unit_id(0),
    _idx_local_idx(0)
  {
    DASH_LOG_TRACE_VAR("GlobUnorderedMapIter(map,pos)", _idx);
    increment(position);
    DASH_LOG_TRACE("GlobUnorderedMapIter(map,pos) >");
  }

  /**
   * Constructor, creates iterator at local position relative to the
   * specified unit's local iteration space.
   */
  GlobUnorderedMapIter(
    map_t       * map,
    dart_unit_t   unit,
    index_type    local_index)
  : _map(map),
    _idx(0),
    _max_idx(_map->size() - 1),
    _myid(dash::myid()),
    _idx_unit_id(unit),
    _idx_local_idx(local_index)
  {
    DASH_LOG_TRACE("GlobUnorderedMapIter(map,unit,lidx)()");
    DASH_LOG_TRACE_VAR("GlobUnorderedMapIter(map,unit,lidx)", unit);
    DASH_LOG_TRACE_VAR("GlobUnorderedMapIter(map,unit,lidx)", local_index);
    // Unit and local offset to global position:
    size_type unit_l_cumul_size_prev = 0;
    if (unit > 0) {
      unit_l_cumul_size_prev = _map->_local_cumul_sizes[unit-1];
    }
    _idx = unit_l_cumul_size_prev + _idx_local_idx;
    DASH_LOG_TRACE_VAR("GlobUnorderedMapIter(map,unit,lidx)", _idx);
    DASH_LOG_TRACE("GlobUnorderedMapIter(map,unit,lidx) >");
  }

  /**
   * Copy constructor.
   */
  GlobUnorderedMapIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Null-pointer constructor.
   */
  GlobUnorderedMapIter(std::nullptr_t)
  : _map(nullptr),
    _idx(-1),
    _max_idx(-1),
    _myid(DART_UNDEFINED_UNIT_ID),
    _idx_unit_id(DART_UNDEFINED_UNIT_ID),
    _idx_local_idx(-1),
    _is_nullptr(true)
  {
    DASH_LOG_TRACE("GlobUnorderedMapIter(nullptr)");
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
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator pointer() const
  {
    return pointer(dart_gptr());
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("GlobUnorderedMapIter.dart_gptr()", _idx);
    dart_gptr_t dart_gptr = _map->globmem().at(
                              _idx_unit_id,
                              _idx_local_idx)
                            .dart_gptr();
    DASH_LOG_TRACE_VAR("GlobUnorderedMapIter.dart_gptr >", dart_gptr);
    return dart_gptr;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const
  {
    auto lptr = local();
    if (lptr != nullptr) {
      return reference(lptr);
    } else {
      return reference(dart_gptr());
    }
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  inline bool is_local() const noexcept
  {
    return (_myid == _idx_unit_id);
  }

  /**
   * Conversion to local bucket iterator.
   */
  local_pointer local() const noexcept
  {
    if (_myid != _idx_unit_id) {
      // Iterator position does not point to local element
      return local_pointer(nullptr);
    }
    return (_map->lbegin() + _idx_local_idx);
  }

  /**
   * Unit and local offset at the iterator's position.
   */
  inline local_index lpos() const noexcept
  {
    local_index local_pos;
    local_pos.unit  = _idx_unit_id;
    local_pos.index = _idx_local_idx;
    return local_pos;
  }

  /**
   * Map iterator to global index domain.
   */
  inline self_t global() const noexcept
  {
    return *this;
  }

  /**
   * Position of the iterator in global index space.
   */
  inline index_type pos() const noexcept
  {
    return _idx;
  }

  /**
   * Position of the iterator in global index range.
   */
  inline index_type gpos() const noexcept
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
    const GlobUnorderedMapIter<K_, M_, H_, P_, A_> & other) const noexcept
  {
    return (this == std::addressof(other) || _idx == other._idx);
  }

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  inline bool operator!=(
    const GlobUnorderedMapIter<K_, M_, H_, P_, A_> & other) const noexcept
  {
    return !(*this == other);
  }

private:
  /**
   * Advance pointer by specified position offset.
   */
  void increment(int offset)
  {
    DASH_LOG_TRACE("GlobUnorderedMapIter.increment()",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "offset:", offset);
    _idx           += offset;
    _idx_local_idx  = _idx;
    auto & l_cumul_sizes = _map->_local_cumul_sizes;
    // Find unit at global offset:
    while (_idx > l_cumul_sizes[_idx_unit_id] &&
           _idx_unit_id < l_cumul_sizes.size()) {
      DASH_LOG_TRACE("GlobUnorderedMapIter.increment",
                     l_cumul_sizes[_idx_unit_id]);
      _idx_unit_id++;
      _idx_local_idx -= l_cumul_sizes[_idx_unit_id];
    }
    DASH_LOG_TRACE("GlobUnorderedMapIter.increment >");
  }

  /**
   * Decrement pointer by specified position offset.
   */
  void decrement(int offset)
  {
    DASH_LOG_TRACE("GlobUnorderedMapIter.decrement()",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "offset:", -offset);
    _idx -= offset;
    DASH_LOG_TRACE("GlobUnorderedMapIter.decrement >");
  }

private:
  /// Pointer to referenced map instance.
  map_t                  * _map           = nullptr;
  /// Current position of the iterator in global canonical index space.
  index_type               _idx           = -1;
  /// Maximum position allowed for this iterator.
  index_type               _max_idx       = -1;
  /// Unit id of the active unit.
  dart_unit_t              _myid          = DART_UNDEFINED_UNIT_ID;
  /// Unit id at the iterator's current position.
  dart_unit_t              _idx_unit_id   = DART_UNDEFINED_UNIT_ID;
  /// Logical offset in local index space at the iterator's current position.
  index_type               _idx_local_idx = -1;
  /// Whether the iterator represents a null pointer.
  bool                     _is_nullptr    = false;

}; // class GlobUnorderedMapIter

template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobUnorderedMapIter<
          Key, Mapped, Hash, Pred, Alloc> & it)
{
  std::ostringstream ss;
  ss << "dash::GlobUnorderedMapIter<"
     << typeid(Key).name()    << ","
     << typeid(Mapped).name() << ">"
     << "("
     << "idx:"  << it._idx           << ", "
     << "unit:" << it._idx_unit_id   << ", "
     << "lidx:" << it._idx_local_idx
     << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__MAP__GLOB_UNORDERED_MAP_ITER_H__INCLUDED
