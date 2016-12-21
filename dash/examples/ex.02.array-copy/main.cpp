#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstddef>

#ifdef DASH_ENABLE_IPM
#include <mpi.h>
#endif

#define DASH__ALGORITHM__COPY__USE_WAIT
#include <libdash.h>

using std::cout;
using std::endl;

template<
  template<typename, typename...> class ARRAY,
  typename T,
  typename... REST >
void fill_array(ARRAY<T, REST...> &array, T value)
{
  std::fill(array.lbegin(), array.lend(), value);
}

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
                           : 0;
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

  std::fill(array.lbegin(), array.lend(), myid);

  array.barrier();

  if (myid == 0) {
    cout << "Array size:        " << array.size() << endl;
  }

  int * local_array = new int[num_elems_copy];

#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "on");    // turn monitoring on
  MPI_Pcontrol(0, "clear"); // clear all performance data
#endif

  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);

#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "off");   // turn monitoring off
#endif

  std::ostringstream ss;
  ss << "Local copy at unit " << myid << ": ";
  for(size_t i = 0; i < num_elems_copy; ++i) {
    ss << local_array[i] << " ";
  }
  ss << endl;
  cout << ss.str();

  array.barrier();

  delete[] local_array;

  dash::finalize();

  return 0;
}

