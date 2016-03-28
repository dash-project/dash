#ifndef DASH__VECTOR_H__
#define DASH__VECTOR_H__

#include <iterator>
#include <limits>

#include <dash/Types.h>
#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/CSRPattern.h>
#include <dash/Shared.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>

namespace dash {

/**
 * \defgroup  DashVectorConcept  Vector Concept
 * Concept of a distributed one-dimensional vector container.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Methods
 *
 * \}
 */

/*
   STANDARD TYPE DEFINITION CONVENTIONS FOR STL CONTAINERS

            value_type  Type of element
        allocator_type  Type of memory manager
             size_type  Unsigned type of container
                        subscripts, element counts, etc.
       difference_type  Signed type of difference between
                        iterators

              iterator  Behaves like value_type*
        const_iterator  Behaves like const value_type*
      reverse_iterator  Behaves like value_type*
const_reverse_iterator  Behaves like const value_type*

             reference  value_type&
       const_reference  const value_type&

               pointer  Behaves like value_type*
         const_pointer  Behaves like const value_type*
*/

// forward declaration
template<
  typename ElementType,
  typename IndexType,
  class    PatternType >
class Vector;

template<
  typename T,
  typename IndexType,
  class    PatternType >
class LocalVectorRef
{
private:
  static const dim_t NumDimensions = 1;

  typedef LocalVectorRef<T, IndexType, PatternType>
    self_t;
  typedef Vector<T, IndexType, PatternType>
    Vector_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

public:
  template <typename T_, typename I_, typename P_>
    friend class LocalVectorRef;

public:
  typedef T                                                  value_type;

  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                          index_type;

  typedef IndexType                                     difference_type;

  typedef T &                                                 reference;
  typedef const T &                                     const_reference;

  typedef T *                                                   pointer;
  typedef const T *                                       const_pointer;

public:
  /// Type alias for LocalVectorRef<T,I,P>::view_type
  typedef LocalVectorRef<T, IndexType, PatternType>
    View;

public:
  /**
   * Constructor, creates a local access proxy for the given vector.
   */
  LocalVectorRef(
    Vector<T, IndexType, PatternType> * vector)
  : _vector(vector)
  { }

  LocalVectorRef(
    /// Pointer to vector instance referenced by this view.
    Vector<T, IndexType, PatternType> * vector,
    /// The view's offset and extent within the referenced vector.
    const ViewSpec_t & viewspec)
  : _vector(vector),
    _viewspec(viewspec)
  { }

  /**
   * Pointer to initial local element in the vector.
   */
  inline const_pointer begin() const noexcept {
    return _vector->_lbegin;
  }

  /**
   * Pointer to initial local element in the vector.
   */
  inline pointer begin() noexcept {
    return _vector->_lbegin;
  }

  /**
   * Pointer past final local element in the vector.
   */
  inline const_pointer end() const noexcept {
    return _vector->_lend;
  }

  /**
   * Pointer past final local element in the vector.
   */
  inline pointer end() noexcept {
    return _vector->_lend;
  }

  /**
   * Number of vector elements in local memory.
   */
  inline size_type size() const noexcept {
    return end() - begin();
  }

  /**
   * Subscript operator, access to local vector element at given position.
   */
  inline value_type operator[](const size_t n) const {
    return (_vector->_lbegin)[n];
  }

  /**
   * Subscript operator, access to local vector element at given position.
   */
  inline reference operator[](const size_t n) {
    return (_vector->_lbegin)[n];
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True
   */
  constexpr bool is_local(
    /// A global vector index
    index_type global_index) const {
    return true;
  }

  /**
   * View at block at given global block offset.
   */
  self_t block(index_type block_lindex)
  {
    DASH_LOG_TRACE("LocalVectorRef.block()", block_lindex);
    ViewSpec<1> block_view = pattern().local_block(block_lindex);
    DASH_LOG_TRACE("LocalVectorRef.block >", block_view);
    return self_t(_vector, block_view);
  }

  /**
   * The pattern used to distribute vector elements to units.
   */
  inline const PatternType & pattern() const {
    return _vector->pattern();
  }

private:
  /// Pointer to vector instance referenced by this view.
  Vector_t * const _vector;
  /// The view's offset and extent within the referenced vector.
  ViewSpec_t      _viewspec;
};


template<
  typename ElementType,
  typename IndexType,
  class    PatternType>
class VectorRef
{
private:
  static const dim_t NumDimensions = 1;

