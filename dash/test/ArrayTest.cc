#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "ArrayTest.h"


// global var
dash::Array<int> array_global;

TEST_F(ArrayTest, Declaration)
{
  dash::Array<int> array_local(19 * dash::size(), dash::BLOCKED);
}

TEST_F(ArrayTest, AllocateEmptyLocal)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("requires at least 2 units");
  }

  int block_size = 19;
  dash::Array<int> array_local(block_size * (dash::size() - 1),
                               dash::BLOCKCYCLIC(block_size));
}

TEST_F(ArrayTest, DelayedAllocation)
{
  dash::Array<int> array_local;

  DASH_LOG_DEBUG("Delayed allocate");
  array_global.allocate(19 * dash::size(), dash::BLOCKED);
  array_local.allocate(19 * dash::size(), dash::BLOCKED);
}

TEST_F(ArrayTest, SingleWriteMultipleRead)
{
  typedef int                                         value_t;
  typedef typename dash::Array<value_t>::pattern_type pattern_t;

  size_t array_size = _num_elem * _dash_size;
  // Create array instances using varying constructor options
  LOG_MESSAGE("Array size: %d", array_size);
  try {
    // Initialize arrays
    LOG_MESSAGE("Initialize arr1");
    dash::Array<value_t> arr1(array_size);
    LOG_MESSAGE("Initialize arr2");
    dash::Array<value_t> arr2(array_size,
                              dash::BLOCKED);
    LOG_MESSAGE("Initialize arr3");
    dash::Array<value_t> arr3(array_size,
                              dash::Team::All());
    LOG_MESSAGE("Initialize arr4");
    dash::Array<value_t> arr4(array_size,
                              dash::CYCLIC,
                              dash::Team::All());
    LOG_MESSAGE("Initialize arr5");
    dash::Array<value_t> arr5(array_size,
                              dash::BLOCKCYCLIC(12));
    LOG_MESSAGE("Initialize arr6");
    pattern_t pat(array_size);
    dash::Array<value_t> arr6(pat);
    // Check array sizes
    ASSERT_EQ(array_size, arr1.size());
    ASSERT_EQ(array_size, arr2.size());
    ASSERT_EQ(array_size, arr3.size());
    ASSERT_EQ(array_size, arr4.size());
    ASSERT_EQ(array_size, arr5.size());
    ASSERT_EQ(array_size, arr6.size());
    // Fill arrays with incrementing values
    if (_dash_id == 0) {
      DASH_LOG_DEBUG("ArrayTest.SingleWriteMultipleRead",
                     "writing array values");
      for (size_t i = 0; i < array_size; ++i) {
        arr1[i] = i;
        arr2[i] = i;
        arr3[i] = i;
        arr4[i] = i;
        arr5[i] = i;
        arr6[i] = i;
      }
    }
    // Units waiting for value initialization
    DASH_LOG_DEBUG("ArrayTest.SingleWriteMultipleRead",
                   "waiting for unit 0 to write array values");
    dash::Team::All().barrier();
    // Read and assert values in arrays
    for (size_t i = 0; i < array_size; ++i) {
      ASSERT_EQ_U(i, static_cast<value_t>(arr1[i]));
      ASSERT_EQ_U(i, static_cast<value_t>(arr2[i]));
      ASSERT_EQ_U(i, static_cast<value_t>(arr3[i]));
      ASSERT_EQ_U(i, static_cast<value_t>(arr4[i]));
      ASSERT_EQ_U(i, static_cast<value_t>(arr5[i]));
      ASSERT_EQ_U(i, static_cast<value_t>(arr6[i]));
    }
  } catch (dash::exception::InvalidArgument & ia) {
    LOG_MESSAGE("ERROR: %s", ia.what());
    ASSERT_FAIL();
  }
}

TEST_F(ArrayTest, TileSize)
{
  typedef int                                            value_t;
  typedef long long                                      index_t;
  typedef dash::TilePattern<1, dash::ROW_MAJOR, index_t> pattern_t;
  typedef dash::Array<value_t, index_t, pattern_t>       array_t;

  size_t nunits          = dash::Team::All().size();
  size_t tilesize        = 1024;
  size_t blocks_per_unit = 3;
  size_t size            = nunits * tilesize * blocks_per_unit;

  array_t arr(size, dash::TILE(tilesize));

  ASSERT_EQ_U(arr.pattern().blocksize(0),
              arr.pattern().block(0).extent(0));

  auto block_0         = arr.pattern().local_block(0);
  auto block_1         = arr.pattern().local_block(1);

  auto block_0_gend    = block_0.offset(0) + block_0.extent(0);
  auto block_1_gbegin  = block_1.offset(0);

  auto block_glob_dist = block_1_gbegin - block_0_gend;

  // Blocked distribution, expect (nunits-1) blocks between to local blocks.
  EXPECT_EQ_U(tilesize * (nunits - 1),
              block_glob_dist);
}

TEST_F(ArrayTest, PatternAllocate)
{
  typedef dash::default_index_t                        index_t;
  typedef dash::Pattern<1, dash::ROW_MAJOR, index_t> pattern_t;
  const size_t size     = 115;
  const size_t tilesize = 10;
  dash::Array<int, index_t, pattern_t> array;

  // Fill
  std::function< void(const int &, index_t)>
  fill = [&array](int el, index_t i) {
    auto coords = array.pattern().coords(i);
    array[i] = coords[0];
  };

  // Verify
  std::function< void(const int &, index_t)>
  verify = [&array](int el, index_t i) {
    auto coords  = array.pattern().coords(i);
    auto desired = coords[0];
    ASSERT_EQ_U(
      desired,
      el);
  };

  {
    const pattern_t pattern(
      dash::SizeSpec<1>(size),
      dash::DistributionSpec<1>(dash::TILE(tilesize)),
      dash::TeamSpec<1>(),
      dash::Team::All());

    DASH_LOG_DEBUG("ArrayTest.PatternAllocate",
                   "allocating array from pattern");
    array.allocate(pattern);
    DASH_LOG_DEBUG("ArrayTest.PatternAllocate",
                   "array pattern leaving scope");
  }

  DASH_LOG_DEBUG("ArrayTest.PatternAllocate",
                 "filling array");
  // Fill
  dash::for_each_with_index(
    array.begin(),
    array.end(),
    fill);

  dash::for_each_with_index(
    array.begin(),
    array.end(),
    verify);
}

TEST_F(ArrayTest, ConstructorNelemInitializerList)
{
  dash::Array<int> target (4 * dash::size(), {0, 1, 2, 3});

  if (dash::myid() == 0) {
    ASSERT_EQ_U(target[0], 0);
    ASSERT_EQ_U(target[1], 1);
    ASSERT_EQ_U(target[2], 2);
    ASSERT_EQ_U(target[3], 3);
  }
}

TEST_F(ArrayTest, TeamSplit)
{
  auto & team_all = dash::Team::All();
  auto   ext_x    = team_all.size();

  if(team_all.size() < 2){
    SKIP_TEST();
  }
  if(!team_all.is_leaf()){
    LOG_MESSAGE("team is already splitted. Skip test");
    SKIP_TEST();
  }

  dash::barrier();

  LOG_MESSAGE("Splitting team ...");
  auto & myteam = team_all.split(2);
  LOG_MESSAGE("... Team split finished");

  dash::Array<double> array_a(ext_x, myteam);

  array_a.barrier();
  // Check if array is allocated
  ASSERT_NE_U(array_a.lbegin(), nullptr);
  team_all.barrier();
}

