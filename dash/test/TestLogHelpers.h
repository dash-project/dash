#ifndef DASH__TEST__TEST_LOG_HELPERS_H__
#define DASH__TEST__TEST_LOG_HELPERS_H__

#include <libdash.h>
#include <iomanip>
#include <functional>
#include <string>

namespace dash {
namespace test {

/**
 * Log the values of a two-dimensional matrix.
 */
template<typename MatrixT>
typename std::enable_if<MatrixT::ndim() == 2, void>::type
print_matrix(
  /// Log message prefix.
  const std::string & name,
  /// Matrix instance to log.
  MatrixT           & matrix,
  int                 precision = 1)
{
  typedef typename MatrixT::value_type value_t;
  typedef typename MatrixT::index_type index_t;
  // Print local copy of matrix to avoid interleaving of matrix values
  // and log messages:
  std::vector< std::vector<value_t> > values;
  for (auto row = 0; row < matrix.extent(1); ++row) {
    std::vector<value_t> row_values;
    for (auto col = 0; col < matrix.extent(0); ++col) {
      value_t value = matrix[col][row];
      row_values.push_back(value);
    }
    values.push_back(row_values);
  }
  DASH_LOG_DEBUG("print_matrix", name);
  for (auto row : values) {
    std::ostringstream ss;
    for (auto val : row) {
      ss << std::setprecision(precision) << std::fixed << std::setw(4)
         << val << " ";
    }
    DASH_LOG_DEBUG("print_matrix", name, ss.str());
  }
}

/**
 * Log the result of a mapping function of a two-dimensional pattern.
 * Example:
 *
 * \code
 *  dash::test::print_pattern_mapping(
 *    "pattern.unit_at", the_pattern_instance, 3,
 *    [](const pattern_t & _pattern, int _x, int _y) -> index_t {
 *        return _pattern.unit_at(coords_t {_x, _y});
 *    });
 * \endcode
 *
 */
template<typename PatternT, typename CallbackFun>
typename std::enable_if<PatternT::ndim() == 2, void>::type
print_pattern_mapping(
  /// Log message prefix.
  const std::string & name,
  /// Pattern instance to log.
  const PatternT    & pattern,
  /// Width of a single mapping result in number of characters.
  int                 field_width,
  /// The mapping function to call for every cell.
  const CallbackFun & callback)
{
  std::vector< std::vector<std::string> > units;
  auto blocksize_x = pattern.blocksize(0);
  auto blocksize_y = pattern.blocksize(1);
  auto n_blocks_x  = pattern.blockspec().extent(0);
  int  row_char_w  = (pattern.extent(0) * (field_width + 1)) +
                     (n_blocks_x * 2) - 1;
  std::string block_row_separator(row_char_w, '-');
  std::vector<std::string> block_row_separator_entry;
  block_row_separator_entry.push_back(" ");
  block_row_separator_entry.push_back(block_row_separator);
  units.push_back(block_row_separator_entry);
  for (auto y = 0; y < pattern.extent(1); ++y) {
    std::vector<std::string> row_units;
    row_units.push_back("|");
    for (auto x = 0; x < pattern.extent(0); ++x) {
      std::ostringstream ss;
      ss << std::setw(field_width + 1) << callback(pattern, x, y);
      if ((x+1) == pattern.extent(0) || (x+1) % blocksize_x == 0) {
        ss << " |";
      }
      row_units.push_back(ss.str());
    }
    units.push_back(row_units);
    if ((y+1) == pattern.extent(1) || (y+1) % blocksize_y == 0) {
      units.push_back(block_row_separator_entry);
    }
  }
  DASH_LOG_DEBUG("print_pattern_mapping", name);
  for (auto row_fmt : units) {
    std::ostringstream ss;
    for (auto entry : row_fmt) {
      ss << entry;
    }
    DASH_LOG_DEBUG("print_pattern_mapping", name, ss.str());
  }
}

} // namespace test
} // namespace dash

#endif // DASH__TEST__TEST_LOG_HELPERS_H__
