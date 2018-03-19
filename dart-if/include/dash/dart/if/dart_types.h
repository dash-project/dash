/**
 * \file dart_types.h
 * \defgroup  DartTypes  Types used in the DART interface
 * \ingroup   DartInterface
 *
 * Definitions of types used in the DART interface.
 *
 */
#ifndef DART_TYPES_H_INCLUDED
#define DART_TYPES_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/**
 * Return values of functions in the DART interface.
 *
 * \ingroup DartTypes
 */
typedef enum
{
  /** Signals success */
  DART_OK           =   0,
  /** An operation is still pending */
  DART_PENDING      =   1,
  /** Invalid operation or parameters */
  DART_ERR_INVAL    =   2,
  /** Missing data encountered */
  DART_ERR_NOTFOUND =   3,
  /** DART has not been initialized */
  DART_ERR_NOTINIT  =   4,
  /** Unspecified error */
  DART_ERR_OTHER    = 999
} dart_ret_t;


/**
 * Return values of DART applications.
 *
 * \ingroup DartTypes
 */
enum {
  /** Signal success */
  DART_EXIT_SUCCESS = EXIT_SUCCESS,
  /** Signal generic abort */
  DART_EXIT_ABORT   = EXIT_FAILURE,
  /** Signal abort after failed assert */
  DART_EXIT_ASSERT  = -6
};


/**
 * Operations to be used for certain RMA and collective operations.
 * \ingroup DartTypes
 */
typedef enum
{
  /** Undefined, do not use */
  DART_OP_UNDEFINED = 0,
  /** Minimum */
  DART_OP_MIN,
  /** Maximum */
  DART_OP_MAX,
  /** Summation */
  DART_OP_SUM,
  /** Product */
  DART_OP_PROD,
  /** Binary AND */
  DART_OP_BAND,
  /** Logical AND */
  DART_OP_LAND,
  /** Binary OR */
  DART_OP_BOR,
  /** Logical OR */
  DART_OP_LOR,
  /** Binary XOR */
  DART_OP_BXOR,
  /** Logical XOR */
  DART_OP_LXOR,
  /** Replace Value */
  DART_OP_REPLACE,
  /** No operation */
  DART_OP_NO_OP
} dart_operation_t;

/**
 * Raw data types supported by the DART interface.
 *
 * \ingroup DartTypes
 */
typedef intptr_t dart_datatype_t;

#define DART_TYPE_UNDEFINED    (dart_datatype_t)(0)
/// integral data types
#define DART_TYPE_BYTE         (dart_datatype_t)(1)
#define DART_TYPE_SHORT        (dart_datatype_t)(2)
#define DART_TYPE_INT          (dart_datatype_t)(3)
#define DART_TYPE_UINT         (dart_datatype_t)(4)
#define DART_TYPE_LONG         (dart_datatype_t)(5)
#define DART_TYPE_ULONG        (dart_datatype_t)(6)
#define DART_TYPE_LONGLONG     (dart_datatype_t)(7)
#define DART_TYPE_ULONGLONG    (dart_datatype_t)(8)
/// floating point data types
#define DART_TYPE_FLOAT        (dart_datatype_t)(9)
#define DART_TYPE_DOUBLE       (dart_datatype_t)(10)
#define DART_TYPE_LONG_DOUBLE  (dart_datatype_t)(11)
/// Reserved, do not use!
#define DART_TYPE_LAST         (dart_datatype_t)(12)


/** size for integral \c size_t */
#if (UINT32_MAX == SIZE_MAX)
#  define DART_TYPE_SIZET DART_TYPE_ULONG
#elif (UINT64_MAX == SIZE_MAX)
#  define DART_TYPE_SIZET DART_TYPE_ULONGLONG
#else
#  error "Cannot determine DART type for size_t!"
#endif

/**
 * Data type for storing a unit ID
 * \ingroup DartTypes
 */
typedef int32_t dart_unit_t;

/**
 * Undefined unit ID.
 * \ingroup DartTypes
 */
#define DART_UNDEFINED_UNIT_ID ((dart_unit_t)(-1))

