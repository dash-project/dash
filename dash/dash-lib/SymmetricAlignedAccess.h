#ifndef SYMMETRIC_ALIGNED_ACCESS_H_INCLUDED
#define SYMMETRIC_ALIGNED_ACCESS_H_INCLUDED

#include <stdexcept>

#include "dart.h"
#include "Types.h"

namespace dash
{

template< typename T>
class SymmetricAlignedAccess
{
  // KF template< class U> friend class GlobRef;
  // KF template< class U> friend class GlobPtr;
  
private:
  dart_team_t     m_teamid;
  dart_gptr_t     m_begin;
  dash::gsize_t   m_index;
  dash::lsize_t   m_nlocalelements;
  
public:
  explicit SymmetricAlignedAccess( dart_team_t   teamid,
				   dart_gptr_t   begin,  // ptr to beginning of allocation
				   dash::lsize_t nelem,  // number of local elements
				   dash::gsize_t index=0 );
  		   
  virtual ~SymmetricAlignedAccess();

  void get_value(T* value_out) const;
  
  void put_value(const T& newValue) const;
  
  void increment(dash::gsize_t i = 1);
  void decrement(dash::gsize_t i = 1);

  bool equals(const SymmetricAlignedAccess<T>& other) const;
  
  bool equals_ignore_index(const SymmetricAlignedAccess<T>& other) const;
  
  bool gt(const SymmetricAlignedAccess<T>& other) const;
  
  bool lt(const SymmetricAlignedAccess<T>& other) const;
  
  gptrdiff_t difference(const SymmetricAlignedAccess<T>& other) const;
  

private:

  dart_gptr_t actual_ptr() const;

};


template<typename T>
SymmetricAlignedAccess<T>::SymmetricAlignedAccess( dart_team_t   teamid,
						   dart_gptr_t   begin,
						   dash::lsize_t nelem,
						   dash::gsize_t index /*=0*/ )
{
  m_teamid = teamid;
  m_begin = begin;
  m_index = index;
  m_nlocalelements = nelem;
}



template<typename T>
SymmetricAlignedAccess<T>::~SymmetricAlignedAccess()
{
}

template<typename T>
void SymmetricAlignedAccess<T>::get_value(T* value_out) const
{
  dart_get_blocking(value_out, actual_ptr(), sizeof(T));
}

template<typename T>
void SymmetricAlignedAccess<T>::put_value(const T& newValue) const
{
  dart_put_blocking(actual_ptr(), (void*) &newValue, sizeof(T));
}

template<typename T>
void SymmetricAlignedAccess<T>::increment(dash::gsize_t numSteps /* = 1 */)
{
  m_index += numSteps;
}

template<typename T>
void SymmetricAlignedAccess<T>::decrement(dash::gsize_t numSteps /* = 1 */)
{
  m_index -= numSteps;
}

template<typename T>
dart_gptr_t SymmetricAlignedAccess<T>::actual_ptr() const
{
  dart_unit_t unit = (dart_unit_t) (m_index / m_nlocalelements);
  dash::lsize_t offs = (dash::lsize_t) (m_index % m_nlocalelements);
  
  dart_gptr_t gptr=m_begin;
  dart_gptr_setunit(&gptr, unit);
  dart_gptr_incaddr(&gptr, offs*sizeof(T));
  
  return gptr;
}

template<typename T>
bool SymmetricAlignedAccess<T>::equals(const SymmetricAlignedAccess<T>& other) const
{
  return this->equals_ignore_index(other) && m_index == other.m_index;
}

template<typename T>
bool SymmetricAlignedAccess<T>::equals_ignore_index(const SymmetricAlignedAccess<T>& other) const
{
#if 0
  return m_begin.unitid == other.m_begin.unitid
    && m_begin.segid == other.m_begin.segid
    && m_begin.flags == other.m_begin.flags
    && m_begin.offset == other.m_begin.offset
    && m_local_size == other.m_local_size && m_teamid == other.m_teamid;
#endif
  return true;
}

template<typename T>
bool SymmetricAlignedAccess<T>::lt(const SymmetricAlignedAccess<T>& other) const
{
  if (!this->equals_ignore_index(other))
    throw std::invalid_argument("incompatible SymmetricAlignedAccess-objects");
  return m_index < other.m_index;
}

template<typename T>
bool SymmetricAlignedAccess<T>::gt(const SymmetricAlignedAccess<T>& other) const
{
  return other.lt(*this);
}

template<typename T>
gptrdiff_t SymmetricAlignedAccess<T>::difference(
						    const SymmetricAlignedAccess<T>& other) const
{
  if (!this->equals_ignore_index(other))
    throw std::invalid_argument("incompatible SymmetricAlignedAccess-objects");
  return gptrdiff_t(m_index) - gptrdiff_t(other.m_index);
}



}




#endif /* SYMMETRIC_ALIGNED_ACCESS_H_INCLUDED */
