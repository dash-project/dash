#ifndef DASH__LINEAR_PATTERN_H_
#define DASH__LINEAR_PATTERN_H_

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
#include <dash/Pattern.h>

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>
#include <dash/internal/PatternArguments.h>

namespace dash {

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 * 
 * \concept{DashPatternConcept}
 */
template<
  typename IndexType = dash::default_index_t>
class LinearPattern
{
private:
  static const dim_t      NumDimensions = 1;
  static const MemArrange Arrangement   = ROW_MAJOR;

public:
  /// Properties guaranteed in pattern property category Blocking
  typedef dash::pattern_blocking_properties<
            // number of elements may differ in blocks
            dash::pattern_blocking_tag::unbalanced >
          blocking_properties;
  /// Properties guaranteed in pattern property category Topology
  typedef dash::pattern_topology_properties<
            // number of blocks assigned to a unit may differ
            dash::pattern_topology_tag::unbalanced >
          topology_properties;
  /// Properties guaranteed in pattern property category Indexing
  typedef dash::pattern_indexing_properties<
            // local indices iterate over block boundaries
            dash::pattern_indexing_tag::local_strided >
          indexing_properties;

private:
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  /// Fully specified type definition of self
  typedef LinearPattern<IndexType>
    self_t;
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

public:
  typedef IndexType   index_type;
  typedef SizeType    size_type;
  typedef ViewSpec_t  viewspec_type;
  typedef struct {
    dart_unit_t unit;
    IndexType   index;
  } local_index_t;
  typedef struct {
    dart_unit_t unit;
    std::array<index_type, NumDimensions> coords;
  } local_coords_t;

private:
  PatternArguments_t          _arguments;
  /// Extent of the linear pattern.
  SizeType                    _size;
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
  /// Maximum extents of a block in this pattern
  SizeType                    _blocksize;
  /// Number of blocks in all dimensions
  BlockSpec_t                 _blockspec;
  /// Arrangement of local blocks in all dimensions
  BlockSpec_t                 _local_blockspec;
  /// A projected view of the global memory layout representing the
  /// local memory layout of this unit's elements.
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
   *   LinearPattern p1(5,3, BLOCKED);
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE));
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              TeamSpec<2>(dash::Team::All(), 1));
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
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
   *   LinearPattern p2(3,3,3, BLOCKED);
   *   // Same as p2
   *   LinearPattern p3(3,3,3);
   *   // A cube with sidelength 3 with blockwise distribution in the third
   *   // dimension
   *   LinearPattern p4(3,3,3, NONE, NONE, BLOCKED);
   * \endcode
   */
  template<typename ... Args>
  LinearPattern(
    /// Argument list consisting of the pattern size (extent, number of 
    /// elements) in every dimension followed by optional distribution     
    /// types.
    SizeType arg,
    /// Argument list consisting of the pattern size (extent, number of 
    /// elements) in every dimension followed by optional distribution     
    /// types.
    Args && ... args)
  : _arguments(arg, args...),
    _size(_arguments.sizespec().size()),
    _distspec(_arguments.distspec()), 
    _team(&_arguments.team()),
    _teamspec(_arguments.teamspec()), 
    _nunits(_team->size()),
    _memory_layout(_arguments.sizespec().extents()),
    _blocksize(initialize_blocksize(
        _arguments.sizespec(),
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        _arguments.sizespec(),
        _distspec,
        _blocksize,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize,
        _distspec,
        _teamspec,
        _local_memory_layout)),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("LinearPattern()", "Constructor with argument list");
    initialize_local_range();
    DASH_LOG_TRACE("LinearPattern()", "LinearPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // How teams are arranged in all dimensions, default is
   *              // an extent of all units in first, and 1 in all higher
   *              // dimensions:
   *              TeamSpec<2>(dash::Team::All(), 1),
   *              // The team containing the units to which the pattern 
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   LinearPattern p1(5,3, BLOCKED);
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE));
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              TeamSpec<2>(dash::Team::All(), 1));
   * \endcode
   */
  LinearPattern(
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
  : _size(sizespec.size()),
    _distspec(dist),
    _team(&team),
    _teamspec(
      teamspec,
      _distspec,
      *_team),
    _nunits(_team->size()),
    _memory_layout(sizespec.extents()),
    _blocksize(initialize_blocksize(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _distspec,
        _blocksize,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize,
        _distspec,
        _teamspec,
        _local_memory_layout)),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("LinearPattern()", "(sizespec, dist, teamspec, team)");
    initialize_local_range();
    DASH_LOG_TRACE("LinearPattern()", "LinearPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // A 5x3 rectangle with blocked distribution in the first dimension
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // The team containing the units to which the pattern 
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              // How teams are arranged in all dimensions, default is
   *              // an extent of all units in first, and 1 in all higher
   *              // dimensions:
   *              TeamSpec<2>(dash::Team::All(), 1),
   *              // The team containing the units to which the pattern 
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   LinearPattern p1(5,3, BLOCKED);
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE));
   *   // Same as
   *   LinearPattern p1(SizeSpec<2>(5,3),
   *              DistributionSpec<2>(BLOCKED, NONE),
   *              TeamSpec<2>(dash::Team::All(), 1));
   * \endcode
   */
  LinearPattern(
    /// Pattern size (extent, number of elements) in every dimension 
    const SizeSpec_t &         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team &                     team = dash::Team::All())
  : _distspec(dist),
    _team(&team),
    _teamspec(_distspec, *_team),
    _nunits(_team->size()),
    _memory_layout(sizespec.extents()),
    _blocksize(initialize_blocksize(
        sizespec,
        _distspec,
        _teamspec)),
    _blockspec(initialize_blockspec(
        sizespec,
        _distspec,
        _blocksize,
        _teamspec)),
    _local_memory_layout(
        initialize_local_extents(_team->myid())),
    _local_blockspec(initialize_local_blockspec(
        _blockspec,
        _blocksize,
        _distspec,
        _teamspec,
        _local_memory_layout)),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("LinearPattern()", "(sizespec, dist, team)");
    initialize_local_range();
    DASH_LOG_TRACE("LinearPattern()", "LinearPattern initialized");
  }

  /**
   * Copy constructor.
   */
  LinearPattern(const self_t & other)
  : _distspec(other._distspec), 
    _team(other._team),
    _teamspec(other._teamspec),
    _nunits(other._nunits),
    _memory_layout(other._memory_layout),
    _blocksize(other._blocksize),
    _blockspec(other._blockspec),
    _local_blockspec(other._local_blockspec),
    _local_memory_layout(other._local_memory_layout),
    _local_capacity(other._local_capacity),
    _lbegin(other._lbegin),
    _lend(other._lend) {
    // No need to copy _arguments as it is just used to
    // initialize other members.
    DASH_LOG_TRACE("LinearPattern(other)", "LinearPattern copied");
  }

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  LinearPattern(self_t & other)
  : LinearPattern(static_cast<const self_t &>(other)) {
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
    // no need to compare all members as most are derived from
    // constructor arguments.
    return(
      _distspec        == other._distspec &&
      _teamspec        == other._teamspec &&
      _memory_layout   == other._memory_layout &&
      _blockspec       == other._blockspec &&
      _blocksize       == other._blocksize &&
      _nunits          == other._nunits
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
  self_t & operator=(const self_t & other) {
    DASH_LOG_TRACE("LinearPattern.=(other)");
    if (this != &other) {
      _distspec            = other._distspec;
      _team                = other._team;
      _teamspec            = other._teamspec;
      _memory_layout       = other._memory_layout;
      _local_memory_layout = other._local_memory_layout;
      _blocksize           = other._blocksize;
      _blockspec           = other._blockspec;
      _local_blockspec     = other._local_blockspec;
      _local_capacity      = other._local_capacity;
      _nunits              = other._nunits;
      _lbegin              = other._lbegin;
      _lend                = other._lend;
      DASH_LOG_TRACE("LinearPattern.=(other)", "LinearPattern assigned");
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
    IndexType vs_coord = coords[0] + viewspec[0].offset;
    return unit_at(vs_coords);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dart_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const {
    dart_unit_t unit_id = (coords[0] / _blocksize)
                           % _teamspec.extent(0);
    DASH_LOG_TRACE("LinearPattern.unit_at",
                   "coords", coords,
                   "> unit id", unit_id);
    return unit_id;
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dart_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec
  ) const {
    return unit_at({ global_pos }, viewspec);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dart_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos
  ) const {
    return unit_at({ global_pos });
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
    return local_coords[0] + viewspec[0].offset;
  }

  /**
   * Convert given local coordinates to linear local offset (index).
   *
   * \see DashPatternConcept
   */
  IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords) const {
    return local_coords[0];
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
    DASH_ASSERT_EQ(
      0, dim, 
      "Wrong dimension for LinearPattern::local_extent. " <<
      "Expected dimension = 0, got " << dim);
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
  IndexType local_extent(dim_t dim) const {
    DASH_ASSERT_EQ(
      0, dim, 
      "Wrong dimension for LinearPattern::local_extent. " <<
      "Expected dimension = 0, got " << dim);
    }
    return local_extents();
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
    DASH_LOG_DEBUG_VAR("LinearPattern.local_extents()", unit);
    SizeType l_extent;
    if (unit == _team->myid()) {
      // Local unit id, get extents from member instance:
      l_extent = _local_size;
    } else {
      // Remote unit id, initialize local memory layout for given unit:
      l_extent = initialize_local_extent(unit);
    }
    DASH_LOG_DEBUG_VAR("LinearPattern.local_extents >", l_extent);
    return l_extents;
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
    const std::array<IndexType, NumDimensions> & global_coords) const {
    local_coords_t l_coords;
    l_coords.coords = local_coords(global_coords);
    l_coords.unit   = unit_at(global_coords);
    return l_coords;
  }

  /**
   * Converts global coordinates to their associated unit's respective 
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> local_coords(
    const std::array<IndexType, NumDimensions> & global_coords) const {
    IndexType local_coord;
    auto b_offset_d       = global_coords[0] % _blocksize;
    auto g_block_offset_d = global_coords[0] / _blocksize;
    auto l_block_offset_d = g_block_offset_d / _nunits;
    local_coord           = b_offset_d +
                            (l_block_offset_d * _blocksize);
    return std::array<IndexType, 1> { local_coord };
  }

  /**
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
    dart_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    DASH_LOG_DEBUG_VAR("LinearPattern.global()", unit);
    DASH_LOG_DEBUG_VAR("LinearPattern.global()", local_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.global", _nunits);
    if (_nunits < 2) {
      return local_coords;
    }
    DASH_LOG_TRACE_VAR("LinearPattern.global", _nblocks);
    // Global coords of the element's block within all blocks.
    // Use initializer so elements are initialized with 0s:
    IndexType block_index;
    // Index of the element:
    IndexType glob_index;

    const Distribution & dist = _distspec[d];
    auto local_index          = local_coords[0];
    auto elem_phase           = local_index_d % _blocksize;
    DASH_LOG_TRACE_VAR("LinearPattern.global", local_index);
    DASH_LOG_TRACE_VAR("LinearPattern.global", elem_phase);
    // Global coords of the element's block within all blocks:
    block_index               = dist.local_index_to_block_coord(
                                  unit,
                                  local_index,
                                  _nunits,
                                  _nblocks,
                                  _blocksize
                                );
    glob_index  = block_index + elem_phase;
    DASH_LOG_TRACE_VAR("LinearPattern.global", block_index);
    DASH_LOG_TRACE_VAR("LinearPattern.global", glob_index);
    return std::array<IndexType, 1> { glob_index };
  }

  /**
   * Converts local coordinates of active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
    const std::array<IndexType, NumDimensions> & local_coords) const {
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
    IndexType local_index) const {
    std::array<IndexType, NumDimensions> local_coords =
      _local_memory_layout.coords(local_index);
    DASH_LOG_TRACE_VAR("LinearPattern.local_to_global_idx()", local_coords);
    std::array<IndexType, NumDimensions> global_coords =
      global(_team->myid(), local_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.local_to_global_idx >", global_coords);
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
    dart_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    std::array<IndexType, NumDimensions> global_coords =
      global(unit, local_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.local_to_global_idx", global_coords);
    return _memory_layout.at(global_coords);
  }

  /**
   * Resolves the unit and the local index from global coordinates.
   * 
   * \see  DashPatternConcept
   */
  local_index_t local_index(
    std::array<IndexType, NumDimensions> & global_coords) const {
    DASH_LOG_TRACE_VAR("LinearPattern.local_index()", global_coords);
    // Local offset of the element within all of the unit's local
    // elements:
    SizeType local_elem_offset = 0;
    auto unit = unit_at(global_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.local_index", unit);
    // Global coords to local coords:
    std::array<IndexType, NumDimensions> l_coords = 
      local_coords(global_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.local_index", l_coords);
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
      local_coords(global_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.at", l_coords);
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
    const ViewSpec_t & viewspec) const {
    DASH_LOG_TRACE_VAR("LinearPattern.at()", global_coords);
    DASH_LOG_TRACE_VAR("LinearPattern.at()", viewspec);
    return global_coords[0] + viewspec[0].offset;
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
    DASH_LOG_TRACE_VAR("LinearPattern.has_local_elements()", dim);
    DASH_LOG_TRACE_VAR("LinearPattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("LinearPattern.has_local_elements()", unit);
    // Apply viewspec offset in dimension to given position
    dim_offset += viewspec[dim].offset;
    // Offset to block offset
    IndexType block_coord_d    = dim_offset / _blocksize_spec.extent(dim);
    DASH_LOG_TRACE_VAR("LinearPattern.has_local_elements", block_coord_d);
    // Coordinate of unit in team spec in given dimension
    IndexType teamspec_coord_d = block_coord_d % _teamspec.extent(dim);
    DASH_LOG_TRACE_VAR("LinearPattern.has_local_elements()",  
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
    auto coords_unit = unit_at(index);
    DASH_LOG_TRACE_VAR("LinearPattern.is_local >", (coords_unit == unit));
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
    return _blocksize;
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
    return _blocksize;
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
    return _local_size;
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
    return _size;
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType size() const {
    return _size;
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
  const DistributionSpec_t & distspec() const {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  SizeSpec_t sizespec() const {
    return SizeSpec_t(std::array<SizeType, 1> { _size });
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  const std::array<SizeType, NumDimensions> & extents() const {
    return std::array<SizeType, 1> { _size };
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
   * View spec (offset and extents) of block at global linear block index in
   * cartesian element space.
   */
  ViewSpec_t block(
    index_type global_block_index) const {
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
    index_type local_block_index) const {
    // Initialize viewspec result with block extents:
    ViewSpec_t block_vs(_blocksize_spec.extents());
    // Local block index to local block coords:
    auto l_block_coords = _local_blockspec.coords(local_block_index);
    auto l_elem_coords  = l_block_coords;
    // TODO: This is convenient but less efficient:
    // Translate local coordinates of first element in local block to global 
    // coordinates:
    for (auto d = 0; d < NumDimensions; ++d) {
      auto blocksize_d  = block_vs[d].extent;
      l_elem_coords[d] *= blocksize_d;
    }
    // Global coordinates of first element in block:
    auto g_elem_coords = global(l_elem_coords);
    for (auto d = 0; d < NumDimensions; ++d) {
      block_vs[d].offset = g_elem_coords[d];
    }
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
    index_type offset = local_block_index * _blocksize;
    return ViewSpec_t({ offset }, { _blocksize });
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
    return 1;
  }

private:
  /**
   * Number of elements in the overflow block of given dimension, with
   * 0 <= \c overflow_blocksize(d) < blocksize(d).
   */
  SizeType overflow_blocksize(
    dim_t dimension) const {
    return _size % _blocksize;
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

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  SizeType initialize_blocksize(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const TeamSpec_t         & teamspec) const {
    DASH_LOG_TRACE_VAR("LinearPattern.init_blocksize", teamspec.size());
    if (teamspec.size() == 0) {
      return 0;
    }
    const Distribution & dist = distspec[0];
    return dist.max_blocksize_in_range(
      sizespec.extent(d),  // size of range (extent)
      teamspec.extent(d)); // number of blocks (units)
  }

  /**
   * Initialize block spec from memory layout, team spec and distribution
   * spec.
   */
  BlockSpec_t initialize_num_blocks(
    const SizeSpec_t         & sizespec,
    const DistributionSpec_t & distspec,
    const SizeType           & blocksize,
    const TeamSpec_t         & teamspec) const {
    if (blocksizespec.size() == 0) {
      return BlockSpec_t();
    }
    // Number of blocks in all dimensions:
    const Distribution & dist = distspec[0];
    DASH_LOG_TRACE_VAR("LinearPattern.init_blockspec", dist.type);
    SizeType n_blocks = dash::math::div_ceil(
                          sizespec.extent(0),
                          blocksize);
    DASH_LOG_TRACE_VAR("LinearPattern.init_blockspec", n_blocks);
    return BlockSpec_t(n_blocks);
  }

  /**
   * Initialize local block spec from global block spec.
   */
  SizeType initialize_num_local_blocks(
    SizeType                    num_blocks,
    SizeType                    blocksize,
    const DistributionSpec_t  & distspec,
    const TeamSpec_t          & teamspec,
    SizeType                    local_size) const {
    auto num_l_blocks = local_size;
    if (blocksize > 0) {
      num_l_blocks /= blocksize;
    } else {
      num_l_block = 0;
    }
    DASH_LOG_TRACE_VAR("LinearPattern.init_num_local_blocks", num_l_blocks);
    return num_l_blocks;
  }

  /**
   * Max. elements per unit (local capacity)
   *
   * Note:
   * Currently calculated as a multiple of full blocks, thus ignoring
   * underfilled blocks.
   */
  SizeType initialize_local_capacity() const {
    SizeType l_capacity = 1;
    if (_nunits == 0) {
      return 0;
    }
    auto max_l_blocks = dash::math::div_ceil(
                          _nblocks,
                          _nunits);
    DASH_LOG_TRACE_VAR("LinearPattern.init_lcapacity.d", _nunits);
    DASH_LOG_TRACE_VAR("LinearPattern.init_lcapacity.d", max_l_blocks_d);
    l_capacity = max_l_blocks * _blocksize;
    DASH_LOG_DEBUG_VAR("LinearPattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range() {
    auto l_size = _local_size; 
    DASH_LOG_DEBUG_VAR("LinearPattern.init_local_range()", l_size);
    if (l_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = global(0);
      // Index past last local index transformed to global index
      _lend   = global(l_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("LinearPattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("LinearPattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  SizeType initialize_local_extent(
    dart_unit_t unit) const {
    DASH_LOG_DEBUG_VAR("LinearPattern.init_local_extent()", unit);
    DASH_LOG_DEBUG_VAR("LinearPattern.init_local_extent()", _nunits);
    if (_nunits == 0) {
      return ::std::array<SizeType, NumDimensions> {  };
    }
    // Coordinates of local unit id in team spec:
    SizeType l_extent     = 0;
    // Minimum number of blocks local to every unit in dimension:
    auto min_local_blocks = _nblocks / _nunits;
    DASH_LOG_TRACE_VAR("LinearPattern.init_local_extent", _nblocks);
    DASH_LOG_TRACE_VAR("LinearPattern.init_local_extent", _blocksize);
    DASH_LOG_TRACE_VAR("LinearPattern.init_local_extent", 
                       min_local_blocks);
    // Possibly there are more blocks than units in dimension and no
    // block left for this unit. Local extent in d then becomes 0.
    l_extent = min_local_blocks * _blocksize;
    if (num_blocks == 1 && _nunits == 1) {
      // One block assigned to one unit, use full extent in dimension:
      l_extent = _size;
    } else {
      // Number of additional blocks for this unit, if any:
      IndexType num_add_blocks = static_cast<IndexType>(
                                   _nblocks % _nunits);
      // Unit id assigned to the last block in dimension:
      auto last_block_unit     = (num_blocks % _nunits == 0)
                                   ? _nunits - 1
                                   : (_nblocks % _nunits) - 1;
      DASH_LOG_TRACE_VAR("LinearPattern.init_local_extents", 
                         last_block_unit);
      DASH_LOG_TRACE_VAR("LinearPattern.init_local_extents", num_add_blocks);
      if (unit < num_add_blocks) {
        // Unit is assigned to an additional block:
        l_extent += _blocksize;
        DASH_LOG_TRACE_VAR("LinearPattern.init_local_extent", l_extent);
      } 
      if (unit == last_block_unit) {
        // If the last block in the dimension is underfilled and
        // assigned to the local unit, subtract the missing extent:
        SizeType undfill_blocksize = underfilled_blocksize(0);
        DASH_LOG_TRACE_VAR("LinearPattern.init_local_extent", 
                           undfill_blocksize);
        l_extent -= undfill_blocksize;
      }
    }

    DASH_LOG_DEBUG_VAR("LinearPattern.init_local_extent >", l_extent);
    return l_extent;
  }
};

} // namespace dash

#endif // DASH__LINEAR_PATTERN_H_
