/* 
 * \file examples/ex.02.array-copy/main.cpp 
 *
 * author(s): Tobias Fuchs, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>

using std::cout;
using std::endl;

template<
  template<typename, typename...> class ARRAY,
  typename T,
  typename... REST
>
void fill_array(ARRAY<T, REST...> &array, T value)
{
  std::fill(array.lbegin(), array.lend(), value);
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dart_unit_t myid       = dash::myid();
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

  cout << endl
       << "Elements per unit: " << num_elems_unit  << endl
       << "Start index:       " << start_index     << endl
       << "Elements to copy:  " << num_elems_copy  << endl;

  fill_array(array, myid);

  array.barrier();

  cout << "Array size:        " << array.size() << endl;

  int local_array[num_elems_copy];

  dash::copy(array.begin() + start_index,
             array.begin() + start_index + num_elems_copy,
             local_array);

  for(int i = 0; i < num_elems_copy; ++i) {
    cout << local_array[i] << " ";
  }
  cout << endl;

  array.barrier();

  dash::finalize();

  return 0;
}

