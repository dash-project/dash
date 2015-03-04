
#ifndef DASH_SHARED_H_INCLUDED
#define DASH_SHARED_H_INCLUDED

#include "Team.h"
//#include "Pattern1D.h"
#include "Pattern.h"
#include "GlobPtr.h"
#include "GlobRef.h"

#include "dart.h"

namespace dash
{

template<typename TYPE, size_t DIM>
class Shared
{
public:
  typedef TYPE value_type;
  typedef GlobRef<value_type> reference;
  typedef GlobPtr<value_type, DIM> pointer;

private:
  dash::Team&      m_team;
  dart_unit_t      m_myid;
  dart_gptr_t      m_dart_gptr;
  pointer*         m_ptr;
  dash::Pattern<DIM>    m_pattern;

public:
  Shared(dash::Team& t=dash::Team::All()) : 
    m_team(t),
    m_pattern(1, BLOCKED, t) {
    dart_team_t teamid = m_team.m_dartid;
    
    size_t lsize = sizeof(value_type);
    m_dart_gptr = DART_GPTR_NULL;

    dart_ret_t ret = 
      dart_team_memalloc_aligned(teamid, lsize, &m_dart_gptr);

    m_ptr = new GlobPtr<value_type, DIM>(m_pattern, m_dart_gptr, 0);
  }

  reference operator=(const TYPE& rhs)
  {
    reference ref = (*m_ptr[0])=rhs;
    return ref;
  }

  operator TYPE() const 
  {
    reference ref = (*m_ptr[0]);
    return TYPE(ref);
  }
};



} // namespace dash

#endif /* DASH_ARRAY_H_INCLUDED */
