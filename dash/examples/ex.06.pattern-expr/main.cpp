#include <libdash.h>

// #include "../bench.h"

using std::cout;
using std::endl;


namespace dash {


  template <class T>
  declype(auto) pattern()


} // namespace dash


template <class T>
pattern_impl(T && lhs) {
  return dash::view::pattern(std::forward<T &&>(lhs));
}

int main() {

  int nlocal = 30;
  int nunits = dash::size();

  dash::Array<int> array(nunits * nlocal);

  const auto & pattern = array | pattern();

  auto pos_a = pattern | local(4);
  auto pos_a = pattern | global(22);

  auto pos_a_gidx = pos_a | global() | index();
  auto pos_b_gidx = pos_b | global() | index();

  return 0;
}





