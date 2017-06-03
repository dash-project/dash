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

template <class ArrayT>
auto initialize_array(ArrayT & array)
-> typename std::enable_if<
              std::is_floating_point<typename ArrayT::value_type>::value,
              void >::type
{
  auto block_size = array.pattern().blocksize(0);
  for (auto li = 0; li != array.local.size(); ++li) {
    auto block_lidx = li / block_size;
    auto block_gidx = (block_lidx * dash::size()) + dash::myid().id;
    auto gi         = (block_gidx * block_size) + (li % block_size);
    array.local[li] = // unit
                      (1.0000 * dash::myid().id) +
                      // local offset
                      (0.0100 * (li+1)) +
                      // global offset
                      (0.0001 * gi);
  }
  array.barrier();
}

template <class ValueRange>
static std::string range_str(
  const ValueRange & vrange) {
  typedef typename ValueRange::value_type value_t;
  std::ostringstream ss;
  auto idx = dash::index(vrange);
  int        i   = 0;
  for (const auto & v : vrange) {
    ss << std::setw(2) << *(dash::begin(idx) + i) << "|"
       << std::fixed << std::setprecision(4)
       << static_cast<const value_t>(v) << " ";
    ++i;
  }
  return ss.str();
}


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dash::global_unit_t myid = dash::myid();
  size_t num_units       = dash::Team::All().size();
  size_t num_elems_unit  = (argc > 1)
                           ? static_cast<size_t>(atoi(argv[1]))
                           : 6;
  size_t num_elems_total = num_elems_unit * num_units;
  size_t start_index     = (argc > 2)
                           ? static_cast<size_t>(atoi(argv[2]))
                           : num_elems_total / 4;
  size_t num_elems_copy  = (argc > 3)
                           ? static_cast<size_t>(atoi(argv[3]))
                           : num_elems_total / 2;

  dash::Array<float> array(num_elems_total);

  if (myid == 0) {
    cout << endl
         << "Elements per unit: " << num_elems_unit  << endl
         << "Start index:       " << start_index     << endl
         << "Elements to copy:  " << num_elems_copy  << endl;
  }
  
  initialize_array(array);

  if (myid == 0) {
    cout << "Array size:        " << array.size() << endl;
    cout << range_str(array) << endl;
  }

  // ----------------------------------------------------------------------
  // global-to-local copy:
  // ----------------------------------------------------------------------

  if (myid == 0) {
    cout << "=== Global to Local =================================" << endl;
  }

  // destination array
  std::vector<float> local_array(num_elems_copy);

  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array.data());

  array.barrier();

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

  if (myid == 0) {
    cout << "=== Global to Global ================================" << endl;
  }

  dash::Array<float> src_array(num_elems_total / 2);

  std::fill(src_array.lbegin(), src_array.lend(), (myid + 1) * 10);
  array.barrier();

  dash::copy(src_array.begin(),
             src_array.end(),
             array.begin() + (array.size() / 4));

  array.barrier();


  dash::finalize();

  return 0;
}