  typedef VectorRef<ElementType, IndexType, PatternType>
    self_t;
  typedef Vector<ElementType, IndexType, PatternType>
    Vector_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

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

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute vector elements to units
  typedef PatternType
    pattern_type;
  typedef VectorRef<ElementType, IndexType, PatternType>
    view_type;
  typedef LocalVectorRef<value_type, IndexType, PatternType>
    local_type;
  /// Type alias for Vector<T,I,P>::local_type
  typedef LocalVectorRef<value_type, IndexType, PatternType>
    Local;
  /// Type alias for Vector<T,I,P>::view_type
  typedef VectorRef<ElementType, IndexType, PatternType>
    View;

public:
  VectorRef(
    /// Pointer to vector instance referenced by this view.
    Vector_t         * vector,
    /// The view's offset and extent within the referenced vector.
    const ViewSpec_t & viewspec)
  : _vector(vector),
    _viewspec(viewspec)
  { }

public:
  inline    Team              & team();

  constexpr size_type           size()             const noexcept;
  constexpr size_type           local_size()       const noexcept;
  constexpr size_type           local_capacity()   const noexcept;
  constexpr size_type           extent(dim_t dim)  const noexcept;
  constexpr Extents_t           extents()          const noexcept;
  constexpr bool                empty()            const noexcept;

  inline    void                barrier()          const;

  inline    const_pointer       data()             const noexcept;
  inline    iterator            begin()                  noexcept;
  inline    const_iterator      begin()            const noexcept;
  inline    iterator            end()                    noexcept;
  inline    const_iterator      end()              const noexcept;
  /// View representing elements in the active unit's local memory.
  inline    local_type          sub_local()              noexcept;
  /// Pointer to first element in local range.
  inline    ElementType       * lbegin()           const noexcept;
  /// Pointer past final element in local range.
  inline    ElementType       * lend()             const noexcept;

  reference operator[](
    /// The position of the element to return
    size_type global_index)
  {
    DASH_LOG_TRACE("VectorRef.[]=", global_index);
    return _vector->_begin[global_index];
  }

  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
    DASH_LOG_TRACE("VectorRef.[]", global_index);
    return _vector->_begin[global_index];
  }

  reference at(
    /// The position of the element to return
    size_type global_pos)
  {
    if (global_pos >= size()) {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in VectorRef.at()" );
    }
    return _vector->_begin[global_pos];
  }

  const_reference at(
    /// The position of the element to return
    size_type global_pos) const
  {
    if (global_pos >= size()) {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in VectorRef.at()" );
    }
    return _vector->_begin[global_pos];
  }

  /**
   * The pattern used to distribute vector elements to units.
   */
  inline const PatternType & pattern() const {
    return _vector->pattern();
  }

private:
  /// Pointer to vector instance referenced by this view.
  Vector_t    * _vector;
  /// The view's offset and extent within the referenced vector.
  ViewSpec_t   _viewspec;

}; // class VectorRef


/**
 * A dynamic vector with support for workload balancing.
 *
 * \concept{DashContainerConcept}
 * \concept{DashVectorConcept}
 */
template<
  typename ElementType,
  typename IndexType   = dash::default_index_t,
  class PatternType    = CSRPattern<1, ROW_MAJOR, IndexType> >
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

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute vector elements to units
  typedef PatternType
    pattern_type;
  typedef LocalVectorRef<value_type, IndexType, PatternType>
    local_type;

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

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global vector instances
   * that are declared before \c dash::Init().
   */
  Vector(
    Team & team = dash::Team::Null())
  : local(this),
    _team(&team),
    _pattern(
      SizeSpec_t(0),
      DistributionSpec_t(dash::BLOCKED),
      team),
    _size(0),
    _lsize(0),
    _lcapacity(0)
  {
    DASH_LOG_TRACE("Vector()", "default constructor");
  }

  /**
   * Constructor, specifies distribution type explicitly.
   */
  Vector(
    size_type                  nelem,
    const DistributionSpec_t & distribution,
    Team                     & team = dash::Team::All())
  : local(this),
    _team(&team),
    _pattern(
      SizeSpec_t(nelem),
      distribution,
      team),
    _size(0),
    _lsize(0),
    _capacity(0),
    _lcapacity(0)
  {
    DASH_LOG_TRACE("Vector()", nelem);
    allocate(_pattern);
  }

