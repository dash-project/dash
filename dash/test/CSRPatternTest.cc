
#include "CSRPatternTest.h"

#include <gtest/gtest.h>

#include <dash/Array.h>

TEST_F(CSRPatternTest, SimpleWriteAndRead){

  using pattern_t = dash::CSRPattern< 1, dash::ROW_MAJOR, int >;
  using extent_t = pattern_t::size_type;
  
  dash::global_unit_t myid = dash::Team::GlobalUnitID( );
  dash::Team & team        = dash::Team::All( );
  size_t nUnits            = team.size();
  
  std::vector<extent_t> local_sizes;
  
  extent_t tmp;
  extent_t sum = 0;
  
  for(size_t unit_idx = 0; unit_idx < nUnits; ++unit_idx) {
      tmp = ( unit_idx + 2 ) * 4;
      local_sizes.push_back(tmp);
      sum += tmp;
  }

  pattern_t pattern( local_sizes );
  dash::Array<size_t, int, pattern_t> array( pattern );
  
  
  EXPECT_EQ_U(local_sizes[myid], array.lsize ( ));
  EXPECT_EQ_U(local_sizes[myid], array.lend() - array.lbegin( ));
  EXPECT_EQ_U(sum              , array.size( ));
  EXPECT_EQ_U(pattern.size( )  , array.size( ));
  
  
  
  for(size_t * i = array.lbegin( ); i < array.lend( ); ++i) {
    *i = myid;
  }
  
  for(size_t * i = array.lbegin( ); i < array.lend( ); ++i){
    EXPECT_EQ_U(*i, myid);
  }
}