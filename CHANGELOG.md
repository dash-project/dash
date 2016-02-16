
# DASH 0.2.0 (2016-02-20)

## Build System

Features:

- Added support for ScaLAPACK.
- Added support for MKL (`-DDASH_ENABLE_MKL`).
- Added support for numalib (`-DDASH_ENABLE_NUMA`).
- Added compiler flags:

    - `MPI_IMPL_ID`: Identification string of the linked MPI implementation
      (dart-mpi only).
    - `DASH_ENABLE_SCALAPACK`: Whether DASH has been compiled with ScaLAPACK
      support.
    - `DASH_ENABLE_MKL`: Whether DASH has been compiled with Intel MKL
      support.

Bugfixes:

- Fixed compiler flags for Cray compiler.

## DART Interface

Features:

- Added type `dart_operation_t` for specification of reduce operations.
- Added type `dart_datatype_t` for portable specification of data types.

Bugfixes:

- Fixed compiler warnings.

## DART-MPI

Features:

- Added logging.

Bugfixes:

- Fixed deallocation of shared memory segments.
- Fixed unnecessary MPI calls.
- Numerous stability fixes.
- Fixed compiler warnings.

## DASH

- Added documentation.

Features:

- Added `dash::ShiftTilePattern` (previously `dash::TilePattern`).
- Added `dash::TilePattern`.
- Added algorithms `dash::fill` and `dash::generate`.
- Added `dash::util::Locality`, a minimalistic helper class for obtaining
  basic locality information like NUMA domains and CPU pinning.
- Added benchmark application for `dash::multiply<dash::Matrix>`.
- Added benchmark application for `dash::copy` (global-to-local copying).
- Added automatic balancing of process grid extents in `dash::TeamSpec`,
  optimizing for surface-to-volume ratio.
- Added tool `dash::tools::PatternVisualizer` to visualize data distributions
  in SVG.
- Added log helper `print_pattern_mapping` to visualize data distributions in
  log output.
- Significantly improved performance of `dash::SUMMA`.
- Added support for irregular blocked distributions (underfilled blocks) for
  `dash::Array`.

Bugfixes:

- Fixed order of coordinate indices in patterns and `dash::Matrix`.
- Fixed definition of assertion macros when configured with runtime
  assertions disabled.
- Fixed compiler warnings.

