#ifndef DASH__VECTOR_H__
#define DASH__VECTOR_H__

#include <dash/Types.h>
#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/GlobAsyncRef.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/HView.h>
#include <dash/Shared.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>

#include <iterator>

namespace dash {

// forward declaration
template<
  typename ElementType,
  typename IndexType,
  class    PatternType >
class Vector;

/**
 * A dynamic array with support for workload balancing.
 */
template<
  typename ElementType,
  typename IndexType   = dash::default_index_t,
  class PatternType    = Pattern<1, ROW_MAJOR, IndexType> >
class Vector
{
private:
  typedef Vector<ElementType, IndexType, PatternType> self_t;

/// Public types as required by iterator concept
public:
  typedef ElementType                                             value_type;
  typedef IndexType                                               index_type;
  typedef typename std::make_unsigned<IndexType>::type             size_type;
  typedef typename std::make_unsigned<IndexType>::type       difference_type;

  typedef       GlobIter<value_type, PatternType>                   iterator;
  typedef const GlobIter<value_type, PatternType>             const_iterator;
  typedef       std::reverse_iterator<      iterator>       reverse_iterator;
  typedef       std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                          const_reference;

  typedef       GlobIter<value_type, PatternType>                    pointer;
  typedef const GlobIter<value_type, PatternType>              const_pointer;

public:
  template<
    typename T_,
    typename I_,
    class P_>
  friend class LocalVectorRef;
  template<
    typename T_,
    typename I_,
    class P_>
  friend class AsyncVectorRef;

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType
    pattern_type;
  typedef LocalVectorRef<value_type, IndexType, PatternType>
    local_type;
  typedef AsyncVectorRef<value_type, IndexType, PatternType>
    async_type;

  typedef LocalVectorRef<value_type, IndexType, PatternType>
    Local;
  typedef VectorRef<ElementType, IndexType, PatternType>
    View;

private:
  typedef DistributionSpec<1>
    DistributionSpec_t;
  typedef SizeSpec<1, size_type>
    SizeSpec_t;

public:
  /// Local proxy object, allows use in range-based for loops.
  local_type           local;
  /// Proxy object, provides non-blocking operations on array.
  async_type           async;

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global array instances
   * that are declared before \c dash::Init().
   */
  Vector(
    Team & team = dash::Team::Null())
  : local(this),
    async(this),
    m_team(&team),
    m_pattern(
      SizeSpec_t(0),
      DistributionSpec_t(dash::BLOCKED),
      team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0)
  {
    DASH_LOG_TRACE("Vector()", "default constructor");
  }

  /**
   * Constructor, specifies distribution type explicitly.
   */
  Vector(
    size_type nelem,
    const DistributionSpec_t & distribution,
    Team & team = dash::Team::All())
  : local(this),
    async(this),
    m_team(&team),
    m_pattern(
      SizeSpec_t(nelem),
      distribution,
      team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0)
  {
    DASH_LOG_TRACE("Vector()", nelem);
    allocate(m_pattern);
  }

  /**
   * Constructor, specifies distribution pattern explicitly.
   */
  Vector(
    const PatternType & pattern)
  : local(this),
    async(this),
    m_team(&pattern.team()),
    m_pattern(pattern),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0)
  {
    DASH_LOG_TRACE("Vector()", "pattern instance constructor");
    allocate(m_pattern);
  }

  /**
   * Delegating constructor, specifies the size of the array.
   */
  Vector(
    size_type nelem,
    Team & team = dash::Team::All())
  : Vector(nelem, dash::BLOCKED, team)
  {
    DASH_LOG_TRACE("Vector()", "finished delegating constructor");
  }

  /**
   * Destructor, deallocates array elements.
   */
  ~Vector()
  {
    DASH_LOG_TRACE_VAR("Vector.~Vector()", this);
    deallocate();
  }

  void push_back(const ElementType & element)
  {
  }

  void pop_back()
  {
  }

  reference back()
  {
  }

  reference front()
  {
  }

  /**
   * View at block at given global block offset.
   */
  View block(index_type block_gindex)
  {
    DASH_LOG_TRACE("Vector.block()", block_gindex);
    ViewSpec<1> block_view = pattern().block(block_gindex);
    DASH_LOG_TRACE("Vector.block >", block_view);
    return View(this, block_view);
  }

  /**
   * Global const pointer to the beginning of the array.
   */
  const_pointer data() const noexcept
  {
    return m_begin;
  }

