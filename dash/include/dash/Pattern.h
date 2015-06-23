#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>
#include <type_traits>

#include <dash/Enums.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>
#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>

namespace dash {

/**
 * \defgroup DASH_CONCEPT_PATTERN Pattern Concept
 * Concept for distribution pattern of n-dimensional containers to
 * units in a team.
 *
 * \ingroup DASH_CONCEPT
 * \{
 * \par Description
 *
 * \}
 */

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 *
 * Consequently, a pattern realizes a projection of a global index
 * range to a local view:
 *
 * Distribution                 | Container                     |
 * ---------------------------- | ----------------------------- |
 * <tt>[ team 0 : team 1 ]</tt> | <tt>[ 0  1  2  3  4  5 ]</tt> |
 * <tt>[ team 1 : team 0 ]</tt> | <tt>[ 6  7  8  9 10 11 ]</tt> |
 *
 * This pattern would assign local indices to teams like this:
 * 
 * Team            | Local indices                 |
 * --------------- | ----------------------------- |
 * <tt>team 0</tt> | <tt>[ 0  1  2  9 10 11 ]</tt> |
 * <tt>team 1</tt> | <tt>[ 3  4  5  6  7  8 ]</tt> |
 *
 *
 * \tparam  NumDimensions  The number of dimensions of the pattern
 * \tparam  Arrangement    The memory order of the pattern (ROW_MAJOR
 *                         or COL_MAJOR), defaults to ROW_MAJOR.
 *                         Memory order defines how elements in the
 *                         pattern will be iterated predominantly
 *                         \see MemArrange
 * 
 * \concept DASH_CONCEPT_PATTERN
 */
template<
  size_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename IndexType     = long long>
class Pattern {
private:
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type SizeType;
  /// N-dimensional size type
  typedef ::std::array<SizeType, NumDimensions> SizeTypeNDim;
  /// Fully specified type definition of self
  typedef Pattern<NumDimensions, Arrangement, IndexType>
    self_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    BlockSpec_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement, IndexType>
    BlockSizeSpec_t;
  typedef DistributionSpec<NumDimensions> DistributionSpec_t;
  typedef TeamSpec<NumDimensions, IndexType> TeamSpec_t;
  typedef SizeSpec<NumDimensions, SizeType>  SizeSpec_t;
  typedef ViewSpec<NumDimensions, IndexType> ViewSpec_t;

private:
  /**
   * Extracting size-, distribution- and team specifications from
   * arguments passed to Pattern varargs constructor.
   *
   * \see Pattern<typename ... Args>::Pattern(Args && ... args)
   */
  class ArgumentParser {
  private:
    /// The extents of the pattern space in every dimension
    SizeSpec_t         _sizespec;
    /// The distribution type for every pattern dimension
    DistributionSpec_t _distspec;
    /// The cartesian arrangement of the units in the team to which the
    /// patterns element are mapped
    TeamSpec_t         _teamspec;
    /// The view specification of the pattern, consisting of offset and
    /// extent in every dimension
    ViewSpec_t         _viewspec;
    /// Number of distribution specifying arguments in varargs
    int                _argc_dist = 0;
    /// Number of size/extent specifying arguments in varargs
    int                _argc_size = 0;
    /// Number of team specifying arguments in varargs
    int                _argc_team = 0;

  public:
    /**
     * Default constructor, used if no argument list is parsed.
     */
    ArgumentParser() {
    }

    /**
     * Constructor, parses settings in argument list and checks for
     * constraints.
     */
    template<typename ... Args>
    ArgumentParser(Args && ... args) {
      static_assert(
        sizeof...(Args) >= NumDimensions,
        "Invalid number of arguments for Pattern::ArgumentParser");
      // Parse argument list:
      check<0>(std::forward<Args>(args)...);
      // Validate number of arguments after parsing:
      if (_argc_size > 0 && _argc_size != NumDimensions) {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Invalid number of size arguments for Pattern(...), " <<
          "expected " << NumDimensions << ", got " << _argc_size);
      }
      if (_argc_dist > 0 && _argc_dist != NumDimensions) {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Invalid number of distribution arguments for Pattern(...), " <<
          "expected " << NumDimensions << ", got " << _argc_dist);
      }
      check_tile_constraints();
    }
  
