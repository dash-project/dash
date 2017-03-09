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
  typedef dash::Array<int> Array_t;
  typedef typename Array_t::index_type indx;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  int block_size = 10;

  Array_t array(nunits * block_size);


  for (int l = 0; l < array.local.size(); l++) {
    array.local[l] = (myid + 1) * 1000 + l;
  }

  array.barrier();

  for (int u = 0; u <= nunits; u++) {
    if (u <= 2 && myid == u) {
      auto   v_local   = dash::local(array);
      auto   v_subl    = dash::sub(2,
                                   block_size - 2,
                                   v_local);
      auto   v_subl_b  = dash::begin(v_subl);
      auto   v_subl_e  = dash::end(v_subl);
      auto   v_subl_bi = dash::begin(dash::index(v_subl));
      auto   v_subl_ei = dash::end(dash::index(v_subl));

      cout << "unit " << u << ": sub(2,blocksize-2, local(array))): \n"
           << "  a.lsize: " << array.pattern().local_size()   << '\n'
           << "  begin:   " << *v_subl_bi << ": " << v_subl_b << '\n'
           << "  end:     " << *v_subl_ei << ": " << v_subl_e << '\n'
           << "  size:    " << v_subl.size()      << '\n';
      cout << "  values:\n";
      for (auto i = v_subl.begin(); i != v_subl.end(); ++i) {
        cout << "    it:" << i << ": "
                          << static_cast<int>(*i) << '\n';
      }
      cout << endl;
    }
    array.barrier();
  }

  if (myid == 0) {
    cout << "------------------------------------------------------" << endl;

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

    cout << "sub_1 = sub(2, blocksize-2, sub_0): \n"
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
    cout << "------------------------------------------------------" << endl;

    auto   v_sub    = dash::sub(3,
                                block_size + 2,
                                array);

    auto v_sub_b    = dash::begin(v_sub);
    auto v_sub_e    = dash::end(v_sub);
    auto v_sub_bi   = dash::begin(dash::index(v_sub));
    auto v_sub_ei   = dash::end(dash::index(v_sub));

    cout << "sub(3,blocksize+2, array)): \n"
         << "  begin:   " << *v_sub_bi << ": " << v_sub_b << '\n'
         << "  end:     " << *v_sub_ei << ": " << v_sub_e << '\n'
         << "  size:    " << v_sub.size()      << '\n';
    cout << "  values:\n";
    for (auto i = v_sub.begin(); i != v_sub.end(); ++i) {
      cout << "    it:" << i << ": "
                        << static_cast<int>(*i) << '\n';
    }

    auto lsub      = dash::local(v_sub);

    auto lsub_b    = dash::begin(lsub);
    auto lsub_e    = dash::end(lsub);
    auto lsub_bi   = dash::begin(dash::index(lsub));
    auto lsub_ei   = dash::end(dash::index(lsub));

    cout << "local(sub(3,blocksize+2, array)): \n"
         << "  begin:   " << *lsub_bi << ": " << lsub_b << '\n'
         << "  end:     " << *lsub_ei << ": " << lsub_e << '\n'
         << "  size:    " << lsub.size() << " = " << (lsub_e - lsub_b)
         << endl;

#if 0
    cout << "  values:\n";
    for (auto i = lsub.begin(); i != lsub.end(); ++i) {
      cout << "    it:" << i << ": "
                        << static_cast<int>(*i) << '\n';
    }

    auto slsub     = dash::sub(0,2, lsub);

    auto slsub_b   = dash::begin(slsub);
    auto slsub_e   = dash::end(slsub);
    auto slsub_bi  = dash::begin(dash::index(slsub));
    auto slsub_ei  = dash::end(dash::index(slsub));

    cout << "sub(0,2, local(sub(3,blocksize+2, array)): \n"
         << "  begin:   " << *slsub_bi << ": " << slsub_b << '\n'
         << "  end:     " << *slsub_ei << ": " << slsub_e << '\n'
         << "  size:    " << slsub.size()      << '\n';
    cout << "  values:\n";
    for (auto i = slsub.begin(); i != slsub.end(); ++i) {
      cout << "    iterator:" << i << ": "
                              << static_cast<int>(*i) << '\n';
    }
    cout << endl;
#endif
  }

  dash::finalize();

  return EXIT_SUCCESS;
}
