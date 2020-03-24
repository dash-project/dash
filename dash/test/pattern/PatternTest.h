#ifndef DASH__TEST__PATTERN_TEST_H_
#define DASH__TEST__PATTERN_TEST_H_

#include "../TestBase.h"

/**
 * Template test function for pattern functionalities
 */


template<typename PatternType, typename ... Args>
void test_pattern(typename PatternType::size_type array_size, Args &&... args) // array_size is first dim size 
{
  using namespace std;

  typedef typename PatternType::size_type          SizeType;
  typedef typename PatternType::index_type         IndexType;

  typedef typename PatternType::distribution_spec  DistributionSpecType; 
  typedef typename PatternType::team_spec          TeamSpecType;
  typedef typename PatternType::size_spec          SizeSpecType;

  const SizeType dims = PatternType::ndim();

  auto num_units = dash::Team::All().size();
  auto unit_id   = dash::myid();

  std::array<IndexType, dims> coord = {0, static_cast<IndexType>(!(bool) args)...}; // test coordinate {0, 0...}

  // test constructors
  //PatternType pattern1(array_size, args...); // check compiling, not check equality with other constructor due to the difference in DistributionSpec

  PatternType pattern2(SizeSpecType(array_size, args...),
                       DistributionSpecType(dash::TILE(array_size / num_units), dash::TILE(args / num_units)...));
  
  PatternType pattern3(SizeSpecType(array_size, args...),
                       DistributionSpecType(dash::TILE(array_size / num_units), dash::TILE(args / num_units)...),
                       TeamSpecType(num_units, static_cast<int>((bool) args)...)); // a hack to input default 1 for higher dims depended on number of args

  PatternType pattern4 = pattern3;

  ASSERT_EQ_U(pattern2 == pattern3, true);
  ASSERT_EQ_U(pattern3 == pattern4, true);

  //test .extents
  ASSERT_EQ_U(pattern2.extent(0), array_size);

  auto l_extents = pattern2.local_extents();
  ASSERT_EQ_U(accumulate(begin(l_extents), end(l_extents), 1, multiplies<SizeType>()), pattern2.local_size());

  auto g_extents = pattern2.extents();
  ASSERT_EQ_U(accumulate(begin(g_extents), end(g_extents), 1, multiplies<SizeType>()), pattern2.size());

  ASSERT_EQ_U(pattern2.local_size(), pattern2.local_capacity()); //assumed balanced extents
  ASSERT_EQ_U(pattern2.size(), pattern2.capacity());

  //test .at
  ASSERT_EQ_U(pattern2.at(coord), 0);

  //test .unit_at, .local, .local_index, local_at, .global, .global_index, , global_at, .is_local
  auto unit_at_coord = pattern2.unit_at(coord);
  auto l_pos = pattern2.local(coord);
  auto l_index = pattern2.local_index(coord);
  auto g_index = pattern2.global_index(unit_at_coord, l_pos.coords); // unit_at({0, 0}) is unit 0

  ASSERT_EQ_U(pattern2.local_at(l_pos.coords), l_index.index);

  ASSERT_EQ_U(pattern2.global(unit_at_coord, l_pos.coords), coord);
  ASSERT_EQ_U(pattern2.global_at(coord), g_index);

  ASSERT_EQ_U(pattern2.is_local(g_index), unit_id == 0);

  //test .block, .block_at, .local_block, .has_local_elements
  auto g_blockid = pattern2.block_at(coord);
  //auto l_blockid = pattern2.local_block_at(coord);

  auto g_view = pattern2.block(g_blockid);
  //auto l_view = pattern2.local_block(l_blockid.index);

  // member function .includes_index is still currently TODO, it is not implemented yet in CartesianIndexSpace class
  //ASSERT_EQ_U(pattern2.has_local_elements(static_cast<dash::dim_t>(0), static_cast<IndexType>(0), unit_at_coord, g_view), unit_id == 0);

  //test BlockSpec, Local BlockSpec, .blocksize, .max_blocksize
  auto block_spec = pattern2.blockspec();
  auto l_block_spec = pattern2.local_blockspec();

  int nblocks =  static_cast<int>(pow(num_units, dims));

  ASSERT_EQ_U(block_spec.size(), nblocks);
  ASSERT_EQ_U(l_block_spec.size(), nblocks / num_units); //assumed balanced extents

  int blocksize = 1;
  for (int dim = 0; dim < dims; dim++) 
    blocksize *= pattern2.blocksize(dim);
  ASSERT_EQ_U(blocksize, pattern2.max_blocksize());

  //test .coords
  IndexType idx = 0;
  ASSERT_EQ_U(pattern2.coords(idx), coord);


  //test .sizespec, .teamspec
  auto size_spec = pattern2.sizespec();
  auto team_spec = pattern2.teamspec();

  ASSERT_EQ_U(size_spec.extent(0), array_size);
  ASSERT_EQ_U(team_spec.size(), num_units);
}


#endif // DASH__TEST__PATTERN_TEST_H_
