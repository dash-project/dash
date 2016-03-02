#include <unistd.h>
#include <iostream>
#include <cstddef>

#define DASH__ALGORITHM__COPY__USE_WAIT
#include <libdash.h>

using std::cout;
using std::endl;

typedef double    value_t;
typedef int64_t   index_t;
typedef uint64_t  extent_t;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

template<typename MatrixType>
void init_values(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  int nrepeat  = 100;
  int nunits   = dash::size();
  int n        = 256 * nunits;
  int npcol    = 2;

  if (argc >= 2) {
    n       = atoi(argv[1]);
  }
  if (argc >= 3) {
    nrepeat = atoi(argv[2]);
  }
  if (argc >= 4) {
    npcol   = atoi(argv[3]);
  }

  if (dash::myid() == 0) {
    cout << "dash::multiply example (n:" << n << " repeat:" << nrepeat << ")"
         << endl;
  }

  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  dash::SizeSpec<2, extent_t> size_spec(n, n);
  dash::TeamSpec<2, index_t>  team_spec(nunits / npcol, npcol);

  auto pattern = dash::make_pattern<
                   dash::summa_pattern_partitioning_constraints,
                   dash::summa_pattern_mapping_constraints,
                   dash::summa_pattern_layout_constraints >(
                     size_spec,
                     team_spec);

  auto tile_rows = pattern.blocksize(0);
  auto tile_cols = pattern.blocksize(1);

  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_c(pattern);

  double time_total_s = 0;
  for (int r = 0; r < nrepeat; ++r) {
    init_values(matrix_a, matrix_b, matrix_c);

    if (dash::myid() == 0 && (nrepeat < 40 || r % (nrepeat / 40) == 0)) {
      cout << "." << std::flush;
    }

    auto ts_start = Timer::Now();
    dash::multiply(matrix_a, matrix_b, matrix_c);
    time_total_s += Timer::ElapsedSince(ts_start) * 1.0e-6;
  }
  time_total_s /= nrepeat;

  if (dash::myid() == 0) {
    std::ostringstream team_ss;
    team_ss << team_spec.extent(0) << "x" << team_spec.extent(1);
    std::string team_extents = team_ss.str();

    std::ostringstream tile_ss;
    tile_ss << tile_rows << "x" << tile_cols;
    std::string tile_extents = tile_ss.str();

    cout << endl;
    cout << std::setw(14) << "units"
         << std::setw(14) << "n"
         << std::setw(14) << "repeat"
         << std::setw(14) << "team"
         << std::setw(14) << "tile"
         << std::setw(14) << "time.s"
         << endl;
    cout << std::setw(14) << nunits
         << std::setw(14) << n
         << std::setw(14) << nrepeat
         << std::setw(14) << team_extents
         << std::setw(14) << tile_extents
         << std::setw(14) << std::setprecision(2) << time_total_s
         << endl;
  }

  dash::finalize();
}

template<typename MatrixType>
void init_values(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c)
{
  // Initialize local sections of matrix C:
  auto unit_id          = dash::myid();
  auto pattern          = matrix_c.pattern();
  auto block_cols       = pattern.blocksize(0);
  auto block_rows       = pattern.blocksize(1);
  auto num_blocks_cols  = pattern.extent(0) / block_cols;
  auto num_blocks_rows  = pattern.extent(1) / block_rows;
  auto num_blocks       = num_blocks_rows * num_blocks_cols;
  auto num_local_blocks = num_blocks / dash::Team::All().size();
  value_t * l_block_elem_a;
  value_t * l_block_elem_b;
  for (auto l_block_idx = 0;
       l_block_idx < num_local_blocks;
       ++l_block_idx)
  {
    auto l_block_a = matrix_a.local.block(l_block_idx);
    auto l_block_b = matrix_b.local.block(l_block_idx);
    l_block_elem_a = l_block_a.begin().local();
    l_block_elem_b = l_block_b.begin().local();
    for (auto phase = 0; phase < block_cols * block_rows; ++phase) {
      value_t value = (100000 * (unit_id + 1)) +
                      (100 * l_block_idx) +
                      phase;
      l_block_elem_a[phase] = value;
      l_block_elem_b[phase] = value;
    }
  }
  dash::barrier();
}
