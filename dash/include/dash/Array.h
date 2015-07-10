/* 
 * dash-lib/Array.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <dash/Types.h>
#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/HView.h>
#include <dash/Shared.h>
#include <dash/Exception.h>

#include <iterator>

/**
 * \defgroup  DashContainerConcept  Container Concept
 * Concept for iterable distributed containers
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Methods
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   </tr>
 *   <tr>
 *     <td>
 *       \c [](gindex)
 *     </td>
 *     <td>
 *       Returns the element located at the given global position
 *       in the container.
 *     </td>
 *   </tr>
 * </table>
 * \}
 */

/**
 * \defgroup  DashArrayConcept  Array Concept
 * A distributed array
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Methods
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   <tr>
 *     <td>
 *       \c [](gindex)
 *     </td>
 *     <td>
 *       Returns the element located at the given global position
 *       in the array.
 *     </td>
 *   </tr>
 * </table>
 * \}
 */

namespace dash {

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
  class PatternType >
class Array;

template<
  typename T,
  typename IndexType,
  class PatternType>
class LocalProxyArray :
  public std::iterator<std::random_access_iterator_tag,
                       T, IndexType, T*, T&> {
private:
  typedef LocalProxyArray<T, IndexType, PatternType>
    self_t;

public:
  template <typename T_, typename I_, typename P_>
    friend class LocalProxyArray;

public: 
  typedef T                                                  value_type;
  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                     difference_type;

  typedef typename std::iterator_traits<self_t>::reference    reference;
  typedef const T &                                     const_reference;

  typedef typename std::iterator_traits<self_t>::pointer        pointer;
  typedef const T *                                       const_pointer;

private:
  Array<T, IndexType, PatternType> * const m_array;
  
public:
  /**
   * Constructor, creates a local access proxy for the given array.
   */
  LocalProxyArray(
    Array<T, IndexType, PatternType> * const array)
  : m_array(array) {
  }

  /**
   * Pointer to initial local element in the array.
   */
  pointer begin() noexcept {
    return m_array->lbegin();
  }
  
  /**
   * Pointer to initial local element in the array.
   */
  constexpr const_pointer begin() const noexcept {
    return m_array->lbegin();
  }
  
  /**
   * Pointer past final local element in the array.
   */
  pointer end() noexcept {
    return m_array->lend();
  }
  
  /**
   * Pointer past final local element in the array.
   */
  constexpr const_pointer end() const noexcept {
    return m_array->lend();
  }

  /**
   * Number of array elements in local memory.
   */
  constexpr size_type size() const noexcept {
    return end() - begin();
  }
  
  /**
   * Subscript operator, access to local array element at given position.
   */
  constexpr value_type operator[](const size_t n) const {
    return m_array->lbegin()[n];
  }
 
  /**
   * Subscript operator, access to local array element at given position.
   */
  reference operator[](const size_t n) {
    return m_array->lbegin()[n];
  }
 
  /**
   * Resolve the global pointer for a local pointer.
   */
  GlobPtr<T> globptr(T * lptr) {
    // Get global begin pointer depending on unit
    GlobPtr<T> gptr = m_array->begin();
    gptr.set_unit(m_array->team().myid());
    // Get local offset
    auto lptrdiff = lptr - begin();
    // Add local offset to global begin pointer
    gptr += static_cast<dash::gptr_diff_t>(lptrdiff);
    return gptr;
  }
};

/**
 * A distributed array.
 *
 * \related dash::min_element
 * \related dash::max_element
 *
 * \concept{DashContainerConcept}
 * \concept{DashArrayConcept}
 *
 * Note: Template parameter IndexType could be deduced from pattern type:
 *       PatternT::index_type
 */
template<
  typename ElementType,
  typename IndexType   = dash::default_index_t,
  class PatternType    = Pattern<1, ROW_MAJOR, IndexType> >
class Array {
private:
  typedef Array<ElementType, IndexType, PatternType> self_t;

/// Public types as required by iterator concept
public: 
  typedef ElementType  value_type;
  typedef IndexType    index_type;
  /// Derive size type from given signed index / gptrdiff type
  typedef typename std::make_unsigned<IndexType>::type size_type;
  /// Derive size type from given signed index / gptrdiff type
  typedef typename std::make_unsigned<IndexType>::type difference_type;