/**
 * Data type for storing a global unit ID
 * \ingroup DartTypes
 */
typedef struct dart_global_unit {
#ifdef __cplusplus
  constexpr dart_global_unit() : id(DART_UNDEFINED_UNIT_ID) { }
  constexpr dart_global_unit(dart_unit_t uid) : id(uid) { }
#endif
  dart_unit_t id;
} dart_global_unit_t;


/**
 * Data type for storing a unit ID
 * relative to a team.
 * \ingroup DartTypes
 */
typedef struct dart_local_unit {
#ifdef __cplusplus
  constexpr dart_local_unit() : id(DART_UNDEFINED_UNIT_ID) { }
  constexpr dart_local_unit(dart_unit_t uid) : id(uid) { }
#endif
  dart_unit_t id;
} dart_team_unit_t;

/**
 * Create a \c dart_team_unit_t from a \ref dart_unit_t.
 * \ingroup DartTypes
 */
static inline
dart_team_unit_t dart_create_team_unit(dart_unit_t unit)
{
  dart_team_unit_t tmp = {unit};
  return tmp;
}

/**
 * Create a \c dart_team_unit_t from a \ref dart_unit_t.
 *
 * This is a wrapper for \ref dart_create_team_unit.
 *
 * \ingroup DartTypes
 */
#define DART_TEAM_UNIT_ID(__u) (dart_create_team_unit(__u))


/**
 * Create a \c dart_global_unit_t from a \ref dart_unit_t.
 *
 * \ingroup DartTypes
 */
static inline
dart_global_unit_t dart_create_global_unit(dart_unit_t unit)
{
  dart_global_unit_t tmp = {unit};
  return tmp;
}

/**
 * Create a \c dart_global_unit_t from a \ref dart_unit_t.
 *
 * This is a wrapper for \ref dart_create_global_unit.
 *
 * \ingroup DartTypes
 */
#define DART_GLOBAL_UNIT_ID(__u) (dart_create_global_unit(__u))

/**
 * A \ref dart_team_unit_t representing an undefined team-relative unit.
 *
 * \see DART_UNDEFINED_UNIT_ID
 *
 * \ingroup DartTypes
 */
#define DART_UNDEFINED_TEAM_UNIT_ID DART_TEAM_UNIT_ID(DART_UNDEFINED_UNIT_ID)

/**
 * A \ref dart_global_unit_t representing an undefined global unit.
 *
 * \see DART_UNDEFINED_UNIT_ID
 *
 * \ingroup DartTypes
 */
#define DART_UNDEFINED_GLOBAL_UNIT_ID DART_GLOBAL_UNIT_ID(DART_UNDEFINED_UNIT_ID)

/**
 * Data type for storing a team ID
 * \ingroup DartTypes
 */
typedef int16_t dart_team_t;

/**
 * Undefined team ID.
 * \ingroup DartTypes
 */
#define DART_UNDEFINED_TEAM_ID ((dart_team_t)(-1))


/**
 * Levels of thread-support offered by DART.
 * \ref DART_THREAD_MULTIPLE is supported if
 * DART has been build with \c DART_ENABLE_THREADSUPPORT
 * and the underlying communication backend supports
 * thread-safe access.
 *
 */
typedef enum
{
  /** No support for thread-based concurrency in DART is provided. */
  DART_THREAD_SINGLE = 0,
  /**
   * Support for thread-based concurrency is provided by DART and
   * the underlying runtime.
   */
  DART_THREAD_MULTIPLE = 10
} dart_thread_support_level_t;

/**
 * Scopes of locality domains.
 *
 * Enum values are ordered by scope level in the locality hierarchy.
 * Consequently, the comparison \c (scope_a > scope_b) is valid
 * and evaluates to \c true if \c scope_a is a parent scope of
 * \c scope_b.
 *
 * \ingroup DartTypes
 */
