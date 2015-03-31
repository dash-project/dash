
#ifndef MEMACCESS_H_INCLUDED 
#define MEMACCESS_H_INCLUDED 

#include <iostream>
#include <sstream>
#include <string>

#include "Debug.h"


namespace dash
{

template<typename T>
class MemAccess 
{
private:
  dart_gptr_t    m_begptr;  // beg. of allocation
  dart_team_t    m_teamid;
  size_t         m_nunits; 
  size_t         m_nlelem;  // number of *local* elements
  
public:
  explicit MemAccess( dart_team_t teamid,
		      dart_gptr_t begptr, 
		      size_t nlelements  );
  
  void get_value(T* ptr, size_t unit, size_t idx) const;
  void put_value(const T& newval, size_t unit, size_t idx) const;
  
  bool equals( const MemAccess<T>& other ) const;

  std::string to_string() const;

  MemAccess<T>& operator=(const MemAccess<T>& other);

private:
  
  // determine the DART gptr for the given index
  dart_gptr_t dart_gptr(size_t unit, size_t idx) const;
  
};

template<typename T>
MemAccess<T>::MemAccess(dart_team_t teamid,
			dart_gptr_t begptr,
			size_t nlelem   ) :
  m_begptr(begptr)
{
  m_teamid = teamid;
  m_nlelem = nlelem;
  
  dart_team_size(m_teamid, &m_nunits);
  
  //fprintf(stderr, "MemAccess created for team %d\n", teamid);
}


template<typename T>
void MemAccess<T>::get_value(T* ptr, 
			     size_t unit, size_t idx) const
{
  dart_gptr_t gptr = dart_gptr(unit, idx);
  
  // BLOCKING !!
  dart_get_blocking(ptr, gptr, sizeof(T));

#ifdef DASH_DEBUG
  char buf[200];
  GPTR_SPRINTF(buf, gptr);
  fprintf(stderr, "get_value %d at %s\n", (*ptr), buf);
#endif 
}

template<typename T>
void MemAccess<T>::put_value(const T& newval, 
			     size_t unit, size_t idx) const
{
  dart_gptr_t gptr = dart_gptr(unit, idx);

#ifdef DASH_DEBUG
  char buf[200];
  GPTR_SPRINTF(buf, gptr);
  fprintf(stderr, "put_value %d at %s\n", newval, buf);
#endif  

  // BLOCKING!!
  dart_put_blocking(gptr, (void*) &newval, sizeof(T));  
}


template<typename T>
dart_gptr_t MemAccess<T>::dart_gptr(size_t unit, size_t idx) const
{
  dart_unit_t lunit, gunit; 
  
  dart_gptr_t gptr=m_begptr;
  dart_team_unit_g2l(m_teamid, gptr.unitid, &lunit);
  lunit = (lunit+unit)%m_nunits;
  dart_team_unit_l2g(m_teamid, lunit, &gunit);
  
  dart_gptr_setunit(&gptr, gunit);
  dart_gptr_incaddr(&gptr, idx*sizeof(T));

  return gptr;
}


template<typename T>
std::string MemAccess<T>::to_string() const
{
  std::ostringstream oss;
  oss << "MemAccess[m_teamid:" << m_teamid;
  oss << ",m_nlelem:"<< m_nlelem;
  oss << "]";
  return oss.str();
}

template<typename T>
bool MemAccess<T>::equals(const MemAccess<T>& other) const
{
  bool equal = DART_GPTR_EQUAL(m_begptr, other.m_begptr);
  return equal;
}
 

 template<typename T>
   dash::MemAccess<T>&  dash::MemAccess<T>::operator=(const dash::MemAccess<T>& other) 
   {
     m_begptr = other.m_begptr;
     m_teamid = other.m_teamid;
     m_nlelem = other.m_nlelem;
     return *this;
   }

} // namespace dash

#endif /* MEMACCESS_H_INCLUDED  */
