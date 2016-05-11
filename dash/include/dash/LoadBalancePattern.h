#ifndef DASH__LOAD_BALANCE_PATTERN_H__INCLUDED
#define DASH__LOAD_BALANCE_PATTERN_H__INCLUDED

#include <functional>
#include <array>
#include <type_traits>

#include <dash/Types.h>
#include <dash/Enums.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>
#include <dash/PatternProperties.h>

#include <dash/util/TeamLocality.h>

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>
#include <dash/internal/PatternArguments.h>

namespace dash {

/**
 * Irregular dynamic pattern.
 *
 * \concept{DashPatternConcept}
 */
template<
  dim_t      NumDimensions,
  MemArrange Arrangement  = dash::ROW_MAJOR,
  typename   IndexType    = dash::default_index_t >
class LoadBalancePattern;

/**
 * Irregular dynamic pattern.
 * Specialization for 1-dimensional data.
 *
 * \concept{DashPatternConcept}
 */
template<
  MemArrange Arrangement,
  typename   IndexType >
class LoadBalancePattern<1, Arrangement, IndexType>
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
  typedef LoadBalancePattern<NumDimensions, Arrangement, IndexType>
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

public:
  typedef IndexType   index_type;
  typedef SizeType    size_type;
  typedef ViewSpec_t  viewspec_type;
  typedef struct {
    dart_unit_t                           unit;
    IndexType                             index;
  } local_index_t;
  typedef struct {
    dart_unit_t                           unit;
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
    const TeamLocality_t & team_loc)
  : _size(sizespec.size()),
    _local_sizes(initialize_local_sizes(
        _size,
        team_loc)),
    _block_offsets(initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> { _size }),
    _blockspec(initialize_blockspec(
        _size,
        _local_sizes)),
    _distspec(dash::BLOCKED),
    _team(&team),
    _myid(_team->myid()),
    _teamspec(*_team),
    _nunits(_team->size()),
    _nblocks(_nunits),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> { _local_size }),
    _local_capacity(initialize_local_capacity())
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
      _teamspec    == other._teamspec &&
      _nblocks     == other._nblocks
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
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dart_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.unit_at()", global_pos);
    DASH_LOG_TRACE_VAR("LoadBalancePattern.unit_at()", viewspec);
    dart_unit_t unit_idx = 0;
    // Apply viewspec offsets to coordinates:
    auto g_coord         = global_pos + viewspec[0].offset;
    for (; unit_idx < _nunits - 1; ++unit_idx) {
      if (_block_offsets[unit_idx+1] >= static_cast<size_type>(g_coord)) {
        DASH_LOG_TRACE_VAR("LoadBalancePattern.unit_at >", unit_idx);
        return unit_idx;
      }
    }
    DASH_LOG_TRACE_VAR("LoadBalancePattern.unit_at >", _nunits-1);
    return _nunits-1;
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
    dart_unit_t unit) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.local_extents()", unit);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.local_extents >", _local_size);
    return std::array<SizeType, 1> { _local_size };
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
    DASH_LOG_TRACE_VAR("LoadBalancePattern.local", _block_offsets.size());
    DASH_ASSERT_GT(_nunits, 0,
                   "team size is 0");
    DASH_ASSERT_GE(_block_offsets.size(), _nunits,
                   "missing block offsets");
    local_index_t l_index;
    index_type    unit_idx = static_cast<index_type>(_nunits-1);
    for (; unit_idx >= 0; --unit_idx) {
      DASH_LOG_TRACE_VAR("LoadBalancePattern.local", unit_idx);
      index_type block_offset = _block_offsets[unit_idx];
      DASH_LOG_TRACE_VAR("LoadBalancePattern.local", block_offset);
      if (block_offset <= g_index) {
        l_index.unit  = unit_idx;
        l_index.index = g_index - block_offset;
        DASH_LOG_TRACE_VAR("LoadBalancePattern.local >", l_index.unit);
        DASH_LOG_TRACE_VAR("LoadBalancePattern.local >", l_index.index);
        return l_index;
      }
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "LoadBalancePattern.local: global index " << g_index << " " <<
      "is out of bounds");
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
    dart_unit_t unit,
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
    return std::array<IndexType, 1> { glob_index };
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
    dart_unit_t unit,
    IndexType l_index) const
  {
    return global(unit, std::array<IndexType, 1> { l_index })[0];
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
    return global(_team->myid(), std::array<IndexType, 1> { l_index })[0];
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
    dart_unit_t unit,
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

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline SizeType local_capacity(
    dart_unit_t unit = DART_UNDEFINED_UNIT_ID) const
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
    dart_unit_t unit = DART_UNDEFINED_UNIT_ID) const
  {
    return _local_size;
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
    return SizeSpec_t(std::array<SizeType, 1> { _size });
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  const std::array<SizeType, NumDimensions> & extents() const
  {
    return std::array<SizeType, 1> { _size };
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
    return std::array<IndexType, 1> { index };
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

private:
  /**
   * Initialize local sizes from pattern size, team and team locality
   * hierarchy.
   */
  std::vector<size_type> initialize_local_sizes(
    size_type              total_size,
    const TeamLocality_t & locality,
    const dash::Team     & team) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes()", total_size);
    std::vector<size_type> l_sizes;
    auto nunits = team.size();
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes()", nunits);
    if (nunits == 1) {
      l_sizes.push_back(total_size);
    }
    if (nunits <= 1) {
      return l_sizes;
    }
    size_type blocksize_u = 0;
    for (size_type u = 0; u < nunits; ++u) {
      l_sizes.push_back(blocksize_u);
    }

    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_local_sizes >", l_sizes);
    return l_sizes;
  }

  BlockSpec_t initialize_blockspec(
    size_type                      size,
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_blockspec", local_sizes);
    BlockSpec_t blockspec({
	static_cast<size_type>(local_sizes.size())
	  });
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
  SizeType initialize_local_capacity() const
  {
    SizeType l_capacity = 0;
    if (_nunits == 0) {
      return 0;
    }
    DASH_LOG_TRACE_VAR("LoadBalancePattern.init_lcapacity", _nunits);
    // Local capacity is maximum number of elements assigned to a single unit,
    // i.e. the maximum local size:
    l_capacity = *(std::max_element(_local_sizes.begin(),
                                    _local_sizes.end()));
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
    dart_unit_t unit) const
  {
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_extent()", unit);
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_extent()", _nunits);
    if (_nunits == 0) {
      return 0;
    }
    // Local size of given unit:
    SizeType l_extent = _local_sizes[static_cast<int>(unit)];
    DASH_LOG_DEBUG_VAR("LoadBalancePattern.init_local_extent >", l_extent);
    return l_extent;
  }

private:
  PatternArguments_t          _arguments;
  /// Extent of the linear pattern.
  SizeType                    _size;
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
  dart_unit_t                 _myid;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// Number of blocks in all dimensions
  SizeType                    _nblocks         = 0;
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