typedef enum
{
  DART_LOCALITY_SCOPE_UNDEFINED  =   -1,
  /** Global locality scope, components may be heterogenous. */
  DART_LOCALITY_SCOPE_GLOBAL     =    0,
  /** Group of domains in specific locality scope, used as parent scope of
   *  domains in a user-defined group at any locality level. */
  DART_LOCALITY_SCOPE_GROUP      =    1,
  /** Interconnect topology scope, components may be heterogenous. */
  DART_LOCALITY_SCOPE_NETWORK    =   50,
  /** Node-level locality scope, components may be heterogenous. */
  DART_LOCALITY_SCOPE_NODE       =  100,
  /** Locality in a group of hetereogenous components in different NUMA
   *  domains. */
  DART_LOCALITY_SCOPE_MODULE     =  200,
  /** Locality of homogenous components in different NUMA domains. */
  DART_LOCALITY_SCOPE_NUMA       =  300,
  /** Locality of homogenous components in the same NUMA domain at
   *  process-level, i.e. of a unit-addressable, homogenous entity.
   *  A single unit corresponds to a DART (e.g. MPI) process and can
   *  occupy multiple homogenous cores, e.g. for multithreading. */
  DART_LOCALITY_SCOPE_UNIT       =  400,
  /** Locality at level of physical processor package. Cannot be
   *  referenced by DART directly. */
  DART_LOCALITY_SCOPE_PACKAGE    =  500,
  /** Locality at processor uncore (system agent) level. Intel only.
   *  Cannot be referenced by DART directly. */
  DART_LOCALITY_SCOPE_UNCORE     =  510,
  /** Locality at level of physical CPU cache. Cannot be referenced by
   *  DART directly. */
  DART_LOCALITY_SCOPE_CACHE      =  530,
  /** Locality at physical processing core level. Cannot be referenced
   *  by DART directly. */
  DART_LOCALITY_SCOPE_CORE       =  550,
  /** Locality at logical CPU level (SMT thread). Cannot be referenced
   *  by DART directly. */
  DART_LOCALITY_SCOPE_CPU        =  600
}
dart_locality_scope_t;

/** Maximum size of a host name string in \ref dart_hwinfo_t */
#define DART_LOCALITY_HOST_MAX_SIZE       ((int)(30))
/** Maximum size of a domain tag string in \ref dart_hwinfo_t */
#define DART_LOCALITY_DOMAIN_TAG_MAX_SIZE ((int)(32))
/** Maximum number of domain scopes in \ref dart_hwinfo_t */
#define DART_LOCALITY_MAX_DOMAIN_SCOPES   ((int)(12))
/** Maximum size of a domain tag string in \ref dart_hwinfo_t
 * \todo Unused? */
#define DART_LOCALITY_UNIT_MAX_CPUS       ((int)(64))
/** Maximum number of NUMA domains supported */
#define DART_LOCALITY_MAX_NUMA_ID         ((int)(16))
/** Maximum number of cache levels supported in \ref dart_hwinfo_t */
#define DART_LOCALITY_MAX_CACHE_LEVELS    ((int)( 5))

typedef struct {
    /** The domain's scope identifier. */
    dart_locality_scope_t           scope;
    /** The domain's relative index among its siblings in the scope. */
    int                             index;
}
dart_locality_scope_pos_t;

/**
 * Hardware locality information for a single locality domain.
 *
 * Note that \c dart_domain_locality_t must have static size as it is
 * used for an all-to-all exchange of locality data across all units
 * using \c dart_allgather.
 *
 * \ingroup DartTypes
 */