  /**
   * Global pointer to the beginning of the array.
   */
  iterator begin() noexcept
  {
    return m_begin;
  }

  /**
   * Global pointer to the beginning of the array.
   */
  const_iterator begin() const noexcept
  {
    return m_begin;
  }

  /**
   * Global pointer to the end of the array.
   */
  iterator end() noexcept
  {
    return m_end;
  }

  /**
   * Global pointer to the end of the array.
   */
  const_iterator end() const noexcept
  {
    return m_end;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  ElementType * lbegin() const noexcept
  {
    return m_lbegin;
  }

  /**
   * Native pointer to the end of the array.
   */
  ElementType * lend() const noexcept
  {
    return m_lend;
  }

  /**
   * Subscript assignment operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  reference operator[](
    /// The position of the element to return
    size_type global_index)
  {
    DASH_LOG_TRACE_VAR("Vector.[]=()", global_index);
    auto global_ref = m_begin[global_index];
    DASH_LOG_TRACE_VAR("Vector.[]= >", global_ref);
    return global_ref;
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
    DASH_LOG_TRACE_VAR("Vector.[]()", global_index);
    auto global_ref = m_begin[global_index];
    DASH_LOG_TRACE_VAR("Vector.[] >", global_ref);
    return global_ref;
  }

  /**
   * Random access assignment operator, range-checked.
   *
   * \see operator[]
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  reference at(
    /// The position of the element to return
    size_type global_pos)
  {
    if (global_pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in Vector.at()" );
    }
    return m_begin[global_pos];
  }

  /**
   * Random access operator, range-checked.
   *
   * \see operator[]
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference at(
    /// The position of the element to return
    size_type global_pos) const
  {
    if (global_pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in Vector.at()" );
    }
    return m_begin[global_pos];
  }

  /**
   * The size of the array.
   *
   * \return  The number of elements in the array.
   */
  inline size_type size() const noexcept
  {
    return m_size;
  }

  /**
   * Maximum number of elements a vector container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  inline size_typew max_size() const noexcept
  {
    return MAX_INT;
  }

  /**
   * Requests that the vector capacity be at least enough to contain n
   * elements.
   * If n is greater than the current vector capacity, the function causes
   * the container to reallocate its storage increasing its capacity to n
   * (or greater).
   * In all other cases, the function call does not cause a reallocation and
   * the vector capacity is not affected.
   * This function has no effect on the vector size and cannot alter its
   * elements.
   */
  void reserve(size_t num_elements)
  {
  }

  /**
   * Resizes the vector so its capacity is changed to the given number of
   * elements. Elements are removed and destroying elements from the back,
   * if necessary.
   */
  void resize(size_t num_elements)
  {
  }

  /**
   * Requests the container to reduce its capacity to fit its size.
   */
  void shrink_to_fit()
  {
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the array.
   *
   * \return  The number of elements in the array.
   */
  inline size_type capacity() const noexcept
  {
    return m_capacity;
  }

  inline iterator erase(const_iterator position)
  {
  }

  inline iterator erase(const_iterator first, const_iterator last)
  {
  }

  /**
   * The team containing all units accessing this array.
   *
   * \return  The instance of Team that this array has been instantiated
   *          with
   */
  inline const Team & team() const noexcept
  {
    return *m_team;
  }

  /**
   * The number of elements in the local part of the array.
   *
   * \return  The number of elements in the array that are local to the
   *          calling unit.
   */
  inline size_type lsize() const noexcept
  {
    return m_lsize;
  }

  /**
   * The capacity of the local part of the array.
   *
   * \return  The number of allocated elements in the array that are local
   *          to the calling unit.
   */
  inline size_type lcapacity() const noexcept
  {
    return m_lcapacity;
  }

  /**
   * Checks whether the array is empty.
   *
   * \return  True if \c size() is 0, otherwise false
   */
  inline bool empty() const noexcept
  {
    return size() == 0;
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True if the array element referenced by the index is held
   *          in the calling unit's local memory
   */
  bool is_local(
    /// A global array index
    index_type global_index) const
  {
    return m_pattern.is_local(global_index, m_myid);
  }

  /**
   * Establish a barrier for all units operating on the array, publishing all
   * changes to all units.
   */
  void barrier() const
  {
    DASH_LOG_TRACE_VAR("Vector.barrier()", m_team);
    m_team->barrier();
    DASH_LOG_TRACE("Vector.barrier()", "passed barrier");
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  inline const PatternType & pattern() const
  {
    return m_pattern;
  }

  template<int level>
  dash::HView<self_t, level> hview()
  {
    return dash::HView<self_t, level>(*this);
  }

  bool allocate(
    size_type nelem,
    dash::DistributionSpec<1> distribution,
    dash::Team & team = dash::Team::All())
  {
    DASH_LOG_TRACE("Vector.allocate()", nelem);
    DASH_LOG_TRACE_VAR("Vector.allocate", m_team->dart_id());
    DASH_LOG_TRACE_VAR("Vector.allocate", team.dart_id());
    // Check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Vector with size 0");
    }
    if (m_team == nullptr || *m_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Vector.allocate",
                     "initializing pattern with Team::All()");
      m_team    = &team;
      m_pattern = PatternType(nelem, distribution, team);
      DASH_LOG_TRACE_VAR("Vector.allocate", team.dart_id());
      DASH_LOG_TRACE_VAR("Vector.allocate", m_pattern.team().dart_id());
    } else {
      DASH_LOG_TRACE("Vector.allocate",
                     "initializing pattern with initial team");
      m_pattern = PatternType(nelem, distribution, *m_team);
    }
    return allocate(m_pattern);
  }

  void deallocate()
  {
    DASH_LOG_TRACE_VAR("Vector.deallocate()", this);
    DASH_LOG_TRACE_VAR("Vector.deallocate()", m_size);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the array:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator list to avoid
    // double-free:
    m_pattern.team().unregister_deallocator(
      this, std::bind(&Vector::deallocate, this));
    // Actual destruction of the array instance:
    DASH_LOG_TRACE_VAR("Vector.deallocate()", m_globmem);
    if (m_globmem != nullptr) {
      delete m_globmem;
      m_globmem = nullptr;
    }
    m_size = 0;
    DASH_LOG_TRACE_VAR("Vector.deallocate >", this);
  }

private:
  bool allocate(
    const PatternType & pattern)
  {
    DASH_LOG_TRACE("Vector._allocate()", "pattern",
                   pattern.memory_layout().extents());
    // Check requested capacity:
    m_size      = pattern.capacity();
    m_team      = &pattern.team();
    if (m_size == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Vector with size 0");
    }
    // Initialize members:
    m_lsize     = pattern.local_size();
    m_lcapacity = pattern.local_capacity();
    m_myid      = pattern.team().myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Vector._allocate", m_lcapacity);
    DASH_LOG_TRACE_VAR("Vector._allocate", m_lsize);
    m_globmem   = new GlobMem_t(pattern.team(), m_lcapacity);
    // Global iterators:
    m_begin     = iterator(m_globmem, pattern);
    m_end       = iterator(m_begin) + m_size;
    // Local iterators:
    m_lbegin    = m_globmem->lbegin(m_myid);
    // More efficient than using m_globmem->lend as this a second mapping
    // of the local memory segment:
    m_lend      = m_lbegin + pattern.local_size();
    DASH_LOG_TRACE_VAR("Vector._allocate", m_myid);
    DASH_LOG_TRACE_VAR("Vector._allocate", m_size);
    DASH_LOG_TRACE_VAR("Vector._allocate", m_lsize);
    // Register deallocator of this array instance at the team
    // instance that has been used to initialized it:
    pattern.team().register_deallocator(
      this, std::bind(&Vector::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the array before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("Vector._allocate",
                     "waiting for allocation of all units");
      m_team->barrier();
    }
    DASH_LOG_TRACE("Vector._allocate >", "finished");
    return true;
  }

private:
  typedef dash::GlobMem<value_type> GlobMem_t;
  /// Team containing all units interacting with the array
  dash::Team         * m_team      = nullptr;
  /// DART id of the unit that created the array
  dart_unit_t          m_myid;
  /// Element distribution pattern
  PatternType          m_pattern;
  /// Global memory allocation and -access
  GlobMem_t          * m_globmem;
  /// Iterator to initial element in the array
  iterator             m_begin;
  /// Iterator to final element in the array
  iterator             m_end;
  /// Element capacity in the vector's currently allocated storage.
  size_type            m_size;
  /// Total number of elements in the array
  size_type            m_size;
  /// Number of local elements in the array
  size_type            m_lsize;
  /// Number allocated local elements in the array
  size_type            m_lcapacity;
  /// Native pointer to first local element in the array
  ElementType        * m_lbegin;
  /// Native pointer past last local element in the array
  ElementType        * m_lend;

};

} // namespace dash

#endif // DASH__VECTOR_H__
