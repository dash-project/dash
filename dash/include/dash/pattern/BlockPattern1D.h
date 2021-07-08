#ifndef DASH__BLOCK_PATTERN_1D_H_
#define DASH__BLOCK_PATTERN_1D_H_

#include <dash/Types.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>

#include <dash/pattern/BlockPattern.h>
#include <dash/pattern/PatternProperties.h>
#include <dash/pattern/internal/PatternArguments.h>

#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>

#include <functional>
#include <array>
#include <type_traits>


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
class BlockPattern<1, Arrangement, IndexType>
{
private:
  static const dim_t NumDimensions = 1;

public:
  static constexpr const char * PatternName = "BlockPattern1D";

public:
  /// Satisfiable properties in pattern property category Partitioning:
  typedef pattern_partitioning_properties<
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
  typedef BlockPattern<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    LocalMemoryLayout_t;
  typedef CartesianSpace<NumDimensions, SizeType>
    BlockSpec_t;
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

  typedef DistributionSpec<NumDimensions>     distribution_spec;
  typedef TeamSpec<NumDimensions, IndexType>  team_spec;
  typedef SizeSpec<NumDimensions, SizeType>   size_spec;

private:
  /// Extent of the linear pattern.
  SizeType                    _size            = 0;
  /// Global memory layout of the pattern.
  MemoryLayout_t              _memory_layout;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC or NONE) of
  /// all dimensions. Defaults to BLOCKED.
  distribution_spec           _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team *                _team            = nullptr;
  /// Cartesian arrangement of units within the team
  team_spec                   _teamspec;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType                    _nunits          = 0;
  /// Maximum extents of a block in this pattern
  SizeType                    _blocksize       = 0;
  /// Number of blocks in all dimensions
  SizeType                    _nblocks         = 0;
  /// Actual number of local elements.
  SizeType                    _local_size      = 0;
  /// Local memory layout of the pattern.
  LocalMemoryLayout_t         _local_memory_layout;
  /// Arrangement of local blocks in all dimensions
  SizeType                    _nlblocks        = 0;
  /// Maximum number of elements assigned to a single unit
  SizeType                    _local_capacity  = 0;
  /// Corresponding global index to first local index of the active unit
  std::array<IndexType, 2>    _lbegin_lend     = { };
  /// Corresponding global index past last local index of the active unit
  IndexType                   _lend            = -1;

