#ifndef DASH__EXAMPLES__UTIL_H__INCLUDED
#define DASH__EXAMPLES__UTIL_H__INCLUDED

#include <libdash.h>

#include <array>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <cstddef>


#define print(stream_expr__) \
  do { \
    std::ostringstream mss; \
    mss << stream_expr__;  \
    std::ostringstream oss; \
    std::istringstream iss(mss.str()); \
    std::string item; \
    while (std::getline(iss, item)) { \
      DASH_LOG_DEBUG("print", item); \
      oss   << "[  U:" << dash::myid() << " ] "; \
      oss   << item << std::endl; \
    } \
    std::cout << oss.str(); \
  } while(0)

#define STEP(stream_m__) do { \
  std::cin.ignore();          \
  print(stream_m__);          \
} while(0)


using namespace dash::internal::logging;

template <class ValueRange>
static std::string range_str(
  const ValueRange & vrange,
  int                prec = 2) {
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
  const auto view_nrows = nview.extents()[0];
  const auto view_ncols = nview.extents()[1];
  const auto nindex     = dash::index(nview);
  std::ostringstream ss;

  for (int r = 0; r < view_nrows; ++r) {
    ss << "\n   ";
    for (int c = 0; c < view_ncols; ++c) {
      int  offset = r * view_ncols + c;
      value_t val = nview.begin()[offset];
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

template <class NViewType>
std::string nviewrc_str(
  const NViewType   & nview,
  int                 prec = 2)
{
  using value_t   = typename NViewType::value_type;
  const auto view_nrows = nview.extents()[0];
  const auto view_ncols = nview.extents()[1];
  const auto nindex     = dash::index(nview);
  std::ostringstream ss;

  ss << "\n      ";
  for (int c = 0; c < view_ncols; ++c) {
    ss << std::setw(8 + prec) << std::left << c;
  }
  for (int r = 0; r < view_nrows; ++r) {
    ss << '\n' << std::right << std::setw(3) << r << "  ";
    for (int c = 0; c < view_ncols; ++c) {
      int  offset = r * view_ncols + c;
      value_t val = nview.begin()[offset];
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

/**
 * Create string describing of pattern instance.
 */
template<typename PatternType>
static
std::string pattern_to_string(
  const PatternType & pattern)
{
  typedef typename PatternType::index_type index_t;

  dash::dim_t ndim = pattern.ndim();

  std::string storage_order = pattern.memory_order() == dash::ROW_MAJOR
                              ? "ROW_MAJOR"
                              : "COL_MAJOR";

  std::array<index_t, 2> blocksize;
  blocksize[0] = pattern.blocksize(0);
  blocksize[1] = pattern.blocksize(1);

  std::ostringstream ss;
  ss << "dash::"
     << PatternType::PatternName
     << "<"
     << ndim << ","
     << storage_order << ","
     << typeid(index_t).name()
     << ">(\n"
     << "        SizeSpec:  " << pattern.sizespec().extents()  << ",\n"
     << "        TeamSpec:  " << pattern.teamspec().extents()  << ",\n"
     << "        BlockSpec: " << pattern.blockspec().extents() << ",\n"
     << "        BlockSize: " << blocksize << " )";

  return ss.str();
}

#endif //  DASH__EXAMPLES__UTIL_H__INCLUDED
