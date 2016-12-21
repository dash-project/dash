#ifndef DASH__SHIFT_TILE_PATTERN_H_
#define DASH__SHIFT_TILE_PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
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

/**
 * Defines how a list of global indices is mapped to single units within
 * a Team.
 * Expects \c extent[d] to be a multiple of \c (blocksize[d] * nunits[d])
 * to ensure the balanced property.
 *
 * \tparam  NumDimensions  The number of dimensions of the pattern
 * \tparam  Arrangement    The memory order of the pattern (ROW_MAJOR
 *                         or COL_MAJOR), defaults to ROW_MAJOR.
 *                         Memory order defines how elements in the
 *                         pattern will be iterated predominantly
 *                         \see MemArrange
 *
 * \concept{DashPatternConcept}
 *
 */
template<
  dim_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename IndexType     = dash::default_index_t>
class ShiftTilePattern
{
public:
  static constexpr char const * PatternName = "ShiftTilePattern";

public:
  /// Satisfiable properties in pattern property category Partitioning:
  typedef pattern_partitioning_properties<
              // Block extents are constant for every dimension.
              pattern_partitioning_tag::rectangular,
              // Identical number of elements in every block.
              pattern_partitioning_tag::balanced
          > partitioning_properties;
  /// Satisfiable properties in pattern property category Mapping:
  typedef pattern_mapping_properties<
              // Same number of blocks assigned to every unit.
              pattern_mapping_tag::balanced,
              // Number of blocks assigned to a unit may differ.
              pattern_mapping_tag::unbalanced,
              // Every unit mapped in any single slice in every dimension.
              pattern_mapping_tag::diagonal
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
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  /// Fully specified type definition of self
  typedef ShiftTilePattern<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    LocalMemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, SizeType>
    BlockSpec_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, SizeType>
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
  dash::Team                * _team            = nullptr;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// The global layout of the pattern's elements in memory respective to
  /// memory order. Also specifies the extents of the pattern space.
  MemoryLayout_t              _memory_layout;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = dash::Team::All().size();
  /// Major tiled dimension, i.e. lowest tiled dimension in row-major,
  /// highest tiled dimension in column-major order
  dim_t                       _major_tiled_dim;
  /// Minor tiled dimension, i.e. any dimension different from major tiled
  /// dimension
  dim_t                       _minor_tiled_dim;
  /// Maximum extents of a block in this pattern
  BlockSizeSpec_t             _blocksize_spec;
  /// Arrangement of blocks in all dimensions
  BlockSpec_t                 _blockspec;
  /// Arrangement of local blocks in all dimensions
  BlockSpec_t                 _local_blockspec;
  /// A projected view of the global memory layout representing the
  /// local memory layout of this unit's elements respective to memory
  /// order.
  LocalMemoryLayout_t         _local_memory_layout;
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
   *   ShiftTilePattern p1(10,20);
   *   auto num_units = dash::Team::All.size();
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20,
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  // How teams are arranged in all dimensions, default
   *                  // is an extent of all units in first, and 1 in all
   *                  // higher dimensions:
   *                  TeamSpec<2>(dash::Team::All(), 1),
   *                  // The team containing the units to which the
   *                  // pattern maps the global indices. Defaults to all
   *                  // all units:
   *                  dash::Team::All());
   * \endcode
   */
  template<typename ... Args>
  ShiftTilePattern(
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
    // Degrading to 1-dimensional team spec for now:
//  _teamspec(_distspec, *_team),
    _teamspec(_arguments.teamspec()),
    _memory_layout(_arguments.sizespec().extents()),
    _nunits(_teamspec.size()),
    _major_tiled_dim(initialize_major_tiled_dim(_distspec)),
    _minor_tiled_dim((_major_tiled_dim + 1) % NumDimensions),
    _blocksize_spec(initialize_blocksizespec(
        _arguments.sizespec(),
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        _arguments.sizespec(),
        _distspec,
        _blocksize_spec,
        _teamspec)),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _major_tiled_dim,
        _nunits)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("ShiftTilePattern()", "Constructor with Argument list");
    initialize_local_range();
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   ShiftTilePattern p1(10,20);
   *   auto num_units = dash::Team::All.size();
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20,
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  // How teams are arranged in all dimensions, default
   *                  // is an extent of all units in first, and 1 in all
   *                  // higher dimensions:
   *                  TeamSpec<2>(dash::Team::All(), 1),
   *                  // The team containing the units to which the
   *                  // pattern maps the global indices. Defaults to all
   *                  // all units:
   *                  dash::Team::All());
   * \endcode
   */
  ShiftTilePattern(
    /// ShiftTilePattern size (extent, number of elements) in every dimension
    const SizeSpec_t         & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist,
    /// Cartesian arrangement of units within the team
    const TeamSpec_t         & teamspec,
    /// Team containing units to which this pattern maps its elements
    dash::Team               & team     = dash::Team::All())
  : _distspec(dist),
    _team(&team),
    // Degrading to 1-dimensional team spec for now:
