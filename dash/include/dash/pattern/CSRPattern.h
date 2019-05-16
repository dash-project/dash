#ifndef DASH__CSR_PATTERN_1D_H_
#define DASH__CSR_PATTERN_1D_H_

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

#include <functional>
#include <array>
#include <utility>
#include <vector>
#include <type_traits>


namespace dash {

#ifndef DOXYGEN

/**
 * Irregular Pattern for Compressed Sparse Row Storage.
 *
 * \concept{DashPatternConcept}
 */
template <
  dim_t      NumDimensions,
  MemArrange Arrangement  = dash::ROW_MAJOR,
  typename   IndexType    = dash::default_index_t
>
class CSRPattern;

#endif // DOXYGEN

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
class CSRPattern<1, Arrangement, IndexType>
{
private:
  static const dim_t NumDimensions = 1;

public:
  static constexpr char const * PatternName = "CSRPattern1D";

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
              // Elements are contiguous in local memory within single block.
              pattern_layout_tag::blocked,
              // Local element order corresponds to a logical linearization
              // within single blocks.
              pattern_layout_tag::linear
          > layout_properties;

private:
  /// Fully specified type definition of self
  typedef CSRPattern<NumDimensions, Arrangement, IndexType>
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

public:
  typedef IndexType   index_type;
  typedef SizeType    size_type;
  typedef ViewSpec_t  viewspec_type;
  typedef struct {
    team_unit_t                            unit;
    IndexType                             index{};
  } local_index_t;
  typedef struct {
    team_unit_t                            unit;
    std::array<index_type, NumDimensions> coords;
  } local_coords_t;

private:
  /// Extent of the linear pattern.
  SizeType                    _size            = 0;
  /// Number of local elements for every unit in the active team.
  std::vector<size_type>      _local_sizes;
  /// Block offsets for every unit. Prefix sum of local sizes.
  std::vector<size_type>      _block_offsets;
  /// Global memory layout of the pattern.
  MemoryLayout_t              _memory_layout;
  /// Number of blocks in all dimensions
  BlockSpec_t                 _blockspec;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC or NONE) of
  /// all dimensions. Defaults to BLOCKED.
  DistributionSpec_t          _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team *                _team            = nullptr;
  /// Cartesian arrangement of units within the team
  TeamSpec_t                  _teamspec;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// Actual number of local elements of the active unit.
  SizeType                    _local_size      = 0;
  /// Local memory layout of the pattern.
  LocalMemoryLayout_t         _local_memory_layout;
  /// Maximum number of elements assigned to a single unit
  SizeType                    _local_capacity  = 0;
  /// Corresponding global index to first local index of the active unit
  IndexType                   _lbegin          = 0;
  /// Corresponding global index past last local index of the active unit
  IndexType                   _lend            = 0;

public:
  /**
   * Constructor, initializes a pattern from an argument list consisting
   * of the pattern size (extent, number of elements) followed by an optional
   * distribution type.
   *
   */
  template<typename ... Args>
  CSRPattern(
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    SizeType arg,
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    Args && ... args)
  : CSRPattern(PatternArguments_t(arg, args...))
  {
    DASH_LOG_TRACE("CSRPattern()", "Constructor with argument list");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("CSRPattern()", "CSRPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and \c Team.
   *
   */
  CSRPattern(
    /// Size spec of the pattern.
    const SizeSpec_t         & sizespec,
    /// Distribution spec.
    const DistributionSpec_t & distspec,
    /// Cartesian arrangement of units within the team
    const TeamSpec_t         & teamspec,
    /// Team containing units to which this pattern maps its elements.
    Team                     & team = dash::Team::All())
  : _size(sizespec.size()),
    _local_sizes(initialize_local_sizes(
        _size,
        distspec,
        team)),
    _block_offsets(initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
    _blockspec(initialize_blockspec(
        _local_sizes)),
    _distspec(DistributionSpec_t()),
    _team(&team),
    _teamspec(
      teamspec,
      _distspec,
      *_team),
    _nunits(_team->size()),
    _local_size(
        initialize_local_extent(
          _team->myid(),
          _local_sizes)),
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(initialize_local_capacity(_local_sizes))
  {
    DASH_LOG_TRACE("CSRPattern()", "(sizespec, dist, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("CSRPattern()", "CSRPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec and \c Team.
   *
   */
  CSRPattern(
    /// Size spec of the pattern.
    const SizeSpec_t         & sizespec,
    /// Distribution spec.
    const DistributionSpec_t & distspec,
    /// Team containing units to which this pattern maps its elements.
    Team                     & team = dash::Team::All())
  : _size(sizespec.size()),
    _local_sizes(initialize_local_sizes(
        _size,
        distspec,
        team)),
    _block_offsets(initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
    _blockspec(initialize_blockspec(
        _local_sizes)),
    _distspec(DistributionSpec_t()),
    _team(&team),
    _teamspec(_distspec, *_team),
    _nunits(_team->size()),
    _local_size(
        initialize_local_extent(
          _team->myid(),
          _local_sizes)),
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(initialize_local_capacity(_local_sizes))
  {
    DASH_LOG_TRACE("CSRPattern()", "(sizespec, dist, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("CSRPattern()", "CSRPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from an argument list consisting
   * of the pattern size (extent, number of elements) followed by an optional
   * distribution type.
   *
   */
  template<typename ... Args>
  CSRPattern(
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
  : CSRPattern(local_sizes, PatternArguments_t(arg, args...))
  {
    DASH_LOG_TRACE("CSRPattern()", "Constructor with argument list");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("CSRPattern()", "CSRPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   */
  CSRPattern(
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
    _blockspec(initialize_blockspec(
        _local_sizes)),
    _distspec(DistributionSpec_t()),
    _team(&team),
    _teamspec(
        teamspec,
        _distspec,
        *_team),
    _nunits(_team->size()),
    _local_size(
        initialize_local_extent(
          _team->myid(),
          _local_sizes)),
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(initialize_local_capacity(_local_sizes))
  {
    DASH_LOG_TRACE("CSRPattern()", "(sizespec, dist, teamspec, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("CSRPattern()", "CSRPattern initialized");
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   */
  CSRPattern(
    /// Number of local elements for every unit in the active team.
    const std::vector<size_type>  & local_sizes,
    /// Team containing units to which this pattern maps its elements
    Team                          & team = dash::Team::All())
  : _size(
      initialize_size(
        local_sizes)),
    _local_sizes(local_sizes),
    _block_offsets(
      initialize_block_offsets(
        _local_sizes)),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
    _blockspec(
      initialize_blockspec(
        _local_sizes)),
    _distspec(DistributionSpec_t()),
    _team(&team),
    _teamspec(_distspec, *_team),
    _nunits(_team->size()),
    _local_size(
        initialize_local_extent(
          _team->myid(),
          _local_sizes)),
    _local_memory_layout(
      std::array<SizeType, 1> {{ _local_size }}),
    _local_capacity(
      initialize_local_capacity(_local_sizes))
  {
    DASH_LOG_TRACE("CSRPattern()", "(sizespec, dist, team)");
    DASH_ASSERT_EQ(
      _local_sizes.size(), _nunits,
      "Number of given local sizes "   << _local_sizes.size() << " " <<
      "does not match number of units" << _nunits);
    initialize_local_range();
    DASH_LOG_TRACE("CSRPattern()", "CSRPattern initialized");
  }

  /**
   * Copy constructor.
   */
  CSRPattern(const self_t & other) = default;

  /**
   * Move constructor.
   */
  CSRPattern(self_t && other)      = default;

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  CSRPattern(self_t & other)
  : CSRPattern(static_cast<const self_t &>(other))
  { }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other)      = default;

  /**
   * Equality comparison operator.
   */
  constexpr bool operator==(
    /// Pattern instance to compare for equality
    const self_t & other) const noexcept
  {
    return (this == &other) ||
           ( // no need to compare all members as most are derived from
             // constructor arguments.
             _size        == other._size &&
             _local_sizes == other._local_sizes &&
             _distspec    == other._distspec &&
             _teamspec    == other._teamspec &&
             _nunits      == other._nunits
           );
  }

  /**
   * Inquality comparison operator.
   */
  constexpr bool operator!=(
    /// Pattern instance to compare for inequality
    const self_t & other) const noexcept
  {
    return !(*this == other);
  }

  /**
   * Resolves the global index of the first local element in the pattern.
   *
   * \see DashPatternConcept
   */
  constexpr IndexType lbegin() const noexcept
  {
    return _lbegin;
  }

  /**
   * Resolves the global index past the last local element in the pattern.
   *
   * \see DashPatternConcept
   */
  constexpr IndexType lend() const noexcept
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
  constexpr team_unit_t unit_at(
    /// Absolute coordinates of the point
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t                           & viewspec) const
  {
    return unit_at(coords[0] + viewspec.offset(0));
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  constexpr team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    return unit_at(g_coords[0]);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  constexpr team_unit_t unit_at(
    /// Global linear element offset
    IndexType          global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec) const
  {
    return unit_at(global_pos + viewspec.offset(0));
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
    DASH_LOG_TRACE_VAR("CSRPattern.unit_at()", g_index);

    for (team_unit_t unit_idx{0}; unit_idx < _nunits; ++unit_idx) {
      if (g_index < _local_sizes[unit_idx]) {
        DASH_LOG_TRACE_VAR("CSRPattern.unit_at >", unit_idx);
        return unit_idx;
      }
      g_index -= _local_sizes[unit_idx];
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "CSRPattern.unit_at: " <<
      "global index " << g_index << " is out of bounds");
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
  inline IndexType extent(dim_t dim) const
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
  inline IndexType local_extent(dim_t dim) const
  {
    DASH_ASSERT_EQ(
      0, dim,
      "Wrong dimension for Pattern::local_extent. " <<
      "Expected dimension = 0, got " << dim);
    return _local_size;
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit, by dimension.
   *
   * \see  local_extent()
   * \see  blocksize()
   * \see  local_size()
   * \see  extent()
   *
   * \see  DashPatternConcept
   */
  constexpr std::array<SizeType, NumDimensions>
  local_extents() const noexcept
  {
    return std::array<SizeType, 1> {{ _local_sizes[_team->myid()] }};
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
      team_unit_t unit) const noexcept
  {
    return std::array<SizeType, 1> {{ _local_sizes[unit] }};
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
  constexpr IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const noexcept
  {
    return local_coords[0] + viewspec.offset(0);
  }

  /**
   * Convert given local coordinates to linear local offset (index).
   *
   * \see DashPatternConcept
   */
  constexpr IndexType local_at(
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
  inline local_coords_t local(
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
  inline local_index_t local(
    IndexType g_index) const
  {
    DASH_LOG_TRACE_VAR("CSRPattern.local()", g_index);
    local_index_t l_index;

    for (team_unit_t unit_idx{0}; unit_idx < _nunits; ++unit_idx) {
      if (g_index < _local_sizes[unit_idx]) {
        l_index.unit  = unit_idx;
        l_index.index = g_index;
        DASH_LOG_TRACE("CSRPattern.local >",
                       "unit:",  l_index.unit,
                       "index:", l_index.index);
        return l_index;
      }
      g_index -= _local_sizes[unit_idx];
    }
    DASH_THROW(
      dash::exception::InvalidArgument,
      "CSRPattern.local: " <<
      "global index " << g_index << " is out of bounds");
  }

  /**
   * Converts global coordinates to their associated unit's respective
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> local_coords(
    const std::array<IndexType, NumDimensions> & g_coords) const
  {
    return std::array<IndexType, 1> {{ local(g_coords[0]).index }};
  }

  /**
   * Converts global coordinates to their associated unit and their respective
   * local index.
   *
   * \see  DashPatternConcept
   */
  constexpr local_index_t local_index(
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
  inline std::array<IndexType, NumDimensions> global(
      team_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const
  {
    DASH_LOG_DEBUG_VAR("CSRPattern.global()", unit);
    DASH_LOG_DEBUG_VAR("CSRPattern.global()", local_coords);
    DASH_LOG_TRACE_VAR("CSRPattern.global", _nunits);
    if (_nunits < 2) {
      return local_coords;
    }
    // Initialize global index with element phase (= local coords):
    index_type glob_index = _block_offsets[unit] + local_coords[0];
    DASH_LOG_TRACE_VAR("CSRPattern.global >", glob_index);
    return std::array<IndexType, 1> {{ glob_index }};
  }

  /**
   * Converts local coordinates of active unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> global(
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
  constexpr IndexType global(
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
  constexpr IndexType global(
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
  constexpr IndexType global_index(
    team_unit_t unit,
    const std::array<IndexType, NumDimensions> & l_coords) const
  {
    return global(unit, l_coords[0]);
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
  constexpr IndexType at(
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
  inline IndexType at(
    const std::array<IndexType, NumDimensions> & g_coords,
    const ViewSpec_t & viewspec) const
  {
    auto vs_coords = g_coords;
    vs_coords[0] += viewspec.offset(0);
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
  constexpr IndexType at(IndexType value, Values ... values) const
  {
    static_assert(
      sizeof...(values) == NumDimensions-1,
      "Wrong parameter number");
    return at(std::array<IndexType, NumDimensions> {{
             value, (IndexType)values...
           }});
  }

  ////////////////////////////////////////////////////////////////////////
  /// is_local
  ////////////////////////////////////////////////////////////////////////

  /**
   * Whether the given global index is local to the specified unit.
   *
   * \see  DashPatternConcept
   */
  constexpr bool is_local(
    IndexType index,
    team_unit_t unit) const noexcept
  {
    return (index >= _block_offsets[unit])
           && ( unit == _nunits-1
                || index <  _block_offsets[unit+1]
              );
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
    bool is_loc = index >= _block_offsets[unit] &&
                  (unit == _nunits-1 ||
                   index <  _block_offsets[unit+1]);
    return is_loc;
  }

  ////////////////////////////////////////////////////////////////////////
  /// block
  ////////////////////////////////////////////////////////////////////////

  /**
   * Cartesian arrangement of pattern blocks.
   */
  constexpr const BlockSpec_t & blockspec() const noexcept
  {
    return _blockspec;
  }

  /**
   * Index of block at given global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr index_type block_at(
    /// Global coordinates of element
    const std::array<index_type, NumDimensions> & g_coords) const
  {
    return static_cast<index_type>(unit_at(g_coords[0]));
  }

  /**
   * View spec (offset and extents) of block at global linear block index
   * in cartesian element space.
   */
  ViewSpec_t block(
    index_type g_block_index) const
  {
    DASH_LOG_DEBUG_VAR("CSRPattern<1>.block >", g_block_index);
    index_type offset = _block_offsets[g_block_index];
    auto block_size   = _local_sizes[g_block_index];
    std::array<index_type, NumDimensions> offsets = {{ offset }};
    std::array<size_type, NumDimensions>  extents = {{ block_size }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("CSRPattern<1>.block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index
   * in global cartesian element space.
   */
  ViewSpec_t local_block(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("CSRPattern<1>.local_block()", l_block_index);
    DASH_ASSERT_EQ(
      0, l_block_index,
      "CSRPattern always assigns exactly 1 block to a single unit");
    index_type block_offset = _block_offsets[_team->myid()];
    size_type  block_size   = _local_sizes[_team->myid()];
    std::array<index_type, NumDimensions> offsets = {{ block_offset }};
    std::array<size_type, NumDimensions>  extents = {{ block_size }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("CSRPattern<1>.local_block >", block_vs);
    return block_vs;
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  inline ViewSpec_t local_block_local(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("CSRPattern<1>.local_block_local >", l_block_index);
    size_type block_size = _local_sizes[_team->myid()];
    std::array<index_type, NumDimensions> offsets = {{ 0 }};
    std::array<size_type, NumDimensions>  extents = {{ block_size }};
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("CSRPattern<1>.local_block_local >", block_vs);
    return block_vs;
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
    dim_t dimension) const noexcept
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
  constexpr SizeType max_blocksize() const noexcept
  {
    return _local_capacity;
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr SizeType local_capacity() const noexcept
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

  constexpr SizeType local_size() const noexcept
  {
    return _local_size;
  }

  constexpr SizeType local_size(team_unit_t unit) const noexcept
  {
    return _local_sizes[unit.id];
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType num_units() const noexcept
  {
    return _nunits;
  }

  /**
   * The maximum number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType capacity() const noexcept
  {
    return _size;
  }

  /**
   * The number of elements arranged in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr IndexType size() const noexcept
  {
    return _size;
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  constexpr dash::Team & team() const noexcept
  {
    return *_team;
  }

  /**
   * Distribution specification of this pattern.
   */
  constexpr const DistributionSpec_t & distspec() const noexcept
  {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  constexpr SizeSpec_t sizespec() const noexcept
  {
    return SizeSpec_t(std::array<SizeType, 1> {{ _size }});
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  constexpr const std::array<SizeType, NumDimensions>
    extents() const noexcept
  {
    return std::array<SizeType, 1> {{ _size }};
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   *
   * \see DashPatternConcept
   */
  constexpr const TeamSpec_t & teamspec() const noexcept
  {
    return _teamspec;
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates.
   *
   * \see DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> coords(
    IndexType index) const noexcept
  {
    return std::array<IndexType, 1> {{ index }};
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates using viewspec.
   *
   * \see DashPatternConcept
   */
  std::array<IndexType, NumDimensions> coords(
    IndexType          index,
    const ViewSpec_t & viewspec) const {
    return std::array<IndexType, 1> {{ index + viewspec.offset(0) }};
  }


  /**
   * Memory order followed by the pattern.
   */
  constexpr static MemArrange memory_order()
  {
    return Arrangement;
  }

  /**
   * Number of dimensions of the cartesian space partitioned by the
   * pattern.
   */
  constexpr static dim_t ndim()
  {
    return 1;
  }

private:

  CSRPattern(
    const PatternArguments_t & arguments)
  : CSRPattern(
      initialize_local_sizes(
        arguments.sizespec().size(),
        arguments.distspec(),
        arguments.team()),
      arguments)
  {}

  CSRPattern(
      std::vector<size_type> local_sizes, const PatternArguments_t &arguments)
    : _size(arguments.sizespec().size())
    , _local_sizes(std::move(local_sizes))
    , _block_offsets(initialize_block_offsets(_local_sizes))
    , _memory_layout(std::array<SizeType, 1>{{_size}})
    , _blockspec(initialize_blockspec(_local_sizes))
    , _distspec(arguments.distspec())
    , _team(&arguments.team())
    , _teamspec(arguments.teamspec())
    , _nunits(_team->size())
    , _local_size(initialize_local_extent(_team->myid(), _local_sizes))
    , _local_memory_layout(std::array<SizeType, 1>{{_local_size}})
    , _local_capacity(initialize_local_capacity(_local_sizes))
  {}

  /**
   * Initialize the size (number of mapped elements) of the Pattern.
   */
  inline SizeType initialize_size(
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_TRACE_VAR("CSRPattern.init_size()", local_sizes);
    size_type size = 0;
    for (size_type unit_idx = 0; unit_idx < local_sizes.size(); ++
         unit_idx) {
      size += local_sizes[unit_idx];
    }
    DASH_LOG_TRACE_VAR("CSRPattern.init_size >", size);
    return size;
  }

  /**
   * Initialize local sizes from pattern size, distribution spec and team
   * spec.
   */
  std::vector<size_type> initialize_local_sizes(
    size_type                  total_size,
    const DistributionSpec_t & distspec,
    const dash::Team         & team) const
  {
    DASH_LOG_TRACE_VAR("CSRPattern.init_local_sizes()", total_size);
    std::vector<size_type> l_sizes;
    auto nunits = team.size();
    DASH_LOG_TRACE_VAR("CSRPattern.init_local_sizes()", nunits);
    if (nunits == 1) {
      l_sizes.push_back(total_size);
    }
    if (nunits <= 1) {
      return l_sizes;
    }
    auto dist_type = distspec[0].type;
    DASH_LOG_TRACE_VAR("CSRPattern.init_local_sizes()", dist_type);
    // Tiled and blocked distribution:
    if (dist_type == dash::internal::DIST_BLOCKED ||
        dist_type == dash::internal::DIST_TILE) {
      auto blocksize = dash::math::div_ceil(total_size, nunits);
      for (size_type u = 0; u < nunits; ++u) {
        l_sizes.push_back(blocksize);
      }
    // Unspecified distribution (default-constructed pattern instance),
    // set all local sizes to 0:
    } else if (dist_type == dash::internal::DIST_UNDEFINED) {
      for (size_type u = 0; u < nunits; ++u) {
        l_sizes.push_back(0);
      }
    // No distribution, assign all indices to unit 0:
    } else if (dist_type == dash::internal::DIST_NONE) {
      l_sizes.push_back(total_size);
      for (size_type u = 0; u < nunits-1; ++u) {
        l_sizes.push_back(0);
      }
    // Incompatible distribution type:
    } else {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "CSRPattern expects TILE (" << dash::internal::DIST_TILE << ") " <<
        "or BLOCKED (" << dash::internal::DIST_BLOCKED << ") " <<
        "distribution, got " << dist_type);
    }
    DASH_LOG_TRACE_VAR("CSRPattern.init_local_sizes >", l_sizes);
    return l_sizes;
  }

  BlockSpec_t initialize_blockspec(
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_TRACE_VAR("CSRPattern.init_blockspec", local_sizes);
    BlockSpec_t blockspec({
	static_cast<size_type>(local_sizes.size())
	  });
    DASH_LOG_TRACE_VAR("CSRPattern.init_blockspec >", blockspec);
    return blockspec;
  }

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  std::vector<size_type> initialize_block_offsets(
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_TRACE_VAR("CSRPattern.init_block_offsets", local_sizes);
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
    DASH_LOG_TRACE_VAR("CSRPattern.init_block_offsets >", block_offsets);
    return block_offsets;
  }

  /**
   * Max. elements per unit (local capacity)
   */
  SizeType initialize_local_capacity(
    const std::vector<size_type> & local_sizes) const
  {
    SizeType l_capacity = 0;
    // Local capacity is maximum number of elements assigned to a single unit,
    // i.e. the maximum local size:
    auto max_local_size = std::max_element(local_sizes.begin(),
                                           local_sizes.end());
    if (max_local_size != local_sizes.end()) {
      l_capacity = *max_local_size;
    }
    DASH_LOG_DEBUG_VAR("CSRPattern.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  void initialize_local_range()
  {
    auto l_size = _local_size;
    DASH_LOG_DEBUG_VAR("CSRPattern.init_local_range()", l_size);
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
    DASH_LOG_DEBUG_VAR("CSRPattern.init_local_range >", _lbegin);
    DASH_LOG_DEBUG_VAR("CSRPattern.init_local_range >", _lend);
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  SizeType initialize_local_extent(
    team_unit_t                    unit,
    const std::vector<size_type> & local_sizes) const
  {
    DASH_LOG_DEBUG_VAR("CSRPattern.init_local_extent()", unit);
    if (local_sizes.size() == 0) {
      return 0;
    }
    // Local size of given unit:
    SizeType l_extent = local_sizes[static_cast<int>(unit)];
    DASH_LOG_DEBUG_VAR("CSRPattern.init_local_extent >", l_extent);
    return l_extent;
  }

}; // class CSRPattern<1>

} // namespace dash

#endif // DASH__CSR_PATTERN_1D_H_
