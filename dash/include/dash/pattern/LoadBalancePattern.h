#ifndef DASH__LOAD_BALANCE_PATTERN_H__INCLUDED
#define DASH__LOAD_BALANCE_PATTERN_H__INCLUDED

#include <functional>
#include <array>
#include <type_traits>

#include <dash/Types.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>

#include <dash/pattern/PatternProperties.h>
#include <dash/pattern/internal/PatternArguments.h>

#include <dash/util/TeamLocality.h>
#include <dash/util/LocalityDomain.h>

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>


namespace dash {

class UnitClockFreqMeasure
{
private:
  typedef dash::util::TeamLocality     TeamLocality_t;
  typedef dash::util::LocalityDomain   LocalityDomain_t;

public:
  /**
   * Returns unit CPU capacities as percentage of the team's total CPU
   * capacity average, e.g. vector of 1's if all units have identical
   * CPU capacity.
   */
  static std::vector<double> unit_weights(
    const TeamLocality_t & tloc)
  {
    std::vector<double> unit_cpu_capacities;
    double sum = 0;

    for (auto u : tloc.global_units()) {
      auto   unit_loc      = tloc.unit_locality(u);
      double unit_cpu_cap  = unit_loc.num_cores() *
                             unit_loc.num_threads() *
                             unit_loc.cpu_mhz();
      sum += unit_cpu_cap;
      unit_cpu_capacities.push_back(unit_cpu_cap);
    }
    double mean = sum / tloc.global_units().size();
    std::transform(unit_cpu_capacities.begin(),
                   unit_cpu_capacities.end(),
                   unit_cpu_capacities.begin(),

                   [&](double v) { return v / mean; });
    return unit_cpu_capacities;
  }
};

class BytesPerCycleMeasure
{
private:
  typedef dash::util::TeamLocality     TeamLocality_t;
  typedef dash::util::LocalityDomain   LocalityDomain_t;

public:
  /**
   * Shared memory bandwidth capacities of every unit factored by the
   * mean memory bandwidth capacity of all units in the team.
   * Consequently, a vector of 1's is returned if all units have identical
   * memory bandwidth.
   *
   * The memory bandwidth balancing weight for a unit is relative to the
   * bytes/cycle measure of its affine core and considers the
   * lower bound ("maximum of minimal") throughput between the unit to
   * any other unit in the host system's shared memory domain.
   *
   * This is mostly relevant for accelerators that have no direct access
   * to the host system's shared memory.
   * For example, Intel MIC accelerators are connected to the host with a
   * 6.2 GB/s PCIE bus and a single MIC core operates at 1.1 Ghz with 4
   * hardware threads.
   * The resulting measure \BpC (bytes/cycle) is calculated as:
   *
   *   Mpk = 6.2 GB/s
   *   Cpk = 1.1 Ghz * 4 = 4.4 G cycles/s
   *   BpC = Mpk / Cpk   = 5.63 bytes/cycle
   *
   * The principal idea is that any data used in operations on the MIC
   * target must be moved over the slow PCIE interconnect first.
   * The offload overhead therefore reduces the amount of data assigned to
   * a MIC accelerator, despite its superior ops/s performance.
   *
   */
  static std::vector<double> unit_weights(
    const TeamLocality_t & tloc)
  {
    std::vector<double> unit_mem_perc;

#if 0
    // TODO: Calculate and assign neutral weights for units located at
    //       cores with unknown memory bandwidth.

    std::vector<size_t> unit_mem_capacities;
    size_t total_mem_capacity = 0;

    // Calculate average memory bandwidth first:
    for (auto u : tloc.units()) {
      auto & unit_loc     = tloc.unit_locality(u);
      size_t unit_mem_cap = std::max<int>(0, unit_loc.max_shmem_mbps());
      if (unit_mem_cap > 0) {
        total_mem_capacity += unit_mem_cap;
      }
      unit_mem_capacities.push_back(unit_mem_cap);
    }
    if (total_mem_capacity == 0) {
      total_mem_capacity = tloc.units().size();
    }
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_mem_bandwidth_weights",
                       total_mem_capacity);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_mem_bandwidth_weights",
                       unit_mem_capacities);