typedef struct
{
    /** Hostname of the domain's node or 0 if unspecified. */
    char  host[DART_LOCALITY_HOST_MAX_SIZE];

    /** Total number of CPUs in the associated domain. */
    int   num_cores;

    int   num_numa;

    int   numa_id;

    /** The unit's affine core, unique identifier within a processing
     *  module. */
    int   core_id;
    /** The unit's affine processing unit (SMP), unique identifier
     *  within a processing module. */
    int   cpu_id;

    /** Minimum clock frequency of CPUs in the domain. */
    int   min_cpu_mhz;
    /** Maximum clock frequency of CPUs in the domain. */
    int   max_cpu_mhz;

    /** Cache sizes by cache level (L1, L2, L3). */
    int   cache_sizes[DART_LOCALITY_MAX_CACHE_LEVELS];
    /** Cache line sizes by cache level (L1, L2, L3). */
    int   cache_line_sizes[DART_LOCALITY_MAX_CACHE_LEVELS];
    /** IDs of cache modules by level (L1, L2, L3), unique within domain. */
    int   cache_ids[DART_LOCALITY_MAX_CACHE_LEVELS];

    /** Minimum number of CPU threads per core. */
    int   min_threads;
    /** Maximum number of CPU threads per core. */
    int   max_threads;

    /** Maximum local shared memory bandwidth in MB/s. */
    int   max_shmem_mbps;

    /** Maximum allocatable memory per node in bytes */
    int   system_memory_bytes;

    /** Maximum memory per numa node in bytes. */
    int   numa_memory_bytes;

    /** Ancestor locality scopes in bottom-up hierarchical order. */
    dart_locality_scope_pos_t scopes[DART_LOCALITY_MAX_DOMAIN_SCOPES];

    int   num_scopes;
}
dart_hwinfo_t;

typedef struct
{
    /** Hostname of the module's parent node */
    char  host[DART_LOCALITY_HOST_MAX_SIZE];

    /** Hostname of the module, including the parent hostname prefix. */
    char  module[DART_LOCALITY_HOST_MAX_SIZE];

    /** The module's parent scope and its relative position in the scope. */
    dart_locality_scope_pos_t pos;
}
dart_module_location_t;