  /**
   * Constructor, specifies distribution pattern explicitly.
   */
  Vector(
    const PatternType & pattern)
  : local(this),
    _team(&pattern.team()),
    _pattern(pattern),
    _size(0),
    _lsize(0),
    _capacity(0),
    _lcapacity(0)
  {
    DASH_LOG_TRACE("Vector()", "pattern instance constructor");
    allocate(_pattern);
  }

  /**
   * Delegating constructor, specifies the size of the vector.
   */
  Vector(
    size_type   nelem,
    Team      & team = dash::Team::All())
  : Vector(nelem, dash::BLOCKED, team)
  {
    DASH_LOG_TRACE("Vector()", "finished delegating constructor");
  }

  /**
   * Destructor, deallocates vector elements.
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
   * Global const pointer to the beginning of the vector.
   */
  const_pointer data() const noexcept
  {
    return _begin;
  }

  /**
   * Global pointer to the beginning of the vector.
   */
  iterator begin() noexcept
  {
    return _begin;
  }

  /**
   * Global pointer to the beginning of the vector.
   */
  const_iterator begin() const noexcept
  {
    return _begin;
  }

  /**
   * Global pointer to the end of the vector.
   */
  iterator end() noexcept
  {
    return _end;
  }

  /**
   * Global pointer to the end of the vector.
   */
  const_iterator end() const noexcept
  {
    return _end;
  }

  /**
   * Native pointer to the first local element in the vector.
   */
  ElementType * lbegin() const noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer to the end of the vector.
   */
  ElementType * lend() const noexcept
  {
    return _lend;
  }

  /**
   * Subscript assignment operator, not range-checked.
   *
   * \return  A global reference to the element in the vector at the given
   *          index.
   */
  reference operator[](
    /// The position of the element to return
    size_type global_index)
  {
    DASH_LOG_TRACE_VAR("Vector.[]=()", global_index);
    auto global_ref = _begin[global_index];
    DASH_LOG_TRACE_VAR("Vector.[]= >", global_ref);
    return global_ref;
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the vector at the given
   *          index.
   */
  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
    DASH_LOG_TRACE_VAR("Vector.[]()", global_index);
    auto global_ref = _begin[global_index];
    DASH_LOG_TRACE_VAR("Vector.[] >", global_ref);
    return global_ref;
  }

  /**
   * Random access assignment operator, range-checked.
   *
   * \see operator[]
   *
   * \return  A global reference to the element in the vector at the given
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
    return _begin[global_pos];
  }

  /**
   * Random access operator, range-checked.
   *
   * \see operator[]
   *
   * \return  A global reference to the element in the vector at the given
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
    return _begin[global_pos];
  }

  /**
   * The size of the vector.
   *
   * \return  The number of elements in the vector.
   */
  inline size_type size() const noexcept
  {
    return _size;
  }

  /**
   * Maximum number of elements a vector container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  inline size_type max_size() const noexcept
  {
    return std::numeric_limits<int>::max();
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
   * of the vector.
   *
   * \return  The number of elements in the vector.
   */
  inline size_type capacity() const noexcept
  {
    return _capacity;
  }

  inline iterator erase(const_iterator position)
  {
    return _begin;
  }

  inline iterator erase(const_iterator first, const_iterator last)
  {
    return _end;
  }

  /**
   * The team containing all units accessing this vector.
   *
   * \return  The instance of Team that this vector has been instantiated
   *          with
   */
  inline const Team & team() const noexcept
  {
    return *_team;
  }

  /**
   * The number of elements in the local part of the vector.
   *
   * \return  The number of elements in the vector that are local to the
   *          calling unit.
   */
  inline size_type lsize() const noexcept
  {
    return _lsize;
  }

  /**
   * The capacity of the local part of the vector.
   *
   * \return  The number of allocated elements in the vector that are local
   *          to the calling unit.
   */
  inline size_type lcapacity() const noexcept
  {
    return _lcapacity;
  }

  /**
   * Checks whether the vector is empty.
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
   * \return  True if the vector element referenced by the index is held
   *          in the calling unit's local memory
   */
  bool is_local(
    /// A global vector index
    index_type global_index) const
  {
    return _pattern.is_local(global_index, _myid);
  }

  /**
   * Establish a barrier for all units operating on the vector, publishing all
   * changes to all units.
   */
  void barrier() const
  {
    DASH_LOG_TRACE_VAR("Vector.barrier()", _team);
    _team->barrier();
    DASH_LOG_TRACE("Vector.barrier()", "passed barrier");
  }

  /**
   * The pattern used to distribute vector elements to units.
   */
  inline const PatternType & pattern() const
  {
    return _pattern;
  }

