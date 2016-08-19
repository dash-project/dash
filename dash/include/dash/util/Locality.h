#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <dash/Init.h>

#include <dash/util/Config.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>


std::ostream & operator<<(
  std::ostream                 & os,
  const dart_domain_locality_t & domain_loc);

std::ostream & operator<<(
  std::ostream                 & os,
  const dart_unit_locality_t   & unit_loc);

namespace dash {

#ifndef DOXYGEN
template<
    typename ElementType,
    typename IndexType,
    class    PatternType>
class Array;
#endif // DOXYGEN

namespace util {

class Locality
{
public:
  friend void dash::init(int *argc, char ***argv);
  friend void dash::init_thread(int *argc, char ***argv, int *concurrency);

public:
  typedef struct
  {
    int  unit;
    char host[40];
    char domain[20];
    int  cpu_id;
    int  num_cores;
    int  numa_id;
    int  num_threads;
  }
  UnitPinning;

  typedef enum
  {
    Undefined = DART_LOCALITY_SCOPE_UNDEFINED,
    Global    = DART_LOCALITY_SCOPE_GLOBAL,
    Group     = DART_LOCALITY_SCOPE_GROUP,
    Network   = DART_LOCALITY_SCOPE_NETWORK,
    Node      = DART_LOCALITY_SCOPE_NODE,
    Module    = DART_LOCALITY_SCOPE_MODULE,
    NUMA      = DART_LOCALITY_SCOPE_NUMA,
    Unit      = DART_LOCALITY_SCOPE_UNIT,
    Package   = DART_LOCALITY_SCOPE_PACKAGE,
    Uncore    = DART_LOCALITY_SCOPE_UNCORE,
    Core      = DART_LOCALITY_SCOPE_CORE,
    CPU       = DART_LOCALITY_SCOPE_CPU
  }
  Scope;

public:

  static inline int NumNodes()
  {
    return (_domain_loc == nullptr)
           ? -1 : std::max<int>(_domain_loc->num_nodes, 1);
  }

  static inline int NumSockets()
  {
    return (_domain_loc == nullptr)
           ? -1 : std::max<int>(_domain_loc->hwinfo.num_sockets, 1);
  }

  static inline int NumNUMANodes()
  {
    return (_domain_loc == nullptr)
           ? -1 : std::max<int>(_domain_loc->hwinfo.num_numa, 1);
  }

  static inline int NumCores()
  {
    return (_domain_loc == nullptr)
           ? -1 : std::max<int>(_domain_loc->hwinfo.num_cores, 1);
  }

  static inline int MinThreads()
  {
    return (_domain_loc == nullptr)
           ? -1 : std::max<int>(_domain_loc->hwinfo.min_threads, 1);
  }

  static inline int MaxThreads()
  {
    return (_domain_loc == nullptr)
           ? -1 : std::max<int>(_domain_loc->hwinfo.max_threads, 1);
  }

  /**
   * Number of CPU cores currently available to the active unit.
   */
  static inline int NumUnitDomainCores()
  {
    return dash::util::Locality::NumCores();
  }

  /**
   * Number of threads currently available to the active unit.
   *
   * The returned value is calculated from unit locality data and hardware
   * specifications and can, for example, be used to set the \c num_threads
   * parameter of OpenMP sections:
   *
   * \code
   * #ifdef DASH_ENABLE_OPENMP
   *   auto n_threads = dash::util::Locality::NumUnitDomainThreads();
   *   if (n_threads > 1) {
   *     #pragma omp parallel num_threads(n_threads) private(t_id)
   *     {
   *        // ...
   *     }
   * #endif
   * \endcode
   *
   * The following configuration keys affect the number of available
   * threads:
   *
   * - <tt>DASH_DISABLE_THREADS</tt>:
   *   If set, disables multi-threading at unit scope and this method
   *   returns 1.
   * - <tt>DASH_MAX_SMT</tt>:
   *   If set, virtual SMT CPUs (hyperthreads) instead of physical cores
   *   are used to determine availble threads.
   * - <tt>DASH_MAX_UNIT_THREADS</tt>:
   *   Specifies the maximum number of threads available to a single
   *   unit.
   *
   * Note that these settings may differ between hosts.
   *
   * Example for MPI:
   *
   * <tt>
   * mpirun -host node.0 -env DASH_MAX_UNIT_THREADS 4 -n 16 myprogram
   *      : -host node.1 -env DASH_MAX_UNIT_THREADS 2 -n 32 myprogram
   * </tt>
   *
   * The DASH configuration can also be changed at run time with the
   * \c dash::util::Config interface.
   *
   * \see dash::util::Config
   * \see dash::util::TeamLocality
   *
   */
  static inline int NumUnitDomainThreads()
  {
    auto n_threads = dash::util::Locality::NumCores();
    if (dash::util::Config::get<bool>("DASH_DISABLE_THREADS")) {
      // Threads disabled in unit scope:
      n_threads  = 1;
    } else if (dash::util::Config::get<bool>("DASH_MAX_SMT")) {
      // Configured to use SMT (hyperthreads):
      n_threads *= dash::util::Locality::MaxThreads();
    } else {
      // Start one thread on every physical core assigned to this unit:
      n_threads *= dash::util::Locality::MinThreads();
    }
    if (dash::util::Config::is_set("DASH_MAX_UNIT_THREADS")) {
      n_threads  = std::min(dash::util::Config::get<int>(
                              "DASH_MAX_UNIT_THREADS"),
                            n_threads);
    }
    return n_threads;
  }

