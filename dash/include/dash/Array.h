/* 
 * dash-lib/Array.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <stdexcept>
#include <algorithm>

#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/HView.h>
#include <dash/Shared.h>

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

  // get a global pointer for a local pointer
  GlobPtr<T> globptr(T* lptr) {
    GlobPtr<T> gptr = m_ptr->begin();
    gptr.set_unit(dash::myid());
    auto lptrdiff = lptr - begin();
    gptr += static_cast<long long>(lptrdiff);
    return gptr;
  }
};

template<typename ELEMENT_TYPE>
class Array {
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
  
  dash::Team   & m_team;
  dart_unit_t    m_myid;
  Pattern<1>     m_pattern;  
  GlobMem*       m_globmem; 
  iterator       m_begin;
  size_type      m_size;   // total size (# elements)
  size_type      m_lsize;  // local size (# local elements)
  
  ELEMENT_TYPE * m_lbegin;
  ELEMENT_TYPE * m_lend;
  
public:
/* Check requirements on element type 
   is_trivially_copyable is not implemented presently, and is_trivial
   is too strict (e.g. fails on std::pair).

   static_assert(std::is_trivially_copyable<ELEMENT_TYPE>::value,
     "Element type must be trivially copyable");
   static_assert(std::is_trivial<ELEMENT_TYPE>::value,
     "Element type must be trivially copyable");
*/

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

  bool allocate(
    size_t nelem,
    dash::DistributionSpec<1> ds) {
    assert(nelem > 0);
    
    m_pattern = Pattern<1>(nelem, ds, m_team);

    m_size  = m_pattern.capacity();
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

    // determine the real number of local elements
    for( ; m_lsize>0; m_lsize-- ) {
      if( m_pattern.unit_and_elem_to_index(m_myid, m_lsize-1)>= 0 )
	break;
    }
    
    dart_gptr_incaddr(&gptr, m_lsize*sizeof(ELEMENT_TYPE));
    dart_gptr_getaddr(gptr, &addr);
    m_lend=static_cast<ELEMENT_TYPE*>(addr);    
    
    return true;
  }

  bool deallocate() {
    if( m_size>0 ) {
      delete m_globmem;
      m_size=0;
    }
  }

  /// Local proxy object enables arr.local to be used in range-based for loops
  LocalProxyArray<value_type> local;

  /// Delegating constructor, specify pattern explicitly
  Array(
    const dash::Pattern<1> & pat)
  : Array(pat.capacity(), pat.distspec(), pat.team()) {
  }
  
  /// Delegating constructor, only specify the size
  Array(
    size_t nelem,
    Team & t = dash::Team::All())
  : Array(nelem, dash::BLOCKED, t) {
  }

  ~Array() {
    deallocate();
  }

  const_pointer data() const noexcept {
    return m_begin;
  }
  
  iterator begin() noexcept {
    iterator res = iterator(data());
    //cout<<"#### "<<res<<endl;
    return res;
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
    if (!(pos<size()))  {
      throw std::out_of_range("Out of range");
    }
    return begin()[pos];
  }

  constexpr size_type size() const noexcept {
    return m_size;
  }

  constexpr size_type lsize() const noexcept {
    return m_lsize;
  }
  
  constexpr bool empty() const noexcept {
    return size() == 0;
  }

  bool is_local(size_type n) const {
    auto coord = m_pattern.coords(n);
    return m_pattern.index_to_unit(coord) == m_myid;
  }
  
  void barrier() const {
    m_team.barrier();
  }

  Pattern<1> & pattern() {
    return m_pattern;
  }

  Team & team() {
    return m_team;
  }

  template<int level>
  dash::HView<Array<ELEMENT_TYPE>, level> hview() {
    return dash::HView<Array<ELEMENT_TYPE>, level>(*this);
  }

  // find the location of the global min. element
  // TODO: support custom comparison operator, similar to 
  // std::min_element()
  GlobPtr<ELEMENT_TYPE> min_element() {
    typedef dash::GlobPtr<ELEMENT_TYPE> globptr_t;

    dash::Array<globptr_t> minarr(m_team.size());

    // find the local min. element in parallel
    ELEMENT_TYPE *lmin =std::min_element(lbegin(), lend());     
    minarr[m_team.myid()] = local.globptr(lmin);

    dash::Shared<globptr_t> min;

    // find the global min. element
    if(m_team.myid()==0) {
      globptr_t minloc=minarr[0];
      ELEMENT_TYPE minval=*minloc;
      for( auto i=1; i<minarr.size(); i++ ) {
	if( *((globptr_t)minarr[i])<minval ) 
	  minloc=minarr[i];
      }
      min.set(minloc);
    }
    m_team.barrier();

    return min.get();
  }
};


}; // namespace dash

#endif /* ARRAY_H_INCLUDED */
