#ifndef DASH__BLOCK_PATTERN_H_
#define DASH__BLOCK_PATTERN_H_

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
  dim_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename IndexType     = dash::default_index_t>
class Pattern {
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
  typedef CartesianSpace<NumDimensions, SizeType>
    BlockSpec_t;
  typedef CartesianSpace<NumDimensions, SizeType>
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
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// The global layout of the pattern's elements in memory respective to
  /// memory order. Also specifies the extents of the pattern space.
  MemoryLayout_t              _memory_layout;
  /// The view specification of the pattern, consisting of offset and
  /// extent in every dimension
  ViewSpec_t                  _viewspec;
  /// Maximum extents of a block in this pattern
  BlockSizeSpec_t             _blocksize_spec;
  /// Number of blocks in all dimensions
  BlockSpec_t                 _blockspec;
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
    _teamspec(_arguments.teamspec()), 
    _nunits(_team.size()),
    _memory_layout(_arguments.sizespec().extents()),
    _viewspec(_arguments.viewspec()),
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
        local_extents(_team.myid())),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("Pattern()", "Constructor with argument list");
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
    _nunits(_team.size()),
    _memory_layout(sizespec.extents()),
    _viewspec(_memory_layout.extents()),
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
        local_extents(_team.myid())),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("Pattern()", "(sizespec, dist, teamspec, team)");
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
    _nunits(_team.size()),
    _memory_layout(sizespec.extents()),
    _viewspec(_memory_layout.extents()),
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
        local_extents(_team.myid())),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("Pattern()", "(sizespec, dist, team)");
    initialize_local_range();
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
    _local_capacity(other._local_capacity),
    _viewspec(other._viewspec),
    _lbegin(other._lbegin),
    _lend(other._lend) {
    // No need to copy _arguments as it is just used to
    // initialize other members.
  }

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  Pattern(self_t & other)
  : Pattern(static_cast<const self_t &>(other)) {
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(
    /// Pattern instance to compare for equality
    const self_t & other
  ) const {
    if (this == &other) {
      return true;
    }
    // no need to compare local memory layout as it is
    // derived from other members
    // no need to compare lbegin, lend as both are
    // derived from other members
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
    /// Pattern instance to compare for inequality
    const self_t & other
  ) const {
    return !(*this == other);
  }

  /**
   * Assignment operator.
   */
  Pattern & operator=(const Pattern & other) {
    if (this != &other) {
      _distspec            = other._distspec;
      _teamspec            = other._teamspec;
      _memory_layout       = other._memory_layout;
      _local_memory_layout = other._local_memory_layout;
      _blocksize_spec      = other._blocksize_spec;
      _blockspec           = other._blockspec;
      _local_capacity      = other._local_capacity;
      _viewspec            = other._viewspec;
      _nunits              = other._nunits;
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
  dart_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const {
    dart_unit_t unit_id = 0;
    std::array<IndexType, NumDimensions> unit_coords;
    // Coord to block coord to unit coord:
    for (auto d = 0; d < NumDimensions; ++d) {
      unit_coords[d] = (coords[d] / _blocksize_spec.extent(d)) 
                         % _teamspec.extent(d);
    }
    // Unit coord to unit id:
    unit_id = _teamspec.at(unit_coords);
    DASH_LOG_TRACE("Pattern.unit_at",
                   "coords", coords,
                   "> unit id", unit_id);
    return unit_id;
  }

  /**
   * Convert given point in pattern to its assigned unit id.
   */
  template<typename ... Values>
  dart_unit_t unit_at(
    /// Absolute coordinates of the point
    Values ... values
  ) const {
    std::array<IndexType, NumDimensions> inputindex =
      { (IndexType)values... };
    return unit_at(inputindex, _viewspec);
  }

  /**
   * Convert given local coordinates to linear local offset (index).
   *
   * \see DashPatternConcept
   */
  IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    auto coords = local_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      coords[d] += viewspec[d].offset;
    }
    return _local_memory_layout.at(coords);
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
  IndexType extent(dim_t dim) const {
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
   * \see  blocksize()
   * \see  local_size()
   * \see  extent()
   *
   * \see  DashPatternConcept
   */
  IndexType local_extent(dim_t dim) const {
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
   * Converts global coordinates to their associated unit's respective 
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords_to_local(
    const std::array<IndexType, NumDimensions> & global_coords) const {
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
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords_to_global(
    dart_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    DASH_LOG_DEBUG_VAR("Pattern.coords_to_global()", local_coords);
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
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", d);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", 
                         unit_ts_coord[d]);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", local_index_d);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", num_units_d);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", block_index[d]);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", blocksize);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", blocksize_d);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", 
                         elem_block_offset_d);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", block_coord[d]);
      DASH_LOG_TRACE_VAR("Pattern.coords_to_global.d", glob_index[d]);
    }
    DASH_LOG_TRACE_VAR("Pattern.coords_to_global", block_index);
    DASH_LOG_TRACE_VAR("Pattern.coords_to_global", block_coord);
    DASH_LOG_DEBUG_VAR("Pattern.coords_to_global", glob_index);
    return glob_index;
  }

  /**
   * Resolve an element's linear global index from a given unit's local
   * coordinates of that element.
   *
   * \see  at
   *
   * \see  DashPatternConcept
   */
  IndexType local_coords_to_global_index(
    dart_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    std::array<IndexType, NumDimensions> global_coords =
      coords_to_global(unit, local_coords);
    DASH_LOG_TRACE_VAR("Pattern.local_to_global_idx", global_coords);
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
    std::array<IndexType, NumDimensions> local_coords =
      _local_memory_layout.coords(local_index);
    DASH_LOG_TRACE_VAR("Pattern.local_to_global_idx()", local_coords);
    std::array<IndexType, NumDimensions> global_coords =
      coords_to_global(dash::myid(), local_coords);
    DASH_LOG_TRACE_VAR("Pattern.local_to_global_idx >", global_coords);
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
    const std::array<IndexType, NumDimensions> & global_coords,
    const ViewSpec_t & viewspec) const {
    auto coords = global_coords;
    for (auto d = 0; d < NumDimensions; ++d) {
      coords[d] += viewspec[d].offset;
    }
    DASH_LOG_TRACE_VAR("Pattern.at()", coords);
    DASH_LOG_TRACE_VAR("Pattern.at()", viewspec);
    return at(coords);
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
    const std::array<IndexType, NumDimensions> & global_coords) const {
    // Local offset of the element within all of the unit's local
    // elements:
    SizeType local_elem_offset = 0;
    auto unit = unit_at(global_coords);
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords = 
      coords_to_local(global_coords);
    DASH_LOG_TRACE_VAR("Pattern.at", l_coords);
    if (unit == _team.myid()) {
      // Coords are local to this unit, use pre-generated local memory 
      // layout
      return _local_memory_layout.at(l_coords);
    } else {
      // Cannot use _local_memory_layout as it is only defined for the 
      // active unit but does not specify local memory of other units.
      // Generate local memory layout for unit assigned to coords:
      auto l_mem_layout = LocalMemoryLayout_t(local_extents(unit));
      return l_mem_layout.at(l_coords);
    }
  }

  /**
   * Resolves the unit and the local index from global coordinates.
   * 
   * \see  DashPatternConcept
   */
  local_index_t at_unit(
    std::array<IndexType, NumDimensions> & global_coords) const {
    // Local offset of the element within all of the unit's local
    // elements:
    SizeType local_elem_offset = 0;
    auto unit = unit_at(global_coords);
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords = 
      coords_to_local(global_coords);
    DASH_LOG_TRACE_VAR("Pattern.at", l_coords);
    if (unit == _team.myid()) {
      // Coords are local to this unit, use pre-generated local memory 
      // layout
      return local_index_t { unit, _local_memory_layout.at(l_coords) };
    } else {
      // Cannot use _local_memory_layout as it is only defined for the 
      // active unit but does not specify local memory of other units.
      // Generate local memory layout for unit assigned to coords:
      auto l_mem_layout = LocalMemoryLayout_t(local_extents(unit));
      return local_index_t { unit, l_mem_layout.at(l_coords) };
    }
  }

  /**
   * Global coordinates to local index.
   *
   * Convert given coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  template<typename ... Values>
  IndexType at(IndexType value, Values ... values) const {
    static_assert(
      sizeof...(values) == NumDimensions-1,
      "Wrong parameter number");
    std::array<IndexType, NumDimensions> inputindex = { 
      value, (IndexType)values...
    };
    return at(inputindex);
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
    DASH_LOG_TRACE_VAR("Pattern.has_local_elements()", dim);
    DASH_LOG_TRACE_VAR("Pattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("Pattern.has_local_elements()", unit);
    // Apply viewspec offset in dimension to given position
    dim_offset += viewspec[dim].offset;
    // Offset to block offset
    IndexType block_coord_d    = dim_offset / _blocksize_spec.extent(dim);
    DASH_LOG_TRACE_VAR("Pattern.has_local_elements", block_coord_d);
    // Coordinate of unit in team spec in given dimension
    IndexType teamspec_coord_d = block_coord_d % _teamspec.extent(dim);
    DASH_LOG_TRACE_VAR("Pattern.has_local_elements()",  
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
    DASH_LOG_TRACE_VAR("Pattern.is_local >", (coords_unit == unit));
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
    dart_unit_t unit = team().myid();
    return is_local(index, unit);
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
  constexpr SizeType local_capacity() const {
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
  constexpr SizeType local_size() const {
    return _local_memory_layout.size();
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType num_units() const {
    return _nunits;
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

  /**
   * Number of elements in the overflow block of given dimension, with
   * 0 <= \c overflow_blocksize(d) < blocksize(d).
   */
  SizeType overflow_blocksize(
    dim_t dimension) const {
    return _memory_layout.extent(dimension) % blocksize(dimension);
  }

  /**
   * Number of elements missing in the overflow block of given dimension
   * compared to the regular blocksize (\see blocksize(d)), with
   * 0 <= \c underfilled_blocksize(d) < blocksize(d).
   */
  SizeType underfilled_blocksize(
    dim_t dimension) const {
    // Underflow blocksize = regular blocksize - overflow blocksize:
    auto ovf_blocksize = overflow_blocksize(dimension);
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
    const TeamSpec_t         & teamspec) const {
    DASH_LOG_TRACE_VAR("Pattern.init_blocksizespec", teamspec.size());
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
    const TeamSpec_t         & teamspec) const {
    if (blocksizespec.size() == 0) {
      return BlockSpec_t();
    }
    // Number of blocks in all dimensions:
    std::array<SizeType, NumDimensions> n_blocks;
    for (auto d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = distspec[d];
      DASH_LOG_TRACE_VAR("Pattern.init_blockspec", dist.type);
      SizeType max_blocksize_d  = blocksizespec.extent(d);
      SizeType max_blocks_d     = dash::math::div_ceil(
        sizespec.extent(d),
        max_blocksize_d);
      n_blocks[d] = max_blocks_d;
    }
    DASH_LOG_TRACE_VAR("Pattern.init_blockspec", n_blocks);
    return BlockSpec_t(n_blocks);
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
    if (_teamspec.size() == 0) {
      return 0;
    }
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
      DASH_LOG_TRACE_VAR("Pattern.init_lcapacity.d", d);
      DASH_LOG_TRACE_VAR("Pattern.init_lcapacity.d", num_units_d);
      DASH_LOG_TRACE_VAR("Pattern.init_lcapacity.d", 
                         dim_max_blocksize);
      DASH_LOG_TRACE_VAR("Pattern.init_lcapacity.d", dim_num_blocks);
    }
    DASH_LOG_DEBUG_VAR("Pattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range() {
    auto l_size = _local_memory_layout.size(); 
    DASH_LOG_DEBUG_VAR("Pattern.init_local_range()", l_size);
    if (l_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = local_to_global_index(0);
      // Index past last local index transformed to global index
      _lend   = local_to_global_index(l_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("Pattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("Pattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  std::array<SizeType, NumDimensions> local_extents(
    dart_unit_t unit) const {
    if (_nunits == 0) {
      return ::std::array<SizeType, NumDimensions> {  };
    }
    // Coordinates of local unit id in team spec:
    auto unit_ts_coords = _teamspec.coords(unit);
    DASH_LOG_DEBUG_VAR("Pattern.local_extents()", unit);
    DASH_LOG_DEBUG_VAR("Pattern.local_extents()", _nunits);
    DASH_LOG_TRACE_VAR("Pattern.local_extents", unit_ts_coords);
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
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", d);
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", unit_ts_coord);
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", num_elem_d);
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", num_units_d);
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", blocksize_d);
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", 
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
        auto last_block_unit_d = (num_blocks_d % num_units_d == 0)
                                 ? num_units_d - 1
                                 : (num_blocks_d % num_units_d) - 1;
        DASH_LOG_TRACE_VAR("Pattern.local_extents.d", 
                           last_block_unit_d);
        DASH_LOG_TRACE_VAR("Pattern.local_extents.d", num_add_blocks);
        if (unit_ts_coord < num_add_blocks) {
          // Unit is assigned to an additional block:
          l_extents[d] += blocksize_d;
          DASH_LOG_TRACE_VAR("Pattern.local_extents.d", l_extents[d]);
        } 
        if (unit_ts_coord == last_block_unit_d) {
          // If the last block in the dimension is underfilled and
          // assigned to the local unit, subtract the missing extent:
          SizeType undfill_blocksize_d = underfilled_blocksize(d);
          DASH_LOG_TRACE_VAR("Pattern.local_extents", 
                             undfill_blocksize_d);
          l_extents[d] -= undfill_blocksize_d;
        }
      }
      DASH_LOG_TRACE_VAR("Pattern.local_extents.d", l_extents[d]);
    }
    DASH_LOG_DEBUG_VAR("Pattern.local_extents >", l_extents);
    return l_extents;
  }
};

} // namespace dash

#endif // DASH__BLOCK_PATTERN_H_
