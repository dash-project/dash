#include <libdash.h>
#include <iostream>

// For more information on HDF5 files see
// https://www.hdfgroup.org/HDF5

#define FILENAME "example.hdf5"

using std::cout;
using std::cerr;
using std::endl;

typedef dash::Pattern<1, dash::ROW_MAJOR, long> pattern_t;
typedef dash::Array<int, long, pattern_t>       array_t;

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  long      extent = 1000;
	int       myid   = dash::myid();

	pattern_t pattern_a(extent, dash::TILE(10));
	pattern_t pattern_b(extent, dash::TILE(7));
  array_t   array_a(pattern_a);
	array_t   array_b(pattern_b);

	// Fill Array
	dash::fill(array_a.begin(), array_a.end(), myid);
	dash::fill(array_b.begin(), array_a.end(), myid * 10);

	// Write Array to HDF5 file using defaults
	{
		if(myid == 0 ) { 
			cout << endl << "Write Array A to " << FILENAME << " / data" << endl;
		}
		dash::io::StoreHDF::write(array_a, FILENAME, "data");
		dash::barrier();
	}
	// Restore values from HDF5 dataset. Pattern gets reconstructed from
	// hdf5 metadata
	{
		if(myid == 0){
			cout << endl << "Read " << FILENAME << " / data into Array C," 
					 << " reconstruct pattern" << endl;
		}
		// Use delayed allocation
		array_t array_c;
		dash::io::StoreHDF::read(array_c, FILENAME, "data");
	}

	// OK, that was easy. Now let's have a slightly more complex setup
	// Convert between two patterns
	//
	{
		if(myid == 0){
			cout << endl << "Read " << FILENAME << " / data into already allocated Array C" << endl;
		}
		// pass allocated array to define custom pattern
		array_t array_c(pattern_b); // tilesize=7
		dash::io::StoreHDF::read(array_c, FILENAME, "data");
		if(myid == 0){
			cout << "Array A Pattern: " << array_a.pattern() << endl;
			cout << "Array C Pattern: " << array_c.pattern() << endl;
		}
	}

	// Store multiple datasets in single file
	{
		// use options object to add a dataset instead
		// of overwriting the hdf5 file
		if(myid == 0){
			cout << endl << "Add dataset temperature to " << FILENAME << endl;
		}
		auto fopts = dash::io::StoreHDF::get_default_options();
		fopts.overwrite_file = false; // Do not overwrite existing file

		dash::io::StoreHDF::write(array_b, FILENAME, "temperature", fopts);
		dash::barrier();
	}

	// Update dataset. IMPORTANT: the dataset extents must not change!
	{
		if(myid == 0){
			cout << endl << "Modify " << FILENAME << " / temperature dataset" << endl;
		}
		auto fopts = dash::io::StoreHDF::get_default_options();
		fopts.overwrite_file = false;
		fopts.modify_dataset = true;

		dash::io::StoreHDF::write(array_a, FILENAME, "temperature", fopts);
		dash::barrier();
	}

	// Clean up
	//remove(FILENAME);

  dash::finalize();

  return EXIT_SUCCESS;
}
