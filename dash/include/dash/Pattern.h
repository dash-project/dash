#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>

#include <dash/Enums.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>

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
  typedef CartCoord<NumDimensions, Arrangement>  MemoryLayout_t;
  typedef ViewSpec<NumDimensions>                ViewSpec_t;

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
      _argc_team += teamSpec.rank();
      _teamspec   = teamSpec;
    }
    /// Pattern matching for one optional parameter specifying the 
    /// team.
    template<int count>
    void check(dash::Team & team) {
      _teamspec = TeamSpec<NumDimensions>(team);
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
  ArgumentParser     m_arguments;
  DistributionSpec_t m_distspec;
  TeamSpec_t         m_teamspec;
  MemoryLayout_t     m_memory_layout;
  ViewSpec_t         m_viewspec;

private:
  /// The local offsets of the pattern in all dimensions
  long long          m_local_offset[NumDimensions];
  /// The local extents of the pattern in all dimensions
  size_t             m_local_extent[NumDimensions];
  /// The global extents of the pattern in all dimensions
  size_t             m_extent[NumDimensions];
  size_t             m_local_size = 1;
  size_t             m_nunits     = dash::Team::All().size();
  size_t             m_blocksz;
  dash::Team &       m_team       = dash::Team::All();

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
  : m_arguments(args...),
    m_distspec(m_arguments.distspec()), 
    m_teamspec(m_arguments.teamspec()), 
    m_memory_layout(m_arguments.sizespec()), 
    m_viewspec(m_arguments.viewspec()) {
    m_nunits = m_teamspec.size();
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
    const TeamSpec_t & teamorg      = TeamSpec_t::TeamSpec(),
    dash::Team & team               = dash::Team::All()) 
  : m_memory_layout(sizespec),
    m_distspec(dist),
    m_teamspec(teamorg),
    m_team(team) {
    m_nunits   = m_team.size();
    m_viewspec = ViewSpec_t(m_memory_layout);
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
    dash::Team & team               = dash::Team::All())
  : m_memory_layout(sizespec),
    m_distspec(dist),
    m_teamspec(m_team) {
    m_team     = team;
    m_nunits   = m_team.size();
    m_viewspec = ViewSpec<NumDimensions>(m_memory_layout);
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   * TODO: Will be renamed to \c unit_at_coord.
   */
  template<typename ... Values>
  long long atunit(Values ... values) const {
    assert(sizeof...(Values) == NumDimensions);
    std::array<long long, NumDimensions> inputindex = { values... };
    return atunit_(inputindex, m_viewspec);
  }

  long long local_extent(size_t dim) const {
    assert(dim < NumDimensions && dim >= 0);
    return m_local_extent[dim];
  }

  /**
   * The actual number of elements in this pattern that are local to the
   * calling unit.
   *
   * \see blocksize()
   */
  long long local_size() const {
    return m_local_size;
  }

  /**
   * TODO: Will be renamed to \c local_to_global_index
   */
  long long unit_and_elem_to_index(
    size_t unit,
    size_t elem) {
    // TODO
  }

  size_t max_elem_per_unit() const {
    // TODO
    return 0;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   * TODO: Will be renamed to \c unit_at_coord.
   */
  size_t index_to_unit(
    std::array<long long, NumDimensions> input) const {
    // return atunit_(input, m_viewspec);
    // TODO
    return 0;
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * TODO: Will be renamed to \c coord_to_local_index.
   */
  size_t index_to_elem(
    std::array<long long, NumDimensions> input) const {
    // return at_(input, m_viewspec);
    // TODO
    return 0;
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * TODO: Will be renamed to \c coord_to_local_index.
   */
  size_t index_to_elem(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> & viewspec) const {
    // return at_(input, viewspec);
    // TODO
    return 0;
  }

  template<typename ... values>
  long long at(values ... Values) const {
    static_assert(
      sizeof...(Values) == NumDimensions,
      "Wrong parameter number");
    std::array<long long, NumDimensions> inputindex = { Values... };
    return at_(inputindex, m_viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear global index.
   * TODO: Will be renamed to \c coord_to_index.
   */
  long long glob_index_to_elem(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> & viewspec) const {
    // TODO
    return 0;
  }

  /**
   * Maximum number of elements in the block in given dimension, equivalent
   * to the local capacity of every unit in this pattern.
   *
   * \param  dim  The dimension in the pattern
   * \return  The blocksize in the given dimension
   */
  long long blocksize(size_t dimension) const {
    DistEnum dist = m_distspec[dimension];
    switch (dist.type) {
      case DistEnum::disttype::NONE:
        return m_memory_layout.extent(dimension);
      case DistEnum::disttype::BLOCKED:
        return div_ceil(m_memory_layout.extent(dimension),
                        m_teamspec[dimension]);
      case DistEnum::disttype::CYCLIC:
        return 1;
      case DistEnum::disttype::BLOCKCYCLIC:
        return dist.blocksz;
      case DistEnum::disttype::TILE:
        return dist.blocksz;
      default:
        return -1;
    }
    // return dist.blocksize_of_range(
    //          m_extent[dimension], // rangesize
    //          num_units()          // number of blocks
    //        );
  }

  /**
   * Whether the given dimension offset involves any local part
   */
  bool is_local(
    size_t dim,
    size_t index,
    ViewSpec<NumDimensions> & viewspec) {
    // TODO
    return true;
  }

/// DONE ////

  /**
   * The number of units to which this pattern's elements are mapped.
   */
  long long num_units() const {
    return m_teamspec.size();
  }

  /**
   * Assignment operator.
   */
  Pattern & operator=(const Pattern & other) {
    if (this != &other) {
      m_distspec      = other.m_distspec;
      m_teamspec      = other.m_teamspec;
      m_memory_layout = other.m_memory_layout;
      m_viewspec      = other.m_viewspec;
      m_nunits        = other.m_nunits;
      m_blocksz       = other.m_blocksz;
    }
    return *this;
  }

  /**
   * The maximum number of elements arranged in this pattern.
   */
  long long capacity() const {
    return m_memory_layout.size();
  }

  /**
   * The Team containing the units to which this pattern's elements are
   * mapped.
   */
  dash::Team & team() const {
    return m_team;
  }

  DistributionSpec<NumDimensions> distspec() const {
    return m_distspec;
  }

  SizeSpec<NumDimensions> sizespec() const {
    return m_memory_layout;
  }

  TeamSpec_t teamspec() const {
    return m_teamspec;
  }

  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  std::array<long long, NumDimensions> coords(long long index) const {
    return m_memory_layout.coords(index);
  }

private:
  std::array<long long, NumDimensions> coord_to_block_coord(
    std::array<long long, NumDimensions> & coord ) const {
    std::array<long long, NumDimensions> block_coord;
    for (int d = 0; d < NumDimensions; ++d) {
      block_coord[d] = coord[d] / blocksize(d);
    }
    return block_coord;
  }

  /**
   * Specify the memory layout's distribution in the given dimension.
   */
  void distribute(size_t dimension, DistEnum distribution) {
  }

  long long modulo(const long long i, const long long k) const {
    long long res = i % k;
    if (res < 0)
      res += k;
    return res;
  }

  long long div_ceil(const long long i, const long long k) const {
    if (i % k == 0)
      return i / k;
    else
      return i / k + 1;
  }

  long long div_floor(const long long i, const long long k) const {
    return i / k;
  }
};

template<typename FwdIterator>
void forall(
  FwdIterator begin,
  FwdIterator end,
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
