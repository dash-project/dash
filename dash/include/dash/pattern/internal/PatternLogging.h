#ifndef DASH__INTERNAL__PATTERN_LOGGING_H__
#define DASH__INTERNAL__PATTERN_LOGGING_H__

#include <dash/util/PatternMetrics.h>
#include <dash/internal/Logging.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <functional>
#include <string>
#include <vector>


namespace dash {
namespace internal {

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

  std::vector<std::string> entries;
  entries.emplace_back("[");
  team_unit_t last_unit = pattern.unit_at(0);
  for (index_t i = 0; i < pattern.extent(0); ++i) {
    std::ostringstream ss;
    team_unit_t entry_unit = pattern.unit_at(i);
    if (entry_unit != last_unit) {
      entries.emplace_back("|");
      last_unit = entry_unit;
    }
    ss << std::setw(field_width) << callback(pattern, i) << " ";
    entries.push_back(ss.str());
  }
  entries.emplace_back("]");

  DASH_LOG_DEBUG("print_pattern_mapping", name, pattern_name);
  std::ostringstream ss;
  for (const auto& entry : entries) {
    ss << entry;
  }
  DASH_LOG_DEBUG("print_pattern_mapping", name, ss.str());
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
  block_row_separator_entry.emplace_back(" ");
  block_row_separator_entry.push_back(block_row_separator);
  units.push_back(block_row_separator_entry);
  for (auto row = 0; row < pattern.extent(0); ++row) {
    std::vector<std::string> row_units;
    row_units.emplace_back("|");
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
    for (const auto& entry : row_fmt) {
      ss << entry;
    }
    DASH_LOG_DEBUG("print_pattern_mapping", name, ss.str());
  }
}

template<typename PatternT>
void print_pattern_metrics(const PatternT & pattern)
{
  using std::cout;
  using std::endl;

  dash::util::PatternMetrics<PatternT> pm(pattern);

  size_t block_kbytes = pattern.blocksize(0) * pattern.blocksize(1) *
                        sizeof(double) /
                        1024;

  cout << "Pattern Metrics:\n"
       << "    Partitioning:\n"
       << "        block size:         " << block_kbytes    << " KB\n"
       << "        number of blocks:   " << pm.num_blocks() << "\n"
       << "        blockspec:          " << pattern.blockspec() << "\n"
       << "    Mapping imbalance:\n"
       << "        teamspec:           " << pattern.teamspec() << "\n"
       << "        min. blocks/unit:   " << pm.min_blocks_per_unit()
       << " = " << pm.min_elements_per_unit() << " elements\n"
       << "        max. blocks/unit:   " << pm.max_blocks_per_unit()
       << " = " << pm.max_elements_per_unit() << " elements\n"
       << "        imbalance factor:   " << std::setprecision(4)
                                         << pm.imbalance_factor()     << "\n"
       << "        balanced units:     " << pm.num_balanced_units()   << "\n"
       << "        imbalanced units:   " << pm.num_imbalanced_units() << "\n"
       << "\n"
       << endl;
}

} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__PATTERN_LOGGING_H__