  template<int level>
  dash::HView<self_t, level> hview()
  {
    return dash::HView<self_t, level>(*this);
  }

  bool allocate(
    size_type                   nelem,
    dash::DistributionSpec<1>   distribution,
    dash::Team                & team = dash::Team::All())
  {
    DASH_LOG_TRACE("Vector.allocate()", nelem);
    DASH_LOG_TRACE_VAR("Vector.allocate", _team->dart_id());
    DASH_LOG_TRACE_VAR("Vector.allocate", team.dart_id());
    // Check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Vector with size 0");
    }
    if (_team == nullptr || *_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Vector.allocate",
                     "initializing pattern with Team::All()");
      _team    = &team;
      _pattern = PatternType(nelem, distribution, team);
      DASH_LOG_TRACE_VAR("Vector.allocate", team.dart_id());
      DASH_LOG_TRACE_VAR("Vector.allocate", _pattern.team().dart_id());
    } else {
      DASH_LOG_TRACE("Vector.allocate",
                     "initializing pattern with initial team");
      _pattern = PatternType(nelem, distribution, *_team);
    }
    return allocate(_pattern);
  }

  void deallocate()
  {
    DASH_LOG_TRACE_VAR("Vector.deallocate()", this);
    DASH_LOG_TRACE_VAR("Vector.deallocate()", _size);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the vector:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator list to avoid
    // double-free:
    _pattern.team().unregister_deallocator(
      this, std::bind(&Vector::deallocate, this));
    // Actual destruction of the vector instance:
    DASH_LOG_TRACE_VAR("Vector.deallocate()", _globmem);
    if (_globmem != nullptr) {
      delete _globmem;
      _globmem = nullptr;
    }
    _size = 0;
    DASH_LOG_TRACE_VAR("Vector.deallocate >", this);
  }

private:
  bool allocate(
    const PatternType & pattern)
  {
    DASH_LOG_TRACE("Vector._allocate()", "pattern",
                   pattern.memory_layout().extents());
    // Check requested capacity:
    _size      = pattern.capacity();
    _team      = &pattern.team();
    if (_size == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Vector with size 0");
    }
    // Initialize members:
    _lsize     = pattern.local_size();
    _lcapacity = pattern.local_capacity();
    _myid      = pattern.team().myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Vector._allocate", _lcapacity);
    DASH_LOG_TRACE_VAR("Vector._allocate", _lsize);
    _globmem   = new GlobMem_t(pattern.team(), _lcapacity);
    // Global iterators:
    _begin     = iterator(_globmem, pattern);
    _end       = iterator(_begin) + _size;
    // Local iterators:
    _lbegin    = _globmem->lbegin(_myid);
    // More efficient than using _globmem->lend as this a second mapping
    // of the local memory segment:
    _lend      = _lbegin + pattern.local_size();
    DASH_LOG_TRACE_VAR("Vector._allocate", _myid);
    DASH_LOG_TRACE_VAR("Vector._allocate", _size);
    DASH_LOG_TRACE_VAR("Vector._allocate", _lsize);
    // Register deallocator of this vector instance at the team
    // instance that has been used to initialized it:
    pattern.team().register_deallocator(
      this, std::bind(&Vector::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the vector before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("Vector._allocate",
                     "waiting for allocation of all units");
      _team->barrier();
    }
    DASH_LOG_TRACE("Vector._allocate >", "finished");
    return true;
  }

private:
  typedef dash::GlobMem<value_type> GlobMem_t;
  /// Team containing all units interacting with the vector
  dash::Team         * _team      = nullptr;
  /// DART id of the unit that created the vector
  dart_unit_t          _myid;
  /// Element distribution pattern
  PatternType          _pattern;
  /// Global memory allocation and -access
  GlobMem_t          * _globmem;
  /// Iterator to initial element in the vector
  iterator             _begin;
  /// Iterator past the last element in the vector
  iterator             _end;
  /// Number of elements in the vector
  size_type            _size;
  /// Number of local elements in the vector
  size_type            _lsize;
  /// Element capacity in the vector's currently allocated storage.
  size_type            _capacity;
  /// Element capacity in the vector's currently allocated local storage.
  size_type            _lcapacity;
  /// Native pointer to first local element in the vector
  ElementType        * _lbegin;
  /// Native pointer past the last local element in the vector
  ElementType        * _lend;

};

} // namespace dash

#endif // DASH__VECTOR_H__
