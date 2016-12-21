#ifndef DASH__TEST__TEST_LOG_HELPERS_H__
#define DASH__TEST__TEST_LOG_HELPERS_H__

#include <libdash.h>
#include <iomanip>
#include <functional>
#include <string>

namespace dash {
namespace test {

/**
 * Log the values of a one-dimensional submatrix.
 */
template<typename MatrixT>
typename std::enable_if<MatrixT::ndim() == 1, void>::type
print_matrix(
  /// Log message prefix.
  const std::string & name,
  /// Matrix instance to log.
  MatrixT           & matrix,
  int                 precision = 1)
{
  typedef typename MatrixT::value_type value_t;
  // Print local copy of matrix to avoid interleaving of matrix values
  // and log messages:
  std::vector< std::vector<value_t> > values;
  std::vector<value_t> col_header;
  for (auto col = 0; col < matrix.extent(1); ++col) {
    col_header.push_back(col);
  }
  values.push_back(col_header);
  for (auto row = 0; row < matrix.extent(0); ++row) {
    std::vector<value_t> row_values;
    for (auto col = 0; col < matrix.extent(1); ++col) {
      value_t value = matrix.begin()[row * matrix.extent(1) + col];
      row_values.push_back(value);
    }
    values.push_back(row_values);
  }
  DASH_LOG_DEBUG("print_matrix", name);
  int row_idx = -1;
  for (auto row : values) {
    std::ostringstream ss;
    if (row_idx < 0) {
      // do not print row index for column header:
      ss << std::setw(5) << " ";
    } else {
      ss << std::setw(3) << row_idx << ":  ";
    }
    row_idx++;
    for (auto val : row) {
      ss << std::setprecision(row_idx == 0 ? 0 : precision)
         << std::fixed << std::setw(precision + 3)
         << val << " ";
    }
    DASH_LOG_DEBUG("print_matrix", name, ss.str());
  }
}

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
  // Print local copy of matrix to avoid interleaving of matrix values
  // and log messages:
  std::vector< std::vector<value_t> > values;
  std::vector<value_t> col_header;
  for (auto col = 0; col < matrix.extent(1); ++col) {
    col_header.push_back(col);
  }
  values.push_back(col_header);
  for (auto row = 0; row < matrix.extent(0); ++row) {
    std::vector<value_t> row_values;
    for (auto col = 0; col < matrix.extent(1); ++col) {
      value_t value = matrix[row][col];
      row_values.push_back(value);
    }
    values.push_back(row_values);
  }
  DASH_LOG_DEBUG("print_matrix", name);
  int row_idx = -1;
  for (auto row : values) {
    std::ostringstream ss;
    if (row_idx < 0) {
      // do not print row index for column header:
      ss << std::setw(5) << " ";
    } else {
      ss << std::setw(3) << row_idx << ":  ";
    }
    row_idx++;
    for (auto val : row) {
      ss << std::setprecision(row_idx == 0 ? 0 : precision)
         << std::fixed << std::setw(precision + 3)
         << val << " ";
    }
    DASH_LOG_DEBUG("print_matrix", name, ss.str());
  }
}

/**
 * Log the values of a three-dimensional matrix.
 */
template<typename MatrixT>
typename std::enable_if<MatrixT::ndim() == 3, void>::type
print_matrix(
  /// Log message prefix.
  const std::string & name,
  /// Matrix instance to log.
  MatrixT           & matrix,
  int                 precision = 1)
{
  typedef typename MatrixT::value_type value_t;

  std::vector< std::vector< std::vector<value_t> > > values;
  /// Offset of two-dimensional slice in third dimension to print
  for (auto slice_offs = 0; slice_offs < matrix.extent(0); ++slice_offs) {
    // Print local copy of matrix to avoid interleaving of matrix values
    // and log messages:
    std::vector< std::vector< value_t > > slice_values;
    for (auto row = 0; row < matrix.extent(1); ++row) {
      std::vector<value_t> row_values;
      for (auto col = 0; col < matrix.extent(2); ++col) {
        value_t value = matrix[slice_offs][row][col];
        row_values.push_back(value);
      }
      slice_values.push_back(row_values);
    }
    values.push_back(slice_values);
  }
  for (auto slice_offs = 0; slice_offs < matrix.extent(0); ++slice_offs) {
    DASH_LOG_DEBUG("print_matrix", name, "slice z:", slice_offs);
    auto slice_values = values[slice_offs];
    for (auto row : slice_values) {
      std::ostringstream ss;
      for (auto val : row) {
        ss << std::setprecision(precision) << std::fixed << std::setw(4)
           << val << " ";
      }
      DASH_LOG_DEBUG("print_matrix", name, "slice z:", slice_offs, "|",
                     ss.str());
    }
  }
}

/**
 * Log the result of a mapping function of a one-dimensional pattern.
 * Example:
 *
 * \code
 *  dash::test::print_pattern_mapping(
 *    "pattern.unit_at", the_pattern_instance, 3,
 *    [](const pattern_t & _pattern, index_t _index) -> index_t {
 *        return _pattern.unit_at(coords_t { _index });
 *    });
 * \endcode
 *
 */
template<typename PatternT, typename CallbackFun>
typename std::enable_if<PatternT::ndim() == 1, void>::type
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
  typedef typename PatternT::index_type index_t;
  std::string pattern_name = PatternT::PatternName;

  std::ostringstream name_ss;
  name_ss << std::left << std::setw(25) << name;
  std::string name_prefix = name_ss.str();

  std::vector<std::string> entries;
  entries.push_back("[");
  dash::team_unit_t last_unit = pattern.unit_at(0);
  for (index_t i = 0; i < pattern.extent(0); ++i) {
    std::ostringstream ss;
    dash::team_unit_t entry_unit = pattern.unit_at(i);
    if (entry_unit != last_unit) {
      entries.push_back("| ");
      last_unit = entry_unit;
    }
    ss << std::setw(field_width) << callback(pattern, i) << " ";
    entries.push_back(ss.str());
  }
  entries.push_back("]");

  DASH_LOG_DEBUG("print_pattern_mapping", name_prefix, pattern_name);
  std::ostringstream ss;
  for (auto entry : entries) {
    ss << entry;
  }
  DASH_LOG_DEBUG("print_pattern_mapping", name_prefix, ss.str());
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
  auto blocksize_row = pattern.blocksize(0);
  auto blocksize_col = pattern.blocksize(1);
  auto n_blocks_col  = pattern.blockspec().extent(1);
  int  row_char_w    = (pattern.extent(1) * (field_width + 1)) +
                       (n_blocks_col * 2) - 1;
  std::string block_row_separator(row_char_w, '-');
  std::vector<std::string> block_row_separator_entry;
  block_row_separator_entry.push_back(" ");
  block_row_separator_entry.push_back(block_row_separator);
  units.push_back(block_row_separator_entry);
  for (auto row = 0; row < pattern.extent(0); ++row) {
    std::vector<std::string> row_units;
    row_units.push_back("|");
    for (auto col = 0; col < pattern.extent(1); ++col) {
      std::ostringstream ss;
      ss << std::setw(field_width + 1) << callback(pattern, row, col);
      if ((col+1) == pattern.extent(1) || (col+1) % blocksize_col == 0) {
        ss << " |";
      }
      row_units.push_back(ss.str());
    }
    units.push_back(row_units);
    if ((row+1) == pattern.extent(0) || (row+1) % blocksize_row == 0) {
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
