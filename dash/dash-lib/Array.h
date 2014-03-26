#ifndef DASH_ARRAY_H_INCLUDED
#define DASH_ARRAY_H_INCLUDED

#include <stdexcept>

#include "dart.h"

namespace dash
{
template<typename ELEM_TYPE>
class array
{
public:
  typedef ELEM_TYPE                       value_type;
  typedef GlobPtr<value_type>             pointer;
  typedef const GlobPtr<value_type>       const_pointer;
  typedef GlobRef<value_type>             reference;
  typedef const GlobRef<value_type>       const_reference;
  typedef GlobPtr<value_type>             iterator;
  typedef const GlobRef<value_type>       const_iterator;
  typedef dash::gsize_t                   size_type;
  typedef dash::gptrdiff_t                difference_type;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

private:
  pointer*   m_ptr;
  size_type  m_size;
  int        m_team_id;
  
public:
  // KF XXX: why public?
  dart_gptr_t  m_dart_ptr;
  
  array(size_type size, int team_id)
  {
    m_team_id = team_id;
    m_size    = size;
    
    size_t teamsize;
    dart_team_size(team_id, &teamsize);
    
    if( (size%teamsize) > 0 ) {
      throw 
	std::invalid_argument("size has to be a multiple of dart_team_size(team_id)");
    }
    
    lsize_t local_size = 
      lsize_t(size * sizeof(ELEM_TYPE) / teamsize);

    dart_team_memalloc_aligned(team_id, local_size, &m_dart_ptr );
    
    m_ptr = new GlobPtr<ELEM_TYPE>(team_id, m_dart_ptr, local_size);
  }

  // Capacity.
  constexpr size_type size() const noexcept
  {
    return m_size;
  }
  
  constexpr size_type max_size() const noexcept
  {
    return m_size;
  }

  constexpr bool empty() const noexcept
  {
    return size() == 0;
  }
  
  pointer data() noexcept
  {
    return *m_ptr;
  }
  
  const_pointer data() const noexcept
  {
    return *m_ptr;
  }
  
  // Iterators.
  iterator begin() noexcept
  {
    return iterator(data());
  }
  
  iterator end() noexcept
  {
    return iterator(data() + m_size);
  }

  reference operator[](size_type n)
  {
    return begin()[n];
  }
  
#if 0  
  reverse_iterator rbegin() noexcept
  {
    return reverse_iterator(end());
  }
  
  reverse_iterator rend() noexcept
  {
    return reverse_iterator(begin());
  }
  
  reference operator[](size_type n)
  {
    return (*m_ptr)[n];
  }
  
  reference at(size_type n)
  {
    if (n >= m_size)
      throw std::out_of_range("array::at");
    return (*m_ptr)[n];
  }
  
  reference front()
  {
    return *begin();
  }
  
  reference back()
  {
    return m_size ? *(end() - 1) : *end();
  }  
#endif  
  
};
 
}

#endif /* DASH_ARRAY_H_INCLUDED */
