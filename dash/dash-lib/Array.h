
#ifndef DASH_ARRAY_H_INCLUDED
#define DASH_ARRAY_H_INCLUDED

#include <stdexcept>

#include "GlobPtr.h"
#include "Team.h"
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

template<typename ELEMENT_TYPE>
class array
{
public:
  typedef ELEMENT_TYPE value_type;

  // NO allocator_type 
  typedef size_t size_type;
  typedef size_t difference_type;

  typedef       GlobPtr<value_type>         iterator;
  typedef const GlobPtr<value_type>   const_iterator;
  typedef std::reverse_iterator<      iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>       reference;
  typedef const GlobRef<value_type> const_reference;

  typedef       GlobPtr<value_type>       pointer;
  typedef const GlobPtr<value_type> const_pointer;

private:
  dash::Team&  m_team;
  dart_gptr_t  m_gptr;
  size_type    m_size;  // total size (# of elements)
  size_type    m_lsize; // local size (# of local elements)
  size_type    m_realsize;
  pointer*     m_ptr;


public: 
  array(size_t nelem, Team &t=dash::TeamAll) : m_team(t) 
  {
    // array is a friend of Team and can access
    // the protected member m_dartid
    dart_team_t teamid = t.m_dartid;

    size_t lel = nelem/(m_team.size());
    if( nelem%m_team.size()!=0 ) {
      lel+=1;
    }
    size_t lsz = lel * sizeof(value_type);

    //fprintf(stderr, "Allocating memory of local-size %d\n", lsz);

    dart_team_memalloc_aligned(teamid, lsz, &m_gptr);
    m_ptr = new GlobPtr<value_type>(teamid, m_gptr, lel);
    
    m_size = nelem;
    m_lsize = lel;
    m_realsize = lel * m_team.size();
  }

  constexpr size_type size() const noexcept
  {
    return m_size;
  }
  
  constexpr size_type max_size() const noexcept
  {
    return m_realsize;
  }

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

  iterator lbegin() noexcept
  {
    return iterator(data() + m_team.myid()*m_lsize);
  }

  iterator lend() noexcept
  {
    size_type end = (m_team.myid()+1)*m_lsize;
    if( m_size<end ) end = m_size;
    
    return iterator(data() + end);
  }

  reference operator[](size_type n)
  {
    return begin()[n];
    //return m_ptr[n];
  }

};

} // namespace dash

#endif /* DASH_ARRAY_H_INCLUDED */
