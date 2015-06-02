/* 
 * dash-lib/Shared.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef SHARED_H_INCLUDED
#define SHARED_H_INCLUDED

#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>

namespace dash {

template<typename ELEMENT_TYPE>
class Shared {
public:
  typedef ELEMENT_TYPE  value_type;
  typedef size_t        size_type;
  typedef size_t        difference_type;  

  typedef       GlobRef<value_type>       reference;
  typedef const GlobRef<value_type> const_reference;
  
  typedef       GlobPtr<value_type>       pointer;
  typedef const GlobPtr<value_type> const_pointer;

private:
  typedef dash::GlobMem<value_type> GlobMem;
  GlobMem *     m_globmem; 
  dash::Team &  m_team;
  pointer       m_ptr;

public:
  Shared(Team & t = dash::Team::All())
  : m_team(t) {
    if (m_team.myid() == 0) {
      m_globmem = new GlobMem(1);
      m_ptr     = m_globmem->begin();
    }
    dart_bcast(
      &m_ptr,
      sizeof(pointer),
	    0,
      m_team.dart_id());
  }
  
  ~Shared() {
    if (m_team.myid() == 0) {
      delete m_globmem;
    }
  }

  void set(ELEMENT_TYPE val) noexcept {
    *m_ptr = val;
  }

  reference get() noexcept {
    return *m_ptr;
  }

};

} // namespace dash

#endif /* SHARED_H_INCLUDED */