//  _teamspec(_distspec, *_team),
//  _teamspec(
//    teamspec,
//    _distspec,
//    *_team),
    _teamspec(teamspec),
    _memory_layout(sizespec.extents()),
    _nunits(_teamspec.size()),
    _major_tiled_dim(initialize_major_tiled_dim(_distspec)),
    _minor_tiled_dim((_major_tiled_dim + 1) % NumDimensions),
    _blocksize_spec(initialize_blocksizespec(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _distspec,
        _blocksize_spec,
        _teamspec)),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _major_tiled_dim,
        _nunits)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("ShiftTilePattern()", "(sizespec, dist, teamspec, team)");
    initialize_local_range();
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   ShiftTilePattern p1(10,20);
   *   auto num_units = dash::Team::All.size();
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20,
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   ShiftTilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  // How teams are arranged in all dimensions, default
   *                  // is an extent of all units in first, and 1 in all
   *                  // higher dimensions:
   *                  TeamSpec<2>(dash::Team::All(), 1),
   *                  // The team containing the units to which the
   *                  // pattern maps the global indices. Defaults to all
   *                  // all units:
   *                  dash::Team::All());
   * \endcode
   */
  ShiftTilePattern(
    /// ShiftTilePattern size (extent, number of elements) in every dimension
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
    _memory_layout(sizespec.extents()),
    _nunits(_teamspec.size()),
    _major_tiled_dim(initialize_major_tiled_dim(_distspec)),
    _minor_tiled_dim((_major_tiled_dim + 1) % NumDimensions),
    _blocksize_spec(initialize_blocksizespec(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _distspec,
        _blocksize_spec,
        _teamspec)),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _major_tiled_dim,
        _nunits)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("ShiftTilePattern()", "(sizespec, dist, team)");
    initialize_local_range();
  }

  /**
   * Copy constructor.
   */
  ShiftTilePattern(const self_t & other)
  : _distspec(other._distspec),
    _team(other._team),
    _teamspec(other._teamspec),
    _memory_layout(other._memory_layout),
    _nunits(other._nunits),
    _major_tiled_dim(other._major_tiled_dim),
    _minor_tiled_dim(other._minor_tiled_dim),
    _blocksize_spec(other._blocksize_spec),
    _blockspec(other._blockspec),
    _local_blockspec(other._local_blockspec),
    _local_memory_layout(other._local_memory_layout),
    _local_capacity(other._local_capacity),
    _lbegin(other._lbegin),
    _lend(other._lend) {
  }

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  ShiftTilePattern(self_t & other)
  : ShiftTilePattern(static_cast<const self_t &>(other)) {
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(
    /// ShiftTilePattern instance to compare for equality
    const self_t & other
  ) const {
    if (this == &other) {
      return true;
    }
    // no need to compare all members as most are derived from
    // constructor arguments.
    return(
      _distspec       == other._distspec &&
      _teamspec       == other._teamspec &&
      _memory_layout  == other._memory_layout &&
      _blockspec      == other._blockspec &&
      _blocksize_spec == other._blocksize_spec &&
      _nunits         == other._nunits
    );
  }

  /**
   * Inquality comparison operator.
   */
  bool operator!=(
    /// ShiftTilePattern instance to compare for inequality
    const self_t & other
  ) const {
    return !(*this == other);
  }

  /**
   * Assignment operator.
   */
  ShiftTilePattern & operator=(const ShiftTilePattern & other) {
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
      _minor_tiled_dim     = other._minor_tiled_dim;
      _major_tiled_dim     = other._major_tiled_dim;
      _lbegin              = other._lbegin;
      _lend                = other._lend;
    }
    return *this;
  }

  /**
   * Resolves the global index of the first local element in the pattern.
   *
   * \see DashPatternConcept
   */
  IndexType lbegin() const {
    return _lbegin;
  }

  /**
   * Resolves the global index past the last local element in the pattern.
   *
   * \see DashPatternConcept
   */
  IndexType lend() const {
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
    /// Absolute coordinates of the point relative to the given view.
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) of the coordinates.
    const ViewSpec_t & viewspec) const
  {
    DASH_LOG_TRACE("ShiftTilePattern.unit_at()",
                   "coords:",   coords,
                   "viewspec:", viewspec);
    // Unit id from diagonals in cartesian index space,
    // e.g (x + y + z) % nunits
    team_unit_t unit_id{0};
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord     = coords[d] + viewspec.offset(d);
      // Global block coordinate:
      auto block_coord  = vs_coord / _blocksize_spec.extent(d);
      unit_id          += block_coord;
    }
    unit_id %= _nunits;
    DASH_LOG_TRACE_VAR("ShiftTilePattern.unit_at >", unit_id);
    return unit_id;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const
  {
    DASH_LOG_TRACE("ShiftTilePattern.unit_at()",
                   "coords:",    coords,
                   "blocksize:", _blocksize_spec.extents());
    // Unit id from diagonals in cartesian index space,
    // e.g (x + y + z) % nunits
    team_unit_t unit_id{0};
    for (auto d = 0; d < NumDimensions; ++d) {
      // Global block coordinate:
      auto block_coord  = coords[d] / _blocksize_spec.extent(d);
      unit_id          += block_coord;
    }
    unit_id %= _nunits;
    DASH_LOG_TRACE_VAR("ShiftTilePattern.unit_at >", unit_id);
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
  SizeType extent(dim_t dim) const {
    if (dim >= NumDimensions || dim < 0) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for ShiftTilePattern::local_extent. "
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
  SizeType local_extent(dim_t dim) const {
    if (dim >= NumDimensions || dim < 0) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for ShiftTilePattern::local_extent. "
        << "Expected dimension between 0 and " << NumDimensions-1 << ", "
        << "got " << dim);
    }
    return _local_memory_layout.extent(dim);
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
      team_unit_t unit = UNDEFINED_TEAM_UNIT_ID) const {
    // Same local memory layout for all units:
    return _local_memory_layout.extents();
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
    /// View specification (local offsets) to apply on \c local_coords
    const ViewSpec_t & viewspec) const
  {
    DASH_LOG_TRACE("ShiftTilePattern.local_at()", local_coords,
                   "view:",         viewspec,
                   "local blocks:", _local_blockspec.extents());
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the local block containing the element:
    std::array<IndexType, NumDimensions> block_coords_l;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_offset_d  = viewspec.offset(d);
      auto vs_coord_d   = local_coords[d] + vs_offset_d;
      auto block_size_d = _blocksize_spec.extent(d);
      phase_coords[d]   = vs_coord_d % block_size_d;
      block_coords_l[d] = vs_coord_d / block_size_d;
    }
    DASH_LOG_TRACE("ShiftTilePattern.local_at",
                   "local block coords:", block_coords_l,
                   "phase coords:",       phase_coords);
    // Number of blocks preceeding the coordinates' block:
    auto block_offset_l = _local_blockspec.at(block_coords_l);
    auto local_index    =
           block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_at >", local_index);
    return local_index;
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
    DASH_LOG_TRACE("ShiftTilePattern.local_at()", local_coords,
                   "local blocks:", _local_blockspec.extents());
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the local block containing the element:
    std::array<IndexType, NumDimensions> block_coords_l;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord_d   = local_coords[d];
      auto block_size_d = _blocksize_spec.extent(d);
      phase_coords[d]   = vs_coord_d % block_size_d;
      block_coords_l[d] = vs_coord_d / block_size_d;
    }
    DASH_LOG_TRACE("ShiftTilePattern.local_at",
                   "local block coords:", block_coords_l,
                   "phase coords:",       phase_coords);
    // Number of blocks preceeding the coordinates' block:
    auto block_offset_l = _local_blockspec.at(block_coords_l);
    auto local_index    =
           block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_at >", local_index);
    return local_index;
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
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local()", g_index);
    // TODO: Implement dedicated method for this, conversion to/from
    //       global coordinates is expensive.
    auto g_coords = coords(g_index);
    return local_index(g_coords);
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
    std::array<IndexType, NumDimensions> local_coords = global_coords;
    auto blocksize_d = _blocksize_spec.extent(_major_tiled_dim);
    auto coord_d     = global_coords[_major_tiled_dim];
    local_coords[_major_tiled_dim] =
      // Local block offset
      (coord_d / (blocksize_d * _nunits)) * blocksize_d +
      // Phase
      (coord_d % blocksize_d);
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
    DASH_LOG_TRACE_VAR("Pattern.local_index()", global_coords);
    // Local offset of the element within all of the unit's local
    // elements:
    auto unit = unit_at(global_coords);
