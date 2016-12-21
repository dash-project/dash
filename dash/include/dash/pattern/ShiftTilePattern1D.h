#ifndef DASH__SHIFT_TILE_PATTERN_1D_H_
#define DASH__SHIFT_TILE_PATTERN_1D_H_

#ifndef DOXYGEN

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

#include <dash/pattern/ShiftTilePattern.h>

namespace dash {

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 *
 * \concept{DashPatternConcept}
 */
template<
  MemArrange Arrangement,
  typename   IndexType
>
class ShiftTilePattern<1, Arrangement, IndexType>
{
private:
  static const dim_t      NumDimensions = 1;

public:
  static constexpr char const * PatternName = "ShiftTilePattern<1>";

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
    team_unit_t                            unit;
    IndexType                             index;
  } local_index_t;
  typedef struct {
    team_unit_t                            unit;
    std::array<index_type, NumDimensions> coords;
  } local_coords_t;

private:
  PatternArguments_t          _arguments;
  /// Extent of the linear pattern.
  SizeType                    _size;
  /// Global memory layout of the pattern.
  MemoryLayout_t              _memory_layout;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC or NONE) of
  /// all dimensions. Defaults to BLOCKED.
  DistributionSpec_t          _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team *                _team            = nullptr;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// Maximum extents of a block in this pattern
  SizeType                    _blocksize       = 0;
  /// Number of blocks in all dimensions
  SizeType                    _nblocks         = 0;
  /// Actual number of local elements.
  SizeType                    _local_size;
  /// Local memory layout of the pattern.
  LocalMemoryLayout_t         _local_memory_layout;
  /// Arrangement of local blocks in all dimensions
  SizeType                    _nlblocks;
  /// Maximum number of elements assigned to a single unit
  SizeType                    _local_capacity;
  /// Corresponding global index to first local index of the active unit
  IndexType                   _lbegin;
  /// Corresponding global index past last local index of the active unit
  IndexType                   _lend;