/**
 * A domain is a group of processing entities such as cores in a specific
 * NUMA domain or a Intel MIC entity.
 * Domains are organized in a hierarchy.
 * In this, a domain may consist of heterogenous child domains.
 * Processing entities in domains on the lowest locality level are
 * homogenous.
 *
 * Domains represent the actual hardware topology but also can represent
 * grouping from user-defined team specifications.
 *
 * \ingroup DartTypes
 *
 * Use cases:
 *
 * - To determine whether units in a domain have access to common shared
 *   memory, test if domain descriptor field
 *
 *     - \c num_nodes is set to 1, or
 *     - \c scope is set to \c DART_LOCALITY_SCOPE_NODE or greater.
 *
 *
 * - The maximum number of threads for a single unit, e.g. for MKL routines,
 *   can be calculated as:
 *
 *     <tt>
 *       (dloc.num_cores x dloc.num_threads)
 *     </tt>
 *
 *   from a domain descriptor \c dloc with scope \c DART_LOCALITY_SCOPE_UNIT.
 *
 *
 * - A simple metric of processing power of components in a homogenous domain
 *   (minimum number of instructions per second) can be calculated as:
 *
 *     <tt>
 *       dmhz(dloc) = (dloc.num_cores x dloc.min_threads x dloc.min_cpu_mhz)
 *     </tt>
 *
 *   This metric then can be used to balance workload between homogenous
 *   domains with different processing components.
 *   A simple balance factor \c wb can be calculated as:
 *
 *     <tt>
 *       wb = dmhz(dloc_a) / dmhz(dloc_b)
 *     </tt>
 *
 *   from domain descriptors \c dloc_a and \c dloc_b.
 *
 *
 * Illustrating example:
 *
 * \code
 *   domain (top level, heterogenous)
 *   domain_tag:  "."
 *   host:        "number-crunch-9000"
 *   scope:       DART_LOCALITY_SCOPE_GLOBAL
 *   level:         0
 *   num_nodes:     4
 *   num_cores:   544 (4 nodes x 136 cores per node)
 *   min_threads:   2
 *   max_threads:   4
 *   num_domains:   4 (4 nodes)
 *   domains:
 *   :
 *   |-- domain (compute node, heterogenous)
 *   :   domain_tag:  ".0"
 *   :   scope:       DART_LOCALITY_SCOPE_NODE
 *   :   level:         1
 *   :   num_nodes:     1
 *   :   num_cores:   136 (16 host cores + 2x60 MIC cores)
 *   :   min_threads:   2
 *   :   max_threads:   4
 *   :   num_domains:   3 (1 host + 2 MICs)
 *   :   domains:
 *   :   :
 *   :   |-- domain (host, homogenous)
 *   :   :   domain_tag:  ".0.0"
 *   :   :   scope:       DART_LOCALITY_SCOPE_PROC_GROUP
 *   :   :   level:         2
 *   :   :   num_nodes:     1
 *   :   :   num_numa:      2
 *   :   :   num_cores:    16
 *   :   :   min_threads:   2
 *   :   :   max_threads:   2
 *   :   :   num_domains:   2
 *   :   :   :
 *   :   :   |-- domain (NUMA domain at host)
 *   :   :   :   domain_tag:  ".0.0.1"
 *   :   :   :   scope:       DART_LOCALITY_SCOPE_UNIT
 *   :   :   :   level:        3
 *   :   :   :   num_nodes:    1
 *   :   :   :   num_numa:     1
 *   :   :   :   num_cores:    8
 *   :   :   :   num_domains:  8
 *   :   :   :   :
 *   :   :   :   '
 *   :   :   :   ...
 *   :   :   :
 *   :   :   '-- domain (NUMA domain at host)
 *   :   :       domain_tag:  ".0.0.1"
 *   :   :       scope:       DART_LOCALITY_SCOPE_UNIT
 *   :   :       level:        3
 *   :   :       num_nodes:    1
 *   :   :       num_numa:     1
 *   :   :       num_cores:    8
 *   :   :       num_domains:  8
 *   :   :       :
 *   :   :       '
 *   :   :       ...
 *   :   :
 *   :   |-- domain (MIC, homogenous)
 *   :   :   domain_tag:  ".0.1"
 *   :   :   scope:       DART_LOCALITY_SCOPE_PROC_GROUP
 *   :   :   level:         2
 *   :   :   num_nodes:     1
 *   :   :   num_cores:    60
 *   :   :   min_threads:   4
 *   :   :   max_threads:   4
 *   :   :   num_domains:   0
 *   :   :
 *   :   '-- domain (MIC, homogenous)
 *   :       domain_tag:  ".0.2"
 *   :       scope:       DART_LOCALITY_SCOPE_PROC_GROUP
 *   :       level:         2
 *   :       num_nodes:     1
 *   :       num_cores:    60
 *   :       min_threads:   4
 *   :       max_threads:   4
 *   :       domains:
 *   :       num_domains:   2
 *   :       :
 *   :       |-- domain (unit of MIC cores, homogenous)
 *   :       :   domain_tag:  ".0.2.0"
 *   :       :   scope:       DART_LOCALITY_SCOPE_UNIT
 *   :       :   level:        3
 *   :       :   num_nodes:    1
 *   :       :   num_cores:   30
 *   :       :   num_domains:  0
 *   :       :
 *   :       '-- domain (unit of MIC cores, homogenous)
 *   :           domain_tag:  ".0.2.1"
 *   :           scope:       DART_LOCALITY_SCOPE_UNIT
 *   :           level:        3
 *   :           num_nodes:    1
 *   :           num_cores:   30
 *   :           num_domains:  0
 *   :
 *   |-- domain (compute node, heterogenous)
 *   :   domain_tag:  ".1"
 *   :   scope:       DART_LOCALITY_SCOPE_NODE
 *   :   level:         1
 *   :   num_cores:   136
 *   :   num_domains:   3
 *   :   domains:
 *   :   :
 *   :   '
 *   :   ...
 *   '
 *   ...
 * \endcode
 *
 */
struct dart_domain_locality_s
{
    /** Hostname of the domain's node or 0 if unspecified. */
    char host[DART_LOCALITY_HOST_MAX_SIZE];

