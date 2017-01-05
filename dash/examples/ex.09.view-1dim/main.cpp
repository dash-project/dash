/**
 * \example ex.09.view-1dim/main.cpp
 * Illustrating view modifiers on a 1-dimensional array.
 */

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

// TODO:
// Expressions
//
//   // local index of first local array element:
//   auto lbeg_li = dash::index(dash::begin(dash::local(a)));
//                  // -> 0
//   // global index of first local array element
//   auto lbeg_gi = dash::index(dash::global(dash::begin(dash::local(a)));
//                  // -> a.pattern().global(0)
//
// ... should be valid; requires dash::Array<T>::local_type::pointer
// to provide method .pos() -> index_type.

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  int block_size = 10;

  dash::Array<int> array(nunits * block_size);

  for (int l = 0; l < array.local.size(); l++) {
    array.local[l] = (myid + 1) * 1000 + l;
  }

  array.barrier();

  for (int u = 0; u <= nunits; u++) {
    if (u <= 2 && myid == u) {
      auto & v_local   = dash::local(array);
      auto   v_subl    = dash::sub(4,
                                   block_size - 4,
                                   v_local);

      cout << "unit " << u << ": sub(+4,-4, local(array))): \n"
           << "  begin:   " << dash::begin(v_subl) << '\n'
           << "  end:     " << dash::end(v_subl)   << '\n'
           << "  size:    " << v_subl.size()       << '\n';
      cout << "  values:\n";
      for (auto i = v_subl.begin(); i != v_subl.end(); ++i) {
        cout << "    iterator:" << i << ": "
                                << static_cast<int>(*i) << '\n';
      }
      cout << endl;
    }
    array.barrier();
  }

  if (myid == 0) {
    auto sub_0 = dash::sub(block_size / 2 * (nunits-1),
                           block_size / 2 * (nunits-1) + block_size,
                           array);
    auto sub_1 = dash::sub(2,
                           block_size - 2,
                           sub_0);
    cout << "sub_0 = sub(<block range>, array): \n"
         << "  index(begin):   " << dash::index(dash::begin(sub_0)) << '\n'
         << "  index(end):     " << dash::index(dash::end(sub_0))   << '\n'
         << "  size:           " << sub_0.size()                    << '\n'
         << "  values:\n";
    for (auto i = sub_0.begin(); i != sub_0.end(); ++i) {
      cout << "    index:" << dash::index(i) << " iterator:" << i << ": "
                           << static_cast<int>(*i) << '\n';
    }
    cout << endl;

    cout << "sub_1 = sub(begin+2, end-2, sub_0): \n"
         << "  index(begin):   " << dash::index(dash::begin(sub_1)) << '\n'
         << "  index(end):     " << dash::index(dash::end(sub_1))   << '\n'
         << "  size:           " << sub_1.size()                    << '\n'
         << "  values:\n";
    for (auto i = sub_1.begin(); i != sub_1.end(); ++i) {
      cout << "    index:" << dash::index(i) << " iterator:" << i << ": "
                           << static_cast<int>(*i) << '\n';
    }
    cout << endl;
  }

  if (myid == 0) {
    auto   v_sub   = dash::sub(1,
                               block_size - 1,
                               array);
    auto & lsub    = dash::local(v_sub);
//  auto   v_sublsub = dash::sub(2,
//                               block_size - 2,
//                               v_lsub);

    auto lsub_b    = dash::begin(lsub);
    auto lsub_e    = dash::end(lsub);
    auto lsub_bi   = dash::begin(dash::index(lsub));
    auto lsub_ei   = dash::end(dash::index(lsub));

    cout << "local(sub(+1,-1, array)): \n"
         << "  begin:   " << *lsub_bi << ": " << lsub_b << '\n'
         << "  end:     " << *lsub_ei << ": " << lsub_e << '\n'
         << "  size:    " << lsub.size()     << '\n';
    cout << "  values:\n";
    for (auto i = lsub.begin(); i != lsub.end(); ++i) {
      cout << "    iterator:" << i << ": "
                              << static_cast<int>(*i) << '\n';
    }
    cout << endl;
  }

  dash::finalize();

  return EXIT_SUCCESS;
}
