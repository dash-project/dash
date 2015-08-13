#ifndef DASH__TILE_PATTERN_H_
#define DASH__TILE_PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
#include <array>
#include <type_traits>

#include <dash/Types.h>
#include <dash/Enums.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>
#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>
#include <dash/internal/PatternArguments.h>

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
 */
template<
  dim_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename IndexType     = dash::default_index_t>
class TilePattern {
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
  typedef IndexType index_type;
  typedef SizeType  size_type;
  typedef struct {
    dart_unit_t unit;
    IndexType   index;
  } local_index_t;

private:
  PatternArguments_t          _arguments;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
  /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
  /// dimensions
  DistributionSpec_t          _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team &                _team            = dash::Team::All();
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
  /// The view specification of the pattern, consisting of offset and
  /// extent in every dimension
  ViewSpec_t                  _viewspec;
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
  : _arguments(arg, args...),
    _distspec(_arguments.distspec()), 
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
        _major_tiled_dim)),
    _local_memory_layout(
        initialize_local_extents(_team.myid())),
    _local_capacity(initialize_local_capacity()),
    _viewspec(_arguments.viewspec()) {
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
    const SizeSpec_t &         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist     = DistributionSpec_t(),
    /// Cartesian arrangement of units within the team
    const TeamSpec_t &         teamspec = TeamSpec_t::TeamSpec(),
    /// Team containing units to which this pattern maps its elements
    dash::Team &               team     = dash::Team::All()) 
  : _distspec(dist),
    _team(team),
    _teamspec(
      teamspec,
      _distspec,
      _team),
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
        _major_tiled_dim)),
    _local_memory_layout(
        initialize_local_extents(_team.myid())),
    _local_capacity(initialize_local_capacity()),
    _viewspec(_memory_layout.extents()) {
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
    const SizeSpec_t &         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team &                     team = dash::Team::All())
  : _distspec(dist),
    _team(team),
    _teamspec(_distspec, _team),
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
        _major_tiled_dim)),
    _local_memory_layout(
        initialize_local_extents(_team.myid())),
    _local_capacity(initialize_local_capacity()),
    _viewspec(_memory_layout.extents()) {
    DASH_LOG_TRACE("TilePattern()", "(sizespec, dist, team)");
    initialize_local_range();
  }

  /**
   * Copy constructor.
   */
  TilePattern(const self_t & other)
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
    _viewspec(other._viewspec),
    _lbegin(other._lbegin),
    _lend(other._lend) {
  }

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  TilePattern(self_t & other)
  : TilePattern(static_cast<const self_t &>(other)) {
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(
    /// TilePattern instance to compare for equality
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
      _viewspec       == other._viewspec &&
      _blockspec      == other._blockspec &&
      _blocksize_spec == other._blocksize_spec &&
      _nunits         == other._nunits
    );
  }

  /**
   * Inquality comparison operator.
   */
  bool operator!=(
    /// TilePattern instance to compare for inequality
    const self_t & other
  ) const {
    return !(*this == other);
  }

  /**
   * Assignment operator.
   */
  TilePattern & operator=(const TilePattern & other) {
    if (this != &other) {
      _distspec            = other._distspec;
      _teamspec            = other._teamspec;
      _memory_layout       = other._memory_layout;
      _local_memory_layout = other._local_memory_layout;
      _blocksize_spec      = other._blocksize_spec;
      _blockspec           = other._blockspec;
      _local_blockspec     = other._local_blockspec;
      _local_capacity      = other._local_capacity;
      _viewspec            = other._viewspec;
      _nunits              = other._nunits;
      _minor_tiled_dim     = other._minor_tiled_dim;
      _major_tiled_dim     = other._major_tiled_dim;
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

  /**
   * Convert given point in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dart_unit_t unit_at(
    /// Absolute coordinates of the point
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    // Apply viewspec offsets to coordinates:
    std::array<IndexType, NumDimensions> vs_coords;
    // Unit id from diagonals in cartesian index space,
    // e.g (x + y + z) % nunits
    dart_unit_t unit_id = 0;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord     = coords[d] + viewspec[d].offset;
      // Global block coordinate:
      auto block_coord  = vs_coord / _blocksize_spec.extent(d);
      unit_id          += block_coord;
    }
    unit_id %= _nunits;
    DASH_LOG_TRACE("TilePattern.unit_at", "> unit id", unit_id);
    return unit_id;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dart_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const {
    return unit_at(coords, _viewspec);
  }

  /**
   * Convert given global point in pattern to its assigned unit id.
   */
  template<typename ... Values>
  dart_unit_t unit_at(
    /// Absolute coordinates of the point
    IndexType value, Values ... values
  ) const {
    std::array<IndexType, NumDimensions> global_coords =
      { value, (IndexType)values... };
    return unit_at(global_coords, _viewspec);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   */
  template<typename ... Values>
  dart_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec
  ) const {
    std::array<IndexType, NumDimensions> global_coords =
      _memory_layout.coords(global_pos);
    return unit_at(global_coords, viewspec);
  }

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
    const ViewSpec_t & viewspec) const {
    DASH_LOG_DEBUG_VAR("TilePattern.local_at()", local_coords);
    IndexType l_index = 0;
    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the local block containing the element:
    std::array<IndexType, NumDimensions> block_coords_l;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord_d   = local_coords[d] + viewspec[d].offset;
      phase_coords[d]   = vs_coord_d % _blocksize_spec.extent(d);
      block_coords_l[d] = vs_coord_d / _blocksize_spec.extent(d);
    }
    DASH_LOG_DEBUG_VAR("TilePattern.local_at", block_coords_l);
    DASH_LOG_DEBUG_VAR("TilePattern.local_at", phase_coords);
    DASH_LOG_DEBUG_VAR("TilePattern.local_at", _local_blockspec.extents());
    // Number of blocks preceeding the coordinates' block:
    auto block_offset_l = _local_blockspec.at(block_coords_l);
    return block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
  }

  /**
   * Convert given local coordinates to linear local offset (index).
   *
   * \see DashPatternConcept
   */
  IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords) const {
    return local_at(local_coords, _viewspec);
  }

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
  SizeType local_extent(dim_t dim) const {
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
  std::array<SizeType, NumDimensions> local_extents(
    dart_unit_t unit) const {
    // Same local memory layout for all units:
    return _local_memory_layout.extents();
  }

  /**
   * Converts global coordinates to their associated unit's respective 
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords_to_local(
    const std::array<IndexType, NumDimensions> & global_coords) const {
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
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords_to_global(
    dart_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    DASH_LOG_DEBUG_VAR("TilePattern.local_to_global()", local_coords);
    DASH_LOG_DEBUG_VAR("TilePattern.local_to_global()", unit);
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
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global", _major_tiled_dim);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global", _minor_tiled_dim);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global", l_block_coord_min);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global", l_block_coord_maj);
    // Apply diagonal shift in major tiled dimension:
    auto num_shift_blocks = (_nunits + unit -
                              (l_block_coord_min % _nunits))
                            % _nunits;
    num_shift_blocks     += _nunits * l_block_coord_maj;
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global", num_shift_blocks);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global", blocksize_maj);
    global_coords[_major_tiled_dim] =
      (num_shift_blocks * blocksize_maj) +
      local_coords[_major_tiled_dim] % blocksize_maj;
    DASH_LOG_DEBUG_VAR("TilePattern.local_to_global >", global_coords);
    return global_coords;
  }

  /**
   * Resolve an element's linear global index from a given unit's local
   * coordinates of that element.
   *
   * \see  at
   *
   * \see  DashPatternConcept
   */
  IndexType local_to_global_index(
    dart_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx()", local_coords);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx()", unit);
    std::array<IndexType, NumDimensions> global_coords =
      coords_to_global(unit, local_coords);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx", global_coords);
    return _memory_layout.at(global_coords);
  }

  /**
   * Resolve an element's linear global index from the calling unit's local
   * index of that element.
   *
   * \see  at  Inverse of local_to_global_index
   *
   * \see  DashPatternConcept
   */
  IndexType local_to_global_index(
    IndexType local_index) const {
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx()", local_index);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx()", dash::myid());
    std::array<IndexType, NumDimensions> local_coords =
      _local_memory_layout.coords(local_index);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx", local_coords);
    std::array<IndexType, NumDimensions> global_coords =
      coords_to_global(dash::myid(), local_coords);
    DASH_LOG_TRACE_VAR("TilePattern.local_to_global_idx", global_coords);
    return _memory_layout.at(global_coords);
  }

  /**
   * Global coordinates and viewspec to local index.
   *
   * Convert given global coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & coords,
    const ViewSpec_t & viewspec) const {
    // Note:
    // Expects extent[d] to be a multiple of blocksize[d] * nunits[d]
    // to ensure the balanced property.

    // Phase coordinates of element:
    std::array<IndexType, NumDimensions> phase_coords;
    // Coordinates of the block containing the element:
    std::array<IndexType, NumDimensions> block_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      auto vs_coord     = coords[d] + viewspec[d].offset;
      phase_coords[d]   = vs_coord % _blocksize_spec.extent(d);
      block_coords[d]   = vs_coord / _blocksize_spec.extent(d);
    }
    DASH_LOG_TRACE_VAR("TilePattern.at()", coords);
    DASH_LOG_TRACE_VAR("TilePattern.at()", block_coords);
    DASH_LOG_TRACE_VAR("TilePattern.at()", phase_coords);
    // Number of blocks preceeding the coordinates' block, equivalent
    // to linear global block offset divided by team size:
    DASH_LOG_TRACE_VAR("TilePattern.at()", _blockspec.extents());
    auto block_offset   = _blockspec.at(block_coords);
    auto block_offset_l = block_offset / _nunits;
    DASH_LOG_TRACE_VAR("TilePattern.at()", block_offset);
    DASH_LOG_TRACE_VAR("TilePattern.at()", _nunits);
    DASH_LOG_TRACE_VAR("TilePattern.at()", block_offset_l);
    return block_offset_l * _blocksize_spec.size() + // preceeding blocks
           _blocksize_spec.at(phase_coords);         // element phase
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
    std::array<IndexType, NumDimensions> global_coords) const {
    return at(global_coords, _viewspec);
  }

  /**
   * Global coordinates to local index.
   *
   * Convert given coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  template<typename ... Values>
  IndexType at(Values ... values) const {
    static_assert(
      sizeof...(values) == NumDimensions,
      "Wrong parameter number");
    std::array<IndexType, NumDimensions> inputindex = { 
      (IndexType)values...
    };
    return at(inputindex, _viewspec);
  }

  /**
   * Resolves the unit and the local index from global coordinates.
   * 
   * \see  DashPatternConcept
   */
  local_index_t local(
    std::array<IndexType, NumDimensions> & global_coords) const {
    DASH_LOG_TRACE_VAR("Pattern.local()", global_coords);
    // Local offset of the element within all of the unit's local
    // elements:
    SizeType local_elem_offset = 0;
    auto unit = unit_at(global_coords);
    DASH_LOG_TRACE_VAR("Pattern.local >", unit);
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords = 
      coords_to_local(global_coords);
    DASH_LOG_TRACE_VAR("Pattern.local >", l_coords);
    return local_index_t { unit, local_at(l_coords) };
  }

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
    dart_unit_t unit,
    /// Viewspec to apply
    const ViewSpec_t & viewspec) const {
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", dim);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()", unit);
    // Apply viewspec offset in dimension to given position
    dim_offset += viewspec[dim].offset;
    // Offset to block offset
    IndexType block_coord_d    = dim_offset / _blocksize_spec.extent(dim);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements", block_coord_d);
    // Coordinate of unit in team spec in given dimension
    IndexType teamspec_coord_d = block_coord_d % _teamspec.extent(dim);
    DASH_LOG_TRACE_VAR("TilePattern.has_local_elements()",  
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
    dart_unit_t unit) const {
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
  bool is_local(
    IndexType index) const {
    return is_local(index, team().myid());
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
  SizeType max_blocksize() const {
    return _blocksize_spec.size();
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  SizeType local_capacity() const {
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
  SizeType local_size() const {
    return _local_memory_layout.size();
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  IndexType num_units() const {
    return _teamspec.size();
  }

  /**
   * The maximum number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  IndexType capacity() const {
    return _memory_layout.size();
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  IndexType size() const {
    return _memory_layout.size();
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  dash::Team & team() const {
    return _team;
  }

  /**
   * Distribution specification of this pattern.
   */
  const DistributionSpec_t & distspec() const {
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
   */
  const MemoryLayout_t & memory_layout() const {
    return _memory_layout;
  }

  /**
   * Cartesian index space representing the underlying local memory model
   * of this pattern for the calling unit.
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
   * View specification of this pattern as offset and extent in every
   * dimension.
   *
   * \see DashPatternConcept
   */
  const ViewSpec_t & viewspec() const {
    return _viewspec;
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

private:
  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  BlockSizeSpec_t initialize_blocksizespec(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const TeamSpec_t         & teamspec) const {
    // Number of blocks in all dimensions:
    std::array<SizeType, NumDimensions> n_blocks;
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
    const TeamSpec_t         & teamspec) const {
    // Number of blocks in all dimensions:
    std::array<SizeType, NumDimensions> n_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = distspec[d];
      SizeType max_blocksize_d  = blocksizespec.extent(d);
      SizeType max_blocks_d     = dash::math::div_ceil(
        sizespec.extent(d),
        max_blocksize_d);
      n_blocks[d] = max_blocks_d;
    }
    return BlockSpec_t(n_blocks);
  }

  /**
   * Initialize local block spec from global block spec, major tiled
   * dimension, and team spec.
   */
  BlockSpec_t initialize_local_blockspec(
    const BlockSpec_t        & blockspec,
    dim_t                      major_tiled_dim) const {
    // Number of local blocks in all dimensions:
    auto l_blocks = blockspec.extents();
    l_blocks[major_tiled_dim] /= _nunits;
    DASH_LOG_TRACE_VAR("TilePattern.init_local_blockspec", l_blocks);
    return BlockSpec_t(l_blocks);
  }

  /**
   * Max. elements per unit (local capacity)
   *
   * Note:
   * Currently calculated as (num_local_blocks * block_size), thus
   * ignoring underfilled blocks.
   */
  SizeType initialize_local_capacity() {
    SizeType l_capacity = 1;
    for (auto d = 0; d < NumDimensions; ++d) {
      SizeType num_units_d      = _teamspec.extent(d);
      const Distribution & dist = _distspec[d];
      // Block size in given dimension:
      auto dim_max_blocksize    = _blocksize_spec.extent(d);
      // Maximum number of occurrences of a single unit in given
      // dimension:
      // TODO: Should be dist.max_local_elements_in_range for later
      //       support of dash::BALANCED_*
      SizeType dim_num_blocks   = dist.max_local_blocks_in_range(
                                    // size of range:
                                    _memory_layout.extent(d),
                                    // number of units:
                                    num_units_d
                                  );
      l_capacity *= dim_max_blocksize * dim_num_blocks;
      DASH_LOG_TRACE_VAR("TilePattern.init_lcapacity.d", d);
      DASH_LOG_TRACE_VAR("TilePattern.init_lcapacity.d", num_units_d);
      DASH_LOG_TRACE_VAR("TilePattern.init_lcapacity.d", 
                         dim_max_blocksize);
      DASH_LOG_TRACE_VAR("TilePattern.init_lcapacity.d", dim_num_blocks);
    }
    DASH_LOG_DEBUG_VAR("TilePattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range() {
    auto local_size = _local_memory_layout.size();
    DASH_LOG_DEBUG_VAR("TilePattern.initialize_local_range()", local_size);
    if (local_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = local_to_global_index(0);
      // Index past last local index transformed to global index
      _lend   = local_to_global_index(local_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("TilePattern.initialize_local_range >", 
                       _local_memory_layout.extents());
    DASH_LOG_DEBUG_VAR("TilePattern.initialize_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("TilePattern.initialize_local_range >", _lend);
  }

  /**
   * Return major dimension with tiled distribution, i.e. lowest tiled 
   * dimension.
   */
  dim_t initialize_major_tiled_dim(const DistributionSpec_t & ds) {
    DASH_LOG_TRACE("TilePattern.init_major_tiled_dim()");
    for (auto d = 0; d < NumDimensions; ++d) {
      if (ds[d].type == dash::internal::DIST_TILE) return d;
    }
    DASH_THROW(dash::exception::InvalidArgument,
              "Distribution is not tiled in any dimension");
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  std::array<SizeType, NumDimensions> initialize_local_extents(
    dart_unit_t unit) const {
    // Coordinates of local unit id in team spec:
    auto unit_ts_coords = _teamspec.coords(unit);
    DASH_LOG_DEBUG_VAR("TilePattern._local_extents()", unit);
    DASH_LOG_TRACE_VAR("TilePattern._local_extents", unit_ts_coords);
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
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", d);
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", unit_ts_coord);
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", num_elem_d);
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", num_units_d);
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", blocksize_d);
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", 
                         min_local_blocks_d);
      // Possibly there are more blocks than units in dimension and no
      // block left for this unit. Local extent in d then becomes 0.
      l_extents[d] = min_local_blocks_d * blocksize_d;
      DASH_LOG_TRACE_VAR("TilePattern._local_extents.d", l_extents[d]);
    }
    DASH_LOG_DEBUG_VAR("TilePattern._local_extents >", l_extents);
    return l_extents;
  }
};

} // namespace dash

#endif // DASH__TILE_PATTERN_H_
