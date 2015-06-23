/* 
 * dash-lib/Array.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/HView.h>
#include <dash/Shared.h>
#include <dash/Exception.h>

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
class LocalProxyArray {
public: 
  /// Derive size type from given signed index / gptrdiff type
  typedef typename std::make_unsigned<IndexType>::type size_type;
  
private:
  Array<T, IndexType, PatternType> *m_ptr;
  
public:
  LocalProxyArray(Array<T, IndexType, PatternType> * ptr) {
    m_ptr = ptr;
  }
  
  T * begin() noexcept {
    return m_ptr->lbegin();
  }
  
  const T * begin() const noexcept {
    return m_ptr->lbegin();
  }
  
  T * end() noexcept {
    return m_ptr->lend();
  }
  
  const T * end() const noexcept {
    return m_ptr->lend();
  }

  size_type size() const noexcept {
    return end() - begin();
  }
  
  T & operator[](size_type n) {
    return begin()[n];
  }

  // get a global pointer for a local pointer
  GlobPtr<T> globptr(T* lptr) {
    GlobPtr<T> gptr = m_ptr->begin();
    gptr.set_unit(dash::myid());
    auto lptrdiff = lptr - begin();
    gptr += static_cast<long long>(lptrdiff);
    return gptr;
  }
};

/**
 * A distributed array.
 *
 * \concept{dash_container_concept}
 * \concept{dash_array_concept}
 */
template<
  typename ElementType,
  typename IndexType   = long long,
  class PatternType    = Pattern<1, ROW_MAJOR, IndexType> >
class Array {
private:
  typedef Array<ElementType, IndexType, PatternType> self_t;

/// Public types as required by iterator concept
public: 
  typedef ElementType  value_type;
  /// Derive size type from given signed index / gptrdiff type
  typedef typename std::make_unsigned<IndexType>::type size_type;
  /// Derive size type from given signed index / gptrdiff type
  typedef typename std::make_unsigned<IndexType>::type difference_type;

  typedef       GlobIter<value_type>                          iterator;
  typedef const GlobIter<value_type>                    const_iterator;
  typedef std::reverse_iterator<      iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  
  typedef       GlobRef<value_type>                          reference;
  typedef const GlobRef<value_type>                    const_reference;
  
  typedef       GlobIter<value_type>                           pointer;
  typedef const GlobIter<value_type>                     const_pointer;

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType pattern_type;
  
private:
  typedef dash::GlobMem<value_type> GlobMem_t;
  
  dash::Team         & m_team;
  dart_unit_t          m_myid;
  PatternType          m_pattern;  
  GlobMem_t          * m_globmem; 
  /// Iterator to initial element in the array
  iterator             m_begin;
  /// Iterator to final element in the array
  iterator             m_end;
  /// Total number of elements in the array
  size_type            m_size;
  /// Number of local elements in the array
  size_type            m_lsize;
  /// Native pointer to first local element in the array
  ElementType        * m_lbegin;
  /// Native pointer past last local element in the array
  ElementType        * m_lend;
  
public:
/* Check requirements on element type 
   is_trivially_copyable is not implemented presently, and is_trivial
   is too strict (e.g. fails on std::pair).

   static_assert(std::is_trivially_copyable<ElementType>::value,
     "Element type must be trivially copyable");
   static_assert(std::is_trivial<ElementType>::value,
     "Element type must be trivially copyable");
*/

  /// Local proxy object enables arr.local to be used in range-based for
  /// loops
  LocalProxyArray<value_type, IndexType, PatternType> local;

public:
  Array(
    Team & t = dash::Team::All())
  : m_team(t),
    m_pattern(0, dash::BLOCKED, t),
    local(this),
    m_size(0) {
  }

  Array(
    size_t nelem,
    dash::DistributionSpec<1> ds,
    Team & t = dash::Team::All())
  : m_team(t),
    m_pattern(nelem, ds, t),
    local(this) {
    allocate(nelem, ds);
  }  

  /**
   * Delegating constructor, specifies distribution pattern explicitly
   */
  Array(
    const PatternType & pat)
  : Array(pat.capacity(), pat.distspec(), pat.team()) {
  }
  
  /**
   * Delegating constructor, specifies the size of the array.
   */
  Array(
    size_t nelem,
    Team & t = dash::Team::All())
  : Array(nelem, dash::BLOCKED, t) {
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
    iterator res = iterator(data());
    return res;
  }
  
  /**
   * Global pointer to the beginning of the array.
   */
  const_iterator begin() const noexcept {
    iterator res = iterator(data());
    return res;
  }

  /**
   * Global pointer to the end of the array.
   */
  const_iterator end() const noexcept {
    return m_end;
  }

  /**
   * Global pointer to the end of the array.
   */
  iterator end() noexcept {
    return m_end;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  ElementType * lbegin() noexcept {
    return m_lbegin;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  const ElementType * lbegin() const noexcept {
    return m_lbegin;
  }

  /**
   * Native pointer to the end of the array.
   */
  ElementType * lend() noexcept {
    return m_lend;
  }

  /**
   * Native pointer to the end of the array.
   */
  const ElementType * lend() const noexcept {
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
    size_type index) {
    return begin()[index];
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference operator[](
    /// The position of the element to return
    size_type index) const {
    return begin()[index];
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
    size_type pos) {
    if (pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << pos 
          << " is out of range " << size() 
          << " in Array.at()" );
    }
    return begin()[pos];
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
    size_type pos) const {
    if (pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << pos 
          << " is out of range " << size() 
          << " in Array.at()" );
    }
    return begin()[pos];
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
   * The size of the local part of array.
   *
   * \return  The number of elements in the array that are local to the
   *          calling unit.
   */
  constexpr size_type lsize() const noexcept {
    return m_lsize;
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
    size_type index) const {
    auto coord = m_pattern.coords(index);
    return m_pattern.index_to_unit(coord) == m_myid;
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
  PatternType & pattern() {
    return m_pattern;
  }

  /**
   * The team containing all units operating on the array.
   */
  Team & team() {
    return m_team;
  }

  template<int level>
  dash::HView<self_t, level> hview() {
    return dash::HView<self_t, level>(*this);
  }

  bool allocate(
    size_t nelem,
    dash::DistributionSpec<1> ds) {
    // check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate 0 elements in Array.allocate()");
    }
    // initialize members:
    m_pattern = PatternType(nelem, ds, m_team);
    m_size    = m_pattern.capacity();
    m_lsize   = m_pattern.local_size();
    m_myid    = m_team.myid();
    m_globmem = new GlobMem_t(m_team, m_lsize);
    m_begin   = iterator(m_globmem, m_pattern);
    m_end     = iterator(m_begin) + m_size;
    m_lbegin  = m_globmem->lbegin();
    m_lend    = m_globmem->lend();
    return true;
  }

  bool deallocate() {
    if (m_size > 0) {
      delete m_globmem;
      m_size = 0;
      return true;
    }
    return false;
  }
};

} // namespace dash

#endif /* ARRAY_H_INCLUDED */
