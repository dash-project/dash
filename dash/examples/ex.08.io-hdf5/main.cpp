#include <libdash.h>
#include <iostream>

// For more information on HDF5 files see
// https://www.hdfgroup.org/HDF5


using std::cout;
using std::cerr;
using std::endl;

#define FILENAME "example.hdf5"

typedef dash::Pattern<1, dash::ROW_MAJOR, long> pattern_t;
typedef dash::Array<int, long, pattern_t>       array_t;

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  long      extent = 1000;

	pattern_t pattern_a(extent, dash::TILE(10));
	pattern_t pattern_b(extent, dash::TILE(7));
  array_t   array_a(pattern_a);
	array_t   array_b(pattern_b);

	// Fill Array
	dash::fill(array_a.begin(), array_a.end(), dash::myid());
	dash::fill(array_b.begin(), array_a.end(), dash::myid() * 10);

	// Write Array to HDF5 file using defaults
	{
		dash::io::StoreHDF::write(array_a, FILENAME, "data");
		dash::barrier();
	}
	// Restore values from HDF5 dataset. Pattern gets reconstructed from
	// hdf5 metadata
	{
		// Use delayed allocation
		array_t array_c;
		dash::io::StoreHDF::read(array_c, FILENAME, "data");
	}

	// OK, that was easy. Now let's have a slightly more complex setup
	// Convert between two patterns
	//
	{
		// pass allocated array to define custom pattern
		array_t array_c(pattern_b); // tilesize=7
		dash::io::StoreHDF::read(array_c, FILENAME, "data");
		// Cout pattern
	}

	// Store multiple datasets in single file
	{
		// use options object to add a dataset instead
		// of overwriting the hdf5 file
		auto fopts = dash::io::StoreHDF::get_default_options();
		fopts.overwrite_file = false; // Do not overwrite existing file

		dash::io::StoreHDF::write(array_b, FILENAME, "temperature", fopts);
		dash::barrier();
	}

	// Update dataset. IMPORTANT: the dataset extens must not change!
	{
		auto fopts = dash::io::StoreHDF::get_default_options();
		fopts.overwrite_file = false;
		fopts.modify_dataset = true;

		dash::io::StoreHDF::write(array_a, FILENAME, "temperature", fopts);
		dash::barrier();
	}

	// Clean up
	remove(FILENAME);

  dash::finalize();

  return EXIT_SUCCESS;
}