    /**
     * Hierarchical domain identifier, represented as dot-separated list
     * of relative indices on every level in the locality hierarchy.
     */
    char domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];

    struct dart_domain_locality_s ** aliases;

    int                              num_aliases;

    /** Locality scope of the domain. */
    dart_locality_scope_t            scope;
    /** Level in the domain locality hierarchy. */
    int                              level;

    /** The domain's global index within its scope. */
    int                              global_index;
    /** The domain's index within its parent domain. */
    int                              relative_index;

    /** Pointer to descriptor of parent domain or 0 if no parent domain
     *  is specified. */
    struct dart_domain_locality_s  * parent;

    /** Number of subordinate domains. */
    int                              num_domains;
    /** Array of subordinate domains of size \c num_domains or 0 if no
     *  subdomains are specified. */
    struct dart_domain_locality_s ** children;

    /** Whether sub-domains have identical hardware configuration. */
    int                              is_symmetric;

    /** Team associated with the domain. */
    dart_team_t                      team;
    /** Number of units in the domain. */
    int                              num_units;
    /** Global IDs of units in the domain. */
    dart_global_unit_t             * unit_ids;

    /* The number of compute nodes in the domain. */
    int                              num_nodes;
    /* Node (machine) index of the domain or -1 if domain contains
     * multiple compute nodes. */
    int                              node_id;

    /* Number of cores in the domain. Cores may be heterogeneous unless
     * `is_symmetric` is different from 0. */
    int                              num_cores;

    /* The minimum size of the physical or logical shared memory
     * accessible by all units in the domain.
     */
    int                              shared_mem_bytes;
};
struct dart_domain_locality_s;
typedef struct dart_domain_locality_s
    dart_domain_locality_t;

/**
 * Locality and topology information of a single unit.
 * Processing entities grouped in a single unit are homogenous.
 * Each unit is a member of one specific locality domain.
 *
 * Note that \c dart_unit_locality_t must have static size as it is
 * used for an all-to-all exchange of locality data across all units
 * using \c dart_allgather.
 *
 * \ingroup DartTypes
 */
typedef struct {
    /** Unit ID relative to team. */
    dart_team_unit_t         unit;

    /** Team ID. */
    dart_team_t              team;

    /** Hardware specification of the unit's affinity. */
    dart_hwinfo_t            hwinfo;

    char                     domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
}
dart_unit_locality_t;

/**
 * \ingroup DartTypes
 */
typedef struct
{
  int log_enabled;
}
dart_config_t;

/**
 * Create a strided data type using blocks of size \c blocklen and a stride
 * of \c stride. The number of elements copied using the resulting datatype
 * has to be a multiple of \c blocklen.
 *
 * \param      basetype   The type of elements in the blocks.
 * \param      stride     The stride between blocks.
 * \param      blocklen   The number of elements of type \c basetype in each block.
 * \param[out] newtype    The newly created data type.
 *
 * \return \ref DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartTypes
 */
dart_ret_t
dart_type_create_strided(
  dart_datatype_t   basetype,
  size_t            stride,
  size_t            blocklen,
  dart_datatype_t * newtype);


/**
 * Create an indexed data type using \c count blocks of size \c blocklen[i]
 * with offsets \c offset[i] for each <tt>0 <= i < count</tt>. The number of
 * elements copied using the resulting datatype has to be a multiple of
 * Sum(\c blocklen[0:i]).
 *
 * \param      basetype The type of elements in the blocks.
 * \param      count    The number of blocks.
 * \param      blocklen The number of elements of type \c basetype in block[i].
 * \param      offset   The offset of block[i].
 * \param[out] newtype  The newly created data type.
 *
 * \return \ref DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartTypes
 */
dart_ret_t
dart_type_create_indexed(
  dart_datatype_t   basetype,
  size_t            count,
  const size_t      blocklen[],
  const size_t      offset[],
  dart_datatype_t * newtype);

/**
 * Destroy a data type that was previously created using
 * \ref dart_type_create_strided or \ref dart_type_create_indexed.
 *
 * Data types can be destroyed before pending operations using that type have
 * completed. However, after destruction a type may not be used to start
 * new operations.
 *
 * \param      dart_type The type to be destroyed.
 *
 * \return \ref DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartTypes
 */
dart_ret_t
dart_type_destroy(dart_datatype_t *dart_type);

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_TYPES_H_INCLUDED */