    const SizeSpec_t & sizespec() const {
      return _sizespec;
    }
    const DistributionSpec_t & distspec() const {
      return _distspec;
    }
    const TeamSpec_t & teamspec() const {
      return _teamspec;
    }
    const ViewSpec_t & viewspec() const {
      return _viewspec;
    }

  private:
    /// Pattern matching for extent value of type int.
    template<int count>
    void check(int extent) {
      check<count>((SizeType)(extent));
    }
    /// Pattern matching for extent value of type int.
    template<int count>
    void check(unsigned int extent) {
      check<count>((SizeType)(extent));
    }
    /// Pattern matching for extent value of type size_t.
    template<int count>
    void check(unsigned long extent) {
      check<count>((SizeType)(extent));
    }
    /// Pattern matching for extent value of type IndexType.
    template<int count>
    void check(SizeType extent) {
      _argc_size++;
      _sizespec[count] = extent;
    }
    /// Pattern matching for up to \c NumDimensions optional parameters
    /// specifying the distribution pattern.
    template<int count>
    void check(const TeamSpec_t & teamSpec) {
      _argc_team++;
      _teamspec   = teamSpec;
    }
    /// Pattern matching for one optional parameter specifying the 
    /// team.
    template<int count>
    void check(dash::Team & team) {
      if (_argc_team == 0) {
        _teamspec = TeamSpec_t(_distspec, team);
      }
    }
    /// Pattern matching for one optional parameter specifying the 
    /// size (extents).
    template<int count>
    void check(const SizeSpec_t & sizeSpec) {
      _argc_size += NumDimensions;
      _sizespec   = sizeSpec;
    }
    /// Pattern matching for one optional parameter specifying the 
    /// distribution.
    template<int count>
    void check(const DistributionSpec_t & ds) {
      _argc_dist += NumDimensions;
      _distspec   = ds;
    }
    /// Pattern matching for up to NumDimensions optional parameters
    /// specifying the distribution.
    template<int count>
    void check(const Distribution & ds) {
      _argc_dist++;
      int dim = count - NumDimensions;
      _distspec[dim] = ds;
    }
    /// Isolates first argument and calls the appropriate check() function
    /// on each argument via recursion on the argument list.
    template<int count, typename T, typename ... Args>
    void check(T t, Args && ... args) {
      check<count>(t);
      if (sizeof...(Args) > 0) {
        check<count + 1>(std::forward<Args>(args)...);
      }
    }
    /// Check pattern constraints for tile
    void check_tile_constraints() const {
      bool has_tile = false;
      bool invalid  = false;
      for (unsigned int i = 0; i < NumDimensions-1; i++) {
        if (_distspec.dim(i).type == dash::internal::DIST_TILE)
          has_tile = true;
        if (_distspec.dim(i).type != _distspec.dim(i+1).type)
          invalid = true;
      }
      assert(!(has_tile && invalid));
      if (has_tile) {
        for (unsigned int i = 0; i < NumDimensions; i++) {
          assert(
            _sizespec.extent(i) % (_distspec.dim(i).blocksz)
            == 0);
        }
      }
    }
  };

