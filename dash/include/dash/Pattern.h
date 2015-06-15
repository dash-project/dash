#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>

#include <dash/Enums.h>
#include <dash/Exception.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>
#include <dash/internal/Math.h>

namespace dash {

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 *
 * Consequently, a pattern realizes a projection of a global index
 * range to a local view:
 *
 * Distribution                 |      Container:
 * ---------------------------- | -----------------------------
 * <tt>[ team 0 | team 1 ]</tt> | <tt>[ 0  1  2  3  4  5 ]</tt>
 * <tt>[ team 1 | team 0 ]</tt> | <tt>[ 6  7  8  9 10 11 ]</tt>
 *  
 * This pattern would assign local indices to teams like this:
 * 
 * Team            | Local indices
 * --------------- | -----------------------------
 * <tt>team 0</tt> | <tt>[ 0  1  2  9 10 11 ]</tt>
 * <tt>team 1</tt> | <tt>[ 3  4  5  6  7  8 ]</tt>
 */
template<
  size_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR>
class Pattern {
private:
  typedef DistributionSpec<NumDimensions>        DistributionSpec_t;
  typedef TeamSpec<NumDimensions>                TeamSpec_t;
  typedef SizeSpec<NumDimensions>                SizeSpec_t;
  typedef ViewSpec<NumDimensions>                ViewSpec_t;
  typedef CartCoord<NumDimensions, Arrangement>  MemoryLayout_t;
  typedef CartCoord<NumDimensions, Arrangement>  BlockSpec_t;
  typedef CartCoord<NumDimensions, Arrangement>  BlockSizeSpec_t;

private:
  /**
   * Extracting size-, distribution- and team specifications from
   * arguments passed to Pattern varargs constructor.
   *
   * \see Pattern<typename ... Args>::Pattern(Args && ... args)
   */
  class ArgumentParser {
  private:
    SizeSpec_t         _sizespec;
    DistributionSpec_t _distspec;
    TeamSpec_t         _teamspec;
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
      if (_argc_team > 0 && _argc_team != NumDimensions) {
        DASH_THROW(
          dash::exception::InvalidArgument,
          "Invalid number of team spec arguments for Pattern(...), " <<
          "expected " << NumDimensions << ", got " << _argc_team);
      }
      check_tile_constraints();
      check_distribution_constraints();
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
//    _argc_team += teamSpec.rank();
      _teamspec   = teamSpec;
    }
    /// Pattern matching for one optional parameter specifying the 
    /// team.
    template<int count>
    void check(dash::Team & team) {
      _teamspec = TeamSpec<NumDimensions>(_distspec, team);
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
    void check(const DistEnum & ds) {
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
        if (_distspec.dim(i).type == DistEnum::disttype::TILE)
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
    /// Check pattern constraints on distribution specification.
    void check_distribution_constraints() const {
      int n_validdist = 0;
      for (int i = 0; i < NumDimensions; i++) {
        if (_distspec.dim(i).type != DistEnum::disttype::NONE)
          n_validdist++;
      }
      assert(n_validdist == _teamspec.rank());
    }
  };

private:
  ArgumentParser     _arguments;
  /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
  /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
  /// dimensions
  DistributionSpec_t _distspec;
  /// Cartesian arrangement of units within the team
  TeamSpec_t         _teamspec;
  MemoryLayout_t     _memory_layout;
  ViewSpec_t         _viewspec;
  /// Number of blocks in all dimensions.
  BlockSpec_t        _blockspec;
  /// Maximum extents of a block in this pattern.
  BlockSizeSpec_t    _blocksize_spec;

private:
  /// The local offsets of the pattern in all dimensions
  long long          m_local_offset[NumDimensions];
  /// The local extents of the pattern in all dimensions
  size_t             _local_extent[NumDimensions];
  /// The global extents of the pattern in all dimensions
  size_t             _extent[NumDimensions];
  size_t             _local_size = 1;
  size_t             _nunits     = dash::Team::All().size();
  size_t             _max_blocksize;
  dash::Team &       _team       = dash::Team::All();

public:
  /**
   * Constructor, initializes a pattern from an argument list consisting
   * of the pattern size (extent, number of elements) in every dimension 
   * followed by optional distribution types.
   *
   * Examples:
   *
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
   *
   */
  template<typename ... Args>
  Pattern(
    /// Argument list consisting of the pattern size (extent, number of 
    /// elements) in every dimension followed by optional distribution     
    /// types.
    Args && ... args)
  : _arguments(args...),
    _distspec(_arguments.distspec()), 
    _teamspec(_arguments.teamspec()), 
    _memory_layout(_arguments.sizespec()), 
    _viewspec(_arguments.viewspec()) {
    _nunits        = _teamspec.size();
    _max_blocksize = 1;
    // Number of blocks in all dimensions:
    std::array<size_t, NumDimensions> n_blocks;
    // Extents of a single block:
    std::array<size_t, NumDimensions> s_blocks;
    for (int d = 0; d < NumDimensions; ++d) {
      n_blocks[d]      = num_blocks(d);
      s_blocks[d]      = blocksize(d);
      _max_blocksize *= s_blocks[d];
    }
    _blockspec      = BlockSpec_t(n_blocks);
    _blocksize_spec = BlockSizeSpec_t(s_blocks);
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec, \c TeamSpec and a \c Team.
   *
   * Examples:
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
   */
  Pattern(
    /// Pattern size (extent, number of elements) in every dimension 
    const SizeSpec_t & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Cartesian arrangement of units within the team
    const TeamSpec_t & teamorg      = TeamSpec_t::TeamSpec(),
    /// Team containing units to which this pattern maps its elements
    dash::Team & team               = dash::Team::All()) 
  : _memory_layout(sizespec),
    _distspec(dist),
    _teamspec(_distspec, teamorg),
    _team(team) {
    _nunits   = _team.size();
    _viewspec = ViewSpec_t(_memory_layout.extents());
    _max_blocksize = 1;
    // Number of blocks in all dimensions:
    std::array<size_t, NumDimensions> n_blocks;
    // Extents of a single block:
    std::array<size_t, NumDimensions> s_blocks;
    for (int d = 0; d < NumDimensions; ++d) {
      n_blocks[d]      = num_blocks(d);
      s_blocks[d]      = blocksize(d);
      _max_blocksize *= s_blocks[d];
    }
    _blockspec      = BlockSpec_t(n_blocks);
    _blocksize_spec = BlockSizeSpec_t(s_blocks);
  }

  /**
   * Constructor, initializes a pattern from explicit instances of
   * \c SizeSpec, \c DistributionSpec and a \c Team.
   *
   * Examples:
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
   */
  Pattern(
    /// Pattern size (extent, number of elements) in every dimension 
    const SizeSpec_t & sizespec,
    /// Distribution type (BLOCKED, CYCLIC, BLOCKCYCLIC, TILE or NONE) of
    /// all dimensions. Defaults to BLOCKED in first, and NONE in higher
    /// dimensions
    const DistributionSpec_t & dist = DistributionSpec_t(),
    /// Team containing units to which this pattern maps its elements
    dash::Team & team               = dash::Team::All())
  : _memory_layout(sizespec),
    _distspec(dist),
    _teamspec(_distspec, _team) {
    _team          = team;
    _nunits        = _team.size();
    _viewspec      = ViewSpec<NumDimensions>(_memory_layout);
    _max_blocksize = 1;
    // Number of blocks in all dimensions:
    std::array<size_t, NumDimensions> n_blocks;
    // Extents of a single block:
    std::array<size_t, NumDimensions> s_blocks;
    for (int d = 0; d < NumDimensions; ++d) {
      n_blocks[d]      = num_blocks(d);
      s_blocks[d]      = blocksize(d);
      _max_blocksize *= s_blocks[d];
    }
    _blockspec      = BlockSpec_t(n_blocks);
    _blocksize_spec = BlockSizeSpec_t(s_blocks);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   */
  long long unit_at(
    const std::array<long long, NumDimensions> & coords,
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
    return block_coords_to_unit(block_coords);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   */
  template<typename ... Values>
  long long unit_at(Values ... values) const {
    if (sizeof...(Values) != NumDimensions) {
      DASH_THROW(
        dash::exception::OutOfRange,
        "Wrong number of arguments for Pattern::unit_at. "
        << "Expected " << NumDimensions << ", "
        << "got" << sizeof...(Values));
    }
    std::array<long long, NumDimensions> inputindex = { values... };
    return unit_at(inputindex, _viewspec);
  }

  long long local_at(
    const std::array<long long, NumDimensions> & coords,
    const ViewSpec_t & viewspec) const {
    // TODO
    return 0;
  }

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
   * calling unit.
   *
   * \see blocksize()
   */
  long long local_size() const {
    return _local_size;
  }

  /**
   * Inverse of \see index_to_elem.
   * Will be renamed to \c local_to_global_index
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
    // Resolve local element offset from local block offset:
    size_t local_elem_offset  = local_block_offset * max_blocksize();
    // Offset of the referenced index within its block:
    size_t elem_block_offset  = _blocksize_spec.at(relative_coords);
#if 0
    std::cout << "block offset: " << block_offset << ", "
              << "local block offset: " << local_block_offset << ", "
              << "local elem offset: " << local_elem_offset << ", "
              << "elem block offset: " << elem_block_offset
              << std::endl;
#endif
    return local_elem_offset + elem_block_offset;
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
    DistEnum dist = _distspec[dimension];
    return dist.blocksize_in_range(
      _memory_layout.extent(dimension), // size of range (extent)
      _teamspec.extent(dimension));     // number of blocks (units)
  }

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
      DistEnum dist = _distspec[d];
      // Block size in given dimension:
      auto dim_max_blocksize = dist.blocksize_in_range(
                                 // size of range:
                                 _memory_layout.extent(d),
                                 // number of blocks:
                                 num_units
                               );
      // Maximum number of occurrences of a single unit in given dimension:
      size_t dim_num_blocks = dash::math::div_ceil(
                                _memory_layout.extent(d),
                                dim_max_blocksize);
      // Accumulate result:
      max_elements *= (dim_num_blocks * dim_max_blocksize / num_units);
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

  DistributionSpec_t distspec() const {
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
   * Inverse of \see at(...).
   */
  std::array<long long, NumDimensions> coords(long long index) const {
    return _memory_layout.coords(index);
  }

private:
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
    size_t num_units = units_in_dimension(dimension);
    DistEnum dist = _distspec[dimension];
    // Block size in given dimension:
    auto dim_max_blocksize = dist.blocksize_in_range(
                            // size of range:
                            _memory_layout.extent(dimension),
                            // number of blocks:
                            num_units
                          );
    // Maximum number of occurrences of a single unit in given dimension:
    return dash::math::div_ceil(
             _memory_layout.extent(dimension),
             dim_max_blocksize);
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
      DistEnum dist = _distspec[d];
      unit_id += dist.block_coord_to_unit_offset(
                    block_coords[d], // block coordinate
                    d                // dimension
                 );
  //  unit_id %= _teamspec.extent(d);
    }
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
  auto pattern = begin.pattern();
  for (long long i = 0; i < pattern.sizespec.size(); i++) {
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
