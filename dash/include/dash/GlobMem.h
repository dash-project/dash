/* 
 * dash-lib/GlobMem.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef GLOBMEM_H_INCLUDED
#define GLOBMEM_H_INCLUDED

#include <dash/dart/if/dart.h>
#include <dash/GlobPtr.h>
#include <dash/Team.h>

namespace dash
{

enum class GlobMemKind {
  COLLECTIVE, LOCAL };

constexpr GlobMemKind COLLECTIVE {GlobMemKind::COLLECTIVE};
constexpr GlobMemKind COLL       {GlobMemKind::COLLECTIVE};
constexpr GlobMemKind LOCAL      {GlobMemKind::LOCAL};


template<typename T>
void put_value(const T& newval, const GlobPtr<T>& gptr)
{
  //fprintf(stderr, "put_value %d\n", newval);
  
  // BLOCKING !!
  dart_put_blocking(gptr.dartptr(), 
		    (void*)&newval, sizeof(T)); 
}

template<typename T>
void get_value(T* ptr, const GlobPtr<T>& gptr)
{ 
  // BLOCKING !!
  dart_get_blocking(ptr, gptr.dartptr(), 
		    sizeof(T));
  //fprintf(stderr, "get_value %d\n", ptr);
}



template<typename TYPE>
class GlobMem
{
private:
  dart_gptr_t   m_begptr;   
  dart_team_t   m_teamid;
  size_t        m_nunits;
  size_t        m_nlelem;
  GlobMemKind   m_kind;

public:
  GlobMem(
    Team & team,
    size_t nlelem // # of local elements
  ) {
    m_begptr = DART_GPTR_NULL;
    m_teamid = team.dart_id();
    m_nlelem = nlelem;
    m_kind   = COLLECTIVE;
    
    dart_team_size(m_teamid, &m_nunits);
    
    size_t lsize = sizeof(TYPE)*nlelem;
    
    dart_team_memalloc_aligned(m_teamid, 
			       lsize, &m_begptr);
  }

  GlobMem(size_t nlelem) // #of local elements
  {
    m_begptr = DART_GPTR_NULL;
    m_teamid = DART_TEAM_NULL;
    m_nlelem = nlelem;
    m_nunits = 1;
    m_kind   = LOCAL;

    size_t lsize = sizeof(TYPE)*nlelem;

    dart_memalloc(lsize, &m_begptr);
  }

  ~GlobMem() {
    if (!DART_GPTR_ISNULL(m_begptr)) {
      if (m_kind == COLLECTIVE) {
        dart_team_memfree(m_teamid, m_begptr);
      } else {
        dart_memfree(m_begptr);
      } 
    }
  }

  GlobPtr<TYPE> begin() {
    return GlobPtr<TYPE>(m_begptr);
  }

  template<typename T=TYPE>
  void put_value(const T& newval, size_t idx)
  {
    // idx to gptr
  }

  template<typename T=TYPE>
  void get_value(T* ptr, size_t idx)
  {
  }

  GlobPtr<TYPE> get_globptr(size_t unit, size_t idx) 
  {
    //fprintf(stderr, "get_globptr\n");
    dart_unit_t lunit, gunit; 
    
    dart_gptr_t gptr = m_begptr;
    dart_team_unit_g2l(m_teamid, gptr.unitid, &lunit);
    lunit = (lunit + unit) % m_nunits;
    dart_team_unit_l2g(m_teamid, lunit, &gunit);
    
    dart_gptr_setunit(&gptr, gunit);
    dart_gptr_incaddr(&gptr, idx * sizeof(TYPE));

    return GlobPtr<TYPE>(gptr);
  }
};

template<typename T>
GlobPtr<T> memalloc(size_t nelem) 
{
  dart_gptr_t gptr;
  size_t lsize = sizeof(T) * nelem;
  
  dart_memalloc(lsize, &gptr);
  return GlobPtr<T>(gptr);
}


}; // namespace dash

#endif /* GLOBMEM_H_INCLUDED */

