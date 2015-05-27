/* 
 * dash-lib/Shared.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef PATTERN1D_H_INCLUDED
#define PATTERN1D_H_INCLUDED

#include <assert.h>
#include <functional>
#include "Team.h"

namespace dash
{

// distribution specification
struct DistSpec   
{
  enum disttype {BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
		 CYCLIC,       // = BLOCKCYCLIC(1)    
		 BLOCKCYCLIC}; // general blocked distribution
  
  disttype   type;
  long long  blocksz;
};

struct RangeSpec
{
  long long begin;
  long long nelem;    // size of the index space
};

struct ExtentSpec
{
  RangeSpec range;
  DistSpec  dist;
};


constexpr DistSpec BLOCKED {DistSpec::BLOCKED, -1};
constexpr DistSpec CYCLIC {DistSpec::CYCLIC, -1}; 
constexpr DistSpec BLOCKCYCLIC(int bs)
{
  return {DistSpec::BLOCKCYCLIC, bs}; 
}


constexpr ExtentSpec EXTENT(long long b, long long e, 
			    DistSpec ds=BLOCKED)
{
  return {{b,e}, ds};
}

constexpr ExtentSpec EXTENT(size_t size, 
			    DistSpec ds=BLOCKED)
{
  return {{0,(long long)size-1}, ds};
}


class Pattern1D
{
private:
  DistSpec     m_dist;
  RangeSpec    m_range;
  
  dash::Team&  m_team;
  long long    m_nunits;

private:

  inline long long modulo(long long i, long long k) const {
    long long res = i % k;
    if(res<0) res+=k;
    return res;
  }

public:
  
  Pattern1D(long long nelem, 
	    DistSpec ds=BLOCKED,
	    dash::Team& team = dash::Team::All()) : m_team(team)
  { 
    m_dist        = ds;
    m_range.nelem = nelem;
    m_nunits      = team.size();
    
    switch( ds.type ) {
    case DistSpec::BLOCKED:
      m_dist.blocksz = nelem/m_nunits+(nelem%m_nunits>0?1:0);
      break;
      
    case DistSpec::CYCLIC:
      m_dist.blocksz=1;
      break;
      
    case DistSpec::BLOCKCYCLIC:
      m_dist.blocksz=ds.blocksz;
      break;
    }      

    if( m_range.nelem!=0 ) {
      assert(m_dist.blocksz>0);
      assert(m_range.nelem>0);
      assert(m_nunits>0);
      assert(m_dist.blocksz<=m_range.nelem);
    }
  }

  // delegating constructor
  Pattern1D(ExtentSpec es,
	    dash::Team& team = dash::Team::All()) : 
    
    Pattern1D(es.range.nelem,
	      es.dist, 
	      team)
  {}

#if 0
  template<typename... Exts>
  explicit Pattern1D(Exts... exts) : m_extent{exts...} , m_team(dash::Team::All())
  {}
#endif 

  dash::Team& team() const {
    return m_team; 
  }

  // TODO: 
  // - function to check that the pattern specification is 
  //   the same across the team

  long long index_to_unit( long long i ) const {
    // i -> [0, nunits)
    long long idx       = modulo(i, m_range.nelem);

    long long blockid   = idx / m_dist.blocksz;
    long long blockunit = blockid % m_nunits;
    
    assert(0<=blockunit && blockunit<m_nunits);

    return blockunit;
  }

  long long index_to_elem( long long i ) const {
    // i -> [0, nelem)
    long long idx = modulo(i, m_range.nelem);

    long long blockid   = idx / m_dist.blocksz;
    long long blockoffs = blockid / m_nunits;
    long long elemoffs  = m_dist.blocksz * blockoffs + 
      idx % m_dist.blocksz;
    
    return elemoffs;
  }

  long long index_to_block( long long i ) {
    // i -> [0, nelem)
    long long idx = modulo(i, m_range.nelem);
    
    long long blockid   = idx / m_dist.blocksz;
    long long blockoffs = blockid / m_nunits;
    
    return blockoffs;
  }  

  // max. number of blocks per unit
  long long max_blocks_per_unit() const {
    long long res=-1;
    
    switch( m_dist.type ) {
    case DistSpec::BLOCKED:
      res = 1;
      break;
      
    case DistSpec::CYCLIC:
      res = (m_range.nelem)/m_nunits + 
	((m_range.nelem)%m_nunits>0?1:0);
      break;
      
    case DistSpec::BLOCKCYCLIC:
      res = (m_range.nelem)/m_dist.blocksz + 
	((m_range.nelem)%m_dist.blocksz>0?1:0);
      
      res = res/m_nunits+(res%m_nunits>0?1:0);
      break;
    }      
    
    assert(res>0);
    return res;
  }

  // max. number of elements per unit
  // (always a multiple of the blocksize)
  long long max_elem_per_unit() const {
    long long res = max_blocks_per_unit()*m_dist.blocksz;
    return res;
  }
  
  long long nelem() const {
    return m_range.nelem;
  }

  long long nunits() const {
    return m_nunits;
  }
    
  long long unit_and_elem_to_index( long long unit, 
				    long long elem ) 
  {
    long long blockoffs = elem / m_dist.blocksz + 1;
    long long i = (blockoffs-1) * m_dist.blocksz * m_nunits +
      unit * m_dist.blocksz + elem % m_dist.blocksz;
    
    long long idx = modulo(i, m_range.nelem);
    
    if( i<0 || i>=m_range.nelem ) // check if i in range
      return -1;
    else
      return idx;
  }
  
  Pattern1D& operator= (const Pattern1D& other)
  {
    if (this != &other) {
      m_dist   = other.m_dist;
      m_range  = other.m_range;
      m_nunits = other.m_nunits;
    }
    return *this;
  }
  
  void forall(std::function<void(long long)> func)
  {
    for( long long i=0; i<m_range.nelem; i++ ) {
      long long idx = unit_and_elem_to_index(m_team.myid(), i);
      if( idx<0  ) {
	break;
      }
      func(idx);
    }
  } 

  DistSpec distspec() const {
    return m_dist;
  }
  
};


} // namespace dash


#endif /* PATTERN1D_H_INCLUDED */