public:
  constexpr BlockPattern() = delete;

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
  BlockPattern(
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    SizeType arg,
    /// Argument list consisting of the pattern size (extent, number of
    /// elements) in every dimension followed by optional distribution
    /// types.
    Args && ... args)
  : BlockPattern(PatternArguments_t(arg, args...))
  { }

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
  BlockPattern(
    /// Pattern size (extent, number of elements) in every dimension
    const size_spec          sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC or NONE).
    const distribution_spec  dist,
    /// Cartesian arrangement of units within the team
    const team_spec          teamspec,
    /// Team containing units to which this pattern maps its elements
    dash::Team &             team     = dash::Team::All())
  : _size(sizespec.size()),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
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
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _nlblocks(initialize_num_local_blocks(
        _blocksize,
        _local_size)),
    _local_capacity(initialize_local_capacity()),
    _lbegin_lend(initialize_local_range(_local_size))
  { }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec and a \c Team.
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
  BlockPattern(
    /// Pattern size (extent, number of elements) in every dimension
    const size_spec         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE).
    /// Defaults to BLOCKED.
    const distribution_spec dist = distribution_spec(),
    /// Team containing units to which this pattern maps its elements
    Team &                   team = dash::Team::All())
  : _size(sizespec.size()),
    _memory_layout(std::array<SizeType, 1> {{ _size }}),
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
    _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
    _nlblocks(initialize_num_local_blocks(
        _blocksize,
        _local_size)),
    _local_capacity(initialize_local_capacity()),
    _lbegin_lend(initialize_local_range(_local_size))
  { }

  /**
   * Move constructor.
   */
  constexpr BlockPattern(self_t && other)      = default;

  /**
   * Copy constructor.
   */
  constexpr BlockPattern(const self_t & other) = default;

  /**
   * Copy constructor using non-const lvalue reference parameter.
   *
   * Introduced so variadic constructor is not a better match for
   * copy-construction.
   */
  constexpr BlockPattern(self_t & other)
  : BlockPattern(static_cast<const self_t &>(other)) {
  }

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && other)         = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other)    = default;

  /**
   * Equality comparison operator.
   */
  constexpr bool operator==(
    /// Pattern instance to compare for equality
    const self_t & other
  ) const {
    // no need to compare all members as most are derived from
    // constructor arguments.
    return (
      this == &other
      || ( _size        == other._size &&
           _distspec    == other._distspec &&
           _teamspec    == other._teamspec &&
           _nblocks     == other._nblocks &&
           _nlblocks    == other._nlblocks &&
           _local_size  == other._local_size &&
           _blocksize   == other._blocksize &&
           _nunits      == other._nunits &&
           _lbegin_lend == other._lbegin_lend
         )
      );
  }

  /**
   * Inquality comparison operator.
   */
  constexpr bool operator!=(
    /// Pattern instance to compare for inequality
    const self_t & other
  ) const {
    return !(*this == other);
  }

  /**
   * Resolves the global index of the first local element in the pattern.
   *
   * \see DashPatternConcept
   */
  constexpr IndexType lbegin() const {
    return _lbegin_lend[0];
  }

  /**
   * Resolves the global index past the last local element in the pattern.
   *
   * \see DashPatternConcept
   */
  constexpr IndexType lend() const {
    return _lbegin_lend[1];
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
    const ViewSpec_t & viewspec) const {
    return team_unit_t (((coords[0] + viewspec.offset(0)) / _blocksize)
                        % _nunits);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  constexpr team_unit_t unit_at(
    const std::array<IndexType, NumDimensions> & coords) const {
    return team_unit_t((coords[0] / _blocksize) % _nunits);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  constexpr team_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos,
    /// View to apply global position
    const ViewSpec_t & viewspec
  ) const {
    return team_unit_t(((global_pos + viewspec.offset(0)) / _blocksize)
                       % _nunits);
  }

  /**
   * Convert given global linear index to its assigned unit id.
   *
   * \see DashPatternConcept
   */
  constexpr team_unit_t unit_at(
    /// Global linear element offset
    IndexType global_pos
  ) const {
    return team_unit_t((global_pos / _blocksize) % _nunits);
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
   * The number of elements in this pattern in the given dimension.
   *
   * \see  blocksize()
   * \see  local_size()
   * \see  local_extent()
   *
   * \see  DashPatternConcept
   */
  template <dim_t dim = 0>
  constexpr IndexType extent() const {
    static_assert(
      0 == dim,
      "Wrong dimension for Pattern::local_extent. "
      "Expected dimension = 0");
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
   * calling unit in the given dimension.
   *
   * \see  local_extents()
   * \see  blocksize()
   * \see  local_size()
   * \see  extent()
   *
   * \see  DashPatternConcept
   */
  template <dim_t dim = 0>
  constexpr IndexType local_extent() const {
    static_assert(
      0 == dim,
      "Wrong dimension for Pattern::local_extent. "
      "Expected dimension = 0");
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
  constexpr std::array<SizeType, NumDimensions> local_extents(
    team_unit_t unit) const {
    return std::array<SizeType, 1> {{
              unit == _team->myid()
              ? _local_size
              : initialize_local_extent(unit)
           }};
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
  constexpr std::array<SizeType, NumDimensions> local_extents() const {
    return std::array<SizeType, 1> {{  _local_size }};
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
  constexpr IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & local_coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    return local_coords[0] + viewspec.offset(0);
  }

  /**
   * Convert given local coordinates to linear local offset (index).
   *
   * \see DashPatternConcept
   */
  constexpr IndexType local_at(
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
  constexpr local_coords_t local(
    const std::array<IndexType, NumDimensions> & global_coords) const {
    return local_coords_t {
             unit_at(global_coords),      // .unit
             local_coords(global_coords)  // .coords
           };
  }

  /**
   * Converts global index to its associated unit and respective local index.
   *
   * TODO: Unoptimized
   *
   * \see  DashPatternConcept
   */
  constexpr local_index_t local(
    IndexType g_index) const {
    return local_index(coords(g_index));
  }

  /**
   * Converts global coordinates to their associated unit's respective
   * local coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> local_coords(
    const std::array<IndexType, NumDimensions> & global_coords
  ) const noexcept {
    return std::array<IndexType, 1> {{
             static_cast<IndexType>(
               (((global_coords[0] / _blocksize) / _nunits) * _blocksize)
               + (global_coords[0] % _blocksize)
             )
           }};
  }

  /**
   * Resolves the unit and the local index from global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr local_index_t local_index(
    const std::array<IndexType, NumDimensions> & g_coords) const {
    return local_index_t {
             team_unit_t(unit_at(g_coords[0])),
             at(g_coords)
           };
  }

  ////////////////////////////////////////////////////////////////////////////
  /// global
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Converts local coordinates of a given unit to global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> global(
    team_unit_t unit,
    const std::array<IndexType, NumDimensions> & local_coords) const {
    return (_nunits < 2)
            ? local_coords
            : std::array<IndexType, 1> {{
                static_cast<IndexType>(
                  (( _distspec[0].local_index_to_block_coord(
                       static_cast<IndexType>(unit),
                       local_coords[0],
                       _nunits)
                   ) * _blocksize)
                  + (local_coords[0] % _blocksize)
                )
              }};
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
   * Resolve an element's linear global index from the calling unit's local
   * index of that element.
   *
   * \see  at  Inverse of global()
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
    return global(unit, l_coords)[0];
  }

  /**
   * Convert given global coordinates and viewspec to linear global offset
   * (index).
   *
   * \see DashPatternConcept
   */
  constexpr IndexType global_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & global_coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    return global_coords[0] + viewspec.offset(0);
  }

  /**
   * Convert given global coordinates to linear global offset (index).
   *
   * \see DashPatternConcept
   */
  constexpr IndexType global_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & global_coords) const {
    return global_coords[0];
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
  constexpr IndexType at(
    const std::array<IndexType, 1> & g_coords,
    const ViewSpec_t & viewspec) const {
    return local_coords(
             std::array<IndexType, 1> {{
               g_coords[0] + viewspec.offset(0)
             }}
           )[0];
  }

  /**
   * Global coordinates to local index.
   *
   * Convert given coordinate in pattern to its linear local index.
   *
   * \see  DashPatternConcept
   */
  template<typename ... Values>
  constexpr IndexType at(IndexType value, Values ... values) const {
    static_assert(
      sizeof...(values) == NumDimensions-1,
      "Wrong parameter number");
    return at(std::array<IndexType, NumDimensions> {
                value, (IndexType)values...
              });
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
  constexpr bool has_local_elements(
    /// Dimension to check
    dim_t dim,
    /// Offset in dimension
    IndexType dim_offset,
    /// DART id of the unit
    team_unit_t unit) const {
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
  constexpr bool is_local(
    IndexType index,
    team_unit_t unit) const {
    return unit_at(index) == unit;
  }

  /**
   * Whether the given global index is local to the unit that created
   * this pattern instance.
   *
   * \see  DashPatternConcept
   */
  constexpr bool is_local(
    IndexType index) const {
    return is_local(index, team().myid());
  }

  ////////////////////////////////////////////////////////////////////////////
  /// block
  ////////////////////////////////////////////////////////////////////////////

  /**
   * Cartesian arrangement of pattern blocks.
   */
  constexpr BlockSpec_t blockspec() const {
    return BlockSpec_t({ _nblocks });
  }

  /**
   * Cartesian arrangement of local pattern blocks.
   */
  constexpr BlockSpec_t local_blockspec() const
  {
    return BlockSpec_t({ _nlblocks });
  }

  /**
   * Gobal index of block at given global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr index_type block_at(
    /// Global coordinates of element
    const std::array<index_type, NumDimensions> & g_coords) const {
    return g_coords[0] / _blocksize;
  }

  /**
   * Local index of block at given global coordinates.
   *
   * \see  DashPatternConcept
   */
  constexpr local_index_t local_block_at(
    /// Global coordinates of element
    const std::array<index_type, NumDimensions> & g_coords) const {
    return local_index_t {
             // unit id:
             static_cast<team_unit_t>(
                (g_coords[0] / _blocksize) % _teamspec.size()),
             // local block index:
             static_cast<index_type>(
                (g_coords[0] / _blocksize) / _teamspec.size())
           };
  }

  /**
   * View spec (offset and extents) of block at global linear block index in
   * cartesian element space.
   */
  constexpr ViewSpec_t block(
    /// Global block index
    index_type g_block_index) const
  {
    return ViewSpec_t(
      {{ static_cast<index_type>(g_block_index * _blocksize) }},
      {{ static_cast<size_type>(
          _blocksize - ( g_block_index < _nblocks - 1
                         ? 0
                         : underfilled_blocksize(0) )
         ) }}
    );
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * global cartesian element space.
   */
  constexpr ViewSpec_t local_block(
    /// Local block index
    index_type l_block_index) const
  {
    // Local block index to local block coords:
    return ViewSpec_t(
      {{ static_cast<index_type>( global(l_block_index * _blocksize) ) }},
      {{ static_cast<size_type>(
          (l_block_index == (_nlblocks - 1)
          ? (_local_size % _blocksize == 0
             ? _blocksize
             : _local_size % _blocksize )
          : _blocksize )
         ) }}
    );
  }

  /**
   * View spec (offset and extents) of block at local linear block index in
   * local cartesian element space.
   */
  ViewSpec_t local_block_local(
    index_type l_block_index) const
  {
    DASH_LOG_DEBUG_VAR("BlockPattern<1>.local_block_local()", l_block_index);
    index_type offset = l_block_index * _blocksize;
    std::array<index_type, NumDimensions> offsets = {{ offset }};
    std::array<size_type, NumDimensions>  extents = {{ _blocksize }};
    if(l_block_index == (_nlblocks - 1))
    {
      size_type remaining = _local_size % extents[0];
      extents[0] = (remaining == 0) ? extents[0] : remaining;
    }
    ViewSpec_t block_vs(offsets, extents);
    DASH_LOG_DEBUG_VAR("BlockPattern<1>.local_block_local >", block_vs);
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
  constexpr SizeType max_blocksize() const {
    return _blocksize;
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   *
   * \see  DashPatternConcept
   */
  constexpr SizeType local_capacity() const
  {
    return _local_capacity;
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * specified unit in total.
   *
   * \see  blocksize()
   * \see  local_extent()
   * \see  local_capacity()
   *
   * \see  DashPatternConcept
   */
  constexpr SizeType local_size(team_unit_t unit) const
  {
    return (unit == _team->myid().id)
           ? _local_size
           : initialize_local_extent(unit);
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
  constexpr const distribution_spec & distspec() const noexcept {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  constexpr size_spec sizespec() const {
    return size_spec(std::array<SizeType, 1> {{ _size }});
  }

  /**
   * Size specification of the index space mapped by this pattern.
   *
   * \see DashPatternConcept
   */
  constexpr const std::array<SizeType, NumDimensions> extents() const {
    return std::array<SizeType, 1> {{ _size }};
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   *
   * \see DashPatternConcept
   */
  constexpr const team_spec & teamspec() const {
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
    return std::array<IndexType, 1> {{ index }};
  }

  /**
   * Convert given global linear offset (index) to global cartesian
   * coordinates using viewspec.
   *
   * \see DashPatternConcept
   */
  constexpr std::array<IndexType, NumDimensions> coords(
    IndexType          index,
    const ViewSpec_t & viewspec) const {
    return std::array<IndexType, 1> {{ index + viewspec.offset(0) }};
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
   * Number of elements missing in the overflow block of given dimension
   * compared to the regular blocksize (\see blocksize(d)), with
   * 0 <= \c underfilled_blocksize(d) < blocksize(d).
   */
  constexpr SizeType underfilled_blocksize(
    dim_t dimension) const {
    // Underflow blocksize = regular blocksize - overflow blocksize:
    return ( _blocksize == 0 || _size % _blocksize == 0
             ? 0
             : _blocksize - (_size % _blocksize)
           );
  }

private:

  BlockPattern(const PatternArguments_t & arguments)
  :  _size(arguments.sizespec().size()),
     _memory_layout(std::array<SizeType, 1> {{ _size }}),
     _distspec(arguments.distspec()),
     _team(&arguments.team()),
     _teamspec(arguments.teamspec()),
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
     _local_memory_layout(std::array<SizeType, 1> {{ _local_size }}),
     _nlblocks(initialize_num_local_blocks(
         _blocksize,
         _local_size)),
     _local_capacity(initialize_local_capacity()),
     _lbegin_lend(initialize_local_range(_local_size))
  {}

  /**
   * Initialize block size specs from memory layout, team spec and
   * distribution spec.
   */
  SizeType initialize_blocksize(
    SizeType                   size,
    const distribution_spec  & distspec,
    SizeType                   nunits) const {
    DASH_LOG_TRACE_VAR("BlockPattern<1>.init_blocksize", nunits);
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
    DASH_LOG_TRACE("BlockPattern<1>.init_num_blocks()",
                   "size", size, "blocksize", blocksize, "nunits", nunits);
    SizeType n_blocks = dash::math::div_ceil(
                          size,
                          blocksize);
    DASH_LOG_TRACE_VAR("BlockPattern<1>.init_blockspec", n_blocks);
    return n_blocks;
  }

  /**
   * Initialize local block spec from global block spec.
   */
  SizeType initialize_num_local_blocks(
    SizeType                    blocksize,
    SizeType                    local_size) const {
    auto num_l_blocks = local_size;
    if (blocksize > 0) {
      num_l_blocks = dash::math::div_ceil(
                       num_l_blocks,
                       blocksize);
    } else {
      num_l_blocks = 0;
    }
    DASH_LOG_TRACE_VAR("BlockPattern<1>.init_num_local_blocks", num_l_blocks);
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
    DASH_LOG_TRACE_VAR("BlockPattern<1>.init_lcapacity.d", _nunits);
    DASH_LOG_TRACE_VAR("BlockPattern<1>.init_lcapacity.d", max_l_blocks);
    l_capacity = max_l_blocks * _blocksize;
    DASH_LOG_DEBUG_VAR("BlockPattern<1>.init_lcapacity >", l_capacity);
    return l_capacity;
  }

  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   */
  constexpr std::array<IndexType, 2>
  initialize_local_range(SizeType l_size) const {
    return (l_size == 0)
             ? std::array<IndexType, 2> {{ 0, 0 }}
             : std::array<IndexType, 2> {{
                 global(0),
                 global(l_size - 1) + 1
               }};
  }

  /**
   * Resolve extents of local memory layout for a specified unit.
   */
  constexpr SizeType initialize_local_extent(
    team_unit_t unit) const
  {
    return (_nunits == 0
           ? 0
           : (_nblocks == 1 && _nunits == 1
             ? _size
               // Possibly there are more blocks than units in dimension
               // and no block left for this unit. Local extent in d then
               // becomes 0.
             : ((_nblocks / _nunits) * _blocksize
               // Unbalanced blocks owned by the unit:
               + (static_cast<SizeType>(unit) < (_nblocks % _nunits)
                 ? _blocksize
                 : 0 )
               // If the last block in the dimension is underfilled and
               // assigned to the local unit, subtract the missing extent:
               - (static_cast<SizeType>(unit) ==
                    // Unit id assigned to the last block:
                    (_nblocks % _nunits == 0
                    ? _nunits - 1
                    : ((_nblocks % _nunits) - 1 ))
                 ? underfilled_blocksize(0)
                 : 0 )
               )
             )
           );

  }
};

} // namespace dash

#endif // DASH__BLOCK_PATTERN_1D_H_