#if __OLD__
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords =
      local_coords(global_coords);
    // Local coords to local offset:
    auto l_index = local_at(l_coords);
#endif
    auto l_index = at(global_coords);
    DASH_LOG_TRACE_VAR("Pattern.local_index >", l_index);

    return local_index_t { team_unit_t(unit), l_index };
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
      const std::array<IndexType, NumDimensions> & local_coords) const {
    DASH_LOG_DEBUG("ShiftTilePattern.global()",
                   "unit:",    unit,
                   "lcoords:", local_coords);
    // Global coordinate of local element:
    std::array<IndexType, NumDimensions> global_coords = local_coords;
    // Local block coordinate of local element:
    auto blocksize_maj     = _blocksize_spec.extent(_major_tiled_dim);
    auto blocksize_min     = _blocksize_spec.extent(_minor_tiled_dim);
    auto l_block_coord_maj = local_coords[_major_tiled_dim] /
                               blocksize_maj;
    auto l_block_coord_min = (NumDimensions > 1)
                             ? local_coords[_minor_tiled_dim] /
                               blocksize_min
                             : 0;
    DASH_LOG_TRACE("ShiftTilePattern.global",
                   "minor tiled dim:",   _minor_tiled_dim,
                   "major tiled dim:",   _major_tiled_dim,
                   "l_block_coord_min:", l_block_coord_min,
                   "l_block_coord_maj:", l_block_coord_maj);
    // Apply diagonal shift in major tiled dimension:
    auto num_shift_blocks = (_nunits + unit -
                              (l_block_coord_min % _nunits))
                            % _nunits;
    num_shift_blocks     += _nunits * l_block_coord_maj;
    DASH_LOG_TRACE("ShiftTilePattern.global",
                   "num_shift_blocks:", num_shift_blocks,
                   "blocksize_maj:",    blocksize_maj);
    global_coords[_major_tiled_dim] =
      (num_shift_blocks * blocksize_maj) +
      local_coords[_major_tiled_dim] % blocksize_maj;
    DASH_LOG_DEBUG_VAR("ShiftTilePattern.global >", global_coords);
    return global_coords;
  }

  /**
   * Converts local coordinates of a active unit to global coordinates.
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
    DASH_LOG_TRACE("ShiftTilePattern.global()",
                   "local_index:", local_index,
                   "unit:",        team().myid());
    auto block_size    = _blocksize_spec.size();
    auto phase         = local_index % block_size;
    auto l_block_index = local_index / block_size;
    // Block coordinate in local memory:
    auto l_block_coord = _local_blockspec.coords(l_block_index);
    // Coordinate of element in block:
    auto phase_coord   = _blocksize_spec.coords(phase);
    DASH_LOG_TRACE("ShiftTilePattern.global",
                   "local block index:",  l_block_index,
                   "local block coords:", l_block_coord,
                   "phase coords:",       phase_coord);
    // Coordinate of element in local memory:
    std::array<IndexType, NumDimensions> l_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      l_coords[d] = l_block_coord[d] * _blocksize_spec.extent(d) +
                    phase_coord[d];
    }
    std::array<IndexType, NumDimensions> g_coords =
      global(team().myid(), l_coords);
    auto offset = _memory_layout.at(g_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.global >", offset);
    return offset;
  }

  /**
   * Resolve an element's linear global index from a given unit's local
   * coordinates of that element.
   *
   * \see  at
   * \see  global_at
   *
   * \see  DashPatternConcept
   */
  IndexType global_index(
    team_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    DASH_LOG_TRACE("ShiftTilePattern.global_index()",
                   "unit:",         unit,
                   "local_coords:", local_coords);
    std::array<IndexType, NumDimensions> global_coords =
      global(unit, local_coords);
    auto g_index = _memory_layout.at(global_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.global_index >", g_index);
    return g_index;
  }

  /**
   * Global coordinates and viewspec to global position in the pattern's
   * iteration order.
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
    const std::array<IndexType, NumDimensions> & global_coords,
    const ViewSpec_t                           & viewspec) const
  {
    DASH_LOG_TRACE("ShiftTilePattern.global_at()",
                   "gcoords:",  global_coords,
                   "viewspec:", viewspec);
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord     = global_coords[d] + viewspec.offset(d);
      phase_coords[d]   = vs_coord % _blocksize_spec.extent(d);
      block_coords[d]   = vs_coord / _blocksize_spec.extent(d);
    }
    DASH_LOG_TRACE("ShiftTilePattern.global_at",
                   "block coords:", block_coords,
                   "phase coords:", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset:
    auto block_index = _blockspec.at(block_coords);
    DASH_LOG_TRACE("ShiftTilePattern.global_at",
                   "block index:",   block_index);
    auto offset = block_index * _blocksize_spec.size() + // preceed. blocks
                  _blocksize_spec.at(phase_coords);      // element phase
    DASH_LOG_TRACE_VAR("ShiftTilePattern.global_at >", offset);
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
    DASH_LOG_TRACE("ShiftTilePattern.global_at()",
                   "gcoords:",  global_coords);
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord     = global_coords[d];
      phase_coords[d]   = vs_coord % _blocksize_spec.extent(d);
      block_coords[d]   = vs_coord / _blocksize_spec.extent(d);
    }
    DASH_LOG_TRACE("ShiftTilePattern.global_at",
                   "block coords:", block_coords,
                   "phase coords:", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset:
    auto block_index = _blockspec.at(block_coords);
    DASH_LOG_TRACE("ShiftTilePattern.global_at",
                   "block index:",   block_index);
    auto offset = block_index * _blocksize_spec.size() + // preceed. blocks
                  _blocksize_spec.at(phase_coords);      // element phase
    DASH_LOG_TRACE_VAR("ShiftTilePattern.global_at >", offset);
    return offset;
  }

  ////////////////////////////////////////////////////////////////////////////
  /// at
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Global coordinates and viewspec to local index.
   *
   * NOTE:
   * Expects extent[d] to be a multiple of blocksize[d] * nunits[d]
   * to ensure the balanced property.
   *
   * \see  global_at
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & global_coords,
    const ViewSpec_t                           & viewspec) const
  {
    DASH_LOG_TRACE("ShiftTilePattern.at()",
                   "gcoords:",  global_coords,
                   "viewspec:", viewspec);
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord     = global_coords[d] + viewspec.offset(d);
      phase_coords[d]   = vs_coord % _blocksize_spec.extent(d);
      block_coords[d]   = vs_coord / _blocksize_spec.extent(d);
    }
    DASH_LOG_TRACE("ShiftTilePattern.at",
                   "block_coords:", block_coords,
                   "phase_coords:", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset divided by team size:
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", _blockspec.extents());
    auto block_index   = _blockspec.at(block_coords);
    auto block_index_l = block_index / _nunits;
    DASH_LOG_TRACE("ShiftTilePattern.at",
                   "global block index:",block_index,
                   "nunits:",            _nunits,
                   "local block index:", block_index_l);
    auto offset = block_index_l * _blocksize_spec.size() + // preceed. blocks
                  _blocksize_spec.at(phase_coords);        // element phase
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at >", offset);
    return offset;
  }

  /**
   * Global coordinates to local index.
   *
   * Convert given global coordinates in pattern to their respective
   * linear local index.
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    std::array<IndexType, NumDimensions> global_coords) const
  {
    // Note:
    // Expects extent[d] to be a multiple of blocksize[d] * nunits[d]
    // to ensure the balanced property.
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at()", global_coords);
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto coord      = global_coords[d];
      phase_coords[d] = coord % _blocksize_spec.extent(d);
      block_coords[d] = coord / _blocksize_spec.extent(d);
    }
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", block_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset divided by team size:
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", _blockspec.extents());
    auto block_offset   = _blockspec.at(block_coords);
    auto block_offset_l = block_offset / _nunits;
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", block_offset);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.at", block_offset_l);
    return block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
  }

  /**
   * Global coordinates to local index.
   *
   * Convert given coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  template<typename ... Values>
  IndexType at(Values ... values) const
  {
    static_assert(
      sizeof...(values) == NumDimensions,
      "Wrong parameter number");
    std::array<IndexType, NumDimensions> inputindex = {
      (IndexType)values...
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
    /// team-local id of the unit
    team_unit_t unit,
    /// Viewspec to apply
    const ViewSpec_t & viewspec) const
  {
    DASH_LOG_TRACE_VAR("ShiftTilePattern.has_local_elements()", dim);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.has_local_elements()", unit);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.has_local_elements()", viewspec);
    // Apply viewspec offset in dimension to given position
    dim_offset += viewspec[dim].offset;
    // Offset to block offset
    IndexType block_coord_d    = dim_offset / _blocksize_spec.extent(dim);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.has_local_elements", block_coord_d);
    // Coordinate of unit in team spec in given dimension
    IndexType teamspec_coord_d = block_coord_d % _teamspec.extent(dim);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.has_local_elements",
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
    IndexType    index,
    team_unit_t unit) const
  {
    auto glob_coords = coords(index);
    auto coords_unit = unit_at(glob_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.is_local >", (coords_unit == unit));
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
    return is_local(index, team().myid());
  }

  ////////////////////////////////////////////////////////////////////////////
  /// block
  ////////////////////////////////////////////////////////////////////////////

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
    DASH_LOG_TRACE("ShiftTilePattern.block_at",
                   "coords", g_coords,
                   "> block index", block_idx);
    return block_idx;
  }

  /**
   * View spec (offset and extents) of block at global linear block index in
   * global cartesian element space.
   */
  ViewSpec_t block(
    index_type global_block_index) const
  {
    DASH_LOG_TRACE_VAR("ShiftTilePattern.block()", global_block_index);
    // block index -> block coords -> offset
    auto block_coords = _blockspec.coords(global_block_index);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.block", block_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.block", _blocksize_spec.extents());
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      extents[d] = _blocksize_spec.extent(d);
      offsets[d] = block_coords[d] * extents[d];
    }
    DASH_LOG_TRACE("ShiftTilePattern.block",
                   "offsets:", offsets,
                   "extents:", extents);
    auto block_vs = ViewSpec_t(offsets, extents);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   */
  ViewSpec_t local_block(
    index_type local_block_index) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_block()", local_block_index);
    // Local block index to local block coords:
    auto l_block_coords = _local_blockspec.coords(local_block_index);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_block()", l_block_coords);
    std::array<index_type, NumDimensions> l_elem_coords;
    // TODO: This is convenient but less efficient:
    // Translate local coordinates of first element in local block to global
    // coordinates:
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d = _blocksize_spec.extent(d);
      l_elem_coords[d] = static_cast<index_type>(
                           l_block_coords[d] * blocksize_d);
    }
    // Global coordinates of first element in block:
    auto g_elem_coords = global(l_elem_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_block()", g_elem_coords);
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      offsets[d] = g_elem_coords[d];
      extents[d] = _blocksize_spec.extent(d);
    }
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_block >", block_vs);
#ifdef __TODO__
    // Coordinates of the unit within the team spec:
    std::array<IndexType, NumDimensions> unit_ts_coord =
      _teamspec.coords(unit);
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = _distspec[d];
      auto blocksize_d          = block_vs[d].extent;
      auto num_units_d          = _teamspec.extent(d);
      auto num_blocks_d         = _blockspec.extent(d);
      // Local to global block coords:
      auto g_block_coord_d      = (l_block_coords[d] + _myid) *
                                  _teamspec.extent(d);
      block_vs[d].offset        = g_block_coord_d * blocksize_d;
    }