  typedef       GlobIter<value_type, PatternType>             iterator;
  typedef const GlobIter<value_type, PatternType>       const_iterator;
  typedef std::reverse_iterator<      iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  
  typedef       GlobRef<value_type>                          reference;
  typedef const GlobRef<value_type>                    const_reference;
  
  typedef       GlobIter<value_type, PatternType>              pointer;
  typedef const GlobIter<value_type, PatternType>        const_pointer;

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType
    pattern_type;
  typedef LocalProxyArray<value_type, IndexType, PatternType>
    local_type;
  
private:
  typedef dash::GlobMem<value_type> GlobMem_t;
  /// Team containing all units interacting with the array
  dash::Team         & m_team;
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

public:
  /// Local proxy object enables arr.local to be used in range-based for
  /// loops.
  local_type           local;
  
public:
/* 
   Check requirements on element type 
   is_trivially_copyable is not implemented presently, and is_trivial
   is too strict (e.g. fails on std::pair).

   static_assert(std::is_trivially_copyable<ElementType>::value,
     "Element type must be trivially copyable");
   static_assert(std::is_trivial<ElementType>::value,
     "Element type must be trivially copyable");
*/

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global array instances
   * that are declared before \c dash::Init().
   */
  Array(
    Team & team = dash::Team::Null())
  : m_team(team),
    m_pattern(0, dash::BLOCKED, team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    local(this) {
    DASH_LOG_TRACE("Array()", "default constructor");
  }

  /**
   * Constructor, specifies distribution type explicitly.
   */
  Array(
    size_type nelem,
    dash::DistributionSpec<1> distribution,
    Team & team = dash::Team::All())
  : m_team(team),
    m_pattern(nelem, distribution, team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    local(this) {
    DASH_LOG_TRACE("Array()", nelem);
    allocate(m_pattern);
  }  

  /**
   * Constructor, specifies distribution pattern explicitly.
   */
  Array(
    const PatternType & pattern)
  : m_team(pattern.team()),
    m_pattern(pattern),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    local(this) {
    DASH_LOG_TRACE("Array()", "pattern instance constructor");
    allocate(m_pattern);
  }
  
  /**
   * Delegating constructor, specifies the size of the array.
   */
  Array(
    size_type nelem,
    Team & team = dash::Team::All())
  : Array(nelem, dash::BLOCKED, team) {
    DASH_LOG_TRACE("Array()", "finished delegating constructor");
  }

  /**
   * Destructor, deallocates array elements.
   */
  ~Array() {
    deallocate();
  }

  /**
   * Global const pointer to the beginning of the array.
   */
  const_pointer data() const noexcept {
    return m_begin;
  }
  
  /**
   * Global pointer to the beginning of the array.
   */
  iterator begin() noexcept {
    iterator res = iterator(m_begin);
    return res;
  }
  
  /**
   * Global pointer to the beginning of the array.
   */
  const_iterator begin() const noexcept {
    return m_begin;
  }

  /**
   * Global pointer to the end of the array.
   */
  iterator end() noexcept {
    iterator res = iterator(m_end);
    return res;
  }

  /**
   * Global pointer to the end of the array.
   */
  const_iterator end() const noexcept {
    return m_end;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  ElementType * const lbegin() const noexcept {
    return m_lbegin;
  }

  /**
   * Native pointer to the end of the array.
   */
  ElementType * const lend() const noexcept {
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
    size_type global_index) {
    DASH_LOG_TRACE("Array.[]=", global_index);
    return m_begin[global_index];
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const {
    DASH_LOG_TRACE("Array.[]", global_index);
    return m_begin[global_index];
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
    size_type global_pos) {
    if (global_pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos 
          << " is out of range " << size() 
          << " in Array.at()" );
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
    size_type global_pos) const {
    if (global_pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos 
          << " is out of range " << size() 
          << " in Array.at()" );
    }
    return m_begin[global_pos];
  }

  /**
   * The size of the array.
   *
   * \return  The number of elements in the array.
   */
  constexpr size_type size() const noexcept {
    return m_size;
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the array.
   *
   * \return  The number of elements in the array.
   */
  constexpr size_type capacity() const noexcept {
    return m_size;
  }

  /**
   * The team containing all units accessing this array.
   *
   * \return  The instance of Team that this array has been instantiated
   *          with
   */
  const Team & team() const noexcept {
    return m_team;
  }

  /**
   * The number of elements in the local part of the array.
   *
   * \return  The number of elements in the array that are local to the
   *          calling unit.
   */
  constexpr size_type lsize() const noexcept {
    return m_lsize;
  }

  /**
   * The capacity of the local part of the array.
   *
   * \return  The number of allocated elements in the array that are local
   *          to the calling unit.
   */
  constexpr size_type lcapacity() const noexcept {
    return m_lcapacity;
  }
  
  /**
   * Checks whether the array is empty.
   *
   * \return  True if \c size() is 0, otherwise false
   */
  constexpr bool empty() const noexcept {
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
    index_type global_index) const {
    return m_pattern.is_local(global_index, m_myid);
  }
  
  /**
   * Establish a barrier for all units operating on the array.
   */
  void barrier() const {
    m_team.barrier();
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  const PatternType & pattern() const {
    return m_pattern;
  }

  template<int level>
  dash::HView<self_t, level> hview() {
    return dash::HView<self_t, level>(*this);
  }

  bool allocate(
    size_type nelem,
    dash::DistributionSpec<1> distribution,
    dash::Team & team = dash::Team::All()) {
    DASH_LOG_TRACE("Array.allocate()", nelem);
    // Check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Array with size 0");
    }
    if (m_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with Team::All()");
      m_pattern = PatternType(nelem, distribution, team);
    } else {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with initial team");
      m_pattern = PatternType(nelem, distribution, m_team);
    }
    return allocate(m_pattern);
  }

  bool deallocate() {
    if (m_size > 0) {
      delete m_globmem;
      m_size = 0;
      return true;
    }
    return false;
  }

private:
  bool allocate(
    const PatternType & pattern) {
    DASH_LOG_TRACE("Array.allocate()", "pattern", 
                   pattern.memory_layout().extents());
    // Check requested capacity:
    m_size      = m_pattern.capacity();
    if (m_size == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Array with size 0");
    }
    // Initialize members:
    m_lsize     = m_pattern.local_size();
    m_lcapacity = m_pattern.local_capacity();
    m_myid      = m_team.myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Array.allocate", m_lcapacity);
    DASH_LOG_TRACE_VAR("Array.allocate", m_lsize);
    m_globmem   = new GlobMem_t(m_team, m_lcapacity);
    // Global iterators:
    m_begin     = iterator(m_globmem, m_pattern);
    m_end       = iterator(m_begin) + m_size;
    // Local iterators:
    m_lbegin    = m_globmem->lbegin(m_myid);
    m_lend      = m_globmem->lend(m_myid);
    DASH_LOG_TRACE_VAR("Array.allocate", m_myid);
    DASH_LOG_TRACE_VAR("Array.allocate", m_size);
    DASH_LOG_TRACE_VAR("Array.allocate", m_lsize);
    DASH_LOG_TRACE("Array.allocate() finished");
    return true;
  }
};

} // namespace dash

#endif /* ARRAY_H_INCLUDED */
