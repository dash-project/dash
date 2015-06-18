#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>

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
 * Distribution                 | Container
 * ---------------------------- | -----------------------------
 * <tt>[ team 0 : team 1 ]</tt> | <tt>[ 0  1  2  3  4  5 ]</tt>
 * <tt>[ team 1 : team 0 ]</tt> | <tt>[ 6  7  8  9 10 11 ]</tt>
 *
 * This pattern would assign local indices to teams like this:
 * 
 * Team            | Local indices
 * --------------- | -----------------------------
 * <tt>team 0</tt> | <tt>[ 0  1  2  9 10 11 ]</tt>
 * <tt>team 1</tt> | <tt>[ 3  4  5  6  7  8 ]</tt>
 *
 *
 * \tparam  NumDimensions  The number of dimensions of the pattern
 * \tparam  Arrangement    The memory order of the pattern (ROW_ORDER
 *                         or COL_ORDER), defaults to ROW_ORDER.
 *                         Memory order defines how elements in the
 *                         pattern will be iterated predominantly
 *                         \see MemArrange
 * 
 * \concept DASH_CONCEPT_PATTERN
 */
template<
  size_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR>
class Pattern {
private:
  typedef Pattern<NumDimensions, Arrangement>             self_t;
  typedef DistributionSpec<NumDimensions>                 DistribSpec_t;
  typedef TeamSpec<NumDimensions>                         TeamSpec_t;
  typedef SizeSpec<NumDimensions>                         SizeSpec_t;
  typedef ViewSpec<NumDimensions>                         ViewSpec_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement> MemoryLayout_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement> BlockSpec_t;
  typedef CartesianIndexSpace<NumDimensions, Arrangement> BlockSizeSpec_t;

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
    DistribSpec_t _distspec;
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
    const DistribSpec_t & distspec() const {
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
      check<count>((long long)(extent));
    }
    /// Pattern matching for extent value of type size_t.
    template<int count>
    void check(size_t extent) {
      check<count>((long long)(extent));
    }
    /// Pattern matching for extent value of type long long.
    template<int count>
    void check(long long extent) {
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
        _teamspec = TeamSpec<NumDimensions>(_distspec, team);
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
    void check(const DistributionSpec<NumDimensions> & ds) {
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
    void check(T t, Args &&... args) {
      check<count>(t);
      check<count + 1>(std::forward<Args>(args)...);
    }
    /// Check pattern constraints for tile
    void check_tile_constraints() const {
      bool has_tile = false;
      bool invalid  = false;
      for (int i = 0; i < NumDimensions-1; i++) {
        if (_distspec.dim(i).type == dash::internal::DIST_TILE)
          has_tile = true;
        if (_distspec.dim(i).type != _distspec.dim(i+1).type)
          invalid = true;
      }
      assert(!(has_tile && invalid));
      if (has_tile) {
        for (int i = 0; i < NumDimensions; i++) {
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
  DistribSpec_t _distspec;
  /// Team containing the units to which the patterns element are mapped
  dash::Team &       _team       = dash::Team::All();
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
  /// Maximum extents of a block in this pattern.
  BlockSizeSpec_t    _blocksize_spec;
  /// Local extents of the pattern in all dimensions
  size_t             _local_extent[NumDimensions];
  /// Actual number of elements local to the active unit
  size_t             _local_size = 1;
  /// Total amount of units to which this pattern's elements are mapped
  size_t             _nunits     = dash::Team::All().size();
  /// Maximum number of elements in a single block
  size_t             _max_blocksize;

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
    size_t arg, Args && ... args)
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
    const SizeSpec_t &         sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistribSpec_t & dist      = DistribSpec_t(),
    /// Cartesian arrangement of units within the team
    const TeamSpec_t &         teamorg   = TeamSpec_t::TeamSpec(),
    /// Team containing units to which this pattern maps its elements
    dash::Team &               team      = dash::Team::All()) 
  : _distspec(dist),
    _memory_layout(sizespec),
    _team(team),
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
    const SizeSpec_t &        sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistribSpec_t & dist    = DistribSpec_t(),
    /// Team containing units to which this pattern maps its elements
    Team &                     team    = dash::Team::All())
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
    _nunits(other._nunits),
    _local_size(other._local_size),
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
  long long unit_at(
    /// Absolute coordinates of the point
    const std::array<long long, NumDimensions> & coords,
    /// View specification (offsets) to apply on \c coords
    const ViewSpec_t & viewspec) const {
    // Apply viewspec offsets to coordinates:
    std::array<long long, NumDimensions> vs_coords;
    for (int d = 0; d < NumDimensions; ++d) {
      vs_coords[d] = coords[d] + viewspec[d].offset;
    }
    // Index of block containing the given coordinates:
    std::array<long long, NumDimensions> block_coords = 
      coords_to_block_coords(vs_coords);
    // Unit assigned to this block index:
    long long unit_id = block_coords_to_unit(block_coords);
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
  long long unit_at(
    /// Absolute coordinates of the point
    Values ... values
  ) const {
    std::array<long long, NumDimensions> inputindex = { values... };
    return unit_at(inputindex, _viewspec);
  }

  /**
   * Convert given relative coordinates to local offset.
   */
  long long local_at(
    /// Point in local memory
    const std::array<long long, NumDimensions> & coords,
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
  long long local_extent(size_t dim) const {
    if (dim >= NumDimensions || dim < 0); {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong dimension for Pattern::local_extent. "
        << "Expected dimension between 0 and " << NumDimensions-1 << ", "
        << "got" << dim);
    }
    // TODO
    return _local_extent[dim];
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit in total.
   *
   * \see blocksize()
   * \see local_extent()
   */
  long long local_size() const {
    // TODO
    return _local_size;
  }

  /**
   * Will be renamed to \c local_to_global_index
   *
   * \see index_to_elem Inverse of unit_and_elem_to_index
   */
  long long unit_and_elem_to_index(
    size_t unit,
    size_t elem) {
    // TODO
    return 0;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   * 
   * Will be renamed to \c unit_at_coords.
   */
  size_t index_to_unit(
    const std::array<long long, NumDimensions> & input) const {
    return unit_at(input, _viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * 
   * Will be renamed to \c coords_to_local_index.
   */
  size_t index_to_elem(
    std::array<long long, NumDimensions> input) const {
    return index_to_elem(input, _viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * 
   * Will be renamed to \c coords_to_local_index.
   */
  size_t index_to_elem(
    const std::array<long long, NumDimensions> & coords,
    const ViewSpec_t & viewspec) const {
    // Convert coordinates to linear global index respective to memory
    // order:
    long long glob_index = _memory_layout.at(coords);
    std::array<long long, NumDimensions> block_coords =
      coords_to_block_coords(coords);
    // Global index of block start point:
    std::array<long long, NumDimensions> block_begin_coords(block_coords);
    // Input coords transformed to their offset within the block:
    std::array<long long, NumDimensions> relative_coords(coords);
    for (int d = 0; d < NumDimensions; ++d) {
      block_begin_coords[d] *= blocksize(d);
      relative_coords[d] -= block_begin_coords[d];
    }
    // Block offset, i.e. number of blocks in front of referenced block:
    size_t block_offset       = _blockspec.at(block_coords);
    // Offset of the referenced block within all blocks local to its unit,
    // i.e. nth block of this unit:
    size_t local_block_offset = block_offset / _teamspec.size();
    // Local offset of the first element in the block:
    // TODO: Should be actual_block_size instead of max_blocksize
    size_t block_base_offset  = local_block_offset * max_blocksize();
    // Offset of the referenced index within its block:
    size_t elem_block_offset  = _blocksize_spec.at(relative_coords);
    // Local offset of the element with all of the unit's local elements:
    size_t local_elem_offset  = block_base_offset + elem_block_offset;
    DASH_LOG_TRACE("Pattern.index_to_elem",
                   "block offset", block_offset,
                   "local block offset", local_block_offset,
                   "block base offset", block_base_offset,
                   "elem block offset", elem_block_offset,
                   "local elem offset", local_elem_offset);

    return local_elem_offset;
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   */
  size_t at(
    const std::array<long long, NumDimensions> & coords,
    const ViewSpec_t & viewspec) const {
    return index_to_elem(coords, _viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   */
  template<typename ... Values>
  size_t at(Values ... values) const {
    static_assert(
      sizeof...(values) == NumDimensions,
      "Wrong parameter number");
    std::array<long long, NumDimensions> inputindex = { 
      (long long)values...
    };
    return index_to_elem(inputindex, _viewspec);
  }

  /**
   * Whether the given dimension offset involves any local part
   */
  bool is_local(
    size_t index,
    size_t unit_id,
    size_t dim,
    const ViewSpec<NumDimensions> & viewspec) const {
    // TODO
    return true;
  }

  /**
   * Maximum number of elements in a single block in the given dimension.
   *
   * \param  dim  The dimension in the pattern
   * \return  The blocksize in the given dimension
   */
  size_t blocksize(size_t dimension) const {
    return _blocksize_spec.extent(dimension);
  }

  /**
   * Maximum number of elements in a single block in all dimensions.
   *
   * \return  The maximum number of elements in a single block assigned to
   *          a unit.
   */
  size_t max_blocksize() const {
    return _max_blocksize;
  }

  /**
   * Maximum number of elements assigned to a single unit in total,
   * equivalent to the local capacity of every unit in this pattern.
   */
  size_t max_elem_per_unit() const {
    size_t max_elements = 1;
    for (int d = 0; d < NumDimensions; ++d) {
      size_t num_units = units_in_dimension(d);
      Distribution dist = _distspec[d];
      // Block size in given dimension:
      auto dim_max_blocksize = blocksize(d);
      // Maximum number of occurrences of a single unit in given dimension:
      size_t dim_num_blocks  = dist.max_local_blocks_in_range(
                                 // size of range:
                                 _memory_layout.extent(d),
                                 // number of blocks:
                                 num_units
                               );
      max_elements *= dim_max_blocksize * dim_num_blocks;
      DASH_LOG_TRACE("Pattern.max_elem_per_unit",
                     "Dim", d, "nunits", num_units,
                     "dim_max_blocksize", dim_max_blocksize,
                     "dim_num_blocks", dim_num_blocks,
                     "max_elements", max_elements);
    }
    return max_elements;
  }

  /**
   * The number of units to which this pattern's elements are mapped.
   */
  long long num_units() const {
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
  long long capacity() const {
    return _memory_layout.size();
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  dash::Team & team() const {
    return _team;
  }

  DistribSpec_t distspec() const {
    return _distspec;
  }

  SizeSpec_t sizespec() const {
    return SizeSpec_t(_memory_layout.extents());
  }

  const MemoryLayout_t & memory_layout() const {
    return _memory_layout;
  }

  TeamSpec_t teamspec() const {
    return _teamspec;
  }

  const ViewSpec_t & viewspec() const {
    return _viewspec;
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates.
   *
   * \see at(...) Inverse of coords(index)
   */
  std::array<long long, NumDimensions> coords(long long index) const {
    return _memory_layout.coords(index);
  }

private:
  /**
   * Initialize block- and block size specs from memory layout, team spec
   * and distribution spec. Called from constructors.
   */
  void initialize_block_specs() {
    // Number of blocks in all dimensions:
    std::array<size_t, NumDimensions> n_blocks;
    // Extents of a single block:
    std::array<size_t, NumDimensions> s_blocks;
    for (int d = 0; d < NumDimensions; ++d) {
      Distribution dist = _distspec[d];
      size_t blocksize = dist.max_blocksize_in_range(
        _memory_layout.extent(d), // size of range (extent)
        _teamspec.extent(d));     // number of blocks (units)
      size_t num_blocks = dash::math::div_ceil(
        _memory_layout.extent(d),
        blocksize);
      n_blocks[d]     = num_blocks;
      s_blocks[d]     = blocksize;
    }
    _blockspec      = BlockSpec_t(n_blocks);
    _blocksize_spec = BlockSizeSpec_t(s_blocks);
    _max_blocksize  = _blocksize_spec.size();
  }

  /**
   * Number of elements in the overflow block of given dimension, with
   * 0 <= \c overflow_blocksize(d) < blocksize(d).
   */
  size_t overflow_blocksize(int dimension) const {
    return _memory_layout.extent(dimension) % blocksize(dimension);
  }

  /**
   * Number of elements missing in the overflow block of given dimension
   * compared to the regular blocksize (\see blocksize(d)), with
   * 0 <= \c underflow_blocksize(d) < blocksize(d).
   */
  size_t underflow_blocksize(int dimension) const {
    // Underflow blocksize = regular blocksize - overflow blocksize:
    auto regular_blocksize = blocksize(dimension);
    return regular_blocksize - 
             (_memory_layout.extent(dimension) % regular_blocksize) %
               regular_blocksize;
  }

  /**
   * The number of blocks in the given dimension.
   */
  size_t num_blocks(int dimension) const {
    return _blockspec[dimension];
  }

  /**
   * Convert global index coordinates to the coordinates of its block in
   * the pattern.
   */
  std::array<long long, NumDimensions> coords_to_block_coords(
    const std::array<long long, NumDimensions> & coords) const {
    std::array<long long, NumDimensions> block_coords;
    for (int d = 0; d < NumDimensions; ++d) {
      block_coords[d] = coords[d] / blocksize(d);
    }
    return block_coords;
  }

  /**
   * Resolve the associated unit id of the given block.
   */
  size_t block_coords_to_unit(
    std::array<long long, NumDimensions> & block_coords) const {
    size_t unit_id = 0;
    for (int d = 0; d < NumDimensions; ++d) {
      Distribution dist = _distspec[d];
      unit_id += dist.block_coord_to_unit_offset(
                    block_coords[d], // block coordinate
                    d,               // dimension
                    _teamspec.size() // number of units
                 );
    }
    // TODO: Use _teamspec to resolve actual unit id
    return unit_id;
  }

  size_t units_in_dimension(int dimension) const {
    return _teamspec.extent(dimension);
  }
};

template<typename PatternIterator>
void forall(
  PatternIterator begin,
  PatternIterator end,
  ::std::function<void(long long)> func) {
  auto range   = end - begin;
  auto pattern = begin.pattern();
  for (long long i = 0;
       i < pattern.sizespec.size() && range > 0;
       ++i, --range) {
    long long idx = pattern.unit_and_elem_to_index(
      pattern.team().myid(), i);
    if (idx < 0) {
      break;
    }
    func(idx);
  }
}

} // namespace dash

#endif // DASH__PATTERN_H_
