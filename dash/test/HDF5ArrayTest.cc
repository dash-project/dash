#ifdef DASH_ENABLE_HDF5

#include <libdash.h>

#include <limits.h>
#include <gtest/gtest.h>
#include <unistd.h>

// #include <c++/5/chrono>
#include <chrono>

#include "HDF5ArrayTest.h"
#include "TestBase.h"
#include "TestLogHelpers.h"


typedef int value_t;
typedef dash::Array<value_t, long> array_t;

namespace dio = dash::io::hdf5;

using dash::io::hdf5::StoreHDF;
using dash::io::hdf5::DeviceMode;
using dash::io::hdf5::InputStream;
using dash::io::hdf5::OutputStream;
using dash::io::hdf5::dataset;
using dash::io::hdf5::modify_dataset;
using dash::io::hdf5::store_pattern;
using dash::io::hdf5::restore_pattern;
using dash::io::hdf5::setpattern_key;

/**
 * Fill an dash array with a signatures that contains
 * the global coordinates and a secret which can be the unit id
 * for example
 */
template <
  typename T,
  typename IndexT,
  typename PatternT >
void fill_array(dash::Array<T, IndexT, PatternT> & array, T secret = 0)
{
  std::function< void(const T &, IndexT)>
  f = [&array, &secret](T el, IndexT i) {
    auto coords = array.pattern().coords(i);
    // hack
    *(array.begin() + i) = coords[0] + secret;
  };
  dash::for_each_with_index(
    array.begin(),
    array.end(),
    f);
}

/**
 * Counterpart to fill_array which checks if the given array satisfies
 * the desired signature
 */
template <
  typename T,
  typename IndexT,
  typename PatternT >
void verify_array(dash::Array<T, IndexT, PatternT> & array,
                  T secret = 0)
{
  std::function< void(const T &, IndexT)>
  f = [&array, &secret](T el, IndexT i) {
    auto coords  = array.pattern().coords(i);
    auto desired = coords[0] + secret;
    ASSERT_EQ_U(
      desired,
      el);
  };
  dash::for_each_with_index(
    array.begin(),
    array.end(),
    f);
}

TEST_F(HDF5ArrayTest, StoreLargeDashArray)
{
  // Pattern for arr2 array
  size_t nunits           = dash::Team::All().size();
#ifdef DASH_DEBUG
  size_t tilesize         = 4;
#else
  size_t tilesize         = 512 * 512;
#endif
  size_t blocks_per_unit  = 4; //32;
  size_t size             = nunits * tilesize * blocks_per_unit;
  long   mbsize_total     = (size * sizeof(value_t)) / (tilesize);
  long   mbsize_unit      = mbsize_total / nunits;

  // Add some randomness to the data
  std::srand(time(NULL));
  int local_secret = std::rand() % 1000;
  int myid         = dash::myid();

  {
    // Create first array
    array_t arr1(size, dash::TILE(tilesize));

    // Fill Array
    fill_array(arr1, local_secret);

    dash::barrier();

    DASH_LOG_DEBUG("Estimated memory per rank: ", mbsize_unit, "MB");
    DASH_LOG_DEBUG("Estimated memory total: ", mbsize_total, "MB");
    DASH_LOG_DEBUG("Array filled, begin hdf5 store");

    StoreHDF::write(arr1, _filename, _dataset);
    dash::barrier();
  }
  DASH_LOG_DEBUG("Array successfully written ");

  // Create second array
  dash::Array<value_t> arr2;
  dash::barrier();
  StoreHDF::read(arr2, _filename, _dataset);

  dash::barrier();
  verify_array(arr2, local_secret);
}

