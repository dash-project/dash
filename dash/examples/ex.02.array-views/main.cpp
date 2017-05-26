#include <libdash.h>

using std::cout;
using std::cerr;
using std::clog;
using std::cin;
using std::endl;
using std::vector;

using uint = unsigned int;


#define print(stream_expr) \
  do { \
    std::ostringstream ss; \
    ss << stream_expr; \
    cout << ss.str() << endl; \
  } while(0)


inline void sum(const uint nelts,
                const dash::NArray<uint, 2> &matIn,
                const uint myid) {
  uint lclRows = matIn.pattern().local_extents()[0];

  uint const *mPtr;
  uint localSum = 0;

  for (uint i = 0; i < lclRows; ++i) {
    mPtr = matIn.local.row(i).lbegin();

    for (uint j = 0; j < nelts; ++j) {
      localSum += *(mPtr++);
    }
  }
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
                      (0.0001 * (li+1)) +
                      // global offset
                      (0.0100 * gi);
  }
  array.barrier();
}

int main(int argc, char *argv[])
{
  dash::init(&argc, &argv);

  int elem_per_unit    = 7;
  int elem_additional  = 2;
  int array_size       = dash::size() * elem_per_unit +
                           std::min<int>(elem_additional, dash::size());
  int num_local_elem   = elem_per_unit +
                         ( dash::myid() < elem_additional
                         ? 1
                         : 0 );

  dash::Array<float> a(array_size, dash::BLOCKCYCLIC(3));
  initialize_array(a);

  dash::Array<float> a_pre(array_size, dash::BLOCKCYCLIC(3));
  initialize_array(a_pre);

  auto copy_num_elem       = a.size() / 2;
  auto copy_dest_begin_idx = a.size() / 4;
  auto copy_dest_end_idx   = copy_dest_begin_idx + copy_num_elem;

  print("array: " << range_str(a));
  print("ncopy: " << copy_num_elem
                    << "(" << copy_dest_begin_idx
                    << "," << copy_dest_end_idx << "]");

  std::vector<float> buf(copy_num_elem);
  std::iota(buf.begin(), buf.end(), 0.9999);

  a.barrier();

  if (dash::myid() == 0) {
    auto copy_begin_it   = a.begin() + copy_dest_begin_idx;
    auto copy_end_it_exp = copy_begin_it + copy_num_elem;
    auto dest_range      = dash::make_range(copy_begin_it,
                                            copy_end_it_exp);

    const auto & dest_range_idx  = dash::index(dest_range);
    const auto & dest_range_org  = dash::origin(dest_range);
    const auto & dest_range_pat  = dest_range_idx.pattern();
    const auto & dest_range_idom = dash::domain(
                                     dash::index(dash::blocks(dest_range)));

    print("dest first block:   " << dest_range_pat.block_at(
                                      dest_range_pat.coords(
                                        dest_range_idom.first())
                                    )); 
    print("dest last  block:   " << dest_range_pat.block_at(
                                      dest_range_pat.coords(
                                        dest_range_idom.last())
                                    ));

    print("copy range type:    " << dash::typestr(dest_range));
    print("copy range origin:  " << dash::typestr(dest_range_org));
    print("copy range pattern: " << dash::typestr(dest_range_pat));
    print("copy range local:   " << dash::view_traits<
                                     decltype(dest_range)
                                    >::is_local::value);
    print("copy index domain:  " << dash::index(dest_range));
    print("copy block dom.idx: " << dash::domain(
                                      dash::index(dash::blocks(dest_range))
                                    ));
    print("copy block indices: " << dash::index(dash::blocks(dest_range)));
    print("copy num blocks:    " << dash::blocks(dest_range).size());
    print("copy dom. begin:    " << copy_begin_it - copy_begin_it.pos());
    print("copy begin:         " << copy_begin_it);
    print("copy end:           " << copy_end_it_exp);

    print("copy range begin:   " << dash::begin(dest_range));
    print("copy range:         " << range_str(dest_range));

    auto dest_blocks     = dash::blocks(dest_range);
    for (auto block : dest_blocks) {
  //    print("copy to block:" << range_str(block));
    }

    // copy local buffer to global array
    auto copy_end_it     = dash::copy(
                             buf.data(),
                             buf.data() + copy_num_elem,
                             copy_begin_it);
  }
  a.barrier();

  print("modified array: " << range_str(a));

  dash::finalize();
}