private:
  ArgumentParser     _arguments;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
  /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
  /// dimensions
  DistributionSpec_t _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team &       _team            = dash::Team::All();
  /// Cartesian arrangement of units within the team
  TeamSpec_t         _teamspec;
  /// The global layout of the pattern's elements in memory respective to
  /// memory order. Also specifies the extents of the pattern space.
  MemoryLayout_t     _memory_layout;
  /// A projected view of the global memory layout representing the
  /// local memory layout of this unit's elements respective to memory
  /// order.
  MemoryLayout_t     _local_memory_layout;
  /// The view specification of the pattern, consisting of offset and
  /// extent in every dimension
  /// The view specification of the pattern, consisting of offset and
  /// extent in every dimension
  ViewSpec_t         _viewspec;
  /// Number of blocks in all dimensions.
  BlockSpec_t        _blockspec;
  /// Number of blocks in all dimensions.
  BlockSpec_t        _blockspec_transposed;
  /// Maximum extents of a block in this pattern.
  BlockSizeSpec_t    _blocksize_spec;
  /// Local extents of the pattern in all dimensions
  SizeTypeNDim       _local_extent    = {  };
  /// Actual number of elements local to the active unit
  SizeType           _local_size      = 1;
  /// Total amount of units to which this pattern's elements are mapped
  SizeType           _nunits          = dash::Team::All().size();
  /// Maximum number of elements in a single block
  SizeType           _max_blocksize;

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
    _memory_layout(_arguments.sizespec()), 
    _viewspec(_arguments.viewspec()) {
    _nunits = _teamspec.size();
    initialize_block_specs();
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
    const SizeSpec_t &    sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Cartesian arrangement of units within the team
    const TeamSpec_t &    teamorg   = TeamSpec_t::TeamSpec(),
    /// Team containing units to which this pattern maps its elements
    dash::Team &          team      = dash::Team::All()) 
  : _distspec(dist),
    _team(team),
    _memory_layout(sizespec),
    _teamspec(
      teamorg,
      _distspec,
      _team) {
    _nunits   = _team.size();
    _viewspec = ViewSpec_t(_memory_layout.extents());
    initialize_block_specs();
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
    const SizeSpec_t &    sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team &                team      = dash::Team::All())
  : _distspec(dist),
    _team(team),
    _teamspec(_distspec, _team),
    _memory_layout(sizespec) {
    _nunits        = _team.size();
    _viewspec      = ViewSpec<NumDimensions>(_memory_layout);
    initialize_block_specs();
  }

  /**
   * Copy constructor.
   */
  Pattern(const self_t & other)
  : _distspec(other._distspec), 
    _teamspec(other._teamspec),
    _memory_layout(other._memory_layout),
    _viewspec(other._viewspec),
    _blockspec(other._blocksize_spec),
    _blocksize_spec(other._blocksize_spec),
    _local_size(other._local_size),
    _nunits(other._nunits),
    _max_blocksize(other._max_blocksize) {
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
    return(
      _distspec       == other._distspec &&
      _teamspec       == other._teamspec &&
      _memory_layout  == other._memory_layout &&
      _viewspec       == other._viewspec &&
      _blockspec      == other._blockspec &&
      _blocksize_spec == other._blocksize_spec &&
      _nunits         == other._nunits &&
      _local_size     == other._local_size
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
   * Convert given point in pattern to its assigned unit id.
   */
  IndexType unit_at(
    /// Absolute coordinates of the point
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    // Apply viewspec offsets to coordinates:
    std::array<IndexType, NumDimensions> vs_coords;
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      vs_coords[d] = coords[d] + viewspec[d].offset;
    }
    // Index of block containing the given coordinates:
    std::array<IndexType, NumDimensions> block_coords = 
      coords_to_block_coords(vs_coords);
    // Unit assigned to this block index:
    IndexType unit_id = block_coords_to_unit(block_coords);
    DASH_LOG_TRACE("Pattern.unit_at",
                   "coords", coords,
                   "block coords", block_coords,
                   "unit id", unit_id);
    return unit_id;
  }

  /**
   * Convert given point in pattern to its assigned unit id.
   */
  template<typename ... Values>
  IndexType unit_at(
    /// Absolute coordinates of the point
    Values ... values
  ) const {
    std::array<IndexType, NumDimensions> inputindex = { values... };
    return unit_at(inputindex, _viewspec);
  }

  /**
   * Convert given relative coordinates to local offset.
   */
  IndexType local_at(
    /// Point in local memory
    const std::array<IndexType, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    // TODO
    return 0;
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit in the given dimension.
   *
   * \see blocksize()
   * \see local_size()
   */
  IndexType local_extent(unsigned int dim) const {
    if (dim >= NumDimensions || dim < 0) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for Pattern::local_extent. "
        << "Expected dimension between 0 and " << NumDimensions-1 << ", "
        << "got " << dim);
    }
    return _local_extent[dim];
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit in total.
   *
   * \see blocksize()
   * \see local_extent()
   */
  size_t local_size() const {
    return _local_size;
  }

  /**
   * Resolve an element's linear global index from a given unit's local
   * index.
   *
   * \see index_to_elem Inverse of local_to_global_index
   */
  std::array<IndexType, NumDimensions> local_to_global_coords(
    size_t unit,
    const std::array<IndexType, NumDimensions> & local_index) {
    DASH_LOG_DEBUG_VAR("Pattern.local_to_global()", local_index);
    SizeType blocksize = max_blocksize();
    SizeType num_units = _teamspec.size();
    // Coordinates of the unit within the team spec:
    std::array<IndexType, NumDimensions> unit_ts_coord =
      _teamspec.coords(unit);
    // Global coords of the element's block within all blocks.
    // Use initializer so elements are initialized with 0s:
    std::array<IndexType, NumDimensions> block_index = {  };
    // Index of the element's block as element offset:
    std::array<IndexType, NumDimensions> block_coord = {  };
    // Index of the element:
    std::array<IndexType, NumDimensions> glob_index;
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = _distspec[d];
      unsigned int d_t          = NumDimensions - 1 - d;
      auto num_units_d          = _teamspec.extent(d); 
      auto num_blocks_d         = _blockspec.extent(d);
      auto blocksize_d          = _blocksize_spec.extent(d);
      auto local_index_d        = local_index[d];
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
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", d);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", unit_ts_coord[d]);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", local_index_d);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", num_units_d);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", block_index[d]);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", blocksize);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", blocksize_d);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", elem_block_offset_d);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", block_coord[d]);
      DASH_LOG_TRACE_VAR("Pattern.local_to_global.d", glob_index[d]);
    }
    DASH_LOG_TRACE_VAR("Pattern.local_to_global", block_index);
    DASH_LOG_TRACE_VAR("Pattern.local_to_global", block_coord);
    DASH_LOG_DEBUG_VAR("Pattern.local_to_global", glob_index);
    return glob_index;
  }

  /**
   * Resolve an element's linear global index from a given unit's local
   * coordinates.
   *
   * \see index_to_elem Inverse of local_to_global_index
   */
  IndexType local_coords_to_global_index(
    size_t unit,
    const std::array<IndexType, NumDimensions> & local_index) {
    std::array<IndexType, NumDimensions> global_coords =
      local_to_global_coords(unit, local_index);
    DASH_LOG_TRACE_VAR("Pattern.local_to_global_idx", global_coords);
    return _memory_layout.at(global_coords);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   * 
   * Will be renamed to \c unit_at_coords.
   */
  size_t index_to_unit(
    const std::array<IndexType, NumDimensions> & input) const {
    return unit_at(input, _viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * 
   * Will be renamed to \c coords_to_local_index.
   */
  IndexType index_to_elem(
    std::array<IndexType, NumDimensions> input) const {
    return index_to_elem(input, _viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * 
   * Will be renamed to \c coords_to_local_index.
   */
  IndexType index_to_elem(
    const std::array<IndexType, NumDimensions> & coords,
    const ViewSpec_t & viewspec) const {
    DASH_LOG_DEBUG_VAR("Pattern.index_to_elem()", coords);
    // Convert coordinates to linear global index respective to memory
    // order:
 // IndexType glob_index = _memory_layout.at(coords);
    std::array<IndexType, NumDimensions> block_coords =
      coords_to_block_coords(coords);
    // Global index of block start point:
    std::array<IndexType, NumDimensions> block_begin_coords(block_coords);
    // Input coords transformed to their offset within the block:
    std::array<IndexType, NumDimensions> relative_coords(coords);
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      block_begin_coords[d] *= blocksize(d);
      relative_coords[d]    -= block_begin_coords[d];
    }
    // Block offset, i.e. number of blocks in front of referenced block:
    SizeType block_offset       = _blockspec.at(block_coords);
    // Offset of the referenced block within all blocks local to its unit,
    // i.e. nth block of this unit:
    SizeType local_block_offset = block_offset / _teamspec.size();
    // Local offset of the first element in the block:
    // TODO: Should be actual_block_size instead of max_blocksize?
    SizeType block_base_offset  = local_block_offset * max_blocksize();
    // Offset of the referenced index within its block:
    SizeType elem_block_offset  = _blocksize_spec.at(relative_coords);
    // Local offset of the element with all of the unit's local elements:
    SizeType local_elem_offset  = block_base_offset + elem_block_offset;
    DASH_LOG_TRACE_VAR("Pattern.index_to_elem", block_offset);
    DASH_LOG_TRACE_VAR("Pattern.index_to_elem", local_block_offset);
    DASH_LOG_TRACE_VAR("Pattern.index_to_elem", block_base_offset);
    DASH_LOG_TRACE_VAR("Pattern.index_to_elem", relative_coords);
    DASH_LOG_TRACE_VAR("Pattern.index_to_elem", elem_block_offset);
    DASH_LOG_DEBUG_VAR("Pattern.index_to_elem", local_elem_offset);
    return local_elem_offset;
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   */
  IndexType at(
    const std::array<IndexType, NumDimensions> & coords,
    const ViewSpec_t & viewspec) const {
    return index_to_elem(coords, _viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   */
  template<typename ... Values>
  IndexType at(Values ... values) const {
    static_assert(
      sizeof...(values) == NumDimensions,
      "Wrong parameter number");
    std::array<IndexType, NumDimensions> inputindex = { 
      (IndexType)values...
    };
    return index_to_elem(inputindex, _viewspec);
  }

  /**
   * Whether the given dimension offset involves any local part
   */
  bool is_local(
    IndexType index,
    size_t unit_id,
    unsigned int dim,
    const ViewSpec<NumDimensions> & viewspec) const {
    // TODO
    return true;
  }

  /**
   * Maximum number of elements in a single block in the given dimension.
   *
   * \return  The blocksize in the given dimension
   */
  SizeType blocksize(
    /// The dimension in the pattern
    unsigned int dimension) const {
    return _blocksize_spec.extent(dimension);
  }

  /**
   * Maximum number of elements in a single block in all dimensions.
   *
   * \return  The maximum number of elements in a single block assigned to
   *          a unit.
   */
  SizeType max_blocksize() const {
    return _max_blocksize;
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   */
  SizeType max_elem_per_unit() const {
    SizeType max_elements = 1;
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      SizeType num_units  = units_in_dimension(d);
      const Distribution & dist = _distspec[d];
      // Block size in given dimension:
      auto dim_max_blocksize = blocksize(d);
      // Maximum number of occurrences of a single unit in given dimension:
      SizeType dim_num_blocks = dist.max_local_blocks_in_range(
                                  // size of range:
                                  _memory_layout.extent(d),
                                  // number of blocks:
                                  num_units
                                );
      max_elements *= dim_max_blocksize * dim_num_blocks;
      DASH_LOG_TRACE_VAR("Pattern.max_elem_per_unit.d", d);
      DASH_LOG_TRACE_VAR("Pattern.max_elem_per_unit.d", num_units);
      DASH_LOG_TRACE_VAR("Pattern.max_elem_per_unit.d", dim_max_blocksize);
      DASH_LOG_TRACE_VAR("Pattern.max_elem_per_unit.d", dim_num_blocks);
      DASH_LOG_TRACE_VAR("Pattern.max_elem_per_unit.d", max_elements);
    }
    DASH_LOG_DEBUG_VAR("Pattern.max_elem_per_unit", max_elements);
    return max_elements;
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   */
  IndexType num_units() const {
    return _teamspec.size();
  }

  /**
   * Assignment operator.
   */
  Pattern & operator=(const Pattern & other) {
    if (this != &other) {
      _distspec       = other._distspec;
      _teamspec       = other._teamspec;
      _memory_layout  = other._memory_layout;
      _viewspec       = other._viewspec;
      _nunits         = other._nunits;
      _max_blocksize  = other._max_blocksize;
      _blockspec      = other._blockspec;
      _blocksize_spec = other._blocksize_spec;
    }
    return *this;
  }

  /**
   * The maximum number of elements arranged in this pattern.
   */
  IndexType capacity() const {
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
  DistributionSpec_t distspec() const {
    return _distspec;
  }

  /**
   * Size specification of the index space mapped by this pattern.
   */
  SizeSpec_t sizespec() const {
    return SizeSpec_t(_memory_layout.extents());
  }

  /**
   * Cartesian index space representing the underlying memory model of this
   * pattern.
   */
  const MemoryLayout_t & memory_layout() const {
    return _memory_layout;
  }

  /**
   * Cartesian arrangement of the Team containing the units to which this
   * pattern's elements are mapped.
   */
  TeamSpec_t teamspec() const {
    return _teamspec;
  }

  /**
   * View specification of this pattern as offset and extent in every
   * dimension.
   */
  const ViewSpec_t & viewspec() const {
    return _viewspec;
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates.
   *
   * \see at(...) Inverse of coords(index)
   */
  std::array<IndexType, NumDimensions> coords(IndexType index) const {
    return _memory_layout.coords(index);
  }

  /**
   * Number of elements in the overflow block of given dimension, with
   * 0 <= \c overflow_blocksize(d) < blocksize(d).
   */
  SizeType overflow_blocksize(unsigned int dimension) const {
    return _memory_layout.extent(dimension) % blocksize(dimension);
  }

  /**
   * Number of elements missing in the overflow block of given dimension
   * compared to the regular blocksize (\see blocksize(d)), with
   * 0 <= \c underfilled_blocksize(d) < blocksize(d).
   */
  SizeType underfilled_blocksize(unsigned int dimension) const {
    // Underflow blocksize = regular blocksize - overflow blocksize:
    auto reg_blocksize = blocksize(dimension);
    auto ovf_blocksize = overflow_blocksize(dimension);
    if (ovf_blocksize == 0) {
      return 0;
    } else {
      return reg_blocksize - ovf_blocksize;
    }
  }

private:
  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec.
   * Called from constructors.
   */
  void initialize_block_specs() {
    // Number of blocks in all dimensions:
    std::array<SizeType, NumDimensions> n_blocks;
    // Extents of a single block:
    std::array<SizeType, NumDimensions> s_blocks;
    // Local unit id:
    auto my_unit_id        = dash::myid();
    // Coordinates of local unit id in team spec:
    auto my_unit_ts_coords = _teamspec.coords(my_unit_id);

    DASH_LOG_TRACE_VAR("Pattern.initialize", my_unit_id);
    DASH_LOG_TRACE_VAR("Pattern.initialize", my_unit_ts_coords);
    //// Pre-initialize block specs:
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = _distspec[d];
      SizeType max_blocksize_d = dist.max_blocksize_in_range(
        _memory_layout.extent(d), // size of range (extent)
        _teamspec.extent(d));     // number of blocks (units)
      SizeType max_blocks_d = dash::math::div_ceil(
        _memory_layout.extent(d),
        max_blocksize_d);
      n_blocks[d] = max_blocks_d;
      s_blocks[d] = max_blocksize_d;
    }
    _blockspec      = BlockSpec_t(n_blocks);
    _blocksize_spec = BlockSizeSpec_t(s_blocks);
    _max_blocksize  = _blocksize_spec.size();
    //// Pre-initialize local extents:
    _local_size     = 1;
    for (unsigned int d = 0; d < NumDimensions; ++d) {
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
      auto my_unit_ts_coord   = my_unit_ts_coords[d];
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", d);
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", my_unit_ts_coord);
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", num_elem_d);
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", num_units_d);
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", num_blocks_d);
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", blocksize_d);
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", min_local_blocks_d);
      _local_extent[d] = min_local_blocks_d * blocksize_d;
      if (num_blocks_d == 1 || num_units_d == 1) {
        // One block with full extent in dimension:
        _local_extent[d] = num_elem_d;
      } else {
        // Number of additional blocks for this unit, if any:
        IndexType num_add_blocks = static_cast<IndexType>(
                                     num_blocks_d % num_units_d);
        DASH_LOG_TRACE_VAR("Pattern.initialize.d", num_add_blocks);
        if (my_unit_ts_coord < num_add_blocks) {
          // Unit is assigned to an additional block:
          _local_extent[d] += blocksize_d;
          DASH_LOG_TRACE_VAR("Pattern.initialize.d", _local_extent[d]);
          // If the last block in the dimension is underfilled and
          // assigned to the local unit, subtract the missing extent:
          if (my_unit_ts_coord == num_add_blocks - 1) {
            // Last block in dimension is assigned to local unit
            SizeType undfill_blocksize_d = underfilled_blocksize(d);
            DASH_LOG_TRACE_VAR("Pattern.initialize", undfill_blocksize_d);
            _local_extent[d] -= undfill_blocksize_d;
          }
        }
      }
      _local_size *= _local_extent[d];
      DASH_LOG_TRACE_VAR("Pattern.initialize.d", _local_extent[d]);
    }
    DASH_LOG_DEBUG_VAR("Pattern.initialize", _local_extent);
  }

  /**
   * The number of blocks in the given dimension.
   */
  SizeType num_blocks(unsigned int dimension) const {
    return _blockspec[dimension];
  }

  /**
   * Convert global index coordinates to the coordinates of its block in
   * the pattern.
   */
  std::array<IndexType, NumDimensions> coords_to_block_coords(
    const std::array<IndexType, NumDimensions> & coords) const {
    std::array<IndexType, NumDimensions> block_coords;
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      block_coords[d] = coords[d] / blocksize(d);
    }
    return block_coords;
  }

  /**
   * Resolve the associated unit id of the given block.
   */
  size_t block_coords_to_unit(
    std::array<IndexType, NumDimensions> & block_coords) const {
    size_t unit_id = 0;
    for (unsigned int d = 0; d < NumDimensions; ++d) {
      const Distribution & dist = _distspec[d];
      unit_id += dist.block_coord_to_unit_offset(
                    block_coords[d], // block coordinate
                    d,               // dimension
                    _teamspec.size() // number of units
                 );
    }
    // TODO: Use _teamspec and _team to resolve actual unit id?
    return unit_id;
  }

  SizeType units_in_dimension(unsigned int dimension) const {
    return _teamspec.extent(dimension);
  }
};

/**
 * Invoke a function on every element in a range distributed by a pattern.
 * Being a collaborative operation, each unit will invoke the given
 * function on its local elements only.
 *
 * \tparam  PatternInterator  An iterator implementing the
 *                            PatternInterator concept
 * \tparam  IndexType         Parameter type expected by function to
 *                            invoke, deduced from parameter \c func
 */
template<typename PatternIterator, typename IndexType>
void forall(
  /// Iterator pointing to the first element of a range
  PatternIterator begin,
  /// Iterator pointing behind the last element of a range
  PatternIterator end,
  /// Function to invoke on every index in the range
  ::std::function<void(IndexType)> func) {
  auto range   = end - begin;
  auto pattern = begin.pattern();
  for (IndexType i = 0;
       i < pattern.sizespec.size() && range > 0;
       ++i, --range) {
    IndexType idx = pattern.local_to_global_index(
      pattern.team().myid(), i);
    if (idx < 0) {
      break;
    }
    func(idx);
  }
}

} // namespace dash

#endif // DASH__PATTERN_H_
