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
    auto         copy_begin_it    = a.begin() + copy_dest_begin_idx;
    auto         copy_end_it_exp  = copy_begin_it + copy_num_elem;

    auto         dest_range       = dash::make_range(copy_begin_it,
                                                     copy_end_it_exp);
 // auto         dest_range       = dash::sub(copy_begin_it.pos(),
 //                                           copy_end_it_exp.pos(),
 //                                           a);
    auto         dest_brange      = dash::blocks(dest_range);

    const auto & dest_range_idx   = dash::index(dest_range);
    const auto & dest_range_org   = dash::origin(dest_range);
    const auto & dest_range_pat   = dest_range_idx.pattern();
    const auto & dest_range_idom  = dash::domain(
                                      dash::index(dest_range));
    // Works:
//  auto dest_brange_idx  = dash::index(dest_brange);
    // Fails:
    auto dest_brange_idx  = dash::index(dash::blocks(dest_range));
    const auto & dest_brange_org  = dash::origin(dash::blocks(dest_range));
    const auto & dest_brange_pat  = dest_brange_idx.pattern();
    const auto & dest_brange_idom = dash::domain(
                                      dash::index(dash::blocks(dest_range)));

    auto dom_first_gidx           = dest_range_idom.first();
    auto dom_last_gidx            = dest_range_idom.last();

    auto first_gidx               = dest_range_idx.first();
    auto last_gidx                = dest_range_idx.last();

    auto first_bidx               = dest_brange_idx.first();
    auto last_bidx                = dest_brange_idx.last();

    print("make_range is range:    " << dash::is_range<
                                          decltype(dest_range)
                                        >::value);
    print("blocks(range) is range: " << dash::is_range<
                                          decltype(dest_brange)
                                        >::value);
    print("d(blocks(rg)) is range: " << dash::is_range<
                                          decltype(dash::domain(dest_brange))
                                        >::value);

    print("make_range is view:     " << dash::is_view<
                                          decltype(dest_range)
                                        >::value);
    print("blocks(range) is view:  " << dash::is_view<
                                          decltype(dest_brange)
                                        >::value);
    print("d(make_range) is view:  " << dash::is_view<
                                          decltype(dash::domain(dest_range))
                                        >::value);

    print("copy   idom range   " << "(" << dom_first_gidx
                                 << "," << last_gidx  << ")");
    print("copy   ridx range   " << "(" << first_gidx
                                 << "," << last_gidx  << ")");
    print("copy   bidx range   " << "(" << first_bidx
                                 << "," << last_bidx  << ")");

    print("array   &pattern:   " << &a.pattern());
    print("begin+1 &pattern:   " << &(dash::index(
                                        dash::sub(2, 8, a)).pattern()));

    print("copy   rg type:     " << dash::typestr(dest_range));
    print("copy b.rg type:     " << dash::typestr(dest_brange));

    print("copy   rg domain:   " << dash::typestr(dest_range_idom));
    print("copy b.rg domain:   " << dash::typestr(dest_brange_idom));
    print("copy   rg &domain:  " << &(dest_range_idom));
    print("copy b.rg &domain:  " << &(dest_brange_idom));
    print("copy   rg domain.sz " << dest_range_idom.size());
    print("copy b.rg domain.sz " << dest_brange_idom.size());

    print("copy   rg origin:   " << dash::typestr(dest_range_org));
    print("copy b.rg origin:   " << dash::typestr(dest_brange_org));
    print("copy   rg &origin:  " << &(dest_range_org));
    print("copy b.rg &origin:  " << &(dest_brange_org));
    print("copy   rg &orig.pat " << &(dest_range_org.pattern()));
    print("copy b.rg &orig.pat " << &(dest_brange_org.pattern()));
    print("copy   rg orig.p.sz " << dest_range_org.pattern().size());
    print("copy b.rg orig.p.sz " << dest_brange_org.pattern().size());
    print("copy   rg &orig.idx " << &(dash::index(dest_range_org).pattern()));
    print("copy b.rg &orig.idx " << &(dash::index(dest_brange_org).pattern()));

    print("copy   rg index:    " << dash::typestr(dest_range_idx));
    print("copy b.rg index:    " << dash::typestr(dest_brange_idx));
    print("copy   rg &index:   " << &(dest_range_idx));
    print("copy b.rg &index:   " << &(dest_brange_idx));
    print("copy   rg index.siz " << (dest_range_idx.size()));
    print("copy b.rg index.siz " << (dest_brange_idx.size()));
    
    print("copy   rg pattern:  " << dash::typestr(dest_range_pat));
    print("copy b.rg pattern:  " << dash::typestr(dest_brange_pat));
    print("copy   rg &pattern: " << &dest_range_pat);
    print("copy b.rg &pattern: " << &dest_brange_pat);
    print("copy   rg pat.size: " << dest_range_pat.size());
    print("copy b.rg pat.size: " << dest_brange_pat.size());

    print("dest first block:   " << dest_range_pat.block_at(
                                      dest_range_pat.coords(
                                        first_gidx)
                                    )); 
    print("dest last  block:   " << dest_range_pat.block_at(
                                      dest_range_pat.coords(
                                        last_gidx)
                                    ));

    print("dest b.rg block0:   " << dest_brange_pat.block_at(
                                      dest_brange_pat.coords(
                                        first_gidx)
                                    )); 
    print("dest b.rg blockN:   " << dest_brange_pat.block_at(
                                      dest_brange_pat.coords(
                                        last_gidx)
                                    ));

    print("copy range local:   " << std::decay<decltype(
                                      dash::index(dash::blocks(dest_range))
                                    )>::type::view_domain_is_local);
    print("copy range local:   " << dash::view_traits<
                                     decltype(dest_range)
                                    >::is_local::value);
    print("copy index domain:  " << dash::index(dest_range));
    print("copy block dom.idx: " << dash::domain(
                                      dash::index(dash::blocks(dest_range))
                                    ));
    print("copy block indices: " << dash::index(dest_brange));
    print("copy num blocks:    " << dest_brange.size());
    print("copy begin:         " << dash::begin(dest_range));
    print("copy end:           " << dash::end(dest_range));

//   print("copy range begin:   " << dash::begin(dest_range));
//   print("copy range:         " << range_str(dest_range));
#if 1
    auto dest_blocks     = dash::blocks(dest_range);
    for (const auto & block : dest_blocks) {
      print("copy to block:" << range_str(block));
    }

    // copy local buffer to global array
    auto copy_end_it     = dash::copy(
                             buf.data(),
                             buf.data() + copy_num_elem,
                             copy_begin_it);
#endif
  }
  a.barrier();

  print("modified array: " << range_str(a));

  dash::finalize();
}