    double avg_mem_capacity = static_cast<double>(total_mem_capacity) /
                              tloc.units().size();

    // Use average value for units with unknown memory bandwidth:
    for (auto membw = unit_mem_capacities.begin();
         membw != unit_mem_capacities.end(); ++membw) {
      if (*membw <= 0) {
        *membw = avg_mem_capacity;
      }
    }
#endif

    std::vector<double> unit_bytes_per_cycle;
    double total_bytes_per_cycle = 0;

    // Calculating bytes/cycle per core for every unit:
    for (auto u : tloc.global_units()) {
      auto   unit_loc     = tloc.unit_locality(u);
      double unit_mem_bw  = std::max<int>(0, unit_loc.max_shmem_mbps());
      double unit_core_fq = unit_loc.num_threads() *
                            unit_loc.cpu_mhz();
      double unit_bps     = unit_mem_bw / unit_core_fq;
      unit_bytes_per_cycle.push_back(unit_bps);
      total_bytes_per_cycle += unit_bps;
    }

    double avg_bytes_per_cycle =
      static_cast<double>(total_bytes_per_cycle) / tloc.global_units().size();

    for (auto unit_bps : unit_bytes_per_cycle) {
      unit_mem_perc.push_back(unit_bps / avg_bytes_per_cycle);
    }
    return unit_mem_perc;
  }
};

/**
 * Irregular dynamic pattern.
 *
 * \todo
 * Should subclass or delegate to dash::CSRPattern as implementation is
 * identical apart from comptation of _local_sizes.
 *
 * \concept{DashPatternConcept}
 */
template<
  dim_t      NumDimensions,
  typename   CompBasedMeasure = UnitClockFreqMeasure,
  typename   MemBasedMeasure  = BytesPerCycleMeasure,
  MemArrange Arrangement      = dash::ROW_MAJOR,
  typename   IndexType        = dash::default_index_t >
class LoadBalancePattern;

/**
 * Irregular dynamic pattern.
 * Specialization for 1-dimensional data.
 *
 * \todo
 * Should subclass or delegate to dash::CSRPattern as implementation is
 * identical apart from comptation of _local_sizes.
 *
 * \todo
 * Performance measures used for load balance weights (CPU capacity, memory
 * bandwidth, ...) should be policies, template parameters implementing
 * well-defined concepts, so this class does not have to be re-implemented
 * for every load-balance scheme.
 * Using CompBasedMeasure, MemBasedMeasure for now.
 *
 * \concept{DashPatternConcept}
 */
template<
  typename   CompBasedMeasure,
  typename   MemBasedMeasure,
  MemArrange Arrangement,
  typename   IndexType >
