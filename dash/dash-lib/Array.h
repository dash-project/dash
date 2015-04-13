/* 
 * dash-lib/Array.h
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <stdexcept>

#include "GlobMem.h"
#include "GlobIter.h"
#include "GlobRef.h"
#include "Team.h"
#include "Pattern1D.h" 

namespace dash
{
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
template<typename T> class Array;


template<typename T>
class LocalProxyArray
{
public: 
  typedef size_t size_type;
  
private:
  Array<T> *m_ptr;
  
public:
  LocalProxyArray(Array<T>* ptr) {
    m_ptr = ptr;
  }
  
  T* begin() const noexcept {
    return m_ptr->lbegin();
  }
  
  T* end() const noexcept {
    return m_ptr->lend();
  }

  size_type size() const noexcept {
    return end()-begin();
  }
  
  T& operator[](size_type n) {
    return begin()[n];
  }
};
 
 

template<typename ELEMENT_TYPE>
class Array
{
public: 
  typedef ELEMENT_TYPE  value_type;

  // NO allocator_type!
  typedef size_t size_type;
  typedef size_t difference_type;

  typedef       GlobIter<value_type>         iterator;
  typedef const GlobIter<value_type>   const_iterator;
  typedef std::reverse_iterator<      iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  
  typedef       GlobRef<value_type>       reference;
  typedef const GlobRef<value_type> const_reference;
  
  typedef       GlobIter<value_type>       pointer;
  typedef const GlobIter<value_type> const_pointer;
  
private:
  typedef dash::GlobMem<value_type>    GlobMem;
  
  dash::Team&   m_team;
  dart_unit_t   m_myid;
  Pattern1D     m_pattern;  
  GlobMem*      m_globmem; 
  iterator      m_begin;
  size_type     m_size;   // total size (# elements)
  size_type     m_lsize;  // local size (# local elements)
  
  ELEMENT_TYPE*  m_lbegin;
  ELEMENT_TYPE*  m_lend;
  
public:
  // check requirements on element type 
  static_assert(std::is_trivial<ELEMENT_TYPE>::value,
		"Element type must be trivially copyable");

  /*
    is_trivially_copyable is not implemented presently, so 
    we use is_trivial instead...

  static_assert(std::is_trivially_copyable<ELEMENT_TYPE>::value,
  "Element type must be trivially copyable");
  */
  
  
public:
  Array(size_t nelem, dash::DistSpec ds,
	Team& t=dash::Team::All() ) : 
    m_team(t),
    m_pattern(nelem, ds, t),
    local(this)
  {
    m_size  = m_pattern.nelem();
    m_lsize = m_pattern.max_elem_per_unit();
    m_myid  = m_team.myid();

    m_globmem = new GlobMem(m_team, m_lsize);

    m_begin = iterator(m_globmem, m_pattern);

    // determine local begin and end addresses
    void *addr; dart_gptr_t gptr; 
    gptr = m_globmem->begin().dartptr();

    dart_gptr_setunit(&gptr, m_myid);
    dart_gptr_getaddr(gptr, &addr);
    m_lbegin=static_cast<ELEMENT_TYPE*>(addr);

    dart_gptr_incaddr(&gptr, m_lsize*sizeof(ELEMENT_TYPE));
    dart_gptr_getaddr(gptr, &addr);
    m_lend=static_cast<ELEMENT_TYPE*>(addr);
  }  

  // this local proxy object enables arr.local to be used in
  // range-based for loops
  LocalProxyArray<value_type> local;

  // delegating constructor : specify pattern explicitly
  Array(const dash::Pattern1D& pat ) : 
    Array(pat.nelem(), pat.distspec(), pat.team())
  { }
  
  // delegating constructor : only specify the size
  Array(size_t nelem, 
	Team &t=dash::Team::All()) : 
    Array(nelem, dash::BLOCKED, t)
  { }

  ~Array() {
    delete m_globmem;
  }

  const_pointer data() const noexcept {
    return m_begin;
  }
  
  iterator begin() noexcept {
    return iterator(data());
  }

  iterator end() noexcept {
    return iterator(data() + m_size);
  }

  ELEMENT_TYPE* lbegin() const noexcept {
    return m_lbegin;
  }

  ELEMENT_TYPE* lend() const noexcept {
    return m_lend;
  }  

  reference operator[](size_type n) {
    return begin()[n];
  }

  reference at(size_type pos) {
    if( !(pos<size()) )  {
      throw std::out_of_range("Out of range");
    }
    return begin()[pos];
  }

  constexpr size_type size() const noexcept {
    return m_size;
  }
  
  constexpr bool empty() const noexcept {
    return size() == 0;
  }

  bool is_local(size_type n) const
  {
    return m_pattern.index_to_unit(n)==m_myid;
  }
  
  void barrier() const {
    m_team.barrier();
  }
};


}; // namespace dash

#endif /* ARRAY_H_INCLUDED */