  static inline void SetNumNodes(int n)
  {
    _domain_loc->num_nodes = n;
  }

  static inline void SetNumSockets(int n)
  {
    if (_unit_loc == nullptr) {
      return;
    }
    _domain_loc->hwinfo.num_sockets = n;
  }

  static inline void SetNumNUMANodes(int n)
  {
    if (_unit_loc == nullptr) {
      return;
    }
    _domain_loc->hwinfo.num_numa = n;
  }

  static inline void SetNumCores(int n)
  {
    _domain_loc->hwinfo.num_cores = n;
  }

  static inline void SetMinThreads(int n)
  {
    _domain_loc->hwinfo.min_threads = n;
  }

  static inline void SetMaxThreads(int n)
  {
    _domain_loc->hwinfo.max_threads = n;
  }

  static int UnitNUMAId()
  {
    return _domain_loc->hwinfo.numa_id;
  }

  static int UnitCPUId()
  {
    return _domain_loc->hwinfo.cpu_id;
  }

  static inline int CPUMaxMhz()
  {
    return (_unit_loc == nullptr)
           ? -1 : std::max<int>(_unit_loc->hwinfo.max_cpu_mhz, 1);
  }

  static inline int CPUMinMhz()
  {
    return (_unit_loc == nullptr)
           ? -1 : std::max<int>(_unit_loc->hwinfo.min_cpu_mhz, 1);
  }

  static inline std::string Hostname()
  {
    return (_domain_loc == nullptr) ? "" : _domain_loc->host;
  }

  static inline std::string Hostname(dart_unit_t unit)
  {
    dart_unit_locality_t * ul;
    dart_unit_locality(DART_TEAM_ALL, unit, &ul);
    return ul->host;
  }

  static const UnitPinning Pinning(dart_unit_t unit)
  {
    dart_unit_locality_t * ul;
    dart_unit_locality(DART_TEAM_ALL, unit, &ul);
    UnitPinning pinning;
    pinning.unit        = ul->unit;
    pinning.num_cores   = ul->hwinfo.num_cores;
    pinning.cpu_id      = ul->hwinfo.cpu_id;
    pinning.numa_id     = ul->hwinfo.numa_id;
    pinning.num_threads = ul->hwinfo.max_threads;
    strncpy(pinning.host,   ul->host,       40);
    strncpy(pinning.domain, ul->domain_tag, 20);
    return pinning;
  }

  static const std::array<int, 3> & CacheSizes()
  {
    return _cache_sizes;
  }

  static const std::array<int, 3> & CacheLineSizes()
  {
    return _cache_line_sizes;
  }

  /**
   * Get local memory of system in MB
   */
  static inline int SystemMemory(){
    return _domain_loc->hwinfo.system_memory; 
  }

  /**
   * Get local memory per NUMA node in MB.
   * If system has no NUMA domains, returns
   * system memory.
   */
  static inline int NUMAMemory(){
    return _domain_loc->hwinfo.numa_memory;
  }
private:
  static void init();

private:
  static dart_unit_locality_t     * _unit_loc;
  static dart_domain_locality_t   * _domain_loc;

  static std::array<int, 3>         _cache_sizes;
  static std::array<int, 3>         _cache_line_sizes;
};

std::ostream & operator<<(
    std::ostream & os,
    const typename dash::util::Locality::UnitPinning & upi);

} // namespace util
} // namespace dash

std::ostream & operator<<(
  std::ostream                 & os,
  dash::util::Locality::Scope    scope);

std::ostream & operator<<(
  std::ostream                 & os,
  dart_locality_scope_t          scope);

#endif // DASH__UTIL__LOCALITY_H__