TEST_F(HDF5ArrayTest, AutoGeneratePattern)
{
  {
    dash::Array<int> array_a(dash::size() * 2);
    // Fill
    fill_array(array_a);
    dash::barrier();

    // Set option
    auto fopts = StoreHDF::hdf5_options();
    fopts.store_pattern = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b;
  StoreHDF::read(array_b, _filename, _dataset);
  dash::barrier();

  // Verify
  verify_array(array_b);
}

// Import data into a already allocated array
// because array_a and array_b are allocated the same way
// it is expected that each unit remains its local ranges
TEST_F(HDF5ArrayTest, PreAllocation)
{
  int ext_x = dash::size() * 2;
  {
    dash::Array<int> array_a(ext_x);
    // Fill
    fill_array(array_a, static_cast<int>(dash::myid()));
    dash::barrier();

    // Set option
    auto fopts = StoreHDF::hdf5_options();
    fopts.store_pattern = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b(ext_x);
  StoreHDF::read(array_b, _filename, _dataset);
  dash::barrier();

  // Verify
  verify_array(array_b, static_cast<int>(dash::myid()));
}

// Test Stream API
TEST_F(HDF5ArrayTest, OutputStreamOpen)
{
  {
    dash::Array<long> array_a(dash::size() * 2);

    fill_array(array_a);
    dash::barrier();

    auto os  = OutputStream(_filename);
    os   << dio::dataset(_dataset)
         << array_a;
  }
  dash::barrier();
  // Import data
  dash::Array<long> array_b;
  auto is = InputStream(_filename);
  is >> dio::dataset(_dataset) >> array_b;

  verify_array(array_b);
}

TEST_F(HDF5ArrayTest, UnderfilledPattern)
{
  int   ext_x = dash::size() * 5 + 1;
  long  tilesize;
  {
    DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "alloc array_a");
    dash::Array<int> array_a(ext_x);
    tilesize = array_a.pattern().blocksize(0);
    // Fill
    DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "fill array_a");
    fill_array(array_a);
    DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "barrier #1");
    dash::barrier();
    // Set option
    auto fopts = StoreHDF::hdf5_options();
    // Important as recreation should create equal pattern
    fopts.store_pattern = true;

    DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "StoreHDF::write");
    StoreHDF::write(array_a, _filename, _dataset, fopts);
    DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "barrier #2");
    dash::barrier();
  }
  dash::Array<int> array_b;
  DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "StoreHDF::read");
  StoreHDF::read(array_b, _filename, _dataset);
  DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "barrier #3");
  dash::barrier();

  // Verify
  // Check extents
  DASH_ASSERT_EQ(
    ext_x,
    array_b.size(),
    "Array extent does not match input array");
  // Check tilesize
  DASH_ASSERT_EQ(
    tilesize,
    array_b.pattern().blocksize(0),
    "Tilesizes do not match");
  // Verify data
  DASH_LOG_TRACE("HDF5ArrayTest.UnderfilledPattern", "verify array_b");
  verify_array(array_b);
}

