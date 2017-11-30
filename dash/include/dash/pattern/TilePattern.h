#ifndef DASH__TILE_PATTERN_H_
#define DASH__TILE_PATTERN_H_

#include <functional>
#include <cstring>
#include <array>
#include <type_traits>
#include <iostream>
#include <sstream>

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
class TilePattern
{
public:
  static constexpr char const * PatternName = "TilePattern";

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
              pattern_mapping_tag::unbalanced
          > mapping_properties;
  /// Satisfiable properties in pattern property category Layout:
  typedef pattern_layout_properties<
              // Elements are contiguous in local memory within single
              // block.
              pattern_layout_tag::blocked,
              // Local element order corresponds to a logical
              // linearization within single blocks.
              pattern_layout_tag::linear
          > layout_properties;

private:
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  /// Fully specified type definition of self
  typedef TilePattern<NumDimensions, Arrangement, IndexType>
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
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
  /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
  /// dimensions
  DistributionSpec_t          _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team                * _team            = nullptr;
  /// The active unit's id.
  team_unit_t                 _myid;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// The global layout of the pattern's elements in memory respective to
  /// memory order. Also specifies the extents of the pattern space.
  MemoryLayout_t              _memory_layout;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = dash::Team::All().size();
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
   *   TilePattern p1(10,20);
   *   auto num_units = dash::Team::All.size();
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20,
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20),
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
  TilePattern(
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    SizeType arg,
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    Args && ... args)
  : TilePattern(PatternArguments_t(arg, args...))
  {
    DASH_LOG_TRACE("TilePattern()", "Constructor with Argument list");
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
   *   TilePattern p1(10,20);
   *   auto num_units = dash::Team::All.size();
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20,
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20),
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
  TilePattern(
    /// TilePattern size (extent, number of elements) in every dimension
    const SizeSpec_t         & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions.
    const DistributionSpec_t & dist,
    /// Cartesian arrangement of units within the team
    const TeamSpec_t         & teamspec,
    /// Team containing units to which this pattern maps its elements
    dash::Team               & team     = dash::Team::All())
  : _distspec(dist),
    _team(&team),
    _myid(_team->myid()),
    _teamspec(
      teamspec,
      _distspec,
      *_team),
    _memory_layout(sizespec.extents()),
    _nunits(_teamspec.size()),
    _blocksize_spec(initialize_blocksizespec(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _blocksize_spec,
        _teamspec)),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize_spec,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_myid)),
    _local_capacity(
        initialize_local_capacity(_local_memory_layout))
  {
    DASH_LOG_TRACE("TilePattern()", "(sizespec, dist, teamspec, team)");
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
   *   TilePattern p1(10,20);
   *   auto num_units = dash::Team::All.size();
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20,
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20),
   *                  DistributionSpec<2>(TILE(10/num_units),
   *                                      TILE(20/num_units)));
   *                  TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   TilePattern p1(SizeSpec<2>(10,20),
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
  TilePattern(
    /// TilePattern size (extent, number of elements) in every dimension
    const SizeSpec_t         & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team                     & team = dash::Team::All())
  : _distspec(dist),
    _team(&team),
    _myid(_team->myid()),
    _teamspec(_distspec, *_team),
    _memory_layout(sizespec.extents()),
    _nunits(_teamspec.size()),
    _blocksize_spec(initialize_blocksizespec(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _blocksize_spec,
        _teamspec)),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize_spec,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_myid)),
    _local_capacity(
        initialize_local_capacity(_local_memory_layout))
  {
    DASH_LOG_TRACE("TilePattern()", "(sizespec, dist, team)");
    initialize_local_range();
  }

  /**
   * Copy constructor.
   */
  TilePattern(const self_t & other) = default;

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  TilePattern(self_t & other)
  : TilePattern(static_cast<const self_t &>(other))
  { }

  /**
   * Assignment operator.
   */
  TilePattern & operator=(const self_t & other) = default;

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const
  {
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
  constexpr bool operator!=(
    /// TilePattern instance to compare for inequality
    const self_t & other) const
  {
    return !(*this == other);
  }

  /**
   * Resolves the global index of the first local element in the pattern.
   *
   * \see DashPatternConcept
   */
  constexpr IndexType lbegin() const {
    return _lbegin;
  }

  /**
   * Resolves the global index past the last local element in the pattern.
   *
   * \see DashPatternConcept
   */
  constexpr IndexType lend() const {
    return _lend;
  }

  ////////////////////////////////////////////////////////////////////////
  /// unit_at
  ////////////////////////////////////////////////////////////////////////

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
    DASH_LOG_TRACE("TilePattern.unit_at()",
                   "coords:",   coords,
                   "viewspec:", viewspec);
    std::array<IndexType, NumDimensions> unit_ts_coords;
    std::array<IndexType, NumDimensions> block_coords;
    // Unit id from diagonals in cartesian index space,
    // e.g (x + y + z) % nunits
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord      = coords[d] + viewspec.offset(d);
      // Global block coordinate:
      block_coords[d]   = vs_coord / _blocksize_spec.extent(d);
      unit_ts_coords[d] = block_coords[d] % _teamspec.extent(d);
    }
    team_unit_t unit_id(_teamspec.at(unit_ts_coords));
    DASH_LOG_TRACE_VAR("TilePattern.unit_at", block_coords);
    DASH_LOG_TRACE_VAR("TilePattern.unit_at", unit_ts_coords);
    DASH_LOG_TRACE_VAR("TilePattern.unit_at >", unit_id);
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
    DASH_LOG_TRACE("TilePattern.unit_at()",
                   "coords:",   coords);
    std::array<IndexType, NumDimensions> unit_ts_coords;
    std::array<IndexType, NumDimensions> block_coords;
    // Unit id from diagonals in cartesian index space,
    // e.g (x + y + z) % nunits
    for (auto d = 0; d < NumDimensions; ++d) {
      // Global block coordinate:
      block_coords[d]   = coords[d] / _blocksize_spec.extent(d);
      unit_ts_coords[d] = block_coords[d] % _teamspec.extent(d);
    }
    team_unit_t unit_id(_teamspec.at(unit_ts_coords));
    DASH_LOG_TRACE_VAR("TilePattern.unit_at", block_coords);
    DASH_LOG_TRACE_VAR("TilePattern.unit_at", unit_ts_coords);
    DASH_LOG_TRACE_VAR("TilePattern.unit_at >", unit_id);
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

  ////////////////////////////////////////////////////////////////////////
  /// extent
  ////////////////////////////////////////////////////////////////////////

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
        "Wrong dimension for TilePattern::local_extent. "
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
  SizeType local_extent(dim_t dim) const
  {
    if (dim >= NumDimensions || dim < 0) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for TilePattern::local_extent. "
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
  constexpr std::array<SizeType, NumDimensions> local_extents(
      team_unit_t unit = UNDEFINED_TEAM_UNIT_ID) const
  {
    return ( ( unit == UNDEFINED_TEAM_UNIT_ID ||
               unit == _myid )
            ? _local_memory_layout.extents()
            : initialize_local_extents(unit) );
  }

  ////////////////////////////////////////////////////////////////////////
  /// local
  ////////////////////////////////////////////////////////////////////////

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
    DASH_LOG_TRACE("TilePattern.local_at()",
                   "local_coords:", local_coords,
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
    DASH_LOG_TRACE("TilePattern.local_at",
                   "local_coords:",       local_coords);
    DASH_LOG_TRACE("TilePattern.local_at",
                   "view:",               viewspec);
    DASH_LOG_TRACE("TilePattern.local_at",
                   "local blocks:",       _local_blockspec.extents());
    DASH_LOG_TRACE("TilePattern.local_at",
                   "local block coords:", block_coords_l);
    DASH_LOG_TRACE("TilePattern.local_at",
                   "phase coords:",       phase_coords);
    // Number of blocks preceeding the coordinates' block:
    auto block_offset_l = _local_blockspec.at(block_coords_l);
    DASH_LOG_TRACE("TilePattern.local_at",
                   "local block offset:", block_offset_l);
    auto local_index    =
           block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
    DASH_LOG_TRACE_VAR("TilePattern.local_at >", local_index);
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
    DASH_LOG_TRACE("TilePattern.local_at()",
                   "local coords:", local_coords,
                   "local blocks:", _local_blockspec.extents());
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the local block containing the element:
    std::array<IndexType, NumDimensions> block_coords_l;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto gcoord_d     = local_coords[d];
      auto block_size_d = _blocksize_spec.extent(d);
      phase_coords[d]   = gcoord_d % block_size_d;
      block_coords_l[d] = gcoord_d / block_size_d;
    }
    DASH_LOG_TRACE("TilePattern.local_at",
                   "local_coords:",       local_coords,
                   "local blocks:",       _local_blockspec.extents(),
                   "local block coords:", block_coords_l,
                   "block size:",         _blocksize_spec.extents(),
                   "phase coords:",       phase_coords);
    // Number of blocks preceeding the coordinates' block:
    auto block_offset_l = _local_blockspec.at(block_coords_l);
    auto local_index    =
           block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
    DASH_LOG_TRACE_VAR("TilePattern.local_at >", local_index);
    return local_index;
  }

  /**
   * Converts global coordinates to their associated unit and its
   * respective local coordinates.
   *
   * \see  DashPatternConcept
   */
  local_coords_t local(
    const std::array<IndexType, NumDimensions> & global_coords) const
  {
    local_coords_t l_coords;
    std::array<IndexType, NumDimensions> local_coords;
    std::array<IndexType, NumDimensions> unit_ts_coords;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      auto nunits_d        = _teamspec.extent(d);
      auto blocksize_d     = _blocksize_spec.extent(d);
      auto block_coord_d   = global_coords[d] / blocksize_d;
      auto phase_d         = global_coords[d] % blocksize_d;
      auto l_block_coord_d = block_coord_d / nunits_d;
      unit_ts_coords[d]    = block_coord_d % nunits_d;
      local_coords[d]      = (l_block_coord_d * blocksize_d) + phase_d;
    }
    l_coords.unit   = _teamspec.at(unit_ts_coords);
    l_coords.coords = local_coords;
    return l_coords;
  }

  /**
   * Converts global index to its associated unit and respective local
   * index.
   *
   * TODO: Unoptimized
   *
   * \see  DashPatternConcept
   */
  constexpr local_index_t local(
    IndexType g_index) const
  {
    // TODO: Implement dedicated method for this, conversion to/from
    //       global coordinates is expensive.
    return local_index(coords(g_index));
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
    for (dim_t d = 0; d < NumDimensions; ++d) {
      auto nunits_d        = _teamspec.extent(d);
      auto blocksize_d     = _blocksize_spec.extent(d);
      auto block_coord_d   = global_coords[d] / blocksize_d;
      auto phase_d         = global_coords[d] % blocksize_d;
      auto l_block_coord_d = block_coord_d / nunits_d;
      local_coords[d]      = (l_block_coord_d * blocksize_d) + phase_d;
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
    DASH_LOG_TRACE_VAR("TilePattern.local_index()", global_coords);
    // Global coordinates to unit and local offset:
    auto       l_pos_coords  = local(global_coords);
    auto       l_coords      = l_pos_coords.coords;
    auto       unit          = l_pos_coords.unit;
    index_type l_index       = -1;
    DASH_LOG_TRACE_VAR("TilePattern.local_index", l_coords);
    DASH_LOG_TRACE_VAR("TilePattern.local_index", unit);

    if (unit == _myid) {
      // Local coords to local offset:
      l_index = local_at(l_coords);
    } else {
      // Global coordinates point to remote location, requires to construct
      // _local_blockspec of remote unit:
      auto remote_l_blockspec = initialize_local_blockspec(
                                  _blockspec, _blocksize_spec, _teamspec,
                                  unit);

      // Phase coordinates of element:
      std::array<IndexType, NumDimensions> phase_coords;
      // Coordinates of the local block containing the element:
      std::array<IndexType, NumDimensions> block_coords_l;
      for (auto d = 0; d < NumDimensions; ++d) {
        auto gcoord_d     = l_coords[d];
        auto block_size_d = _blocksize_spec.extent(d);
        phase_coords[d]   = gcoord_d % block_size_d;
        block_coords_l[d] = gcoord_d / block_size_d;
      }
      DASH_LOG_TRACE("TilePattern.local_index",
                     "local_coords:",       l_coords,
                     "local blocks:",       remote_l_blockspec.extents(),
                     "local block coords:", block_coords_l,
                     "block size:",         _blocksize_spec.extents(),
                     "phase coords:",       phase_coords);
      // Number of blocks preceeding the coordinates' block:
      auto block_offset_l = remote_l_blockspec.at(block_coords_l);
      l_index = block_offset_l * _blocksize_spec.size() + // prec. blocks
                _blocksize_spec.at(phase_coords);         // elem. phase
    }
    DASH_LOG_TRACE_VAR("TilePattern.local_index >", l_index);

    return local_index_t { unit, l_index };
  }

  ////////////////////////////////////////////////////////////////////////
  /// global
  ////////////////////////////////////////////////////////////////////////

  /**
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
    team_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    DASH_LOG_DEBUG("TilePattern.global()",
                   "unit:",    unit,
                   "lcoords:", local_coords);
    // Global coordinate of local element:
    std::array<IndexType, NumDimensions> global_coords;
    std::array<IndexType, NumDimensions> unit_ts_coords = _teamspec.coords(
                                                            unit);
    for (dim_t d = 0; d < NumDimensions; ++d) {
      auto blocksize_d     = _blocksize_spec.extent(d);
      auto nunits_d        = _teamspec.extent(d);
      auto phase           = local_coords[d] % blocksize_d;
      auto l_block_coord_d = local_coords[d] / blocksize_d;
      auto g_block_coord_d = (l_block_coord_d * nunits_d) +
                             unit_ts_coords[d];
      global_coords[d]     = (g_block_coord_d * blocksize_d) + phase;
    }
    DASH_LOG_DEBUG_VAR("TilePattern.global >", global_coords);
    return global_coords;
  }

  /**
   * Converts local coordinates of a active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> global(
    const std::array<IndexType, NumDimensions> & local_coords) const {
    return global(_myid, local_coords);
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
    DASH_LOG_TRACE("TilePattern.global()",
                   "local_index:", local_index,
                   "unit:",        _myid);
    auto block_size    = _blocksize_spec.size();
    auto phase         = local_index % block_size;
    auto l_block_index = local_index / block_size;
    // Block coordinate in local memory:
    auto l_block_coord = _local_blockspec.coords(l_block_index);
    // Coordinate of element in block:
    auto phase_coord   = _blocksize_spec.coords(phase);
    DASH_LOG_TRACE("TilePattern.global",
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
      global(_myid, l_coords);
    auto offset = _memory_layout.at(g_coords);
    DASH_LOG_TRACE_VAR("TilePattern.global >", offset);
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
    DASH_LOG_TRACE("TilePattern.global_index()",
                   "unit:",         unit,
                   "local_coords:", local_coords);
    std::array<IndexType, NumDimensions> global_coords =
      global(unit, local_coords);
    auto g_index = _memory_layout.at(global_coords);
    DASH_LOG_TRACE_VAR("TilePattern.global_index >", g_index);
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
    DASH_LOG_TRACE("TilePattern.global_at()",
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
    DASH_LOG_TRACE("TilePattern.global_at",
                   "block coords:", block_coords,
                   "phase coords:", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset:
    auto block_index = _blockspec.at(block_coords);
    DASH_LOG_TRACE("TilePattern.global_at",
                   "block index:",   block_index);
    auto offset = block_index * _blocksize_spec.size() + // preceed. blocks
                  _blocksize_spec.at(phase_coords);      // element phase
    DASH_LOG_TRACE_VAR("TilePattern.global_at >", offset);
    return offset;
  }

  /**
   * Global coordinates to global position in the pattern's iteration
   * order.
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
    DASH_LOG_TRACE("TilePattern.global_at()",
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
    DASH_LOG_TRACE("TilePattern.global_at",
                   "block coords:", block_coords,
                   "phase coords:", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset:
    auto block_index = _blockspec.at(block_coords);
    DASH_LOG_TRACE("TilePattern.global_at",
                   "block index:",   block_index);
    auto offset = block_index * _blocksize_spec.size() + // preceed. blocks
                  _blocksize_spec.at(phase_coords);      // element phase
    DASH_LOG_TRACE_VAR("TilePattern.global_at >", offset);
    return offset;
  }

  ////////////////////////////////////////////////////////////////////////
  /// at
  ////////////////////////////////////////////////////////////////////////

  /**
   * Global coordinates and viewspec to local index.
   *
   * \see  global_at
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & global_coords,
    const ViewSpec_t                           & viewspec) const
  {
    DASH_LOG_TRACE("TilePattern.at()",
                   "gcoords:",  global_coords,
                   "viewspec:", viewspec);
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    // Local coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> l_block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto nunits_d     = _teamspec.extent(d);
      auto vs_coord     = global_coords[d] + viewspec.offset(d);
      phase_coords[d]   = vs_coord % _blocksize_spec.extent(d);
      block_coords[d]   = vs_coord / _blocksize_spec.extent(d);
      l_block_coords[d] = block_coords[d] / nunits_d;
    }
    index_type l_block_index = _local_blockspec.at(l_block_coords);
    DASH_LOG_TRACE("TilePattern.at",
                   "block_coords:",   block_coords,
                   "l_block_coords:", l_block_coords,
                   "l_block_index:",  l_block_index,
                   "phase_coords:",   phase_coords);
    auto offset = l_block_index * _blocksize_spec.size() + // prec. blocks
                  _blocksize_spec.at(phase_coords);        // elem. phase
    DASH_LOG_TRACE_VAR("TilePattern.at >", offset);
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
    DASH_LOG_TRACE("TilePattern.at()", "gcoords:",  global_coords);
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    // Local coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> l_block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto nunits_d     = _teamspec.extent(d);
      auto gcoord_d     = global_coords[d];
      phase_coords[d]   = gcoord_d % _blocksize_spec.extent(d);
      block_coords[d]   = gcoord_d / _blocksize_spec.extent(d);
      l_block_coords[d] = block_coords[d] / nunits_d;
    }
    index_type l_block_index = _local_blockspec.at(l_block_coords);
    DASH_LOG_TRACE("TilePattern.at",
                   "block_coords:",   block_coords,
                   "l_block_coords:", l_block_coords,
                   "l_block_index:",  l_block_index,
                   "phase_coords:",   phase_coords);
    auto offset = l_block_index * _blocksize_spec.size() + // prec. blocks
                  _blocksize_spec.at(phase_coords);        // elem. phase
    DASH_LOG_TRACE_VAR("TilePattern.at >", offset);
    return offset;
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

  ////////////////////////////////////////////////////////////////////////
  /// is_local
  ////////////////////////////////////////////////////////////////////////

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
    /// DART id of the unit
    team_unit_t unit,
    /// Viewspec to apply
    const ViewSpec_t & viewspec) const
  {
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", dim);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", unit);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", viewspec);
    // Apply viewspec offset in dimension to given position
    dim_offset += viewspec[dim].offset;
    // Offset to block offset
    IndexType block_coord_d    = dim_offset / _blocksize_spec.extent(dim);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements", block_coord_d);
    // Coordinate of unit in team spec in given dimension
    IndexType teamspec_coord_d = block_coord_d % _teamspec.extent(dim);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements",
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
    DASH_LOG_TRACE_VAR("TilePattern.is_local >", (coords_unit == unit));
    return coords_unit == unit;
  }

  /**
   * Whether the given global index is local to the unit that created
   * this pattern instance.
   *
   * \see  DashPatternConcept
   */
  constexpr bool is_local(
    IndexType index) const
  {
    return is_local(index, _myid);
  }

  ////////////////////////////////////////////////////////////////////////
  /// block
  ////////////////////////////////////////////////////////////////////////

  /**
   * Index of block in global block space at given global coordinates.
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
    DASH_LOG_TRACE("TilePattern.block_at",
                   "coords", g_coords,
                   "> block index", block_idx);
    return block_idx;
  }

  /**
   * Unit and local block index at given global coordinates.
   *
   * \see  DashPatternConcept
   */
  local_index_t local_block_at(
    /// Global coordinates of element
    const std::array<index_type, NumDimensions> & g_coords) const
  {
    local_index_t l_pos;

    std::array<IndexType, NumDimensions> l_block_coords;
    std::array<IndexType, NumDimensions> unit_ts_coords;
    for (dim_t d = 0; d < NumDimensions; ++d) {
      auto nunits_d      = _teamspec.extent(d);
      auto blocksize_d   = _blocksize_spec.extent(d);
      auto block_coord_d = g_coords[d] / blocksize_d;
      l_block_coords[d]  = block_coord_d / nunits_d;
      unit_ts_coords[d]  = block_coord_d % nunits_d;
    }
    l_pos.unit  = _teamspec.at(unit_ts_coords);
    l_pos.index = _local_blockspec.at(l_block_coords);

    DASH_LOG_TRACE("TilePattern.local_block_at >",
                   "coords",             g_coords,
                   "unit:",              l_pos.unit,
                   "local block index:", l_pos.index);
    return l_pos;
  }

  /**
   * View spec (offset and extents) of block at global linear block index
   * in global cartesian element space.
   *
   * \see  DashPatternConcept
   */
  ViewSpec_t block(
    index_type global_block_index) const
  {
    DASH_LOG_TRACE_VAR("TilePattern.block()", global_block_index);
    // block index -> block coords -> offset
    auto block_coords = _blockspec.coords(global_block_index);
    DASH_LOG_TRACE_VAR("TilePattern.block", block_coords);
    DASH_LOG_TRACE_VAR("TilePattern.block", _blocksize_spec.extents());
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d = _blocksize_spec.extent(d);
      extents[d] = blocksize_d;
      offsets[d] = block_coords[d] * blocksize_d;
    }
    DASH_LOG_TRACE("TilePattern.block",
                   "offsets:", offsets,
                   "extents:", extents);
    auto block_vs = ViewSpec_t(offsets, extents);
    DASH_LOG_TRACE_VAR("TilePattern.block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at global block coordinates.
   *
   * \see  DashPatternConcept
   */
  ViewSpec_t block(
    /// Global coordinates of element
    const std::array<index_type, NumDimensions> & block_coords) const
  {
    DASH_LOG_TRACE_VAR("TilePattern.block()", block_coords);
    // block coords -> offset
    DASH_LOG_TRACE_VAR("TilePattern.block", _blocksize_spec.extents());
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d = _blocksize_spec.extent(d);
      extents[d] = blocksize_d;
      offsets[d] = block_coords[d] * blocksize_d;
    }
    DASH_LOG_TRACE("TilePattern.block",
                   "offsets:", offsets,
                   "extents:", extents);
    auto block_vs = ViewSpec_t(offsets, extents);
    DASH_LOG_TRACE_VAR("TilePattern.block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   *
   * \see  DashPatternConcept
   */
  constexpr ViewSpec_t local_block(
    index_type local_block_index) const
  {
    return local_block(_myid, local_block_index);
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   *
   * \see  DashPatternConcept
   */
  ViewSpec_t local_block(
    team_unit_t unit,
    index_type   local_block_index) const
  {
    DASH_LOG_TRACE("TilePattern.local_block()",
                   "unit:",       unit,
                   "lblock_idx:", local_block_index,
                   "lblockspec:", _local_blockspec.extents());
    // Local block index to local block coords:
    auto l_block_coords = _local_blockspec.coords(local_block_index);
    auto unit_ts_coords = _teamspec.coords(unit);
    DASH_LOG_TRACE_VAR("TilePattern.local_block", l_block_coords);
    std::array<index_type, NumDimensions> offsets;
    std::array<size_type, NumDimensions>  extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d = _blocksize_spec.extent(d);
      auto nunits_d    = _teamspec.extent(d);
      // Block offsets are lobal coordinates of first block element:
      offsets[d] = ((l_block_coords[d] * nunits_d) + unit_ts_coords[d]) *
                   blocksize_d;
      extents[d] = blocksize_d;
    }
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_TRACE_VAR("TilePattern.local_block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   *
   * \see  DashPatternConcept
   */
  ViewSpec_t local_block_local(
    index_type local_block_index) const
  {
    DASH_LOG_TRACE_VAR("TilePattern.local_block_local()",
                       local_block_index);
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
    DASH_LOG_TRACE_VAR("TilePattern.local_block_local >", block_vs);
    return block_vs;
  }

  /**
   * Cartesian arrangement of pattern blocks.
   */
  constexpr const BlockSpec_t & blockspec() const
  {
    return _blockspec;
  }

  /**
   * Cartesian arrangement of pattern blocks.
   */
  constexpr const BlockSpec_t & local_blockspec() const
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
  constexpr SizeType blocksize(
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
  constexpr SizeType max_blocksize() const {
    return _blocksize_spec.size();
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr SizeType local_capacity() const {
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
  SizeType local_size(team_unit_t unit = UNDEFINED_TEAM_UNIT_ID) const {
    if (unit == UNDEFINED_TEAM_UNIT_ID) {
      return _local_memory_layout.size();
    }
    // Non-local query, requires to construct local memory layout of
    // remote unit:
    return LocalMemoryLayout_t(initialize_local_extents(unit)).size();
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType num_units() const {
    return _teamspec.size();
  }

  /**
   * The maximum number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType capacity() const {
    return _memory_layout.size();
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType size() const {
    return _memory_layout.size();
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  constexpr dash::Team & team() const {
    return *_team;
  }

  /**
   * Distribution specification of this pattern.
   */
  constexpr const DistributionSpec_t & distspec() const {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  constexpr SizeSpec_t sizespec() const {
    return SizeSpec_t(_memory_layout.extents());
  }

  /**
   * Size specification (shape) of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  constexpr const std::array<SizeType, NumDimensions> & extents() const {
    return _memory_layout.extents();
  }

  /**
   * Cartesian index space representing the underlying memory model of the
   * pattern.
   *
   * \see DashPatternConcept
   */
  constexpr const MemoryLayout_t & memory_layout() const {
    return _memory_layout;
  }

  /**
   * Cartesian index space representing the underlying local memory model
   * of this pattern for the calling unit.
   * Not part of DASH Pattern concept.
   */
  constexpr const LocalMemoryLayout_t & local_memory_layout() const {
    return _local_memory_layout;
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   *
   * \see DashPatternConcept
   */
  constexpr const TeamSpec_t & teamspec() const {
    return _teamspec;
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates.
   *
   * \see DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> coords(
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
   * Number of dimensions of the cartesian space partitioned by the
   * pattern.
   */
  constexpr static dim_t ndim() {
    return NumDimensions;
  }

private:

  TilePattern(const PatternArguments_t & arguments)
  : _distspec(arguments.distspec()),
    _team(&arguments.team()),
    _myid(_team->myid()),
    _teamspec(arguments.teamspec()),
    _memory_layout(arguments.sizespec().extents()),
    _nunits(_teamspec.size()),
    _blocksize_spec(initialize_blocksizespec(
        arguments.sizespec(),
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        arguments.sizespec(),
        _blocksize_spec,
        _teamspec)),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize_spec,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_myid)),
    _local_capacity(
        initialize_local_capacity(_local_memory_layout))
  {}

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  BlockSizeSpec_t initialize_blocksizespec(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const TeamSpec_t         & teamspec) const {
    DASH_LOG_TRACE("TilePattern.init_blocksizespec()",
                   "sizespec:", sizespec.extents(),
                   "distspec:", distspec.values(),
                   "teamspec:", teamspec.extents());
    // Extents of a single block:
    std::array<SizeType, NumDimensions> s_blocks;
    if (sizespec.size() == 0 || teamspec.size() == 0) {
      DASH_LOG_TRACE("TilePattern.init_blocksizespec >",
                     "sizespec or teamspec uninitialized",
                     "(default construction?), cancel");
      return BlockSizeSpec_t(s_blocks);
    }
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = distspec[d];
      auto  extent_d   = sizespec.extent(d);
      auto  units_d    = teamspec.extent(d);
      DASH_ASSERT_GT(extent_d, 0,
                     "Extent of size spec in dimension" << d << "is 0");
      DASH_ASSERT_GT(units_d,  0,
                     "Extent of team spec in dimension" << d << "is 0");
      auto blocksize_d = dist.max_blocksize_in_range(
                           extent_d, // size of range (extent)
                           units_d   // number of blocks (units)
                         );
      DASH_ASSERT_EQ(0, extent_d % blocksize_d,
                     "TilePattern requires balanced block sizes: " <<
                     "extent "    << extent_d    << " is no multiple of " <<
                     "block size" << blocksize_d << " in " <<
                     "dimension " << d);
      s_blocks[d] = blocksize_d;
    }
    DASH_LOG_TRACE_VAR("TilePattern.init_blocksizespec >", s_blocks);
    return BlockSizeSpec_t(s_blocks);
  }

  /**
   * Initialize block spec from memory layout, team spec and distribution
   * spec.
   */
  BlockSpec_t initialize_blockspec(
    const SizeSpec_t         & sizespec,
    const BlockSizeSpec_t    & blocksizespec,
    const TeamSpec_t         & teamspec) const
  {
    DASH_LOG_TRACE("TilePattern.init_blockspec()",
                   "pattern size:", sizespec.extents(),
                   "block size:",   blocksizespec.extents(),
                   "team size:",    teamspec.extents());
    // Number of blocks in all dimensions:
    if (teamspec.size() == 0 || sizespec.size() == 0) {
      BlockSpec_t empty_blockspec;
      DASH_LOG_TRACE_VAR("TilePattern.init_blockspec >",
                         empty_blockspec.extents());
      return empty_blockspec;
    }
    std::array<SizeType, NumDimensions> n_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      SizeType max_blocksize_d = blocksizespec.extent(d);
      SizeType max_blocks_d    = dash::math::div_ceil(
                                   sizespec.extent(d),
                                   max_blocksize_d);
      n_blocks[d] = max_blocks_d;
    }
    BlockSpec_t blockspec(n_blocks);
    DASH_LOG_TRACE_VAR("TilePattern.init_blockspec >", n_blocks);
    return blockspec;
  }

  /**
   * Initialize local block spec from global block spec, major tiled
   * dimension, and team spec.
   */
  BlockSpec_t initialize_local_blockspec(
    const BlockSpec_t     & blockspec,
    const BlockSizeSpec_t & blocksizespec,
    const TeamSpec_t      & teamspec,
    team_unit_t              unit_id = UNDEFINED_TEAM_UNIT_ID) const
  {
    DASH_LOG_TRACE_VAR("TilePattern.init_local_blockspec()",
                       blockspec.extents());
    if (unit_id == DART_UNDEFINED_UNIT_ID) {
      unit_id = _myid;
    }
    if (blockspec.size() == 0 || teamspec.size() == 0 ||
        blocksizespec.size() == 0) {
      BlockSpec_t empty_blockspec;
      DASH_LOG_TRACE_VAR("TilePattern.init_local_blockspec >",
                         empty_blockspec.extents());
      return empty_blockspec;
    }
    // Number of local blocks in all dimensions:
    auto l_blocks = blockspec.extents();
    // Coordinates of local unit id in team spec:
    auto unit_ts_coords = _teamspec.coords(unit_id);
    DASH_LOG_TRACE_VAR("TilePattern.init_local_blockspec", unit_ts_coords);
    for (auto d = 0; d < NumDimensions; ++d) {
      // Number of units in dimension:
      auto num_units_d        = _teamspec.extent(d);
      // Number of blocks in dimension:
      auto num_blocks_d       = _blockspec.extent(d);
      // Number of blocks assigned to unit in this dimension:
      auto num_l_blocks_d     = num_blocks_d / num_units_d;
      auto num_odd_blocks_d   = num_blocks_d % num_units_d;
      if (num_odd_blocks_d > unit_ts_coords[d]) {
        ++num_l_blocks_d;
      }
      l_blocks[d] = num_l_blocks_d;
    }
    DASH_LOG_TRACE_VAR("TilePattern.init_local_blockspec >", l_blocks);
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
    const LocalMemoryLayout_t & local_extents) const
  {
    DASH_LOG_TRACE_VAR("TilePattern.init_local_capacity()",
                       local_extents.extents());
    auto l_capacity = local_extents.size();
    DASH_LOG_TRACE_VAR("TilePattern.init_local_capacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range()
  {
    auto local_size = _local_memory_layout.size();
    DASH_LOG_DEBUG_VAR("TilePattern.init_local_range()", local_size);
    if (local_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = global(0);
      // Index past last local index transformed to global index
      _lend   = global(local_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("TilePattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("TilePattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  std::array<SizeType, NumDimensions> initialize_local_extents(
    team_unit_t unit) const
  {
    // Coordinates of local unit id in team spec:
    DASH_LOG_DEBUG_VAR("TilePattern.init_local_extents()", unit);
    if (_teamspec.size() == 0) {
      ::std::array<SizeType, NumDimensions> empty_extents = {{ }};
      DASH_LOG_DEBUG_VAR("TilePattern.init_local_extents >", empty_extents);
      return empty_extents;
    }
    auto unit_ts_coords = _teamspec.coords(unit);
    DASH_LOG_TRACE_VAR("TilePattern.init_local_extents", unit_ts_coords);
    ::std::array<SizeType, NumDimensions> l_extents;
    for (auto d = 0; d < NumDimensions; ++d) {
      // Number of units in dimension:
      auto num_units_d        = _teamspec.extent(d);
      // Number of blocks in dimension:
      auto num_blocks_d       = _blockspec.extent(d);
      // Maximum extent of single block in dimension:
      auto blocksize_d        = _blocksize_spec.extent(d);
      // Number of blocks assigned to unit in this dimension:
      auto num_l_blocks_d     = num_blocks_d / num_units_d;
      auto num_odd_blocks_d   = num_blocks_d % num_units_d;
      if (num_odd_blocks_d > unit_ts_coords[d]) {
        ++num_l_blocks_d;
      }
      // Possibly there are more blocks than units in dimension and no
      // block left for this unit. Local extent in d then becomes 0.
      l_extents[d] = num_l_blocks_d * blocksize_d;
    }
    DASH_LOG_DEBUG_VAR("TilePattern.init_local_extents >", l_extents);
    return l_extents;
  }
};

template<
  dim_t      ND,
  MemArrange Ar,
  typename   Index>
std::ostream & operator<<(
  std::ostream                   & os,
  const TilePattern<ND,Ar,Index> & pattern)
{
  typedef Index index_t;

  dim_t ndim = pattern.ndim();

  std::string storage_order = pattern.memory_order() == ROW_MAJOR
                              ? "ROW_MAJOR"
                              : "COL_MAJOR";

  std::array<index_t, 2> blocksize;
  blocksize[0] = pattern.blocksize(0);
  blocksize[1] = pattern.blocksize(1);

  std::ostringstream ss;
  ss << "dash::"
     << TilePattern<ND,Ar,Index>::PatternName
     << "<"
     << ndim << ","
     << storage_order << ","
     << typeid(index_t).name()
     << ">"
     << "("
     << "SizeSpec:"  << pattern.sizespec().extents()  << ", "
     << "TeamSpec:"  << pattern.teamspec().extents()  << ", "
     << "BlockSpec:" << pattern.blockspec().extents() << ", "
     << "BlockSize:" << blocksize
     << ")";

  return operator<<(os, ss.str());
}

} // namespace dash

#include <dash/pattern/TilePattern1D.h>

#endif // DASH__TILE_PATTERN_H_
