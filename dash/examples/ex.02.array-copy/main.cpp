/* 
 * \example ex.02.array-copy\main.cpp
 *
 * Example illustrating the use of \c dash::copy with a local array
 * as destination.
 */

#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstddef>
#include <vector>

#ifdef DASH_ENABLE_IPM
#include <mpi.h>
#endif

#define DASH__ALGORITHM__COPY__USE_WAIT
#include <libdash.h>

using std::cout;
using std::endl;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dash::global_unit_t myid = dash::myid();
  size_t num_units       = dash::Team::All().size();
  size_t num_elems_unit  = (argc > 1)
                           ? static_cast<size_t>(atoi(argv[1]))
                           : 20;
  size_t start_index     = (argc > 2)
                           ? static_cast<size_t>(atoi(argv[2]))
                           : 10;
  size_t num_elems_copy  = (argc > 3)
                           ? static_cast<size_t>(atoi(argv[3]))
                           : 20;
  size_t num_elems_total = num_elems_unit * num_units;

  dash::Array<int> array(num_elems_total);

  if (myid == 0) {
    cout << endl
         << "Elements per unit: " << num_elems_unit  << endl
         << "Start index:       " << start_index     << endl
         << "Elements to copy:  " << num_elems_copy  << endl;
  }
  
  // fill the local part of the global array each unit is holding with
  // it's DASH ID (\c dash::myid). 
  std::fill(array.lbegin(), array.lend(), myid);

  array.barrier();

  if (myid == 0) {
    cout << "Array size:        " << array.size() << endl;
  }

  // ----------------------------------------------------------------------
  // global-to-local copy:
  // ----------------------------------------------------------------------

  // destination array
  std::vector<int> local_array(num_elems_copy);

  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array.data());

  std::ostringstream ss;
  ss << "Local copy at unit " << myid << ": ";
  for(size_t i = 0; i < num_elems_copy; ++i) {
    ss << local_array[i] << " ";
  }
  ss   << endl;
  cout << ss.str();

  array.barrier();

  // ----------------------------------------------------------------------
  // global-to-global copy:
  // ----------------------------------------------------------------------

  dash::Array<int> src_array(num_elems_total / 2);

  std::fill(src_array.lbegin(), src_array.lend(), (myid + 1) * 10);
  array.barrier();

  dash::copy(src_array.begin(),
             src_array.end(),
             array.begin() + (array.size() / 4));

  array.barrier();


  dash::finalize();

  return 0;
}

