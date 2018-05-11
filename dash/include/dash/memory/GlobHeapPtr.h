#ifndef DASH__MEMORY__GLOB_HEAP_PTR_H__INCLUDED
#define DASH__MEMORY__GLOB_HEAP_PTR_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/GlobSharedRef.h>

#include <dash/internal/Logging.h>

#include <type_traits>
#include <list>
#include <vector>
#include <sstream>
#include <iostream>


namespace dash {

// Forward-declaration
template<
  typename ElementType,
  class    AllocatorType >
class GlobHeapMem;

/**
 * Iterator on global buckets. Represents global pointer type.
 */
template <typename ElementType, class AllocatorType>
class GlobPtr<
        ElementType,
        GlobHeapMem<
          typename std::remove_const<ElementType>::type,
          AllocatorType >
      >
{
  typedef GlobHeapMem<
            typename std::remove_const<ElementType>::type,
            AllocatorType> GlobHeapMemType;
  typedef GlobPtr<ElementType, GlobHeapMemType>   self_t;

  template<
    typename ElementType_,
    class    AllocType_ >
  friend std::ostream & operator<<(
    std::ostream & os,
    const dash::GlobPtr<
            ElementType_,
            GlobHeapMem<
              typename std::remove_const<ElementType_>::type, AllocType_ >
          > & gptr);

public:
  typedef typename GlobHeapMemType::index_type                   index_type;
  typedef typename std::make_unsigned<index_type>::type           size_type;

  typedef ElementType                                            value_type;

  typedef GlobSharedRef<      value_type, self_t>                 reference;
  typedef GlobSharedRef<const value_type, self_t>           const_reference;

  typedef value_type *                                          raw_pointer;

  typedef GlobHeapMemType                                      globmem_type;
  typedef typename GlobHeapMemType::local_pointer             local_pointer;

  typedef struct {
    team_unit_t unit;
    index_type  index;
  } local_index;

  template <typename U>
  struct rebind {
    typedef GlobPtr<
              U,
              GlobHeapMem<
                typename std::remove_const<U>::type,
                typename AllocatorType::template rebind<
                           typename std::remove_const<U>::type
                         >::other
              >
            > other;
  };

private:
  typedef std::vector<std::vector<size_type> >
    bucket_cumul_sizes_map;

private:
  /// Global memory used to dereference iterated values.
  const globmem_type           * _globmem            = nullptr;
  /// Mapping unit id to buckets in the unit's attached local storage.
  const bucket_cumul_sizes_map * _bucket_cumul_sizes = nullptr;
  /// Pointer to first element in local data space.
  local_pointer                  _lbegin;
  /// Current position of the pointer in global canonical index space.
  index_type                     _idx                = 0;
  /// Maximum position allowed for this pointer.
  index_type                     _max_idx            = 0;
  /// Unit id of the active unit.
  team_unit_t                    _myid;
  /// Unit id at the pointer's current position.
  team_unit_t                    _idx_unit_id;
  /// Logical offset in local index space at the pointer's current position.
  index_type                     _idx_local_idx      = -1;
  /// Local bucket index at the pointer's current position.
  index_type                     _idx_bucket_idx     = -1;
  /// Element offset in bucket at the pointer's current position.
  index_type                     _idx_bucket_phase   = -1;

public:
  /**
   * Default constructor.
   */
  GlobPtr()
  : _globmem(nullptr),
    _bucket_cumul_sizes(nullptr),
    _idx(0),
    _max_idx(0),
    _myid(dash::Team::GlobalUnitID()),
    _idx_unit_id(DART_UNDEFINED_UNIT_ID),
    _idx_local_idx(-1),
    _idx_bucket_idx(-1),
    _idx_bucket_phase(-1)
  {
    DASH_LOG_TRACE_VAR("GlobPtr()", _idx);
    DASH_LOG_TRACE_VAR("GlobPtr()", _max_idx);
  }

  /**
   * Constructor, creates a global pointer on global memory from global
   * offset in logical storage order.
   */
  template<typename MemSpaceT>
  GlobPtr(
    const MemSpaceT    * gmem,
	  index_type           position = 0)
  : _globmem(reinterpret_cast<const globmem_type *>(gmem)),
    _bucket_cumul_sizes(&_globmem->_bucket_cumul_sizes),
    _lbegin(_globmem->lbegin()),
    _idx(position),
    _max_idx(gmem->size() - 1),
    _myid(gmem->team().myid()),
    _idx_unit_id(0),
    _idx_local_idx(0),
    _idx_bucket_idx(0),
    _idx_bucket_phase(0)
  {
    DASH_LOG_TRACE("GlobPtr(gmem,idx)", "gidx:", position);
    for (auto unit_bucket_cumul_sizes : *_bucket_cumul_sizes) {
      DASH_LOG_TRACE_VAR("GlobPtr(gmem,idx)",
                         unit_bucket_cumul_sizes);
      size_type bucket_cumul_size_prev = 0;
      for (auto bucket_cumul_size : unit_bucket_cumul_sizes) {
        DASH_LOG_TRACE_VAR("GlobPtr(gmem,idx)", bucket_cumul_size);
        if (position < bucket_cumul_size) {
          DASH_LOG_TRACE_VAR("GlobPtr(gmem,idx)", position);
          _idx_bucket_phase -= bucket_cumul_size;
          position           = 0;
          _idx_local_idx     = position;
          _idx_bucket_phase  = position - bucket_cumul_size_prev;
          break;
        }
        bucket_cumul_size_prev = bucket_cumul_size;
        ++_idx_bucket_idx;
      }
      if (position == 0) {
        break;
      }
      // Advance to next unit, adjust position relative to next unit's
      // local index space:
      position -= unit_bucket_cumul_sizes.back();
      ++_idx_unit_id;
    }
    DASH_LOG_TRACE("GlobPtr(gmem,idx)",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "bucket:", _idx_bucket_idx,
                   "phase:",  _idx_bucket_phase);
  }

  /**
   * Constructor, creates a global pointer on global memory from unit and
   * local offset in logical storage order.
   */
  template<typename MemSpaceT>
  GlobPtr(
    const MemSpaceT    * gmem,
    team_unit_t          unit,
	  index_type           local_index)
  : _globmem(reinterpret_cast<const globmem_type *>(gmem)),
    _bucket_cumul_sizes(&_globmem->_bucket_cumul_sizes),
    _lbegin(_globmem->lbegin()),
    _idx(0),
    _max_idx(gmem->size() - 1),
    _myid(gmem->team().myid()),
    _idx_unit_id(unit),
    _idx_local_idx(0),
    _idx_bucket_idx(0),
    _idx_bucket_phase(0)
  {
    DASH_LOG_TRACE("GlobPtr(gmem,unit,lidx)",
                   "unit:", unit,
                   "lidx:", local_index);
    DASH_ASSERT_LT(unit, _bucket_cumul_sizes->size(), "invalid unit id");

    for (size_type unit = 0; unit < _idx_unit_id; ++unit) {
      auto prec_unit_local_size = (*_bucket_cumul_sizes)[unit].back();
      _idx += prec_unit_local_size;
    }
    increment(local_index);
    DASH_LOG_TRACE("GlobPtr(gmem,unit,lidx) >",
                   "gidx:",   _idx,
                   "maxidx:", _max_idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "bucket:", _idx_bucket_idx,
                   "phase:",  _idx_bucket_phase);
  }

  /**
   * Copy constructor.
   */
  template<typename E_, typename M_>
  GlobPtr(
    const GlobPtr<E_, M_> & other)
  : _globmem(other._globmem),
    _bucket_cumul_sizes(other._bucket_cumul_sizes),
    _lbegin(other._lbegin),
    _idx(other._idx),
    _max_idx(other._max_idx),
    _idx_unit_id(other._idx_unit_id),
    _idx_local_idx(other._idx_local_idx),
    _idx_bucket_idx(other._idx_bucket_idx),
    _idx_bucket_phase(other._idx_bucket_phase)
  { }

  /**
   * Assignment operator.
   */
  template<typename E_, typename M_>
  self_t & operator=(
    const GlobPtr<E_, M_> & other)
  {
    _globmem            = other._globmem;
    _bucket_cumul_sizes = other._bucket_cumul_sizes;
    _lbegin             = other._lbegin;
    _idx                = other._idx;
    _max_idx            = other._max_idx;
    _idx_unit_id        = other._idx_unit_id;
    _idx_local_idx      = other._idx_local_idx;
    _idx_bucket_idx     = other._idx_bucket_idx;
    _idx_bucket_phase   = other._idx_bucket_phase;
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the pointer's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("GlobPtr.dart_gptr()", _idx);
    // Create global pointer from unit, bucket and phase:
    dart_gptr_t dart_gptr = _globmem->dart_gptr_at(
                              _idx_unit_id,
                              _idx_bucket_idx,
                              _idx_bucket_phase);
    DASH_LOG_TRACE_VAR("GlobPtr.dart_gptr >", dart_gptr);
    return dart_gptr;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the pointer's position.
   */
  reference operator*() const
  {
    auto lptr = local();
    if (lptr != nullptr) {
      return reference(static_cast<raw_pointer>(lptr));
    } else {
      return reference(dart_gptr());
    }
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  reference operator[](
    /// The global position of the element
    index_type g_index) const
  {
    DASH_LOG_TRACE_VAR("GlobPtr.[]()", g_index);
    auto git = *this;
    git  += g_index;
    auto lptr = git.local();
    if (lptr != nullptr) {
      return reference(lptr);
    } else {
      auto gref = *git;
      DASH_LOG_TRACE_VAR("GlobPtr.[] >", gref);
      return gref;
    }
  }

  /**
   * Checks whether the element referenced by this global pointer is in
   * the calling unit's local memory.
   */
  inline bool is_local() const
  {
    return (_myid == _idx_unit_id);
  }

  /**
   * Conversion to local bucket pointer.
   */
  local_pointer local() const
  {
    if (_myid != _idx_unit_id) {
      // Iterator position does not point to local element
      return nullptr;
    }
    return (_lbegin + _idx_local_idx);
  }

  /**
   * Unit and local offset at the pointer's position.
   */
  inline local_index lpos() const
  {
    local_index local_pos;
    local_pos.unit  = _idx_unit_id;
    local_pos.index = _idx_local_idx;
    return local_pos;
  }

  /**
   * Map pointer to global index domain.
   */
  inline self_t global() const
  {
    return *this;
  }

  /**
   * Position of the pointer in global index space.
   */
  inline index_type pos() const
  {
    return _idx;
  }

  /**
   * Position of the pointer in global index range.
   */
  inline index_type gpos() const
  {
    return _idx;
  }

  /**
   * The instance of \c GlobStaticMem used by this pointer to resolve
   * addresses in global memory.
   */
  inline const globmem_type & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobStaticMem used by this pointer to resolve
   * addresses in global memory.
   */
  inline globmem_type & globmem()
  {
    return *_globmem;
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

  inline self_t & operator+=(index_type offset)
  {
    increment(offset);
    return *this;
  }

  inline self_t & operator-=(index_type offset)
  {
    increment(offset);
    return *this;
  }

  inline self_t operator+(index_type offset) const
  {
    auto res = *this;
    res.increment(offset);
    return res;
  }

  inline self_t operator-(index_type offset) const
  {
    auto res = *this;
    res.decrement(offset);
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

  template<typename E_, typename M_>
  inline bool operator<(const GlobPtr<E_, M_> & other) const
  {
    return (_idx < other._idx);
  }

  template<typename E_, typename M_>
  inline bool operator<=(const GlobPtr<E_, M_> & other) const
  {
    return (_idx <= other._idx);
  }

  template<typename E_, typename M_>
  inline bool operator>(const GlobPtr<E_, M_> & other) const
  {
    return (_idx > other._idx);
  }

  template<typename E_, typename M_>
  inline bool operator>=(const GlobPtr<E_, M_> & other) const
  {
    return (_idx >= other._idx);
  }

  template<typename E_, typename M_>
  inline bool operator==(const GlobPtr<E_, M_> & other) const
  {
    return _idx == other._idx;
  }

  template<typename E_, typename M_>
  inline bool operator!=(const GlobPtr<E_, M_> & other) const
  {
    return _idx != other._idx;
  }

private:
  /**
   * Advance pointer by specified position offset.
   */
  void increment(int offset)
  {
    DASH_LOG_TRACE("GlobPtr.increment()",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "bidx:",   _idx_bucket_idx,
                   "bphase:", _idx_bucket_phase,
                   "offset:", offset);
    _idx += offset;
    auto current_bucket_size =
           (*_bucket_cumul_sizes)[_idx_unit_id][_idx_bucket_idx];
    if (_idx_local_idx + offset < current_bucket_size) {
      DASH_LOG_TRACE("GlobPtr.increment", "position current bucket");
      // element is in bucket currently referenced by this pointer:
      _idx_bucket_phase += offset;
      _idx_local_idx    += offset;
    } else {
      DASH_LOG_TRACE("GlobPtr.increment",
                     "position in succeeding bucket");
      // iterate units:
      auto unit_id_max = _bucket_cumul_sizes->size() - 1;
      for (; _idx_unit_id <= unit_id_max; ++_idx_unit_id) {
        if (offset == 0) {
          break;
        }
        auto unit_bkt_sizes       = (*_bucket_cumul_sizes)[_idx_unit_id];
        auto unit_bkt_sizes_total = unit_bkt_sizes.back();
        auto unit_num_bkts        = unit_bkt_sizes.size();
        DASH_LOG_TRACE("GlobPtr.increment",
                       "unit:", _idx_unit_id,
                       "remaining offset:", offset,
                       "total local bucket size:", unit_bkt_sizes_total);
        if (_idx_local_idx + offset >= unit_bkt_sizes_total) {
          // offset refers to next unit:
          DASH_LOG_TRACE("GlobPtr.increment",
                         "position in remote range");
          // subtract remaining own local size from remaining offset:
          offset -= (unit_bkt_sizes_total - _idx_local_idx);
          if (_idx_unit_id == unit_id_max) {
            // end pointer, offset exceeds iteration space:
            _idx_bucket_idx    = unit_num_bkts - 1;
            auto last_bkt_size = unit_bkt_sizes.back();
            if (unit_num_bkts > 1) {
              last_bkt_size -= unit_bkt_sizes[_idx_bucket_idx-1];
            }
            _idx_bucket_phase  = last_bkt_size + offset;
            _idx_local_idx    += unit_bkt_sizes_total  + offset;
            break;
          }
          _idx_local_idx    = 0;
          _idx_bucket_idx   = 0;
          _idx_bucket_phase = 0;
        } else {
          // offset refers to current unit:
          DASH_LOG_TRACE("GlobPtr.increment",
                         "position in local range",
                         "current bucket phase:", _idx_bucket_phase,
                         "cumul. bucket sizes:",  unit_bkt_sizes);
          _idx_local_idx += offset;
          // iterate the unit's bucket sizes:
          for (; _idx_bucket_idx < unit_num_bkts; ++_idx_bucket_idx) {
            auto cumul_bucket_size = unit_bkt_sizes[_idx_bucket_idx];
            if (_idx_local_idx < cumul_bucket_size) {
              auto cumul_prev  = _idx_bucket_idx > 0
                                 ? unit_bkt_sizes[_idx_bucket_idx-1]
                                 : 0;
              // offset refers to current bucket:
              _idx_bucket_phase = _idx_local_idx - cumul_prev;
              offset            = 0;
              break;
            }
          }
          if (offset == 0) {
            break;
          }
        }
      }
    }
    DASH_LOG_TRACE("GlobPtr.increment >",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "bidx:",   _idx_bucket_idx,
                   "bphase:", _idx_bucket_phase);
  }

  /**
   * Decrement pointer by specified position offset.
   */
  void decrement(int offset)
  {
    DASH_LOG_TRACE("GlobPtr.decrement()",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "bidx:",   _idx_bucket_idx,
                   "bphase:", _idx_bucket_phase,
                   "offset:", -offset);
    if (offset > _idx) {
      DASH_THROW(dash::exception::OutOfRange,
                 "offset " << offset << " is out of range");
    }
    _idx -= offset;
    if (offset <= _idx_bucket_phase) {
      // element is in bucket currently referenced by this pointer:
      _idx_bucket_phase -= offset;
      _idx_local_idx    -= offset;
    } else {
      // iterate units:
      auto first_unit = _idx_unit_id;
      for (; _idx_unit_id >= 0; --_idx_unit_id) {
        auto unit_bkt_sizes       = (*_bucket_cumul_sizes)[_idx_unit_id];
        auto unit_bkt_sizes_total = unit_bkt_sizes.back();
        auto unit_num_bkts        = unit_bkt_sizes.size();
        if (_idx_unit_id != first_unit) {
          --offset;
          _idx_bucket_idx    = unit_num_bkts - 1;
          _idx_local_idx     = unit_bkt_sizes_total - 1;
          auto last_bkt_size = unit_bkt_sizes.back();
          if (unit_num_bkts > 1) {
            last_bkt_size -= unit_bkt_sizes[_idx_bucket_idx-1];
          }
          _idx_bucket_phase  = last_bkt_size - 1;
        }
        if (offset <= _idx_local_idx) {
          // offset refers to current unit:
          // iterate the unit's bucket sizes:
          for (; _idx_bucket_idx >= 0; --_idx_bucket_idx) {
            auto bucket_size = unit_bkt_sizes[_idx_bucket_idx];
            if (offset <= _idx_bucket_phase) {
              // offset refers to current bucket:
              _idx_local_idx    -= offset;
              _idx_bucket_phase -= offset;
              offset             = 0;
              break;
            } else {
              // offset refers to preceeding bucket:
              _idx_local_idx    -= (_idx_bucket_phase + 1);
              offset            -= (_idx_bucket_phase + 1);
              _idx_bucket_phase  = unit_bkt_sizes[_idx_bucket_idx-1] - 1;
            }
          }
        } else {
          // offset refers to preceeding unit:
          offset -= _idx_local_idx;
        }
        if (offset == 0) {
          break;
        }
      }
    }
    DASH_LOG_TRACE("GlobPtr.decrement >",
                   "gidx:",   _idx,
                   "unit:",   _idx_unit_id,
                   "lidx:",   _idx_local_idx,
                   "bidx:",   _idx_bucket_idx,
                   "bphase:", _idx_bucket_phase);
  }

}; // class GlobPtr

/**
 * Resolve the number of elements between two global bucket pointers.
 *
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<
  typename ElementType,
  class    AlllocType >
auto distance(
  /// Global pointer to the first position in the global sequence
  const dash::GlobPtr<
          ElementType, GlobHeapMem<ElementType, AlllocType>
        > & first,
  /// Global pointer to the final position in the global sequence
  const dash::GlobPtr<
          ElementType, GlobHeapMem<ElementType, AlllocType>
        > & last)
-> typename GlobHeapMem<ElementType, AlllocType>::index_type
{
  return last - first;
}

template<
  typename ElementType,
  class    AllocType >
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobPtr<
          ElementType,
          GlobHeapMem<
            typename std::remove_const<ElementType>::type, AllocType >
        > & gptr)
{
  std::ostringstream ss;
  ss << "dash::GlobPtr<" << typeid(ElementType).name() << ">("
     << "gidx:"   << gptr._idx              << ", ("
     << "unit:"   << gptr._idx_unit_id      << ", "
     << "lidx:"   << gptr._idx_local_idx    << "), ("
     << "bidx:"   << gptr._idx_bucket_idx   << ", "
     << "bphase:" << gptr._idx_bucket_phase << ")"
     << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__MEMORY__GLOB_HEAP_PTR_H__INCLUDED