class LoadBalancePattern<
        1, CompBasedMeasure, MemBasedMeasure, Arrangement, IndexType>
{
private:
  static const dim_t NumDimensions = 1;

public:
  static constexpr char const * PatternName = "LoadBalancePattern1D";

public:
  /// Satisfiable properties in pattern property category Partitioning:
  typedef pattern_partitioning_properties<
              // Minimal number of blocks in every dimension, i.e. one block
              // per unit.
              pattern_partitioning_tag::minimal,
              // Block extents are constant for every dimension.
              pattern_partitioning_tag::rectangular,
              // Identical number of elements in every block.
              pattern_partitioning_tag::balanced,
              // Size of blocks may differ.
              pattern_partitioning_tag::unbalanced,
              // Partitioning is dynamic.
              pattern_partitioning_tag::dynamic
              // Partitioning is load-balanced.
//            pattern_partitioning_tag::load_balanced
          > partitioning_properties;
  /// Satisfiable properties in pattern property category Mapping:
  typedef pattern_mapping_properties<
              // Number of blocks assigned to a unit may differ.
              pattern_mapping_tag::unbalanced
          > mapping_properties;
  /// Satisfiable properties in pattern property category Layout:
  typedef pattern_layout_properties<
              // Elements are contiguous in local memory within single block.
              pattern_layout_tag::blocked,
              // Local element order corresponds to a logical linearization
              // within single blocks.
              pattern_layout_tag::linear
          > layout_properties;

private:
  /// Fully specified type definition of self
  typedef LoadBalancePattern<
            NumDimensions,
            CompBasedMeasure,
            MemBasedMeasure,
            Arrangement,
            IndexType>
    self_t;
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    LocalMemoryLayout_t;
  typedef CartesianSpace<NumDimensions, SizeType>
    BlockSpec_t;
  typedef DistributionSpec<NumDimensions>
    DistributionSpec_t;
  typedef TeamSpec<NumDimensions, IndexType>
    TeamSpec_t;
  typedef SizeSpec<NumDimensions, SizeType>
    SizeSpec_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
  typedef internal::PatternArguments<NumDimensions, IndexType>
    PatternArguments_t;
  typedef dash::util::TeamLocality
    TeamLocality_t;
  typedef dash::util::LocalityDomain
    LocalityDomain_t;

public:
  typedef IndexType   index_type;
  typedef SizeType    size_type;
  typedef ViewSpec_t  viewspec_type;
  typedef struct {
    team_unit_t                            unit;
    IndexType                             index;
  } local_index_t;
  typedef struct {
    team_unit_t                            unit;
    std::array<index_type, NumDimensions> coords;
  } local_coords_t;

public:
  /**
   * Constructor.
   */
  LoadBalancePattern(
    /// Size spec of the pattern.
    const SizeSpec_t     & sizespec,
    /// Locality hierarchy of the team.
    TeamLocality_t       & team_loc)
  : _size(sizespec.size()),
    _unit_cpu_weights(
       CompBasedMeasure::unit_weights(team_loc)),
    _unit_membw_weights(
       MemBasedMeasure::unit_weights(team_loc)),
    _unit_load_weights(
       initialize_load_weights(
         _unit_cpu_weights,
         _unit_membw_weights)),
    _local_sizes(
      initialize_local_sizes(
        sizespec.size(),
        team_loc)),
    _block_offsets(
      initialize_block_offsets(
        _local_sizes)),
    _memory_layout(
      std::array<SizeType, 1> {{ _size }}),
    _blockspec(
      initialize_blockspec(
        _size,
        _local_sizes)),
    _distspec(dash::BLOCKED),
    _team(&team_loc.team()),
    _myid(_team->myid()),
    _teamspec(*_team),
    _nunits(_team->size()),
    _local_size(
      initialize_local_extent(
        _team->myid(),
        _local_sizes)),
    _local_memory_layout(
      std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(
      initialize_local_capacity(
        _local_sizes))
  {
    DASH_LOG_TRACE("LoadBalancePattern()", "(sizespec, dist, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("LoadBalancePattern()", "LoadBalancePattern initialized");
  }

  /**
   * Constructor.
   */
  LoadBalancePattern(
    /// Size spec of the pattern.
    const SizeSpec_t     & sizespec,
    /// Team containing units to which this pattern maps its elements.
    dash::Team           & team = dash::Team::All())
  : LoadBalancePattern(sizespec, TeamLocality_t(team))
  { }

  LoadBalancePattern(const self_t & other) = default;

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  LoadBalancePattern(self_t & other)
  : LoadBalancePattern(static_cast<const self_t &>(other))
  { }

  /**
   * Equality comparison operator.
   */
  bool operator==(
    /// Pattern instance to compare for equality
    const self_t & other) const
  {
    if (this == &other) {
      return true;
    }
    // no need to compare all members as most are derived from
    // constructor arguments.
    return(
      _size        == other._size &&
      _local_sizes == other._local_sizes &&
      _teamspec    == other._teamspec
    );
  }

  /**
   * Inquality comparison operator.
   */
  bool operator!=(
    /// Pattern instance to compare for inequality
    const self_t & other) const
  {
    return !(*this == other);
  }

  /**
   * Resolves the global index of the first local element in the pattern.
   *
   * \see DashPatternConcept
   */
  IndexType lbegin() const
  {
    return _lbegin;
  }

  /**
   * Resolves the global index past the last local element in the pattern.
   *
   * \see DashPatternConcept
   */
  IndexType lend() const
  {
    return _lend;
  }

  ////////////////////////////////////////////////////////////////////////////
  /// unit_at
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Convert given point in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    /// Absolute coordinates of the point
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t                           & viewspec) const
  {
    return unit_at(coords[0] + viewspec[0].offset);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    return unit_at(g_coords[0]);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    /// Global linear element offset
    IndexType          global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec) const
  {
    return unit_at(global_pos + viewspec[0].offset);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    /// Global linear element offset
    IndexType g_index) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.unit_at()", g_index);

    for (team_unit_t unit_idx{0}; unit_idx < _nunits; ++unit_idx) {
      if (g_index < _local_sizes[unit_idx]) {
        DASH_LOG_TRACE_VAR("LoadBalancePattern.unit_at >", unit_idx);
        return unit_idx;
      }
      g_index -= _local_sizes[unit_idx];
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LoadBalancePattern.unit_at: " <<
      "global index " << g_index << " is out of bounds");
  }

  ////////////////////////////////////////////////////////////////////////////
  /// extent
  ////////////////////////////////////////////////////////////////////////////

  /**
   * The number of elements in this pattern in the given dimension.
   *
   * \see  blocksize()
   * \see  local_size()
   * \see  local_extent()
   *
   * \see  DashPatternConcept
   */
  IndexType extent(dim_t dim) const
  {
    DASH_ASSERT_EQ(
      0, dim,
      "Wrong dimension for Pattern::local_extent. " <<
      "Expected dimension = 0, got " << dim);
    return _size;
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit in the given dimension.
   *
   * \see  local_extents()
   * \see  blocksize()
   * \see  local_size()
   * \see  extent()
   *
   * \see  DashPatternConcept
   */
  IndexType local_extent(dim_t dim) const
  {
    DASH_ASSERT_EQ(
      0, dim,
      "Wrong dimension for Pattern::local_extent. " <<
      "Expected dimension = 0, got " << dim);
    return _local_size;
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * given unit, by dimension.
   *
   * \see  local_extent()
   * \see  blocksize()
   * \see  local_size()
   * \see  extent()
   *
   * \see  DashPatternConcept
   */
  std::array<SizeType, NumDimensions> local_extents(
      team_unit_t unit) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.local_extents()", unit);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.local_extents >",
                       _local_sizes[unit]);
    return std::array<SizeType, 1> {{ _local_sizes[unit] }};
  }

  ////////////////////////////////////////////////////////////////////////////
  /// local
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Convert given local coordinates and viewspec to linear local offset
   * (index).
   *
   * \see DashPatternConcept
   */
  IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const
  {
    return local_coords[0] + viewspec[0].offset;
  }

  /**
   * Convert given local coordinates to linear local offset (index).
   *
   * \see DashPatternConcept
   */
  IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    return local_coords[0];
  }

  /**
   * Converts global coordinates to their associated unit and its respective
   * local coordinates.
   *
   * NOTE: Same as \c local_index.
   *
   * \see  DashPatternConcept
   */
  local_coords_t local(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    local_index_t  l_index =  local(g_coords[0]);
    local_coords_t l_coords;
    l_coords.unit      = l_index.unit;
    l_coords.coords[0] = l_index.index;
    return l_coords;
  }

  /**
   * Converts global index to its associated unit and respective local index.
   *
   * NOTE: Same as \c local_index.
   *
   * \see  DashPatternConcept
   */
  local_index_t local(
    IndexType g_index) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.local()", g_index);
    local_index_t l_index;

    for (team_unit_t unit_idx{0}; unit_idx < _nunits; ++unit_idx) {
      if (g_index < _local_sizes[unit_idx]) {
        l_index.unit  = unit_idx;
        l_index.index = g_index;
        DASH_LOG_TRACE("LoadBalancePattern.local >",
                       "unit:",  l_index.unit,
                       "index:", l_index.index);
        return l_index;
      }
      g_index -= _local_sizes[unit_idx];
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LoadBalancePattern.local: " <<
      "global index " << g_index << " is out of bounds");
  }

  /**
   * Converts global coordinates to their associated unit's respective
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> local_coords(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    local_index_t l_index = local(g_coords[0]);
    return std::array<IndexType, 1> {{ l_index.index }};
  }

  /**
   * Converts global coordinates to their associated unit and their
   * respective local index.
   *
   * \see  DashPatternConcept
   */
  local_index_t local_index(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    return local(g_coords[0]);
  }

  ////////////////////////////////////////////////////////////////////////////
  /// global
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
      team_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.global()", unit);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.global()", local_coords);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.global", _nunits);
    if (_nunits < 2) {
      return local_coords;
    }
    // Initialize global index with element phase (= local coords):
    index_type glob_index = _block_offsets[unit] + local_coords[0];
    DASH_LOG_TRACE_VAR("LoadBalancePattern.global >", glob_index);
    return std::array<IndexType, 1> {{ glob_index }};
  }

  /**
   * Converts local coordinates of active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
    const std::array<IndexType, NumDimensions> & l_coords) const
  {
    return global(_team->myid(), l_coords);
  }

  /**
   * Resolve an element's linear global index from the given unit's local
   * index of that element.
   *
   * \see  at  Inverse of local()
   *
   * \see  DashPatternConcept
   */
  IndexType global(
    team_unit_t unit,
    IndexType l_index) const
  {
    return global(unit, std::array<IndexType, 1> {{ l_index }})[0];
  }

  /**
   * Resolve an element's linear global index from the calling unit's local
   * index of that element.
   *
   * \see  at  Inverse of local()
   *
   * \see  DashPatternConcept
   */
  IndexType global(
    IndexType l_index) const
  {
    return global(_team->myid(), std::array<IndexType, 1> {{ l_index }})[0];
  }

  /**
   * Resolve an element's linear global index from a given unit's local
   * coordinates of that element.
   *
   * \see  at
   *
   * \see  DashPatternConcept
   */
  IndexType global_index(
    team_unit_t unit,
    const std::array<IndexType, NumDimensions> & l_coords) const
  {
    auto g_index = global(unit, l_coords[0]);
    return g_index;
  }

  ////////////////////////////////////////////////////////////////////////////
  /// at
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Global coordinates to local index.
   *
   * Convert given global coordinates in pattern to their respective
   * linear local index.
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    return local_coords(g_coords)[0];
  }

  /**
   * Global coordinates and viewspec to local index.
   *
   * Convert given global coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & g_coords,
    const ViewSpec_t & viewspec) const
  {
    auto vs_coords = g_coords;
    vs_coords[0] += viewspec[0].offset;
    return local_coords(vs_coords)[0];
  }

  /**
   * Global coordinates to local index.
   *
   * Convert given coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  template<typename ... Values>
  IndexType at(IndexType value, Values ... values) const
  {
    static_assert(
      sizeof...(values) == NumDimensions-1,
      "Wrong parameter number");
    std::array<IndexType, NumDimensions> inputindex = {
      value, (IndexType)values...
    };
    return at(inputindex);
  }

  ////////////////////////////////////////////////////////////////////////////
  /// is_local
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Whether the given global index is local to the specified unit.
   *
   * \see  DashPatternConcept
   */
  inline bool is_local(
    IndexType index,
    team_unit_t unit) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.is_local()", index);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.is_local()", unit);
    bool is_loc = index >= _block_offsets[unit] &&
                  (unit == _nunits-1 ||
                   index <  _block_offsets[unit+1]);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.is_local >", is_loc);
    return is_loc;
  }

  /**
   * Whether the given global index is local to the unit that created
   * this pattern instance.
   *
   * \see  DashPatternConcept
   */
  inline bool is_local(
    IndexType index) const
  {
    auto unit = team().myid();
    DASH_LOG_TRACE_VAR("LoadBalancePattern.is_local()", index);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.is_local", unit);
    bool is_loc = index >= _block_offsets[unit] &&
                  (unit == _nunits-1 ||
                   index <  _block_offsets[unit+1]);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.is_local >", is_loc);
    return is_loc;
  }

  ////////////////////////////////////////////////////////////////////////////
  /// block
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Cartesian arrangement of pattern blocks.
   */
  const BlockSpec_t & blockspec() const
  {
    return _blockspec;
  }

  /**
   * Index of block at given global coordinates.
   *
   * \see  DashPatternConcept
   */
  index_type block_at(
    /// Global coordinates of element
    const std::array<index_type, NumDimensions> & g_coords) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.block_at()", g_coords);

    index_type block_idx = static_cast<index_type>(unit_at(g_coords[0]));

    DASH_LOG_TRACE_VAR("LoadBalancePattern.block_at >", block_idx);
    return block_idx;
  }

  /**
   * View spec (offset and extents) of block at global linear block index in
   * cartesian element space.
   */
  ViewSpec_t block(
    index_type g_block_index) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern<1>.block >", g_block_index);
    index_type offset = _block_offsets[g_block_index];
    auto block_size   = _local_sizes[g_block_index];
    std::array<index_type, NumDimensions> offsets = {{ offset }};
    std::array<size_type, NumDimensions>  extents = {{ block_size }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern<1>.block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   */
  ViewSpec_t local_block(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern<1>.local_block()", l_block_index);
    DASH_ASSERT_EQ(
      0, l_block_index,
      "LoadBalancePattern always assigns exactly 1 block to a single unit");
    index_type block_offset = _block_offsets[_team->myid()];
    size_type  block_size   = _local_sizes[_team->myid()];
    std::array<index_type, NumDimensions> offsets = {{ block_offset }};
    std::array<size_type, NumDimensions>  extents = {{ block_size }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern<1>.local_block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  ViewSpec_t local_block_local(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern<1>.local_block_local >",
                       l_block_index);
    size_type block_size = _local_sizes[_team->myid()];
    std::array<index_type, NumDimensions> offsets = {{ 0 }};
    std::array<size_type, NumDimensions>  extents = {{ block_size }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern<1>.local_block_local >", block_vs);
    return block_vs;
  }

  /**
   * Maximum number of elements in a single block in the given dimension.
   *
   * \return  The blocksize in the given dimension
   *
   * \see     DashPatternConcept
   */
  SizeType blocksize(
    /// The dimension in the pattern
    dim_t dimension) const
  {
    return _local_capacity;
  }

  /**
   * Maximum number of elements in a single block in all dimensions.
   *
   * \return  The maximum number of elements in a single block assigned to
   *          a unit.
   *
   * \see     DashPatternConcept
   */
  SizeType max_blocksize() const
  {
    return _local_capacity;
  }

  /**
   * Maximum number of elements assigned to a single unit.
   *
   * \see  DashPatternConcept
   */
  inline SizeType local_capacity() const
  {
    return _local_capacity;
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit in total.
   *
   * \see  blocksize()
   * \see  local_extent()
   * \see  local_capacity()
   *
   * \see  DashPatternConcept
   */
  inline SizeType local_size(
    team_unit_t unit = UNDEFINED_TEAM_UNIT_ID) const
  {
    if (unit == UNDEFINED_TEAM_UNIT_ID) {
      unit = _myid;
    }
    return _local_sizes[unit];
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  inline IndexType num_units() const
  {
    return _nunits;
  }

  /**
   * The maximum number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline IndexType capacity() const
  {
    return _size;
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline IndexType size() const
  {
    return _size;
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  inline dash::Team & team() const
  {
    return *_team;
  }

  /**
   * Distribution specification of this pattern.
   */
  const DistributionSpec_t & distspec() const
  {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  SizeSpec_t sizespec() const
  {
    return SizeSpec_t(std::array<SizeType, 1> {{ _size }});
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  const std::array<SizeType, NumDimensions> & extents() const
  {
    return std::array<SizeType, 1> {{ _size }};
  }

  /**
   * Cartesian index space representing the underlying memory model of the
   * pattern.
   *
   * \see DashPatternConcept
   */
  const MemoryLayout_t & memory_layout() const
  {
    return _memory_layout;
  }

  /**
   * Cartesian index space representing the underlying local memory model
   * of this pattern for the calling unit.
   * Not part of DASH Pattern concept.
   */
  const LocalMemoryLayout_t & local_memory_layout() const
  {
    return _local_memory_layout;
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   *
   * \see DashPatternConcept
   */
  const TeamSpec_t & teamspec() const
  {
    return _teamspec;
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates.
   *
   * \see DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType index) const
  {
    return std::array<IndexType, 1> {{ index }};
  }

  /**
   * Memory order followed by the pattern.
   */
  constexpr static MemArrange memory_order()
  {
    return Arrangement;
  }

  /**
   * Number of dimensions of the cartesian space partitioned by the pattern.
   */
  constexpr static dim_t ndim()
  {
    return 1;
  }

  const std::vector<double> & unit_cpu_weights() const
  {
    return _unit_cpu_weights;
  }

  const std::vector<double> & unit_membw_weights() const
  {
    return _unit_membw_weights;
  }

  const std::vector<double> & unit_load_weights() const
  {
    return _unit_load_weights;
  }

private:

  std::vector<double> initialize_load_weights(
    const std::vector<double> & cpu_weights,
    const std::vector<double> & membw_weights) const
  {
    std::vector<double> load_weights;
    if (cpu_weights.size() != membw_weights.size()) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Number of CPU weights and SHMEM weights differ");
    }
    // Trying to resolve the "inverse Roofline model" here:
    // We do not know if the operations on the data that is distributed
    // using this pattern is memory-bound or computation-bound.
    //
    // Most basic model:
    // weight[u] = cpu_weight[u] * membw_weight[u]
    load_weights.reserve(cpu_weights.size());
    std::transform(cpu_weights.begin(), cpu_weights.end(),
                   membw_weights.begin(),
                   std::back_inserter(load_weights),
                   std::multiplies<double>());
    dash::math::div_mean(load_weights.begin(), load_weights.end());
    return load_weights;
  }

  /**
   * Initialize local sizes from pattern size, team and team locality
   * hierarchy.
   */
  std::vector<size_type> initialize_local_sizes(
    size_type              total_size,
    const TeamLocality_t & locality) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes()", total_size);
    std::vector<size_type> l_sizes;
    auto nunits = locality.team().size();
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes()", nunits);
    if (nunits == 1) {
      l_sizes.push_back(total_size);
    }
    if (nunits <= 1) {
      return l_sizes;
    }

    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes",
                       _unit_load_weights);

    double balanced_lsize = static_cast<double>(total_size) / nunits;

    size_t      assigned_capacity = 0;
    // Unit with maximum CPU capacity in team:
    team_unit_t max_cpu_cap_unit{0};

    // Maximum CPU capacity found:
    size_t      unit_max_cpu_cap  = 0;
    for (team_unit_t u{0}; u < nunits; u++) {
      double weight         = _unit_load_weights[u];
      size_t unit_capacity  = weight > 1
                              ? std::ceil(weight * balanced_lsize)
                              : std::floor(weight * balanced_lsize);
      if (unit_capacity > unit_max_cpu_cap) {
        max_cpu_cap_unit = u;
        unit_max_cpu_cap = unit_capacity;
      }
      assigned_capacity += unit_capacity;
      l_sizes.push_back(unit_capacity);
    }
    // Some elements might be unassigned due to rounding.
    // Assign them to the unit with highest CPU capacity:
    l_sizes[max_cpu_cap_unit] += (total_size - assigned_capacity);

    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes >", l_sizes);
    return l_sizes;
  }

  BlockSpec_t initialize_blockspec(
    size_type                      size,
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_blockspec", local_sizes);
    BlockSpec_t blockspec(
      std::array<size_type, 1> {{
        static_cast<size_type>(local_sizes.size())
      }});
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_blockspec >", blockspec);
    return blockspec;
  }

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  std::vector<size_type> initialize_block_offsets(
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_block_offsets", local_sizes);
    std::vector<size_type> block_offsets;
    if (local_sizes.size() > 0) {
      // NOTE: Assuming 1 block for every unit.
      block_offsets.push_back(0);
      for (size_type unit_idx = 0;
           unit_idx < local_sizes.size()-1;
           ++unit_idx)
      {
        auto block_offset = block_offsets[unit_idx] +
                            local_sizes[unit_idx];
        block_offsets.push_back(block_offset);
      }
    }
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_block_offsets >",
                       block_offsets);
    return block_offsets;
  }

  /**
   * Max. elements per unit (local capacity)
   */
  SizeType initialize_local_capacity(
    const std::vector<size_type> & local_sizes) const
  {
    SizeType l_capacity = 0;
    if (_nunits == 0) {
      return 0;
    }
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_lcapacity", _nunits);
    // Local capacity is maximum number of elements assigned to a single
    // unit, i.e. the maximum local size:
    l_capacity = *(std::max_element(local_sizes.begin(),
                                    local_sizes.end()));
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range()
  {
    auto l_size = _local_size;
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_range()", l_size);
    if (l_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = global(0);
      // Index past last local index transformed to global index.
      // global(l_size) would be out of range, so we use the global index
      // to the last element and increment by 1:
      _lend   = global(l_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  SizeType initialize_local_extent(
    team_unit_t                    unit,
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_extent()", unit);
    if (local_sizes.size() == 0) {
      return 0;
    }
    // Local size of given unit:
    SizeType l_extent = local_sizes[unit];
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_extent >", l_extent);
    return l_extent;
  }

private:
  PatternArguments_t          _arguments;
  /// Extent of the linear pattern.
  SizeType                    _size;
  /// Load balance weight by CPU capacity of every unit in the team.
  std::vector<double>         _unit_cpu_weights;
  /// Load balance weight by shared memory bandwidth of every unit in the
  /// team.
  std::vector<double>         _unit_membw_weights;
  /// Load balance weight of every unit in the team.
  std::vector<double>         _unit_load_weights;
  /// Number of local elements for every unit in the active team.
  std::vector<size_type>      _local_sizes;
  /// Block offsets for every unit. Prefix sum of local sizes.
  std::vector<size_type>      _block_offsets;
  /// Global memory layout of the pattern.
  MemoryLayout_t              _memory_layout;
  /// Number of blocks in all dimensions
  BlockSpec_t                 _blockspec;
  /// Distribution types of all dimensions.
  DistributionSpec_t          _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team *                _team            = nullptr;
  /// The active unit's id.
  team_unit_t                 _myid;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// Actual number of local elements of the active unit.
  SizeType                    _local_size;
  /// Local memory layout of the pattern.
  LocalMemoryLayout_t         _local_memory_layout;
  /// Maximum number of elements assigned to a single unit
  SizeType                    _local_capacity;
  /// Corresponding global index to first local index of the active unit
  IndexType                   _lbegin;
  /// Corresponding global index past last local index of the active unit
  IndexType                   _lend;

}; // class LoadBalancePattern

}  // namespace dash

#endif // DASH__LOAD_BALANCE_PATTERN_H__INCLUDED
