#ifndef DASH__MOCK_PATTERN_H_
#define DASH__MOCK_PATTERN_H_

#include <functional>
#include <array>
#include <vector>
#include <type_traits>

#include <dash/Types.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>
#include <dash/Pattern.h>

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>

#include <dash/pattern/internal/PatternArguments.h>

namespace dash {

/**
 * Irregular Pattern for Compressed Sparse Row Storage.
 * 
 * \concept{DashPatternConcept}
 */
template<
  dim_t      NumDimensions,
  MemArrange Arrangement  = dash::ROW_MAJOR,
  typename   IndexType    = dash::default_index_t
>
class MockPattern;

/**
 * Irregular Pattern for Compressed Sparse Row Storage.
 * Specialization for 1-dimensional data.
 * 
 * \concept{DashPatternConcept}
 */
template<
  MemArrange Arrangement,
  typename   IndexType
>
class MockPattern<1, Arrangement, IndexType>
{
private:
  static const dim_t NumDimensions = 1;

public:
  static constexpr char const * PatternName = "MockPattern<1>";

public:
  /// Satisfiable properties in pattern property category Partitioning:
  typedef pattern_partitioning_properties<
              // Minimal number of blocks in every dimension, i.e. one block
              // per unit.
              pattern_partitioning_tag::minimal,
              // Block extents are constant for every dimension.
              pattern_partitioning_tag::rectangular,
              // Varying block sizes.
              pattern_partitioning_tag::unbalanced
          > partitioning_properties;
  /// Satisfiable properties in pattern property category Mapping:
  typedef pattern_mapping_properties<
              // Number of blocks assigned to a unit may differ.
              pattern_mapping_tag::balanced
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
  typedef MockPattern<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    LocalMemoryLayout_t;
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
    dash::team_unit_t                     unit;
    IndexType                             index;
  } local_index_t;
  typedef struct {
    dash::team_unit_t                     unit;
    std::array<index_type, NumDimensions> coords;
  } local_coords_t;

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

