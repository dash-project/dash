#ifndef DASH__EXAMPLES__UTIL_H__INCLUDED
#define DASH__EXAMPLES__UTIL_H__INCLUDED

#include <libdash.h>
#include <string>
#include <sstream>
#include <iomanip>


#define print(stream_expr__) \
  do { \
    std::ostringstream mss; \
    mss << stream_expr__;  \
    std::ostringstream oss; \
    std::istringstream iss(mss.str()); \
    std::string item; \
    while (std::getline(iss, item)) { \
      oss   << "[    " << dash::myid() << " ] "; \
      oss   << item << endl; \
    } \
    cout << oss.str(); \
  } while(0)


using namespace dash::internal::logging;

template <class ValueRange>
static std::string range_str(
  const ValueRange & vrange,
  int                prec = 4) {
  typedef typename ValueRange::value_type value_t;
  std::ostringstream ss;
  auto idx = dash::index(vrange);
  int        i   = 0;
  for (const auto & v : vrange) {
    const value_t val = v;
    ss << std::setw(3) << *(dash::begin(idx) + i) << "|"
       << std::fixed << std::setprecision(prec)
       << TermColorMod(unit_term_colors[(int)val])
       << val
       << TermColorMod(TCOL_DEFAULT);
    ++i;
  }
  return ss.str();
}

template <class NViewType>
std::string nview_str(
  const NViewType   & nview,
  int                 prec = 2)
{
  using value_t   = typename NViewType::value_type;
  auto view_nrows = nview.extents()[0];
  auto view_ncols = nview.extents()[1];
  auto nindex     = dash::index(nview);
  std::ostringstream ss;
  for (int r = 0; r < view_nrows; ++r) {
    ss << '\n' << "  " << std::right << std::setw(2) << r << "  ";
    for (int c = 0; c < view_ncols; ++c) {
      int  offset = r * view_ncols + c;
      value_t val = nview[offset];
      ss << std::fixed << std::setw(3)
         << nindex[offset]
         << TermColorMod(unit_term_colors[(int)val])
         << " "
         << std::fixed << std::setprecision(prec) << val
         << "  "
         << TermColorMod(TCOL_DEFAULT);
    }
  }
  return ss.str();
}

#endif //  DASH__EXAMPLES__UTIL_H__INCLUDED
