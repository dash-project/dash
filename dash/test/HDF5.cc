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

TEST_F(HDFTest, StoreDashArray)
{
	// Pattern for arr2 array
	size_t nunits   				= dash::Team::All().size();
	size_t tilesize 				= 1024*1024;
	size_t blocks_per_unit  = 32;
	size_t size						 	= nunits * tilesize * blocks_per_unit;
	long	 mbsize_total			= (size * sizeof(uvalue_t)) / (1024*1024);
	long	 mbsize_unit			= mbsize_total / nunits;
 
	// Create arrays
	uarray_t arr1(size, dash::TILE(tilesize));
	dash::Array<uvalue_t> arr2;
	
	// Fill Array
	for(int i=0; i<arr1.local.size(); i++){
		arr1.local[i] = i;
	}

	dash::barrier();
	
	if(dash::myid() == 0){
		std::cout << "Estimated memory per rank: " << mbsize_unit << "MB" << std::endl;
		std::cout << "Estimated memory total: " << mbsize_total << "MB" << std::endl;
		std::cout << "Array filled, begin hdf5 store" << std::endl;
	}
	dash::util::StoreHDF::write(arr1, "test.hdf5", "daten");

	dash::barrier();
	
	if(dash::myid() == 0){
		std::cout << "Array successfully written " << std::endl;
	}	
	dash::barrier();
	//delete[] &arr1; // freezes or segfaults

	dash::util::StoreHDF::read(arr2, "test.hdf5", "daten");
	
	dash::barrier();
	for(int i=0; i<arr2.local.size(); i++){
		ASSERT_EQ_U(static_cast<int>(arr2[i]),i);
	}
}

#endif // DASH_ENABLE_HDF5
