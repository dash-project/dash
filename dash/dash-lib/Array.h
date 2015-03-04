
#ifndef DASH_ARRAY_H_INCLUDED
#define DASH_ARRAY_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "Team.h"
//#include "Pattern.h"
#include "Pattern.h"
#include "GlobPtr.h"
#include "GlobRef.h"
#include "HView.h"

#include "dart.h"

namespace dash
{
/* 
   TYPE DEFINITION CONVENTIONS FOR STL CONTAINERS 
   
            value_type  Type of element
        allocator_type  Type of memory manager
             size_type  Unsigned type of container 
                        subscripts, element counts, etc.
       difference_type  Signed type of difference between
                        iterators

              iterator  Behaves like value_type∗
        const_iterator  Behaves like const value_type∗
      reverse_iterator  Behaves like value_type∗
const_reverse_iterator  Behaves like const value_type∗

             reference  value_type&
       const_reference  const value_type&

               pointer  Behaves like value_type∗
         const_pointer  Behaves like const value_type∗
*/

template<typename T, size_t DIM>
class Array;

template<typename T, size_t DIM>
class LocalProxyArray
{
private:
	Array<T, DIM> *m_ptr;

public:
  LocalProxyArray(Array<T, DIM>* ptr) {
    m_ptr = ptr;
  }
  
  T* begin() noexcept
  {
    return m_ptr->lbegin();
  }

  T* end() noexcept
  {
    return m_ptr->lend();
  }
};

template<typename ELEMENT_TYPE, size_t DIM>
class Array
{
public:
  typedef ELEMENT_TYPE value_type;

  // NO allocator_type!
  typedef size_t size_type;
  typedef size_t difference_type;

  typedef       GlobPtr<value_type, DIM>         iterator;
  typedef const GlobPtr<value_type, DIM>   const_iterator;
  typedef std::reverse_iterator<      iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>       reference;
  typedef const GlobRef<value_type> const_reference;

  typedef       GlobPtr<value_type, DIM>       pointer;
  typedef const GlobPtr<value_type, DIM> const_pointer;

private:
  dash::Team&      m_team;
  dart_unit_t      m_myid;
  dash::Pattern<DIM>    m_pattern;
  size_type        m_size;    // total size (#elements)
  size_type        m_lsize;   // local size (#local elements)
  pointer*         m_ptr;
  dart_gptr_t      m_dart_gptr;

#if 0
  // xxx needs fix
  size_type      m_realsize;
#endif 

public:
  LocalProxyArray<ELEMENT_TYPE, DIM> local;

  static_assert(std::is_trivial<ELEMENT_TYPE>::value, 
		"Element type must be trivial copyable");
  /*
  static_assert(std::is_trivially_copyable<ELEMENT_TYPE>::value, 
		"Element type must be trivially copyable");
  */
  
public: 

  Array(const dash::SizeSpec<DIM>& ss, const dash::DistSpec<DIM>& ds=dash::DistSpec<DIM>(), 
	 Team &t=dash::Team::All(), const TeamSpec<DIM> &ts =
			TeamSpec<DIM>()) : 
    m_team(t), 
    m_pattern(ss, ds, ts, t),
    local(this)
  {
    // Array is a friend of class Team 
    dart_team_t teamid = m_team.m_dartid;    
    
    size_t lelem = m_pattern.max_elem_per_unit();
    size_t lsize = lelem* sizeof(value_type);

    m_dart_gptr = DART_GPTR_NULL;
    dart_ret_t ret = 
      dart_team_memalloc_aligned(teamid, lsize, &m_dart_gptr);
    
    m_ptr = new GlobPtr<value_type, DIM>(m_pattern, m_dart_gptr, 0);
    
    m_size     = m_pattern.nelem();
    m_lsize    = lelem;
    
    m_myid = m_team.myid();
  
    //m_realsize = lelem * m_team.size();
  }

  // delegating constructor
  Array(const dash::Pattern<DIM>& pat ) : 
    Array(pat.sizespec(), pat.distspec(), pat.team())
  { }

  // delegating constructor
  Array(size_t nelem, 
	Team &t=dash::Team::All()) : 
    Array(nelem, dash::BLOCKED, t)
  { }

#if 0
  Array(size_t nelem) :
    Array(nelem, dash::BLOCKED, dash::Team::All())
  {}
#endif 

  ~Array() {
    dart_team_t teamid = m_team.m_dartid;
    dart_team_memfree(teamid, m_dart_gptr);
  }

  Pattern<DIM>& pattern() {
    return m_pattern;
  }

  Team& team() {
    return m_team;
  }

  constexpr size_type size() const noexcept
  {
    return m_size;
  }

  //constexpr size_type max_size() const noexcept
  //{
  //return m_realsize;
  // }

  constexpr bool empty() const noexcept
  {
    return size() == 0;
  }

  void barrier() const
  {
    m_team.barrier();
  }

  const_pointer data() const noexcept
  {
    return *m_ptr;
  }
  
  iterator begin() noexcept
  {
    return iterator(data());
  }

  iterator end() noexcept
  {
    return iterator(data() + m_size);
  }

  ELEMENT_TYPE* lbegin() noexcept
  {
    void *addr; 
    dart_gptr_t gptr = m_dart_gptr;

    dart_gptr_setunit(&gptr, m_myid);
    dart_gptr_getaddr(gptr, &addr);
    return (ELEMENT_TYPE*)(addr);
  }

  ELEMENT_TYPE* lend() noexcept
  {
    void *addr; 
    dart_gptr_t gptr = m_dart_gptr;

    dart_gptr_setunit(&gptr, m_myid);
    dart_gptr_incaddr(&gptr, m_lsize*sizeof(ELEMENT_TYPE));
    dart_gptr_getaddr(gptr, &addr);
    return (ELEMENT_TYPE*)(addr);
  }


  void forall(std::function<void(long long)> func) 
  {
    m_pattern.forall(func);
  }


#if 0
  iterator lbegin() noexcept
  {
    // xxx needs fix
    return iterator(data() + m_team.myid()*m_lsize);
  }

  iterator lend() noexcept
  {
    // xxx needs fix
    size_type end = (m_team.myid()+1)*m_lsize;
    if( m_size<end ) end = m_size;
    
    return iterator(data() + end);
  }
#endif

  reference operator[](size_type n)
  {
    return begin()[n];
  }

  reference at(size_type pos)
  {
    if( !(pos < size()) ) 
      throw std::out_of_range("Out of range");

    return operator[](pos);
  }

  bool islocal(size_type n)
  {
    return m_pattern.index_to_unit(n)==m_myid;
  }

  template<int level>
  dash::HView<Array<ELEMENT_TYPE, DIM>, level, DIM> hview() {
    return dash::HView<Array<ELEMENT_TYPE, DIM>, level, DIM>(*this);
  }
};

} // namespace dash

#endif /* DASH_ARRAY_H_INCLUDED */