public:
  /**
   * Constructor, initializes a pattern from an argument list consisting
   * of the pattern size (extent, number of elements) followed by an optional
   * distribution type.
   *
   * Examples:
   *
   * \code
   *   // 500 elements with blocked distribution:
   *   Pattern p1(500, BLOCKED);
   *   // Same as
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<2>(BLOCKED),
   *              TeamSpec<1>(dash::Team::All()),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
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
    _size(_arguments.sizespec().size()),
    _memory_layout(std::array<SizeType, 1> { _size }),
    _distspec(_arguments.distspec()),
    _team(&_arguments.team()),
    _teamspec(_arguments.teamspec()),
    _nunits(_team->size()),
    _blocksize(initialize_blocksize(
        _size,
        _distspec,
        _nunits)),
    _nblocks(initialize_num_blocks(
        _size,
        _blocksize,
        _nunits)),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> { _local_size }),
    _nlblocks(initialize_num_local_blocks(
        _nblocks,
        _blocksize,
        _distspec,
        _nunits,
        _local_size)),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("ShiftTilePattern<1>()", "Constructor with argument list");
    initialize_local_range();
    DASH_LOG_TRACE("ShiftTilePattern<1>()", "ShiftTilePattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // 500 elements with blocked distribution:
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<1>(BLOCKED),
   *              TeamSpec<1>(dash::Team::All()),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   Pattern p1(500, BLOCKED);
   *   // Same as
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<1>(BLOCKED));
   *   // Same as
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<1>(BLOCKED),
   *              TeamSpec<1>(dash::Team::All()));
   * \endcode
   */
  ShiftTilePattern(
    /// Pattern size (extent, number of elements) in every dimension
    const SizeSpec_t &         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC or NONE).
    /// Defaults to BLOCKED.
    const DistributionSpec_t & dist     = DistributionSpec_t(),
    /// Cartesian arrangement of units within the team
    const TeamSpec_t &         teamspec = TeamSpec_t::TeamSpec(),
    /// Team containing units to which this pattern maps its elements
    dash::Team &               team     = dash::Team::All())
  : _size(sizespec.size()),
    _memory_layout(std::array<SizeType, 1> { _size }),
    _distspec(dist),
    _team(&team),
    _teamspec(
      teamspec,
      _distspec,
      *_team),
    _nunits(_team->size()),
    _blocksize(initialize_blocksize(
        _size,
        _distspec,
        _nunits)),
    _nblocks(initialize_num_blocks(
        _size,
        _blocksize,
        _nunits)),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> { _local_size }),
    _nlblocks(initialize_num_local_blocks(
        _nblocks,
        _blocksize,
        _distspec,
        _nunits,
        _local_size)),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("ShiftTilePattern<1>()", "(sizespec, dist, teamspec, team)");
    initialize_local_range();
    DASH_LOG_TRACE("ShiftTilePattern<1>()", "ShiftTilePattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   * Examples:
   *
   * \code
   *   // 500 elements with blocked distribution:
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<1>(BLOCKED),
   *              TeamSpec<1>(dash::Team::All()),
   *              // The team containing the units to which the pattern
   *              // maps the global indices. Defaults to all all units:
   *              dash::Team::All());
   *   // Same as
   *   Pattern p1(500, BLOCKED);
   *   // Same as
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<1>(BLOCKED));
   *   // Same as
   *   Pattern p1(SizeSpec<1>(500),
   *              DistributionSpec<1>(BLOCKED),
   *              TeamSpec<1>(dash::Team::All()));
   * \endcode
   */
  ShiftTilePattern(
    /// Pattern size (extent, number of elements) in every dimension
    const SizeSpec_t &         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team &                     team = dash::Team::All())
  : _size(sizespec.size()),
    _memory_layout(std::array<SizeType, 1> { _size }),
    _distspec(dist),
    _team(&team),
    _teamspec(_distspec, *_team),
    _nunits(_team->size()),
    _blocksize(initialize_blocksize(
        _size,
        _distspec,
        _nunits)),
    _nblocks(initialize_num_blocks(
        _size,
        _blocksize,
        _nunits)),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> { _local_size }),
    _nlblocks(initialize_num_local_blocks(
        _nblocks,
        _blocksize,
        _distspec,
        _nunits,
        _local_size)),
    _local_capacity(initialize_local_capacity()) {
    DASH_LOG_TRACE("ShiftTilePattern<1>()", "(sizespec, dist, team)");
    initialize_local_range();
    DASH_LOG_TRACE("ShiftTilePattern<1>()", "ShiftTilePattern initialized");
  }

  /**
   * Copy constructor.
   */
  ShiftTilePattern(const self_t & other)
  : _size(other._size),
    _memory_layout(other._memory_layout),
    _distspec(other._distspec),
    _team(other._team),
    _teamspec(other._teamspec),
    _nunits(other._nunits),
    _blocksize(other._blocksize),
    _nblocks(other._nblocks),
    _nlblocks(other._nlblocks),
    _local_size(other._local_size),
    _local_memory_layout(other._local_memory_layout),
    _local_capacity(other._local_capacity),
    _lbegin(other._lbegin),
    _lend(other._lend) {
    // No need to copy _arguments as it is just used to
    // initialize other members.
    DASH_LOG_TRACE("ShiftTilePattern<1>(other)", "ShiftTilePattern copied");
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
    /// Pattern instance to compare for equality
    const self_t & other
  ) const {
    if (this == &other) {
      return true;
    }
    // no need to compare all members as most are derived from
    // constructor arguments.
    return(
      _size      == other._size &&
      _distspec  == other._distspec &&
      _teamspec  == other._teamspec &&
      _nblocks   == other._nblocks &&
      _blocksize == other._blocksize &&
      _nunits    == other._nunits
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
    DASH_LOG_TRACE("ShiftTilePattern<1>.=(other)");
    if (this != &other) {
      _size                = other._size;
      _memory_layout       = other._memory_layout;
      _distspec            = other._distspec;
      _team                = other._team;
      _teamspec            = other._teamspec;
      _local_size          = other._local_size;
      _local_memory_layout = other._local_memory_layout;
      _blocksize           = other._blocksize;
      _nblocks             = other._nblocks;
      _nlblocks            = other._nlblocks;
      _local_capacity      = other._local_capacity;
      _nunits              = other._nunits;
      _lbegin              = other._lbegin;
      _lend                = other._lend;
      DASH_LOG_TRACE("ShiftTilePattern<1>.=(other)", "ShiftTilePattern assigned");
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
    /// Absolute coordinates of the point
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at()", coords);
    // Apply viewspec offsets to coordinates:
    team_unit_t unit_id(((coords[0] + viewspec[0].offset) / _blocksize)
                          % _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at >", unit_id);
    return unit_id;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at()", coords);
    team_unit_t unit_id((coords[0] / _blocksize) % _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at >", unit_id);
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
    const ViewSpec_t & viewspec
  ) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at()", global_pos);
    // Apply viewspec offsets to coordinates:
    team_unit_t unit_id(((global_pos + viewspec[0].offset) / _blocksize)
                          % _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at >", unit_id);
    return unit_id;
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  team_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos
  ) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at()", global_pos);
    team_unit_t unit_id((global_pos / _blocksize) % _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.unit_at >", unit_id);
    return unit_id;
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
  IndexType extent(dim_t dim) const {
    DASH_ASSERT_EQ(
      0, dim,
      "Wrong dimension for ShiftTilePattern<1>::local_extent. " <<
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
  IndexType local_extent(dim_t dim) const {
    DASH_ASSERT_EQ(
      0, dim,
      "Wrong dimension for ShiftTilePattern<1>::local_extent. " <<
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
      team_unit_t unit) const {
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.local_extents()", unit);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.local_extents >", _local_size);
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
   * Converts global index to its associated unit and respective local index.
   *
   * TODO: Unoptimized
   *
   * \see  DashPatternConcept
   */
  local_index_t local(
    IndexType g_index) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.local()", g_index);
    index_type  g_block_index = g_index / _blocksize;
    index_type  l_phase       = g_index % _blocksize;
    index_type  l_block_index = g_block_index / _nunits;
    team_unit_t unit(g_block_index % _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.local >", unit);
    index_type  l_index       = (l_block_index * _blocksize) + l_phase;
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.local >", l_index);
    return local_index_t { unit, l_index };
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
    auto g_index        = global_coords[0];
    auto elem_phase     = g_index % _blocksize;
    auto g_block_offset = g_index / _blocksize;
    auto l_block_offset = g_block_offset / _nunits;
    local_coord         = (l_block_offset * _blocksize) + elem_phase;
    return std::array<IndexType, 1> { local_coord };
  }

  /**
   * Converts global coordinates to their associated unit and their respective
   * local index.
   *
   * \see  DashPatternConcept
   */
  local_index_t local_index(
    const std::array<IndexType, NumDimensions> & g_coords) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.local_index()", g_coords);
    index_type  g_block_index = g_coords[0] / _blocksize;
    index_type  l_phase       = g_coords[0] % _blocksize;
    index_type  l_block_index = g_block_index / _nunits;
    team_unit_t unit(g_block_index % _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.local_index >", unit);
    // Global coords to local coords:
    index_type  l_index       = (l_block_index * _blocksize) + l_phase;
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.local_index >", l_index);
    return local_index_t { unit, l_index };
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
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.global()", unit);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.global()", local_coords);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.global", _nunits);
    if (_nunits < 2) {
      return local_coords;
    }
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.global", _nblocks);
    const Distribution & dist = _distspec[0];
    IndexType local_index     = local_coords[0];
    IndexType elem_phase      = local_index % _blocksize;
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.global", local_index);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.global", elem_phase);
    // Global coords of the element's block within all blocks:
    IndexType block_index     = dist.local_index_to_block_coord(
                                  static_cast<IndexType>(unit),
                                  local_index,
                                  _nunits,
                                  _nblocks,
                                  _blocksize
                                );
    IndexType glob_index      = (block_index * _blocksize) + elem_phase;
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.global", block_index);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.global >", glob_index);
    return std::array<IndexType, 1> {{ glob_index }};
  }

  /**
   * Converts local coordinates of active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> global(
    const std::array<IndexType, NumDimensions> & l_coords) const {
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
    IndexType l_index) const {
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
    IndexType l_index) const {
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
    const std::array<IndexType, NumDimensions> & l_coords) const {
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
    const std::array<IndexType, NumDimensions> & g_coords) const {
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
    const ViewSpec_t & viewspec) const {
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
  IndexType at(IndexType value, Values ... values) const {
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
    /// team-local id of the unit
    team_unit_t unit,
    /// Viewspec to apply
    const ViewSpec_t & viewspec) const {
    DASH_ASSERT_EQ(
      0, dim,
      "Wrong dimension for Pattern::has_local_elements. " <<
      "Expected dimension = 0, got " << dim);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.has_local_elements()", unit);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.has_local_elements()", viewspec);
    // Check if unit id lies in cartesian sub-space of team spec
    return _teamspec.includes_index(
              unit,
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
    team_unit_t unit) const {
    auto coords_unit = unit_at(index);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.is_local >", (coords_unit == unit));
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
    const std::array<index_type, 1> & g_coords) const
  {
    index_type block_idx = g_coords[0] / _blocksize;
    DASH_LOG_TRACE("ShiftTilePattern<1>.block_at",
                   "coords", g_coords,
                   "> block index", block_idx);
    return block_idx;
  }

  /**
   * View spec (offset and extents) of block at global linear block index in
   * cartesian element space.
   */
  ViewSpec_t block(
    index_type g_block_index) const
  {
    index_type offset = g_block_index * _size;
    std::array<index_type, NumDimensions> offsets {{ offset }};
    std::array<size_type, NumDimensions>  extents {{ _blocksize }};
    return ViewSpec_t(offsets, extents);
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   */
  ViewSpec_t local_block(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.local_block()", l_block_index);
    // Local block index to local block coords:
    auto l_elem_index = l_block_index * _blocksize;
    auto g_elem_index = global(l_elem_index);
    std::array<index_type, NumDimensions> offsets {{ g_elem_index }};
    std::array<size_type, NumDimensions>  extents {{ _blocksize }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.local_block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  ViewSpec_t local_block_local(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.local_block_local()", l_block_index);
    index_type offset = l_block_index * _blocksize;
    std::array<index_type, NumDimensions> offsets {{ offset }};
    std::array<size_type, NumDimensions>  extents {{ _blocksize }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.local_block_local >", block_vs);
    return block_vs;
  }

  /**
   * Cartesian arrangement of pattern blocks.
   */
  BlockSpec_t blockspec() const {
    BlockSpec_t bspec({{ dash::math::div_ceil(_size, _blocksize) }});
    return bspec;
  }

  /**
   * Cartesian arrangement of pattern blocks.
   */
  const BlockSpec_t & local_blockspec() const
  {
    return BlockSpec_t(_nlblocks);
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
    return SizeSpec_t(std::array<SizeType, 1> {{ _size }});
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
    return std::array<IndexType, 1> {{ index }};
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

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  SizeType initialize_blocksize(
    SizeType                   size,
    const DistributionSpec_t & distspec,
    SizeType                   nunits) const {
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_blocksize", nunits);
    if (nunits == 0) {
      return 0;
    }
    const Distribution & dist = distspec[0];
    return dist.max_blocksize_in_range(
             size,    // size of range (extent)
             nunits); // number of blocks (units)
  }

  /**
   * Initialize block spec from memory layout, team spec and distribution
   * spec.
   */
  SizeType initialize_num_blocks(
    SizeType                   size,
    SizeType                   blocksize,
    SizeType                   nunits) const {
    if (blocksize == 0) {
      return 0;
    }
    DASH_LOG_TRACE("ShiftTilePattern<1>.init_num_blocks()",
                   "size", size, "blocksize", blocksize, "nunits", nunits);
    SizeType n_blocks = dash::math::div_ceil(
                          size,
                          blocksize);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_blockspec", n_blocks);
    return n_blocks;
  }

  /**
   * Initialize local block spec from global block spec.
   */
  SizeType initialize_num_local_blocks(
    SizeType                    num_blocks,
    SizeType                    blocksize,
    const DistributionSpec_t  & distspec,
    SizeType                    nunits,
    SizeType                    local_size) const
  {
    auto num_l_blocks = local_size;
    if (blocksize > 0) {
      num_l_blocks = dash::math::div_ceil(
                       num_l_blocks,
                       blocksize);
    } else {
      num_l_blocks = 0;
    }
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_num_local_blocks", num_l_blocks);
    return num_l_blocks;
  }

  /**
   * Max. elements per unit (local capacity)
   */
  SizeType initialize_local_capacity() const
  {
    SizeType l_capacity = 1;
    if (_nunits == 0) {
      return 0;
    }
    auto max_l_blocks = dash::math::div_ceil(
                          _nblocks,
                          _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_lcapacity.d", _nunits);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_lcapacity.d", max_l_blocks);
    l_capacity = max_l_blocks * _blocksize;
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range()
  {
    auto l_size = _local_size;
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_local_range()", l_size);
    if (l_size == 0) {
      _lbegin = 0;
      _lend   = 0;
    } else {
      // First local index transformed to global index
      _lbegin = global(0);
      // Index past last local index transformed to global index
      _lend   = global(l_size - 1) + 1;
    }
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  SizeType initialize_local_extent(
    team_unit_t unit) const {
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_local_extent()", unit);
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_local_extent()", _nunits);
    if (_nunits == 0) {
      return 0;
    }
    // Coordinates of local unit id in team spec:
    SizeType l_extent     = 0;
    // Minimum number of blocks local to every unit in dimension:
    auto min_local_blocks = _nblocks / _nunits;
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_local_extent", _nblocks);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_local_extent", _blocksize);
    DASH_LOG_TRACE_VAR("ShiftTilePattern<1>.init_local_extent", min_local_blocks);
    l_extent = min_local_blocks * _blocksize;
    DASH_LOG_DEBUG_VAR("ShiftTilePattern<1>.init_local_extent >", l_extent);
    return l_extent;
  }
};

} // namespace dash

#endif // DOXYGEN

#endif // DASH__SHIFT_TILE_PATTERN_1D_H_
