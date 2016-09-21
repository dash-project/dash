#include <libdash.h>
#include <iostream>

#ifdef DASH_ENABLE_PMEM

// For more information on HDF5 files see
// https://www.hdfgroup.org/HDF5

#define POOLNAME "my.pool.pmem"

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  
  using value_t = int;
  using persistent_allocator_type = dash::allocator::PersistentMemoryAllocator<value_t>;

  dash::finalize();

  return EXIT_SUCCESS;
}

#else

int main(int argc, char * argv[])
{
	std::cout << "To run this example build DASH with PMEM support" << std::endl;
	return EXIT_SUCCESS;
}

#endif // DASH_ENABLE_HDF5