TEST_F(HDF5ArrayTest, UnderfilledPatPreAllocate)
{
  int ext_x = dash::size() * 5 + 1;
  {
    dash::Array<int> array_a(ext_x);
    // Fill
    fill_array(array_a);
    dash::barrier();
    // Set option
    auto fopts = StoreHDF::hdf5_options();
    fopts.store_pattern = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b(ext_x);
  StoreHDF::read(array_b, _filename, _dataset);
  dash::barrier();

  // Verify
  // Check extents
  DASH_ASSERT_EQ(
    ext_x,
    array_b.size(),
    "Array extent does not match input array");
  // Verify data
  verify_array(array_b);
}

TEST_F(HDF5ArrayTest, MultipleDatasets)
{
  int    ext_x    = dash::size() * 5;
  int    secret_a = 10;
  double secret_b = 3;
  {
    dash::Array<int>    array_a(ext_x);
    dash::Array<double> array_b(ext_x*2);

    // Fill
    fill_array(array_a, secret_a);
    fill_array(array_b, secret_b);
    dash::barrier();

    // Set option
    StoreHDF::hdf5_options fopts;
    fopts.overwrite_file = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    StoreHDF::write(array_b, _filename, "datasettwo", fopts);
    dash::barrier();
  }
  dash::Array<int>    array_c;
  dash::Array<double> array_d;
  StoreHDF::read(array_c, _filename, _dataset);
  StoreHDF::read(array_d, _filename, "datasettwo");

  dash::barrier();

  // Verify data
  verify_array(array_c, secret_a);
  verify_array(array_d, secret_b);
}

TEST_F(HDF5ArrayTest, ModifyDataset)
{
  int    ext_x    = dash::size() * 5;
  double secret_a = 10;
  double secret_b = 3;
  {
    dash::Array<double> array_a(ext_x);
    dash::Array<double> array_b(ext_x);

    // Fill
    fill_array(array_a, secret_a);
    fill_array(array_b, secret_b);
    dash::barrier();

    // Set option
    auto fopts = StoreHDF::hdf5_options();
    fopts.overwrite_file = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
    // overwrite first data
    fopts.modify_dataset = true;
    StoreHDF::write(array_b, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<double>    array_c;
  StoreHDF::read(array_c, _filename, _dataset);

  dash::barrier();

  // Verify data
  verify_array(array_c, secret_b);
}

TEST_F(HDF5ArrayTest, StreamCreationFlags)
{
  int    ext_x  = dash::size() * 5;
  double secret = 10;
  {
    dash::Array<double> array_a(ext_x);

    // Fill
    fill_array(array_a, secret);
    dash::barrier();

    // Set option

    OutputStream os(_filename, DeviceMode::app);
    os << dio::dataset("settwo")
       << dio::setpattern_key("custom_dash_pattern")
       << dio::store_pattern()
       << array_a
       << dio::modify_dataset()
       << array_a;

    dash::barrier();
  }
  dash::Array<double> array_b;
  InputStream is(_filename);
  is >> dio::dataset("settwo")
     >> dio::setpattern_key("custom_dash_pattern")
     >> dio::restore_pattern()
     >> array_b;

  dash::barrier();

  // Verify data
  verify_array(array_b, secret);
}

TEST_F(HDF5ArrayTest, GroupTest)
{
  int    ext_x    = dash::size() * 5;
  double secret[] = {10,11,12};
  {
    dash::Array<double> array_a(ext_x);
    dash::Array<double> array_b(ext_x);
    dash::Array<double> array_c(ext_x);
  
    // Fill
    fill_array(array_a, secret[0]);
    fill_array(array_b, secret[1]);
    fill_array(array_c, secret[2]);
    dash::barrier();
  
    // Set option
  
    OutputStream os(_filename);
    os << dio::dataset("array_a")
       << array_a
       << dio::dataset("g1/array_b")
       << array_b
       << dio::dataset("g1/g2/array_c")
       << array_c;
  
    dash::barrier();
  }
  dash::Array<double> array_a;
  dash::Array<double> array_b;
  dash::Array<double> array_c;
  InputStream is(_filename);
  is >> dio::dataset("array_a")
     >> array_a
     >> dio::dataset("g1/array_b")
     >> array_b
     >> dio::dataset("g1/g2/array_c")
     >> array_c;
  
  dash::barrier();
  
  // Verify data
  verify_array(array_a, secret[0]);
  verify_array(array_b, secret[1]);
  verify_array(array_c, secret[2]);
}

TEST_F(HDF5ArrayTest, CustomType)
{
  int    ext_x    = dash::size() * 5;
  
  struct value_t { double a; int b; };
  
  value_t fillin;
  fillin.a = 1.0;
  fillin.b = 2;
  
  auto converter = [](){
    hid_t h5tid = H5Tcreate (H5T_COMPOUND, sizeof(value_t));
    H5Tinsert(h5tid, "a_name", HOFFSET(value_t, a), H5T_NATIVE_DOUBLE);
    H5Tinsert(h5tid, "b_name", HOFFSET(value_t, b), H5T_NATIVE_INT);
    return h5tid;
  };
  
  {
    dash::Array<value_t> array_a(ext_x);
  
    // Fill
    dash::fill(array_a.begin(), array_a.end(), fillin);
    dash::barrier();
  
    OutputStream os(_filename);
    os << dio::dataset("array_a")
       << dio::type_converter(converter)
       << array_a;
  
    dash::barrier();
  }
  
  dash::Array<value_t> array_b(ext_x);
  InputStream is(_filename);
  is >> dio::dataset("array_a")
     >> dio::type_converter(converter)
     >> array_b;
  dash::barrier();
  
  std::for_each(array_b.lbegin(), array_b.lend(), [&fillin](value_t & el){
    ASSERT_EQ_U(fillin.a, el.a);
    ASSERT_EQ_U(fillin.b, el.b);
  });
}

/** TODO currently highly experimental */
TEST_F(HDF5ArrayTest, AsyncIO)
{
  int  ext_x  = dash::size() * 1;
#ifndef DASH_DEBUG
  long lext_x = 1024*1024*10; // approx. 40 MB
#else
  long lext_x = ext_x*2;
#endif
  double secret[] = {10, 11, 12};
  {
    dash::Array<double> array_a(ext_x);
    dash::Array<double> array_b(lext_x);
    dash::Array<double> array_c(ext_x);

    // Fill
    fill_array(array_a, secret[0]);
    fill_array(array_b, secret[1]);
    fill_array(array_c, secret[2]);
    dash::barrier();


    // Currently only works if just one container is passed
    OutputStream os(dash::launch::async, _filename);
    os << dio::dataset("array_a")
      << array_a
      << dio::dataset("g1/array_b")
      << array_b
      << dio::dataset("g1/g2/array_c")
      << array_c;
    
    LOG_MESSAGE("Async OS setup");
    // Do some computation intense work
    os.flush();
    LOG_MESSAGE("Async OS flushed");
  }
  
  dash::Array<double> array_a(ext_x);
  dash::Array<double> array_b(lext_x);
  // try unallocated array
  dash::Array<double> array_c;

  // There are still progress problems in async input stream.
  InputStream is(dash::launch::async, _filename);
  is >> dio::dataset("array_a")
    >> array_a
    >> dio::dataset("g1/array_b")
    >> array_b
    >> dio::dataset("g1/g2/array_c")
    >> array_c;
  
  is.flush();

  // Verify data
  verify_array(array_a, secret[0]);
  verify_array(array_b, secret[1]);
  verify_array(array_c, secret[2]);

}

// Run this test after all other tests as it changes the team state
TEST_F(HDF5ArrayTest, TeamSplit)
{
  // TODO Hangs on travis CI
  SKIP_TEST();

  if (dash::size() < 2) {
    SKIP_TEST();
  }

  auto & team_all  = dash::Team::All();
  int    num_split = std::min<int>(team_all.size(), 2);

  if(!team_all.is_leaf()){
    LOG_MESSAGE("team is already splitted. Skip test");
    SKIP_TEST();
  }

  auto & myteam    = team_all.split(num_split);
  LOG_MESSAGE("Splitted team in %d parts, I am %d",
              num_split, myteam.position());

  int    ext_x  = team_all.size() * 5;
  double secret = 10;
  
  if (myteam.position() == 0) {
    {
      dash::Array<double> array_a(ext_x, myteam);
      // Array has to be allocated
      EXPECT_NE_U(array_a.lbegin() , nullptr); 

      fill_array(array_a, secret);
      myteam.barrier();
      LOG_MESSAGE("Team %d: write array", myteam.position());
      OutputStream os(_filename);
      os << dio::dataset("array_a") << array_a;
      LOG_MESSAGE("Team %d: array written", myteam.position());
      myteam.barrier();
    }
  }

  team_all.barrier();

  if (myteam.position() == 1) {
    dash::Array<double> array_a(ext_x, myteam);
    array_a.barrier();
    fill_array(array_a, secret);

    // Array has to be allocated
    EXPECT_NE_U(array_a.lbegin() , nullptr); 

    if(array_a.lbegin() != nullptr){
      LOG_MESSAGE("Team %d: read array", myteam.position());
      InputStream is(_filename);
      is >> dio::dataset("array_a") >> array_a;
      LOG_MESSAGE("Team %d: array read", myteam.position());
      myteam.barrier();
      verify_array(array_a, secret);
    }
  }

  team_all.barrier();
}
#endif // DASH_ENABLE_HDF5