#endif
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  ViewSpec_t local_block_local(
    index_type local_block_index) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_block_local()", local_block_index);
    // Initialize viewspec result with block extents:
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents =
      _blocksize_spec.extents();
    // Local block index to local block coords:
    auto l_block_coords = _local_blockspec.coords(local_block_index);
    // Local block coords to local element offset:
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d  = extents[d];
      offsets[d]        = l_block_coords[d] * blocksize_d;
    }
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.local_block >", block_vs);
    return block_vs;
  }

  /**
   * Cartesian arrangement of pattern blocks.
   */
  const BlockSpec_t & blockspec() const
  {
    return _blockspec;
  }

  /**
   * Cartesian arrangement of pattern blocks.
   */
  const BlockSpec_t & local_blockspec() const
  {
    return _local_blockspec;
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
    dim_t dimension) const {
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
  inline SizeType max_blocksize() const {
    return _blocksize_spec.size();
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline SizeType local_capacity() const {
    // Balanced pattern, local capacity identical for every unit and
    // same as local size.
    return local_size();
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
    return _local_memory_layout.size();
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  inline IndexType num_units() const {
    return _teamspec.size();
  }

  /**
   * The maximum number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline IndexType capacity() const {
    return _memory_layout.size();
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  inline IndexType size() const {
    return _memory_layout.size();
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  inline dash::Team & team() const {
    return *_team;
  }

  /**
   * Distribution specification of this pattern.
   */
  inline const DistributionSpec_t & distspec() const {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  SizeSpec_t sizespec() const {
    return SizeSpec_t(_memory_layout.extents());
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  const std::array<SizeType, NumDimensions> & extents() const {
    return _memory_layout.extents();
  }

  /**
   * Cartesian index space representing the underlying memory model of the
   * pattern.
   *
   * \see DashPatternConcept
   */
  const MemoryLayout_t & memory_layout() const {
    return _memory_layout;
  }

  /**
   * Cartesian index space representing the underlying local memory model
   * of this pattern for the calling unit.
   * Not part of DASH Pattern concept.
   */
  const LocalMemoryLayout_t & local_memory_layout() const {
    return _local_memory_layout;
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   *
   * \see DashPatternConcept
   */
  const TeamSpec_t & teamspec() const {
    return _teamspec;
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates.
   *
   * \see DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType index) const {
    return _memory_layout.coords(index);
  }

  /**
   * Memory order followed by the pattern.
   */
  constexpr static MemArrange memory_order() {
    return Arrangement;
  }

  /**
   * Number of dimensions of the cartesian space partitioned by the pattern.
   */
  constexpr static dim_t ndim() {
    return NumDimensions;
  }

private:
  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  BlockSizeSpec_t initialize_blocksizespec(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const TeamSpec_t         & teamspec) const {
    DASH_LOG_TRACE("ShiftTilePattern.init_blocksizespec()");
    // Extents of a single block:
    std::array<SizeType, NumDimensions> s_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = distspec[d];
      DASH_LOG_TRACE("ShiftTilePattern.init_blocksizespec d",
                     "sizespec extent[d]:", sizespec.extent(d),
                     "teamspec extent[d]:", teamspec.extent(d));
      SizeType max_blocksize_d  = dist.max_blocksize_in_range(
        sizespec.extent(d),  // size of range (extent)
        teamspec.extent(d)); // number of blocks (units)
      s_blocks[d] = max_blocksize_d;
    }
    DASH_LOG_TRACE_VAR("ShiftTilePattern.init_blocksizespec >", s_blocks);
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
    DASH_LOG_TRACE("ShiftTilePattern.init_blockspec()",
                   "pattern size:", sizespec.extents(),
                   "block size:",   blocksizespec.extents(),
                   "team size:",    teamspec.extents());
    // Number of blocks in all dimensions:
    std::array<SizeType, NumDimensions> n_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      SizeType max_blocksize_d = blocksizespec.extent(d);
      SizeType max_blocks_d    = dash::math::div_ceil(
                                   sizespec.extent(d),
                                   max_blocksize_d);
      n_blocks[d] = max_blocks_d;
    }
    BlockSpec_t blockspec(n_blocks);
    DASH_LOG_TRACE_VAR("ShiftTilePattern.init_blockspec >", n_blocks);
    return blockspec;
  }

  /**
   * Initialize local block spec from global block spec, major tiled
   * dimension, and team spec.
   */
  BlockSpec_t initialize_local_blockspec(
    const BlockSpec_t        & blockspec,
    dim_t                      major_tiled_dim,
    size_t                     nunits) const
  {
    DASH_LOG_TRACE_VAR("ShiftTilePattern.init_local_blockspec()",
                       blockspec.extents());
    DASH_LOG_TRACE_VAR("ShiftTilePattern.init_local_blockspec()",
                       nunits);
    // Number of local blocks in all dimensions:
    auto l_blocks = blockspec.extents();
    l_blocks[major_tiled_dim] /= nunits;
    DASH_ASSERT_GT(l_blocks[major_tiled_dim], 0,
                   "ShiftTilePattern: Size must be divisible by team size");
    DASH_LOG_TRACE_VAR("ShiftTilePattern.init_local_blockspec >", l_blocks);
    return BlockSpec_t(l_blocks);
  }

  /**
   * Max. elements per unit (local capacity)
   *
   * Note:
   * Currently calculated as (num_local_blocks * block_size), thus
   * ignoring underfilled blocks.
   */
  SizeType initialize_local_capacity(
    team_unit_t unit = UNDEFINED_TEAM_UNIT_ID) const
  {
    // Assumes balanced distribution property, i.e.
    // range = k * blocksz * nunits
    auto l_capacity = size() / _nunits;
    DASH_LOG_TRACE_VAR("ShiftTilePattern.init_local_capacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range() {
    auto local_size = _local_memory_layout.size();
    DASH_LOG_DEBUG_VAR("ShiftTilePattern.init_local_range()", local_size);
    if (local_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = global(0);
      // Index past last local index transformed to global index
      _lend   = global(local_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("ShiftTilePattern.init_local_range >",
                       _local_memory_layout.extents());
    DASH_LOG_DEBUG_VAR("ShiftTilePattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern.init_local_range >", _lend);
  }

  /**
   * Return major dimension with tiled distribution, i.e. lowest tiled
   * dimension.
   */
  dim_t initialize_major_tiled_dim(const DistributionSpec_t & ds)
  {
    DASH_LOG_TRACE("ShiftTilePattern.init_major_tiled_dim()");
    if (Arrangement == dash::COL_MAJOR) {
      DASH_LOG_TRACE("ShiftTilePattern.init_major_tiled_dim", "column major");
      for (auto d = 0; d < NumDimensions; ++d) {
        if (ds[d].type == dash::internal::DIST_TILE) {
          DASH_LOG_TRACE("ShiftTilePattern.init_major_tiled_dim >", d);
          return d;
        }
      }
    } else {
      DASH_LOG_TRACE("ShiftTilePattern.init_major_tiled_dim", "row major");
      for (auto d = NumDimensions-1; d >= 0; --d) {
        if (ds[d].type == dash::internal::DIST_TILE) {
          DASH_LOG_TRACE("ShiftTilePattern.init_major_tiled_dim >", d);
          return d;
        }
      }
    }
    DASH_THROW(dash::exception::InvalidArgument,
              "Distribution is not tiled in any dimension");
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  std::array<SizeType, NumDimensions> initialize_local_extents(
      team_unit_t unit) const {
    // Coordinates of local unit id in team spec:
#ifdef DASH_ENABLE_LOGGING
    auto unit_ts_coords = _teamspec.coords(unit);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern._local_extents()", unit);
    DASH_LOG_TRACE_VAR("ShiftTilePattern._local_extents", unit_ts_coords);
#endif
    ::std::array<SizeType, NumDimensions> l_extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      // Number of units in dimension:
      auto num_units_d        = _teamspec.extent(d);
      // Number of blocks in dimension:
      auto num_blocks_d       = _blockspec.extent(d);
      // Maximum extent of single block in dimension:
      auto blocksize_d        = _blocksize_spec.extent(d);
      // Minimum number of blocks local to every unit in dimension:
      auto min_local_blocks_d = num_blocks_d / num_units_d;
      // Coordinate of this unit id in teamspec in dimension:
//    auto unit_ts_coord      = unit_ts_coords[d];
      // Possibly there are more blocks than units in dimension and no
      // block left for this unit. Local extent in d then becomes 0.
      l_extents[d] = min_local_blocks_d * blocksize_d;
    }
    DASH_LOG_DEBUG_VAR("ShiftTilePattern._local_extents >", l_extents);
    return l_extents;
  }
};

} // namespace dash

#include <dash/pattern/ShiftTilePattern1D.h>

#endif // DASH__SHIFT_TILE_PATTERN_H_
