#ifndef DASH__BLOCK_PATTERN_H_
#define DASH__BLOCK_PATTERN_H_

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

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>

namespace dash {

#ifndef DOXYGEN

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 *
 * \tparam  NumDimensions  The number of dimensions of the pattern
 * \tparam  Arrangement    The memory order of the pattern (ROW_MAJOR
 *                         or COL_MAJOR), defaults to ROW_MAJOR.
 *                         Memory order defines how elements in the
 *                         pattern will be iterated predominantly
 *                         \see MemArrange
 *
 * \concept{DashPatternConcept}
 */
template<
  dim_t      NumDimensions,
  MemArrange Arrangement   = ROW_MAJOR,
  typename   IndexType     = dash::default_index_t
>
class Pattern
{
public:
  static constexpr char const * PatternName = "BlockPattern";

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
              pattern_partitioning_tag::unbalanced
          > partitioning_properties;
  /// Satisfiable properties in pattern property category Mapping:
  typedef pattern_mapping_properties<
              // Number of blocks assigned to a unit may differ.
              pattern_mapping_tag::unbalanced
          > mapping_properties;
  /// Satisfiable properties in pattern property category Layout:
  typedef pattern_layout_properties<
              // Local indices iterate over block boundaries.
              pattern_layout_tag::canonical,
              // Local element order corresponds to canonical linearization
              // within entire local memory.
              pattern_layout_tag::linear
          > layout_properties;

private:
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  /// Fully specified type definition of self
  typedef Pattern<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    LocalMemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    BlockSpec_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    BlockSizeSpec_t;
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

public:
  typedef IndexType   index_type;
  typedef SizeType    size_type;
  typedef ViewSpec_t  viewspec_type;
  typedef struct {
    team_unit_t unit;
    IndexType   index;
  } local_index_t;
  typedef struct {
    team_unit_t unit;
    std::array<index_type, NumDimensions> coords;
  } local_coords_t;

private:
  PatternArguments_t          _arguments;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
  /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
  /// dimensions
  DistributionSpec_t          _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team *                _team            = nullptr;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// The global layout of the pattern's elements in memory respective to
  /// memory order. Also specifies the extents of the pattern space.
  MemoryLayout_t              _memory_layout;
  /// Maximum extents of a block in this pattern
  BlockSizeSpec_t             _blocksize_spec;
  /// Number of blocks in all dimensions
  BlockSpec_t                 _blockspec;
  /// A projected view of the global memory layout representing the
  /// local memory layout of this unit's elements respective to memory
  /// order.
  LocalMemoryLayout_t         _local_memory_layout;
  /// Arrangement of local blocks in all dimensions
  BlockSpec_t                 _local_blockspec;
  /// Maximum number of elements assigned to a single unit
  SizeType                    _local_capacity;
  /// Corresponding global index to first local index of the active unit
  IndexType                   _lbegin;
  /// Corresponding global index past last local index of the active unit
  IndexType                   _lend;

public:
  /**
   * Constructor, initializes a pattern from an argument list consisting
   * of the pattern size (extent, number of elements) in every dimension
   * followed by optional distribution types.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   Pattern p1(5,3, BLOCKED);
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE));
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // How teams are arranged in all dimensions, default is
   *              // an extent of all units in first, and 1 in all higher
   *              // dimensions:
   *              TeamSpec<2>(dash::Team::All(), 1),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // A cube with sidelength 3 with blockwise distribution in the first
   *   // dimension
   *   Pattern p2(3,3,3, BLOCKED);
   *   // Same as p2
   *   Pattern p3(3,3,3);
   *   // A cube with sidelength 3 with blockwise distribution in the third
   *   // dimension
   *   Pattern p4(3,3,3, NONE, NONE, BLOCKED);
   * \endcode
   */
  template<typename ... Args>
  Pattern(
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    SizeType arg,
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    Args && ... args)
  : _arguments(arg, args...),
    _distspec(_arguments.distspec()),
    _team(&_arguments.team()),
    _teamspec(_arguments.teamspec()),
    _nunits(_teamspec.size()),
    _memory_layout(_arguments.sizespec().extents()),
    _blocksize_spec(initialize_blocksizespec(
        _arguments.sizespec(),
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        _arguments.sizespec(),
        _distspec,
        _blocksize_spec,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize_spec,
        _distspec,
        _teamspec,
        _local_memory_layout)),
    _local_capacity(initialize_local_capacity())
  {
    DASH_LOG_TRACE("BlockPattern()", "Constructor with argument list");
    initialize_local_range();
    DASH_LOG_TRACE("BlockPattern()", "BlockPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // How teams are arranged in all dimensions, default is
   *              // an extent of all units in first, and 1 in all higher
   *              // dimensions:
   *              TeamSpec<2>(dash::Team::All(), 1),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   Pattern p1(5,3, BLOCKED);
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE));
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              TeamSpec<2>(dash::Team::All(), 1));
   * \endcode
   */
  Pattern(
    /// Pattern size (extent, number of elements) in every dimension
    const SizeSpec_t         & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist     = DistributionSpec_t(),
    /// Cartesian arrangement of units within the team
    const TeamSpec_t         & teamspec = TeamSpec_t::TeamSpec(),
    /// Team containing units to which this pattern maps its elements
    dash::Team               & team     = dash::Team::All())
  : _distspec(dist),
    _team(&team),
    _teamspec(
      teamspec,
      _distspec,
      *_team),
    _nunits(_teamspec.size()),
    _memory_layout(sizespec.extents()),
    _blocksize_spec(initialize_blocksizespec(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _distspec,
        _blocksize_spec,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize_spec,
        _distspec,
        _teamspec,
        _local_memory_layout)),
    _local_capacity(initialize_local_capacity())
  {
    DASH_LOG_TRACE("BlockPattern()", "(sizespec, dist, teamspec, team)");
    initialize_local_range();
    DASH_LOG_TRACE("BlockPattern()", "BlockPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // How teams are arranged in all dimensions, default is
   *              // an extent of all units in first, and 1 in all higher
   *              // dimensions:
   *              TeamSpec<2>(dash::Team::All(), 1),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   Pattern p1(5,3, BLOCKED);
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE));
   *   // Same as
   *   Pattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              TeamSpec<2>(dash::Team::All(), 1));
   * \endcode
   */
  Pattern(
    /// Pattern size (extent, number of elements) in every dimension
    const SizeSpec_t         & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team                     & team = dash::Team::All())
  : _distspec(dist),
    _team(&team),
    _teamspec(_distspec, *_team),
    _nunits(_teamspec.size()),
    _memory_layout(sizespec.extents()),
    _blocksize_spec(initialize_blocksizespec(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _distspec,
        _blocksize_spec,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize_spec,
        _distspec,
        _teamspec,
        _local_memory_layout)),
    _local_capacity(initialize_local_capacity())
  {
    DASH_LOG_TRACE("BlockPattern()", "(sizespec, dist, team)");
    initialize_local_range();
    DASH_LOG_TRACE("BlockPattern()", "BlockPattern initialized");
  }

  /**
   * Copy constructor.
   */
  Pattern(const self_t & other)
  : _distspec(other._distspec),
    _team(other._team),
    _teamspec(other._teamspec),
    _nunits(other._nunits),
    _memory_layout(other._memory_layout),
    _blocksize_spec(other._blocksize_spec),
    _blockspec(other._blockspec),
    _local_memory_layout(other._local_memory_layout),
    _local_blockspec(other._local_blockspec),
    _local_capacity(other._local_capacity),
    _lbegin(other._lbegin),
    _lend(other._lend)
  {
    // No need to copy _arguments as it is just used to
    // initialize other members.
    DASH_LOG_TRACE("BlockPattern(other)", "BlockPattern copied");
  }

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  Pattern(self_t & other)
  : Pattern(static_cast<const self_t &>(other))
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
      _distspec        == other._distspec &&
      _teamspec        == other._teamspec &&
      _memory_layout   == other._memory_layout &&
      _blockspec       == other._blockspec &&
      _blocksize_spec  == other._blocksize_spec &&
      _nunits          == other._nunits
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
   * Assignment operator.
   */
  Pattern & operator=(const Pattern & other)
  {
    DASH_LOG_TRACE("BlockPattern.=(other)");
    if (this != &other) {
      _distspec            = other._distspec;
      _team                = other._team;
      _teamspec            = other._teamspec;
      _memory_layout       = other._memory_layout;
      _local_memory_layout = other._local_memory_layout;
      _blocksize_spec      = other._blocksize_spec;
      _blockspec           = other._blockspec;
      _local_blockspec     = other._local_blockspec;
      _local_capacity      = other._local_capacity;
      _nunits              = other._nunits;
      _lbegin              = other._lbegin;
      _lend                = other._lend;
      DASH_LOG_TRACE("BlockPattern.=(other)", "BlockPattern assigned");
    }
    return *this;
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
    const ViewSpec_t & viewspec) const
  {
    // Apply viewspec offsets to coordinates:
    std::array<IndexType, NumDimensions> vs_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      vs_coords[d] = coords[d] + viewspec[d].offset;
    }
    return unit_at(vs_coords);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const
  {
    std::array<IndexType, NumDimensions> unit_coords;
    // Coord to block coord to unit coord:
    for (auto d = 0; d < NumDimensions; ++d) {
      unit_coords[d] = (coords[d] / _blocksize_spec.extent(d))
                         % _teamspec.extent(d);
    }
    // Unit coord to unit id:
    team_unit_t unit_id(_teamspec.at(unit_coords));
    DASH_LOG_TRACE("BlockPattern.unit_at",
                   "coords", coords,
                   "> unit id", unit_id);
    return unit_id;
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec) const
  {
    auto global_coords = _memory_layout.coords(global_pos);
    return unit_at(global_coords, viewspec);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see  blocksize()
   * \see  blockspec()
   * \see  blocksizespec()
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos) const
  {
    auto global_coords = _memory_layout.coords(global_pos);
    return unit_at(global_coords);
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
    if (dim >= NumDimensions || dim < 0) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for Pattern::local_extent. "
        << "Expected dimension between 0 and " << NumDimensions-1 << ", "
        << "got " << dim);
    }
    return _memory_layout.extent(dim);
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
    if (dim >= NumDimensions || dim < 0) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for Pattern::local_extent. "
        << "Expected dimension between 0 and " << NumDimensions-1 << ", "
        << "got " << dim);
    }
    return _local_memory_layout.extent(dim);
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * active unit, by dimension.
   *
   * \see  local_extent()
   * \see  blocksize()
   * \see  local_size()
   * \see  extent()
   *
   * \see  DashPatternConcept
   */
  std::array<SizeType, NumDimensions> local_extents() const
  {
    DASH_LOG_DEBUG_VAR("BlockPattern.local_extents()", _team->myid());
    std::array<SizeType, NumDimensions> l_extents;
    // Local unit id, get extents from member instance:
    l_extents = _local_memory_layout.extents();
    DASH_LOG_DEBUG_VAR("BlockPattern.local_extents >", l_extents);
    return l_extents;
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
    DASH_LOG_DEBUG_VAR("BlockPattern.local_extents()", unit);
    std::array<SizeType, NumDimensions> l_extents;
    if (unit == _team->myid()) {
      // Local unit id, get extents from member instance:
      l_extents = _local_memory_layout.extents();
    } else {
      // Remote unit id, initialize local memory layout for given unit:
      l_extents = initialize_local_extents(unit);
    }
    DASH_LOG_DEBUG_VAR("BlockPattern.local_extents >", l_extents);
    return l_extents;
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
    auto coords = local_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      coords[d] += viewspec.offset(d);
    }
    return _local_memory_layout.at(coords);
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
    return _local_memory_layout.at(local_coords);
  }

  /**
   * Converts global coordinates to their associated unit and its respective
   * local coordinates.
   *
   * TODO: Unoptimized
   *
   * \see  DashPatternConcept
   */
  local_coords_t local(
    const std::array<IndexType, NumDimensions> & global_coords) const
  {
    local_coords_t l_coords;
    l_coords.coords = local_coords(global_coords);
    l_coords.unit   = unit_at(global_coords);
    return l_coords;
  }

  /**
   * Converts global index to its associated unit and respective local index.
   *
   * TODO: Unoptimized
   *
   * \see  DashPatternConcept
   */
  local_index_t local(
    IndexType g_index) const
  {
    DASH_LOG_TRACE_VAR("BlockPattern.local()", g_index);
    // TODO: Implement dedicated method for this, conversion to/from
    //       global coordinates is expensive.
    auto l_coords = coords(g_index);
    return local_index(l_coords);
  }

  /**
   * Converts global coordinates to their associated unit's respective
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> local_coords(
    const std::array<IndexType, NumDimensions> & global_coords) const
  {
    std::array<IndexType, NumDimensions> local_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto block_size_d     = _blocksize_spec.extent(d);
      auto b_offset_d       = global_coords[d] % block_size_d;
      auto g_block_offset_d = global_coords[d] / block_size_d;
      auto l_block_offset_d = g_block_offset_d / _teamspec.extent(d);
      local_coords[d]       = b_offset_d +
                              (l_block_offset_d * block_size_d);
    }
    return local_coords;
  }

  /**
   * Resolves the unit and the local index from global coordinates.
   *
   * \see  DashPatternConcept
   */
  local_index_t local_index(
    const std::array<IndexType, NumDimensions> & global_coords) const
  {
    DASH_LOG_TRACE_VAR("BlockPattern.local_index()", global_coords);
    // Local offset of the element within all of the unit's local
    // elements:
    SizeType local_elem_offset = 0;
    auto unit = unit_at(global_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.local_index", unit);
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords =
      local_coords(global_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.local_index", l_coords);
    if (unit == _team->myid()) {
      // Coords are local to this unit, use pre-generated local memory
      // layout
      return local_index_t { unit, _local_memory_layout.at(l_coords) };
    } else {
      // Cannot use _local_memory_layout as it is only defined for the
      // active unit but does not specify local memory of other units.
      // Generate local memory layout for unit assigned to coords:
      auto l_mem_layout =
        LocalMemoryLayout_t(initialize_local_extents(unit));
      return local_index_t { unit, l_mem_layout.at(l_coords) };
    }
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
    DASH_LOG_DEBUG_VAR("BlockPattern.global()", local_coords);
    if (_teamspec.size() < 2) {
      return local_coords;
    }
    SizeType blocksize = max_blocksize();
    // Coordinates of the unit within the team spec:
    std::array<IndexType, NumDimensions> unit_ts_coord =
      _teamspec.coords(unit);
    // Global coords of the element's block within all blocks.
    // Use initializer so elements are initialized with 0s:
    std::array<IndexType, NumDimensions> block_index;
    // Index of the element's block as element offset:
    std::array<IndexType, NumDimensions> block_coord;
    // Index of the element:
    std::array<IndexType, NumDimensions> glob_index;
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = _distspec[d];
      auto num_units_d          = _teamspec.extent(d);
      auto num_blocks_d         = _blockspec.extent(d);
      auto blocksize_d          = _blocksize_spec.extent(d);
      auto local_index_d        = local_coords[d];
      // TOOD: Use % (blocksize_d - underfill_d)
      auto elem_block_offset_d  = local_index_d % blocksize_d;
      // Global coords of the element's block within all blocks:
      block_index[d] = dist.local_index_to_block_coord(
                         unit_ts_coord[d], // unit ts offset in d
                         local_index_d,
                         num_units_d,
                         num_blocks_d,
                         blocksize
                       );
      block_coord[d] = block_index[d] * blocksize_d;
      glob_index[d]  = block_coord[d] + elem_block_offset_d;
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", d);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d",
                         unit_ts_coord[d]);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", local_index_d);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", num_units_d);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", block_index[d]);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", blocksize);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", blocksize_d);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d",
                         elem_block_offset_d);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", block_coord[d]);
      DASH_LOG_TRACE_VAR("BlockPattern.global.d", glob_index[d]);
    }
    DASH_LOG_TRACE_VAR("BlockPattern.global", block_index);
    DASH_LOG_TRACE_VAR("BlockPattern.global", block_coord);
    DASH_LOG_DEBUG_VAR("BlockPattern.global", glob_index);
    return glob_index;
  }

  /**
   * Converts local coordinates of active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    return global(_team->myid(), local_coords);
  }

  /**
   * Resolve an element's linear global index from the calling unit's local
   * index of that element.
   *
   * \see  at  Inverse of global()
   *
   * \see  DashPatternConcept
   */
  IndexType global(
    IndexType local_index) const
  {
    std::array<IndexType, NumDimensions> local_coords =
      _local_memory_layout.coords(local_index);
    DASH_LOG_TRACE_VAR("BlockPattern.local_to_global_idx()", local_coords);
    std::array<IndexType, NumDimensions> global_coords =
      global(_team->myid(), local_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.local_to_global_idx >", global_coords);
    return _memory_layout.at(global_coords);
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
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    std::array<IndexType, NumDimensions> global_coords =
      global(unit, local_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.local_to_global_idx", global_coords);
    return _memory_layout.at(global_coords);
  }

  /**
   * Global coordinates and viewspec to global position in the pattern's
   * iteration order.
   *
   * \see  at
   * \see  local_at
   *
   * \see  DashPatternConcept
   */
  IndexType global_at(
    const std::array<IndexType, NumDimensions> & view_coords,
    const ViewSpec_t                           & viewspec) const
  {
    DASH_LOG_TRACE("BlockPattern.global_at()",
                   "view coords:",view_coords,
                   "view:",       viewspec);
    // Global coordinates of the element referenced in the view:
    std::array<IndexType, NumDimensions> global_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      global_coords[d] = view_coords[d] + viewspec.offset(d);
    }
    DASH_LOG_TRACE("BlockPattern.global_at",
                   "global coords:", global_coords);
    // Offset in iteration order is identical to offset in canonical order:
    auto offset = _memory_layout.at(global_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.global_at >", offset);
    return offset;
  }

  /**
   * Global coordinates to global position in the pattern's iteration order.
   *
   * NOTE:
   * Expects extent[d] to be a multiple of blocksize[d] * nunits[d]
   * to ensure the balanced property.
   *
   * \see  at
   * \see  local_at
   *
   * \see  DashPatternConcept
   */
  IndexType global_at(
    const std::array<IndexType, NumDimensions> & global_coords) const
  {
    DASH_LOG_TRACE("BlockPattern.global_at()",
                   "gcoords:",  global_coords);
    // Offset in iteration order is identical to offset in canonical order:
    auto offset = _memory_layout.at(global_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.global_at >", offset);
    return offset;
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
    const std::array<IndexType, NumDimensions> & global_coords) const
  {
    // Local offset of the element within all of the unit's local
    // elements:
    SizeType local_elem_offset = 0;
    auto unit = unit_at(global_coords);
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords =
      local_coords(global_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.at", l_coords);
    if (unit == _team->myid()) {
      // Coords are local to this unit, use pre-generated local memory
      // layout
      return _local_memory_layout.at(l_coords);
    } else {
      // Cannot use _local_memory_layout as it is only defined for the
      // active unit but does not specify local memory of other units.
      // Generate local memory layout for unit assigned to coords:
      auto l_mem_layout =
        LocalMemoryLayout_t(initialize_local_extents(unit));
      return l_mem_layout.at(l_coords);
    }
  }

  /**
   * Global coordinates and viewspec to local index.
   *
   * Convert given global coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & global_coords,
    const ViewSpec_t & viewspec) const
  {
    auto coords = global_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      coords[d] += viewspec.offset(d);
    }
    DASH_LOG_TRACE_VAR("BlockPattern.at()", coords);
    DASH_LOG_TRACE_VAR("BlockPattern.at()", viewspec);
    return at(coords);
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
   * Whether there are local elements in a dimension at a given offset,
   * e.g. in a specific row or column.
   *
   * \see  DashPatternConcept
   */
  bool has_local_elements(
    /// Dimension to check
    dim_t dim,
    /// Offset in dimension
    IndexType dim_offset,
    /// local id of the unit
    team_unit_t unit,
    /// Viewspec to apply
    const ViewSpec_t & viewspec) const
  {
    DASH_LOG_TRACE_VAR("BlockPattern.has_local_elements()", dim);
    DASH_LOG_TRACE_VAR("BlockPattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("BlockPattern.has_local_elements()", unit);
    // Apply viewspec offset in dimension to given position
    dim_offset += viewspec[dim].offset;
    // Offset to block offset
    IndexType block_coord_d    = dim_offset / _blocksize_spec.extent(dim);
    DASH_LOG_TRACE_VAR("BlockPattern.has_local_elements", block_coord_d);
    // Coordinate of unit in team spec in given dimension
    IndexType teamspec_coord_d = block_coord_d % _teamspec.extent(dim);
    DASH_LOG_TRACE_VAR("BlockPattern.has_local_elements()",
                       teamspec_coord_d);
    // Check if unit id lies in cartesian sub-space of team spec
    return _teamspec.includes_index(
              teamspec_coord_d,
              dim,
              dim_offset);
  }

  /**
   * Whether the given global index is local to the specified unit.
   *
   * \see  DashPatternConcept
   */
  bool is_local(
    IndexType index,
    team_unit_t unit) const
  {
    auto glob_coords = coords(index);
    auto coords_unit = unit_at(glob_coords);
    DASH_LOG_TRACE_VAR("BlockPattern.is_local >", (coords_unit == unit));
    return coords_unit == unit;
  }

  /**
   * Whether the given global index is local to the unit that created
   * this pattern instance.
   *
   * \see  DashPatternConcept
   */
  bool is_local(
    IndexType index) const
  {
    team_unit_t unit = team().myid();
    return is_local(index, unit);
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
    std::array<index_type, NumDimensions> block_coords;
    // Coord to block coord to unit coord:
    for (auto d = 0; d < NumDimensions; ++d) {
      block_coords[d] = g_coords[d] / _blocksize_spec.extent(d);
    }
    // Block coord to block index:
    auto block_idx = _blockspec.at(block_coords);
    DASH_LOG_TRACE("BlockPattern.block_at",
                   "coords", g_coords,
                   "> block index", block_idx);
    return block_idx;
  }

  /**
   * View spec (offset and extents) of block at global linear block index in
   * cartesian element space.
   */
  ViewSpec_t block(
    index_type global_block_index) const
  {
    // block index -> block coords -> offset
    auto block_coords = _blockspec.coords(global_block_index);
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      extents[d] = _blocksize_spec.extent(d);
      offsets[d] = block_coords[d] * extents[d];
    }
    return ViewSpec_t(offsets, extents);
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   */
  ViewSpec_t local_block(
    index_type local_block_index) const
  {
    // Initialize viewspec result with block extents:
    std::array<SizeType, NumDimensions>  block_vs_extents =
      _blocksize_spec.extents();
    std::array<IndexType, NumDimensions> block_vs_offsets {{ }};
    // Local block index to local block coords:
    auto l_block_coords = _local_blockspec.coords(local_block_index);
    auto l_elem_coords  = l_block_coords;
    // TODO: This is convenient but less efficient:
    // Translate local coordinates of first element in local block to global
    // coordinates:
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d  = block_vs_extents[d];
      l_elem_coords[d] *= blocksize_d;
    }
    // Global coordinates of first element in block:
    auto g_elem_coords = global(l_elem_coords);
    for (auto d = 0; d < NumDimensions; ++d) {
      block_vs_offsets[d] = g_elem_coords[d];
    }
#ifdef __TODO__
    // Coordinates of the unit within the team spec:
    std::array<IndexType, NumDimensions> unit_ts_coord =
      _teamspec.coords(unit);
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = _distspec[d];
      auto blocksize_d          = block_vs_extents[d];
      auto num_units_d          = _teamspec.extent(d);
      auto num_blocks_d         = _blockspec.extent(d);
      // Local to global block coords:
      auto g_block_coord_d      = (l_block_coords[d] + _myid) *
                                  _teamspec.extent(d);
      block_vs_offsets[d]       = g_block_coord_d * blocksize_d;
    }
#endif
    ViewSpec_t block_vs(block_vs_offsets, block_vs_extents);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  ViewSpec_t local_block_local(
    index_type local_block_index) const
  {
    // Local block index to local block coords:
    auto l_block_coords = _local_blockspec.coords(local_block_index);
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents =
      _blocksize_spec.extents();
    // Local block coords to local element offset:
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d = extents[d];
      offsets[d]       = l_block_coords[d] * blocksize_d;
    }
    return ViewSpec_t(offsets, extents);
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
    return _blocksize_spec.extent(dimension);
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
    return _blocksize_spec.size();
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline SizeType local_capacity(
    team_unit_t unit = UNDEFINED_TEAM_UNIT_ID) const
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
    return (unit == UNDEFINED_TEAM_UNIT_ID)
           ? _local_memory_layout.size()
           : initialize_local_extents(unit).size();
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
    return _memory_layout.size();
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline IndexType size() const
  {
    return _memory_layout.size();
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
    return SizeSpec_t(_memory_layout.extents());
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  const std::array<SizeType, NumDimensions> & extents() const
  {
    return _memory_layout.extents();
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
    return _memory_layout.coords(index);
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
    return NumDimensions;
  }

  /**
   * Number of elements missing in the overflow block of given dimension
   * compared to the regular blocksize (\see blocksize(d)), with
   * 0 <= \c underfilled_blocksize(d) < blocksize(d).
   */
  SizeType underfilled_blocksize(
    dim_t dimension) const
  {
    // Underflow blocksize = regular blocksize - overflow blocksize:
    auto ovf_blocksize = _memory_layout.extent(dimension) %
                         blocksize(dimension);
    if (ovf_blocksize == 0) {
      return 0;
    } else {
      auto reg_blocksize = blocksize(dimension);
      return reg_blocksize - ovf_blocksize;
    }
  }

private:
  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  BlockSizeSpec_t initialize_blocksizespec(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const TeamSpec_t         & teamspec) const
  {
    DASH_LOG_TRACE_VAR("BlockPattern.init_blocksizespec", teamspec.size());
    if (teamspec.size() == 0) {
      return BlockSizeSpec_t();
    }
    // Extents of a single block:
    std::array<SizeType, NumDimensions> s_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = distspec[d];
      SizeType max_blocksize_d  = dist.max_blocksize_in_range(
        sizespec.extent(d),  // size of range (extent)
        teamspec.extent(d)); // number of blocks (units)
      s_blocks[d] = max_blocksize_d;
    }
    return BlockSizeSpec_t(s_blocks);
  }

  /**
   * Initialize block spec from memory layout, team spec and distribution
   * spec.
   */
  BlockSpec_t initialize_blockspec(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const BlockSizeSpec_t    & blocksizespec,
    const TeamSpec_t         & teamspec) const
  {
    if (blocksizespec.size() == 0) {
      return BlockSpec_t();
    }
    // Number of blocks in all dimensions:
    std::array<SizeType, NumDimensions> n_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      DASH_LOG_TRACE_VAR("BlockPattern.init_blockspec", distspec[d].type);
      SizeType max_blocksize_d  = blocksizespec.extent(d);
      SizeType max_blocks_d     = dash::math::div_ceil(
                                    sizespec.extent(d),
                                    max_blocksize_d);
      n_blocks[d] = max_blocks_d;
    }
    DASH_LOG_TRACE_VAR("BlockPattern.init_blockspec", n_blocks);
    return BlockSpec_t(n_blocks);
  }

  /**
   * Initialize local block spec from global block spec.
   */
  BlockSpec_t initialize_local_blockspec(
    const BlockSpec_t         & blockspec,
    const BlockSizeSpec_t     & blocksizespec,
    const DistributionSpec_t  & distspec,
    const TeamSpec_t          & teamspec,
    const LocalMemoryLayout_t & local_mem_layout) const
  {
    auto num_l_blocks = local_mem_layout.extents();
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d = blocksizespec.extent(d);
      if (blocksize_d > 0) {
        num_l_blocks[d] = dash::math::div_ceil(
                            num_l_blocks[d],
                            blocksizespec.extent(d));
      } else {
        num_l_blocks[d] = 0;
      }
    }
    DASH_LOG_TRACE_VAR("BlockPattern.init_local_blockspec", num_l_blocks);
    return BlockSpec_t(num_l_blocks);
  }

  /**
   * Max. elements per unit (local capacity)
   *
   * Note:
   * Currently calculated as a multiple of full blocks, thus ignoring
   * underfilled blocks.
   */
  SizeType initialize_local_capacity() const
  {
    SizeType l_capacity = 1;
    if (_teamspec.size() == 0) {
      return 0;
    }
    for (auto d = 0; d < NumDimensions; ++d) {
      auto num_units_d    = _teamspec.extent(d);
      // Maximum number of occurrences of a single unit in given
      // dimension:
      auto num_blocks_d   = dash::math::div_ceil(
                              _memory_layout.extent(d),
                              _blocksize_spec.extent(d));
      auto max_l_blocks_d = dash::math::div_ceil(
                              num_blocks_d,
                              num_units_d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_lcapacity.d", d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_lcapacity.d", num_units_d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_lcapacity.d", max_l_blocks_d);
      l_capacity *= max_l_blocks_d;
    }
    l_capacity *= _blocksize_spec.size();;
    DASH_LOG_DEBUG_VAR("BlockPattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range()
  {
    auto l_size = _local_memory_layout.size();
    DASH_LOG_DEBUG_VAR("BlockPattern.init_local_range()", l_size);
    if (l_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = global(0);
      // Index past last local index transformed to global index
      _lend   = global(l_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("BlockPattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("BlockPattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  std::array<SizeType, NumDimensions> initialize_local_extents(
      team_unit_t unit) const
  {
    DASH_LOG_DEBUG_VAR("BlockPattern.init_local_extents()", unit);
    DASH_LOG_DEBUG_VAR("BlockPattern.init_local_extents()", _nunits);
    if (_nunits == 0) {
      return ::std::array<SizeType, NumDimensions> {{ }};
    }
    // Coordinates of local unit id in team spec:
    auto unit_ts_coords = _teamspec.coords(unit);
    DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents", unit_ts_coords);
    ::std::array<SizeType, NumDimensions> l_extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto num_elem_d         = _memory_layout.extent(d);
      // Number of units in dimension:
      auto num_units_d        = _teamspec.extent(d);
      // Number of blocks in dimension:
      auto num_blocks_d       = _blockspec.extent(d);
      // Maximum extent of single block in dimension:
      auto blocksize_d        = _blocksize_spec.extent(d);
      // Minimum number of blocks local to every unit in dimension:
      auto min_local_blocks_d = num_blocks_d / num_units_d;
      // Coordinate of this unit id in teamspec in dimension:
      auto unit_ts_coord      = unit_ts_coords[d];
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", unit_ts_coord);
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", num_elem_d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", num_units_d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", blocksize_d);
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d",
                         min_local_blocks_d);
      // Possibly there are more blocks than units in dimension and no
      // block left for this unit. Local extent in d then becomes 0.
      l_extents[d] = min_local_blocks_d * blocksize_d;
      if (num_blocks_d == 1 && num_units_d == 1) {
        // One block assigned to one unit, use full extent in dimension:
        l_extents[d] = num_elem_d;
      } else {
        // Number of additional blocks for this unit, if any:
        IndexType num_add_blocks = static_cast<IndexType>(
                                     num_blocks_d % num_units_d);
        // Unit id assigned to the last block in dimension:
        team_unit_t last_block_unit_d((num_blocks_d % num_units_d == 0)
                                        ? num_units_d - 1
                                        : (num_blocks_d % num_units_d) - 1);
        DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d",
                           last_block_unit_d);
        DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", num_add_blocks);
        if (unit_ts_coord < num_add_blocks) {
          // Unit is assigned to an additional block:
          l_extents[d] += blocksize_d;
          DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", l_extents[d]);
        }
        if (unit_ts_coord == last_block_unit_d) {
          // If the last block in the dimension is underfilled and
          // assigned to the local unit, subtract the missing extent:
          SizeType undfill_blocksize_d = underfilled_blocksize(d);
          DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents",
                             undfill_blocksize_d);
          l_extents[d] -= undfill_blocksize_d;
        }
      }
      DASH_LOG_TRACE_VAR("BlockPattern.init_local_extents.d", l_extents[d]);
    }
    DASH_LOG_DEBUG_VAR("BlockPattern.init_local_extents >", l_extents);
    return l_extents;
  }
};

} // namespace dash

#include <dash/pattern/BlockPattern1D.h>

#endif // DOXYGEN

#endif // DASH__BLOCK_PATTERN_H_
