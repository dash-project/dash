# DASH 0.3.0


## DASH Template Library

### Features:

- Added meta-type traits and helpers
- Added range types and range expressions
- Added view types and view expressions for views on multidimensional ranges
  with projections between local and global index space; full support of
  1-dimensional ranges, restricted support of multidimensional projections
- Introduced locality domain concepts and unit locality discovery
  (`dash::util::Locality`, `dash::util::LocalityDomain`)
- Introduced dynamic containers `dash::List` and `dash::UnorderedMap`
- Fixed iteration and pointer arithmetics of `dash::GloPtr` in global address
  space.
- Global dynamic memory allocation: concepts and reference implementations
  (`dash::GlobHeapMem`, `dash::GlobStaticMem`)
- Supporting `dash::Atomic<T>` as container element type
- Well-defined atomic operation semantics for `dash::Shared`
- Added load balance patterns and automatic data distribution based on
  locality information to aid in load balancing
- Improved pattern implementations, rewriting pattern methods as single
  arithmetic expressions
- Introduced parallel IO concepts for DASH containers (`dash::io`),
  currently implemented based on HDF5
- Introduced stencil iterator and halo block concepts
- Using strict unit ID types to distinguish global and team scope
- Using new DASH locality domain concept to provide automatic configuration
  of OpenMP for node-level parallelization
- New algorithms, including `dash::fill`, `dash::generate`, `dash::find`.
- Drastic performance improvements in algorithms, e.g. `dash::min_element`,
  `dash::transform`
- Additional benchmark applications
- Additional example applications, including histogram sort and radix sort
  implementations
- Runtime configuration interface (`dash::util::Config`)
- Improved output format and log targets in unit tests
- Added support for HDF5 groups
- Relaxed restrictions on container element types
- Support patterns with underfilled blocks in `dash::io::hdf5`

### Bugfixes:

- Index calculations in `BlockPattern` with underfilled blocks
- Fixed element access of `.local.begin()` in `dash::Matrix`
- Fixed delayed allocation of `dash::Matrix`
- Conversions of `GlobPtr<T>`, `GlobRef<T>`, `GlobIter<T>`, ... now
  const-correct (e.g., to `GlobIter<const T>` instead of `const GlobIter<T>`)
- Consistent usage of index- and size types
- Numerous stability fixes and performance improvements
- Move-semantics of allocators

### Known limitations:

- Type trait `dash::is_container_compatible` does not check
  `std::is_trivially_copyable` for Cray compilers and GCC <= 4.8.0
  (issue #241)



## DART Interface and Base Library

### Features:

- Introduced strong typing of unit IDs to safely distinguish between global
  IDs (`dart_global_unit_t`) and IDs that are relative to a team
  (`dart_team_unit_t`).
- Added function `dart_allreduce` and `dart_reduce`
- Made global memory allocation and communication operations aware of the 
  underlying data type to improve stability and performance
- Made DART global pointer globally unique to allow copying of global pointer 
  between members of the team that allocated the global memory. Note that a 
  global pointer now contains unit IDs relative to the team that allocated 
  the memory instead of global unit IDs.
- Extended use of `const` specifier in DART communication interface
- Added interface component `dart_locality` implementing topology discovery
  and hierarchical locality description

    - New types:
        - `dart_locality_scope_t`: enum of locality scopes (global, node,
          module, NUMA, ...)
        - `dart_hwinfo_t`: hardware information such as number of NUMA
          domains and cores, CPU clock frequencies, CPU pinning, cache sizes,
          etc.
        - `dart_domain_locality_t`: node in a locality domain hierarchy
          providing locality information such as the number of units in the
          domain and their ids, sub-domains, level in topology, etc.
        - `dart_unit_localiy_t`: locality information for a specific unit

    - New functions:
        - `dart_domain_locality`: Access hierarchical locality description of
          a specified locality domain
        - `dart_team_locality`: Access hierarchical locality description of a
          specified team.
        - `dart_unit_locality`: Access locality description of a specified
          unit

    - New base implementations: \
      Implementations of the locality components to be usable by any DART
      backend:
        - `dart__base__locality__init`
        - `dart__base__locality__finalize`
        - `dart__base__locality__domain`
        - `dart__base__locality__unit`

### Bugfixes:

- Added clarification which DART functionality provides thread-safe access. 
  DART functions can be considered thread-safe as long as they do not operate
  on the same data structures. In particular, thread-concurrent (collective) 
  operations on the same team are not guaranteed to be safe. 

### Known limitations:

- Locality discovery does not support multiple units mapped to same domain
  (issues #239, #161, #153)

## DART-MPI

### Features:

### Bugfixes:

- Fixed numerous memory leaks in dart-mpi

### Known limitations:

- Elements allocated in shared windows are not properly aligned for some 
  versions of OpenMPI (issue #280, fixed since OpenMPI 2.0.2)
- Thread-concurrent access may lead to failures with OpenMPI even if 
  thread support is enabled in DART (build option `ENABLE_THREADSUPPORT`, 
  issue #292)



## Build System

- Improved continuous integration, CI configurations for
  Travis and CircleCI
- Added codedocs (http://codedocs.xyz) in deploy chain to automate API
  documentation updates
- Added readthedocs in deploy chain to generate user guides in distribution
  documentation
- Added NastyMPI test target in continuous integration
- Added docker container build configurations
- Intel MIC architecture build targets (tested on SuperMIC, Knights Corner).
- Support for likwid.
- Support for HDF5.
- Generate cmake package for DASH and DART
- Added build configuration for code coverage tests
- Check MPI implementation for compilance with MPI3
- Enforce minimum C++ compiler versions:
    - GCC: 5.1.0
    - Clang: 3.8.0
    - Intel: 15.0.0

- New compiler flags:
    - `DASH_ENABLE_LIKWID`: Whether DASH has been compiled with likwid
      support.
    - `DASH_ENABLE_HDF5`: Whether DASH has been compiled with HDF5 support.
    - `DASH__ARCH__HAS_RDTSC`: Whether the target architecture provides
      an RDTSC micro-instruction.

- Added compiler wrapper dashc++ (and aliases dashcxx and dashCC) that includes 
  DASH-specific flags compiler and linker flags. To use the wrapper, simply 
  replace mpicxx with dashcxx when building your application.

### Bugfixes:

- Fixed compiler errors for Intel MIC compiler (`icc -mmic`, `mpiicc -mic`).
- Fixed compiler errors for Intel Compiler 16.0.
- Disable hdf5 support if only serial version of hdf5 lib is loaded




# DASH 0.2.0 (2016-03-03)


## Build System

### Features:

- Added support for ScaLAPACK.
- Added support for MKL.
- Added support for numalib.
- Added support for IPM.
- Added support for PLASMA.
- Added support for hwloc.
- Enabled reporting of compiler warnings (`-Wall -pedantic`).
- New compiler flags:

    - `DASH_ENABLE_SCALAPACK`: Whether DASH has been compiled with ScaLAPACK
      support.
    - `DASH_ENABLE_MKL`: Whether DASH has been compiled with Intel MKL
      support.
    - `DASH_ENABLE_NUMA`: Whether DASH has been compiled with numalib support.
    - `DASH_ENABLE_IPM`: Whether DASH has been compiled with support for
      run-time calls of IPM functions.
    - `DASH_ENABLE_PLASMA`: Whether DASH has been compiled with support for
      the PLASMA linear algebra library.

  New compiler flags specific to DART-MPI, also available in DASH when built
  with DART-MPI backend:

    - `DART_MPI_IMPL_ID`: Identification string of the linked MPI
      implementation.
    - `DART_MPI_IS_INTEL`: Defined if DART-MPI uses Intel MPI.
    - `DART_MPI_IS_MPICH`: Defined if DART-MPI uses MPICH, including IBM MPI.
    - `DART_MPI_IS_MVAPICH`: Defined if DART-MPI uses MVAPICH.
    - `DART_MPI_IS_OPENMPI`: Defined if DART-MPI uses OpenMPI.

### Bugfixes:

- Fixed compiler flags for Cray compiler.



## DART Interface

### Features:

- Added type `dart_operation_t` for specification of reduce operations.
- Added type `dart_datatype_t` for portable specification of data types.

### Bugfixes:

- Fixed compiler warnings.



## DART-MPI

### Features:

- Added logging.

### Bugfixes:

- Fixed missing deallocation of shared memory segments when MPI shared windows
  are disabled.
- Reduced MPI calls in rank queries.
- Numerous stability improvements, added checks of parameters and return
  values of MPI calls.
- Fixed compiler warnings.



## DASH

- Added documentation of container concepts.

### Features:

- Added `dash::make_team_spec` providing automatic cartesian arrangement of
  units optimizing communication performance based on pattern properties.
- Added `dash::ShiftTilePattern` (previously `dash::TilePattern`).
- Added `dash::TilePattern`.
- Added algorithms `dash::fill` and `dash::generate`.
- Added `dash::GlobViewIter` for specifying and iterating multi-dimensional
  views e.g. on matrices.
- Added benchmark application `bench.10.summa` for evaluation of
  `dash::multiply<dash::Matrix>`.
- Added benchmark application `bench.07.local-copy` for evaluation of
  `dash::copy` (global-to-local copy operations).
- Extended GUPS benchmark application `bench.03.gups`.
- Extended micro-benchmark `bench.09.multi-reader`.
- Added automatic balancing of process grid extents in `dash::TeamSpec`,
  optimizing for surface-to-volume ratio.
- Added tool `dash::tools::PatternVisualizer` to visualize data distributions
  in SVG graphics.
- Added log helper `print_pattern_mapping` to visualize data distributions in
  log output.
- Significantly improved performance of `dash::summa`.
- Added support for irregular blocked distributions (underfilled blocks) for
  `dash::Array`.
- Added `dash::util::Locality`, a minimalistic helper class for obtaining
  basic locality information like NUMA domains and CPU pinning.
- Added utility class `dash::util::BenchmarkParams` for printing benchmark
  parameters, DASH configuration and environment settings in measurements
  preamble.
- Improved efficiency of `dash::copy` for local-to-local copying.

### Bugfixes:

- Fixed compiler errors for ICC 15.
- Fixed order of coordinate indices in patterns and `dash::Matrix`.
- Fixed definition of assertion macros when configured with runtime
  assertions disabled.
- Added barrier in `dash::Array.deallocate()` to prevent partitions of a
  global array to be freed when going out of scope while other units might
  still attempt to access it.
- Fixed compiler warnings.
- Fixed `dash::copy` for huge ranges.
