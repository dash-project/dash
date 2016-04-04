#ifdef DASH_ENABLE_HDF5

#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "HDF5.h"

#include "limits.h"

typedef int uvalue_t;
typedef dash::Array<
						uvalue_t,
						long> uarray_t;

TEST_F(HDFTest, StoreLargeDashArray)
{
	// Pattern for arr2 array
	size_t nunits   				= dash::Team::All().size();
	size_t tilesize 				= 512*512;
	size_t blocks_per_unit  = 32;
	size_t size						 	= nunits * tilesize * blocks_per_unit;
	long	 mbsize_total			= (size * sizeof(uvalue_t)) / (tilesize);
	long	 mbsize_unit			= mbsize_total / nunits;

	// Add some randomness to the data
	std::srand(time(NULL));	
	int local_secret = std::rand() % 1000;
	int myid				 = dash::myid();

	{ 
		// Create first array
		uarray_t arr1(size, dash::TILE(tilesize));
	
		// Fill Array
		for(int i=0; i<arr1.local.size(); i++){
			arr1.local[i] = local_secret + myid + i;
		}

		dash::barrier();
	
		DASH_LOG_DEBUG("Estimated memory per rank: ", mbsize_unit, "MB");
		DASH_LOG_DEBUG("Estimated memory total: ", mbsize_total, "MB");
		DASH_LOG_DEBUG("Array filled, begin hdf5 store");
		
		dash::util::StoreHDF::write(arr1, "test.hdf5", "data");
		dash::barrier();
	}
	DASH_LOG_DEBUG("Array successfully written ");

	// Create second array
	dash::Array<uvalue_t> arr2;
	dash::barrier();
	dash::util::StoreHDF::read(arr2, "test.hdf5", "data");
	
	dash::barrier();
	for(int i=0; i<arr2.local.size(); i++){
		ASSERT_EQ_U(
			static_cast<int>(arr2.local[i]),
			local_secret + myid + i);
	}
	dash::barrier();
}

TEST_F(HDFTest, StoreMultiDimMatrix)
{
	typedef dash::TilePattern<2>	pattern_t;
	typedef dash::Matrix<
						uvalue_t,
						2,
						long,
						pattern_t>					matrix_t;

	auto numunits =	dash::Team::All().size();
	dash::TeamSpec<2> team_spec(numunits, 1);
	team_spec.balance_extents();

	auto team_extent_x = team_spec.extent(0);
	auto team_extent_y = team_spec.extent(1);

	auto extend_x = 2 * 2 * team_extent_x;
	auto extend_y = 2 * 5 * team_extent_y;

	pattern_t pattern(
							dash::SizeSpec<2>(
								extend_x,
								extend_y),
							dash::DistributionSpec<2>(
								dash::TILE(2),
								dash::TILE(5)),
							team_spec);

	DASH_LOG_DEBUG("Pattern", pattern);

	int myid				 = dash::myid();
	{
		matrix_t mat1(pattern);
		dash::barrier();
		std::cout << "Matrix created" << std::endl;

		// Fill Array
		for(int x=0; x<pattern.local_extent(0); x++){
			for(int y=0; y<pattern.local_extent(1); y++){
				mat1.local[x][y] = myid;
			}
		}
		dash::barrier();
		DASH_LOG_DEBUG("BEGIN STORE HDF");
		dash::util::StoreHDF::write(mat1, "test2.hdf5", "data");
		DASH_LOG_DEBUG("END STORE HDF");
		dash::barrier();
	}
dash::barrier();
}
#if 0
TEST_F(HDFTest, StoreSUMMAMatrix)
{
	// Adopted from SUMMA test case
  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::TeamSpec<2> team_spec(team_size_x, team_size_y);
  team_spec.balance_extents();

  LOG_MESSAGE("Initialize matrix pattern ...");
  auto pattern = dash::make_pattern <
                 dash::summa_pattern_partitioning_constraints,
                 dash::summa_pattern_mapping_constraints,
                 dash::summa_pattern_layout_constraints >(
                   size_spec,
                   team_spec);

}
#endif

#endif // DASH_ENABLE_HDF5
