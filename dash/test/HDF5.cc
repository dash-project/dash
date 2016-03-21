#ifdef DASH_ENABLE_HDF5

#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "HDF5.h"

typedef int uvalue_t;
typedef dash::Array<
						uvalue_t,
						long> uarray_t;

TEST_F(HDFTest, StoreLargeDashArray)
{
	// Pattern for arr2 array
	size_t nunits   				= dash::Team::All().size();
	size_t tilesize 				= 1024*1024;
	size_t blocks_per_unit  = 32;
	size_t size						 	= nunits * tilesize * blocks_per_unit;
	long	 mbsize_total			= (size * sizeof(uvalue_t)) / (1024*1024);
	long	 mbsize_unit			= mbsize_total / nunits;

	{ 
		// Create first array
		uarray_t arr1(size, dash::TILE(tilesize));
	
		// Fill Array
		for(int i=0; i<arr1.local.size(); i++){
			arr1.local[i] = i;
		}

		dash::barrier();
	
		DASH_LOG_DEBUG("Estimated memory per rank: ", mbsize_unit, "MB");
		DASH_LOG_DEBUG("Estimated memory total: ", mbsize_total, "MB");
		DASH_LOG_DEBUG("Array filled, begin hdf5 store");
		
		dash::util::StoreHDF::write(arr1, "test.hdf5", "daten");
		dash::barrier();
	}
	DASH_LOG_DEBUG("Array successfully written ");

	// Create second array
	dash::Array<uvalue_t> arr2;
	dash::barrier();
	dash::util::StoreHDF::read(arr2, "test.hdf5", "daten");
	
	dash::barrier();
	for(int i=0; i<arr2.local.size(); i++){
		ASSERT_EQ_U(static_cast<int>(arr2.local[i]),i);
	}
}

#endif // DASH_ENABLE_HDF5