  /// Mock position, incremented in every call of local()
  mutable IndexType           _mock_idx        = 0;

public:
  /**
   * Constructor, initializes a pattern from an argument list consisting
   * of the pattern size (extent, number of elements) followed by an optional
   * distribution type.
   *
   */
  template<typename ... Args>
  MockPattern(
    /// Number of local elements for every unit in the active team.
    const std::vector<size_type> & local_sizes,
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
    _local_sizes(local_sizes),
    _block_offsets(initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
    _distspec(_arguments.distspec()), 
    _team(&_arguments.team()),
    _teamspec(_arguments.teamspec()), 
    _nunits(_team->size()),
    _blocksize(initialize_blocksize(
        _size,
        _distspec,
        _nunits)),
    _nblocks(_nunits),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(initialize_local_capacity())
  {
    DASH_LOG_TRACE("MockPattern()", "Constructor with argument list");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("MockPattern()", "MockPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   */
  MockPattern(
    /// Number of local elements for every unit in the active team.
    const std::vector<size_type>          & local_sizes,
    /// Cartesian arrangement of units within the team
    const TeamSpec_t                      & teamspec,
    /// Team containing units to which this pattern maps its elements
    dash::Team                            & team     = dash::Team::All()) 
  : _size(initialize_size(
        local_sizes)),
    _local_sizes(local_sizes),
    _block_offsets(initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
    _distspec(DistributionSpec_t()),
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
    _nblocks(_nunits),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(initialize_local_capacity())
  {
    DASH_LOG_TRACE("MockPattern()", "(sizespec, dist, teamspec, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("MockPattern()", "MockPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   */
  MockPattern(
    /// Number of local elements for every unit in the active team.
    const std::vector<size_type> & local_sizes,
    /// Team containing units to which this pattern maps its elements
    Team                         & team = dash::Team::All())
  : _size(initialize_size(
        local_sizes)),
    _local_sizes(local_sizes),
    _block_offsets(initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
    _distspec(DistributionSpec_t()),
    _team(&team),
    _teamspec(_distspec, *_team),
    _nunits(_team->size()),
    _blocksize(initialize_blocksize(
        _size,
        _distspec,
        _nunits)),
    _nblocks(_nunits),
    _local_size(
        initialize_local_extent(_team->myid())),
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(initialize_local_capacity())
  {
    DASH_LOG_TRACE("MockPattern()", "(sizespec, dist, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("MockPattern()", "MockPattern initialized");
  }

  /**
   * Copy constructor.
   */
  MockPattern(const self_t & other)
  : _size(other._size),
    _local_sizes(other._local_sizes),
    _block_offsets(other._block_offsets),
    _memory_layout(other._memory_layout),
    _distspec(other._distspec), 
    _team(other._team),
    _teamspec(other._teamspec),
    _nunits(other._nunits),
    _blocksize(other._blocksize),
    _nblocks(other._nblocks),
    _local_size(other._local_size),
    _local_memory_layout(other._local_memory_layout),
    _local_capacity(other._local_capacity),
    _lbegin(other._lbegin),
    _lend(other._lend) {
    // No need to copy _arguments as it is just used to
    // initialize other members.
    DASH_LOG_TRACE("MockPattern(other)", "MockPattern copied");
  }

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  MockPattern(self_t & other)
  : MockPattern(static_cast<const self_t &>(other)) {
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
      _size        == other._size &&
      _local_sizes == other._local_sizes &&
      _distspec    == other._distspec &&
      _teamspec    == other._teamspec &&
      _nblocks     == other._nblocks &&
      _blocksize   == other._blocksize &&
      _nunits      == other._nunits
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
    DASH_LOG_TRACE("MockPattern.=(other)");
    if (this != &other) {
      _size                = other._size;
      _local_sizes         = other._local_sizes;
      _block_offsets       = other._block_offsets;
      _memory_layout       = other._memory_layout;
      _distspec            = other._distspec;
      _team                = other._team;
      _teamspec            = other._teamspec;
      _local_size          = other._local_size;
      _local_memory_layout = other._local_memory_layout;
      _blocksize           = other._blocksize;
      _nblocks             = other._nblocks;
      _local_capacity      = other._local_capacity;
      _nunits              = other._nunits;
      _lbegin              = other._lbegin;
      _lend                = other._lend;
      DASH_LOG_TRACE("MockPattern.=(other)", "MockPattern assigned");
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
  dash::team_unit_t unit_at(
    /// Absolute coordinates of the point
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    DASH_LOG_TRACE_VAR("MockPattern.unit_at()", coords);
    // Apply viewspec offsets to coordinates:
    dash::team_unit_t unit_id{((coords[0] + viewspec[0].offset) / _blocksize)
                          % _nunits};
    DASH_LOG_TRACE_VAR("MockPattern.unit_at >", unit_id);
    return unit_id;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dash::team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & g_coords) const {
    DASH_LOG_TRACE_VAR("MockPattern.unit_at()", g_coords);
    dash::team_unit_t unit_idx{0};
    auto g_coord         = g_coords[0];
    for (; unit_idx < _nunits - 1; ++unit_idx) {
      if (_block_offsets[unit_idx+1] >= g_coord) {
        DASH_LOG_TRACE_VAR("MockPattern.unit_at >", unit_idx);
        return unit_idx;
      }
    }
    DASH_LOG_TRACE_VAR("MockPattern.unit_at >", _nunits-1);
    return dash::team_unit_t(_nunits-1);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dash::team_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec) const {
    DASH_LOG_TRACE_VAR("MockPattern.unit_at()", global_pos);
    DASH_LOG_TRACE_VAR("MockPattern.unit_at()", viewspec);
    dash::team_unit_t unit_idx{0};
    // Apply viewspec offsets to coordinates:
    auto g_coord         = global_pos + viewspec[0].offset;
    for (; unit_idx < _nunits - 1; ++unit_idx) {
      if (_block_offsets[unit_idx+1] >= g_coord) {
        DASH_LOG_TRACE_VAR("MockPattern.unit_at >", unit_idx);
        return unit_idx;
      }
    }
    DASH_LOG_TRACE_VAR("MockPattern.unit_at >", _nunits-1);
    return dash::team_unit_t(_nunits-1);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  dash::team_unit_t unit_at(
    /// Global linear element offset
    IndexType g_index) const {
    DASH_LOG_TRACE_VAR("MockPattern.unit_at()", g_index);
    dash::team_unit_t unit_idx{0};
    for (; unit_idx < _nunits - 1; ++unit_idx) {
      if (_block_offsets[unit_idx+1] >= g_index) {
        DASH_LOG_TRACE_VAR("MockPattern.unit_at >", unit_idx);
        return unit_idx;
      }
    }
    DASH_LOG_TRACE_VAR("MockPattern.unit_at >", _nunits-1);
    return dash::team_unit_t(_nunits-1);
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
  IndexType local_extent(dim_t dim) const {
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
      dash::team_unit_t unit) const {
    DASH_LOG_DEBUG_VAR("MockPattern.local_extents()", unit);
    DASH_LOG_DEBUG_VAR("MockPattern.local_extents >", _local_size);
    return std::array<SizeType, 1> {{ _local_size }};
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
   * NOTE: Same as \c local_index.
   *
   * \see  DashPatternConcept
   */
  inline local_coords_t local(
    const std::array<IndexType, NumDimensions> & g_coords) const noexcept {
    DASH_LOG_TRACE_VAR("MockPattern.local()", g_coords);
    IndexType g_index = g_coords[0];
    local_index_t l_index;
    l_index.unit  = g_index / _nunits;
    if (_mock_idx == _local_size) { _mock_idx = 0; }
    l_index.index = _mock_idx;
    ++_mock_idx;
    return l_index;
  }

  /**
   * Converts global index to its associated unit and respective local index.
   *
   * NOTE: Same as \c local_index.
   *
   * \see  DashPatternConcept
   */
  inline local_index_t local(
    IndexType g_index) const noexcept {
    DASH_LOG_TRACE_VAR("MockPattern.local()", g_index);
    local_index_t l_index;
    l_index.unit  = g_index / _nunits;
    if (_mock_idx == _local_size) { _mock_idx = 0; }
    l_index.index = _mock_idx;
    ++_mock_idx;
    return l_index;
  }

  /**
   * Converts global coordinates to their associated unit's respective 
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  std::array<IndexType, NumDimensions> local_coords(
    const std::array<IndexType, NumDimensions> & g_coords) const {
    DASH_LOG_TRACE_VAR("MockPattern.local_coords()", g_coords);
    IndexType g_index = g_coords[0];
    for (auto unit_idx = _nunits-1; unit_idx >= 0; --unit_idx) {
      index_type block_offset = _block_offsets[unit_idx];
      if (block_offset <= g_index) {
        auto l_coord = g_index - block_offset;
        DASH_LOG_TRACE_VAR("MockPattern.local_coords >", l_coord);
        return std::array<IndexType, 1> {{ l_coord }};
      }
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "MockPattern.local_coords: global index " << g_index <<
      " is out of bounds");
  }

  /**
   * Converts global coordinates to their associated unit and their respective 
   * local index.
   * 
   * \see  DashPatternConcept
   */
  local_index_t local_index(
    const std::array<IndexType, NumDimensions> & g_coords) const {
    IndexType g_index = g_coords[0];
    DASH_LOG_TRACE_VAR("MockPattern.local_index()", g_coords);
    local_index_t l_index;
    for (auto unit_idx = _nunits-1; unit_idx >= 0; --unit_idx) {
      index_type block_offset = _block_offsets[unit_idx];
      if (block_offset <= g_index) {
        l_index.unit  = unit_idx;
        l_index.index = g_index - block_offset;
        DASH_LOG_TRACE_VAR("MockPattern.local >", l_index.unit);
        DASH_LOG_TRACE_VAR("MockPattern.local >", l_index.index);
        return l_index;
      }
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "MockPattern.local: global index " << g_index < " is out of bounds");
  }

  ////////////////////////////////////////////////////////////////////////////
  /// global
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  inline std::array<IndexType, NumDimensions> global(
    dash::team_unit_t unit,
    const std::array<IndexType, NumDimensions> & l_coords) const noexcept {
    DASH_LOG_DEBUG_VAR("MockPattern.global()", unit);
    DASH_LOG_DEBUG_VAR("MockPattern.global()", l_coords);
    DASH_LOG_TRACE_VAR("MockPattern.global", _nunits);
    if (_nunits < 2) {
      return l_coords;
    }
    // Initialize global index with element phase (= l coords):
    index_type glob_index = _block_offsets[unit] + l_coords[0];
    DASH_LOG_TRACE_VAR("MockPattern.global >", glob_index);
    return std::array<IndexType, 1> {{ glob_index }};
  }

  /**
   * Converts local coordinates of active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  inline std::array<IndexType, NumDimensions> global(
    const std::array<IndexType, NumDimensions> & l_coords) const noexcept {
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
  inline IndexType global(
    dash::team_unit_t unit,
    IndexType l_index) const noexcept {
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
  inline IndexType global(
    IndexType l_index) const noexcept {
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
    dash::team_unit_t unit,
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
  inline IndexType at(
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
  inline IndexType at(
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
  inline IndexType at(IndexType value, Values ... values) const noexcept {
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
    dash::team_unit_t unit,
    /// Viewspec to apply
    const ViewSpec_t & viewspec) const {
    DASH_ASSERT_EQ(
      0, dim, 
      "Wrong dimension for Pattern::has_local_elements. " <<
      "Expected dimension = 0, got " << dim);
    DASH_LOG_TRACE_VAR("MockPattern.has_local_elements()", dim_offset);
    DASH_LOG_TRACE_VAR("MockPattern.has_local_elements()", unit);
    DASH_LOG_TRACE_VAR("MockPattern.has_local_elements()", viewspec);
    DASH_THROW(
      dash::exception::NotImplemented,
      "MockPattern.has_local_elements is not implemented");
  }

  /**
   * Whether the given global index is local to the specified unit.
   *
   * \see  DashPatternConcept
   */
  inline bool is_local(
    IndexType index,
    dash::team_unit_t unit) const noexcept {
    DASH_LOG_TRACE_VAR("MockPattern.is_local()", index);
    DASH_LOG_TRACE_VAR("MockPattern.is_local()", unit);
    bool is_loc = index >= _block_offsets[unit] &&
                  (unit == _nunits-1 ||
                   index <  _block_offsets[unit+1]);
    DASH_LOG_TRACE_VAR("MockPattern.is_local >", is_loc);
    return is_loc;
  }

  /**
   * Whether the given global index is local to the unit that created
   * this pattern instance.
   *
   * \see  DashPatternConcept
   */
  inline bool is_local(
    IndexType index) const noexcept {
    auto unit = team().myid();
    DASH_LOG_TRACE_VAR("MockPattern.is_local()", index);
    DASH_LOG_TRACE_VAR("MockPattern.is_local", unit);
    bool is_loc = index >= _block_offsets[unit] &&
                  (unit == _nunits-1 ||
                   index <  _block_offsets[unit+1]);
    DASH_LOG_TRACE_VAR("MockPattern.is_local >", is_loc);
    return is_loc;
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
    dim_t dimension) const noexcept {
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
  SizeType max_blocksize() const noexcept {
    return _blocksize;
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr SizeType local_capacity() const noexcept {
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
  constexpr SizeType local_size() const noexcept {
    return _local_size;
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType num_units() const noexcept {
    return _nunits;
  }

  /**
   * The maximum number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType capacity() const noexcept {
    return _size;
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType size() const noexcept {
    return _size;
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  constexpr dash::Team & team() const noexcept {
    return *_team;
  }

  /**
   * Distribution specification of this pattern.
   */
  const DistributionSpec_t & distspec() const noexcept {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  SizeSpec_t sizespec() const noexcept {
    return SizeSpec_t(std::array<SizeType, 1> {{ _size }});
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  const std::array<SizeType, NumDimensions> & extents() const noexcept {
    return std::array<SizeType, 1> {{ _size }};
  }

  /**
   * Cartesian index space representing the underlying memory model of the
   * pattern.
   *
   * \see DashPatternConcept
   */
  const MemoryLayout_t & memory_layout() const noexcept {
    return _memory_layout;
  }

  /**
   * Cartesian index space representing the underlying local memory model
   * of this pattern for the calling unit.
   * Not part of DASH Pattern concept.
   */
  const LocalMemoryLayout_t & local_memory_layout() const noexcept {
    return _local_memory_layout;
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   *
   * \see DashPatternConcept
   */
  const TeamSpec_t & teamspec() const noexcept {
    return _teamspec;
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates.
   *
   * \see DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType index) const noexcept {
    return std::array<IndexType, 1> {{ index }};
  }

  /**
   * View spec (offset and extents) of block at global linear block index in
   * cartesian element space.
   */
  ViewSpec_t block(
    index_type g_block_index) const noexcept {
    index_type offset = _block_offsets[g_block_index];
    auto blocksize    = _local_sizes[g_block_index];
    return ViewSpec_t(offset, blocksize);
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   */
  ViewSpec_t local_block(
    index_type l_block_index) const noexcept {
    DASH_LOG_DEBUG_VAR("MockPattern.local_block()", l_block_index);
    DASH_ASSERT_EQ(
      0, l_block_index,
      "MockPattern always assigns exactly 1 block to a single unit");
    index_type block_offset = _block_offsets[_team->myid()];
    size_type  block_size   = _local_sizes[_team->myid()];
    ViewSpec_t block_vs({ block_offset }, { block_size });
    DASH_LOG_DEBUG_VAR("MockPattern.local_block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  ViewSpec_t local_block_local(
    index_type local_block_index) const noexcept {
    size_type block_size = _local_sizes[_team->myid()];
    return ViewSpec_t({ 0 }, { block_size });
  }

  /**
   * Memory order followed by the pattern.
   */
  constexpr static MemArrange memory_order() noexcept {
    return Arrangement;
  }

  /**
   * Number of dimensions of the cartesian space partitioned by the pattern.
   */
  constexpr static dim_t ndim() noexcept {
    return 1;
  }

  /**
   * Initialize the size (number of mapped elements) of the Pattern.
   */
  SizeType initialize_size(
    const std::vector<size_type> & local_sizes) const noexcept {
    DASH_LOG_TRACE_VAR("MockPattern.init_size()", local_sizes);
    size_type size = 0;
    for (size_type unit_idx = 0; unit_idx < local_sizes.size(); ++unit_idx) {
      size += local_sizes[unit_idx];
    }
    DASH_LOG_TRACE_VAR("MockPattern.init_size >", size);
    return size;
  }

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  std::vector<size_type> initialize_block_offsets(
    const std::vector<size_type> & local_sizes) const noexcept {
    DASH_LOG_TRACE_VAR("MockPattern.init_block_offsets", local_sizes);
    std::vector<size_type> block_offsets;
    // NOTE: Assuming 1 block for every unit.
    block_offsets.push_back(0);
    for (auto unit_idx = 0; unit_idx < local_sizes.size() - 1; ++unit_idx) {
      auto block_offset = block_offsets[unit_idx] +
                          local_sizes[unit_idx];
      block_offsets.push_back(block_offset);
    }
    return block_offsets;
  }

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  SizeType initialize_blocksize(
    SizeType                   size,
    const DistributionSpec_t & distspec,
    SizeType                   nunits) const noexcept {
    DASH_LOG_TRACE_VAR("MockPattern.init_blocksize", nunits);
    if (nunits == 0) {
      return 0;
    }
    // NOTE: Assuming 1 block for every unit.
    return 1;
  }

  /**
   * Initialize local block spec from global block spec.
   */
  SizeType initialize_num_local_blocks(
    SizeType                   num_blocks,
    SizeType                   blocksize,
    const DistributionSpec_t & distspec,
    SizeType                   nunits,
    SizeType                   local_size) const {
    auto num_l_blocks = local_size;
    if (blocksize > 0) {
      num_l_blocks = dash::math::div_ceil(
                       num_l_blocks,
                       blocksize);
    } else {
      num_l_blocks = 0;
    }
    DASH_LOG_TRACE_VAR("MockPattern.init_num_local_blocks", num_l_blocks);
    return num_l_blocks;
  }

  /**
   * Max. elements per unit (local capacity)
   */
  SizeType initialize_local_capacity() const {
    SizeType l_capacity = 0;
    if (_nunits == 0) {
      return 0;
    }
    DASH_LOG_TRACE_VAR("MockPattern.init_lcapacity", _nunits);
    // Local capacity is maximum number of elements assigned to a single unit,
    // i.e. the maximum local size:
    l_capacity = *(std::max_element(_local_sizes.begin(), _local_sizes.end()));
    DASH_LOG_DEBUG_VAR("MockPattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range() {
    auto l_size = _local_size; 
    DASH_LOG_DEBUG_VAR("MockPattern.init_local_range()", l_size);
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
    DASH_LOG_DEBUG_VAR("MockPattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("MockPattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  SizeType initialize_local_extent(
    dash::team_unit_t unit) const {
    DASH_LOG_DEBUG_VAR("MockPattern.init_local_extent()", unit);
    DASH_LOG_DEBUG_VAR("MockPattern.init_local_extent()", _nunits);
    if (_nunits == 0) {
      return 0;
    }
    // Local size of given unit:
    SizeType l_extent = _local_sizes[unit];
    DASH_LOG_DEBUG_VAR("MockPattern.init_local_extent >", l_extent);
    return l_extent;
  }

};

} // namespace dash

#endif // DASH__MOCK_PATTERN_H_
