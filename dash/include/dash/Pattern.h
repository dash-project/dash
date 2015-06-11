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
    }
  
    SizeSpec_t sizespec() const {
      return _sizespec;
    }
    DistributionSpec_t distspec() const {
      return _distspec;
    }
    TeamSpec_t teamspec() const {
      return _teamspec;
    }
    ViewSpec_t viewspec() const {
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
      _teamspec = teamSpec;
    }
    /// Pattern matching for one optional parameter specifying the 
    /// team.
    template<int count>
    void check(dash::Team & team) {
      _argc_team += NumDimensions;
      _teamspec   = TeamSpec<NumDimensions>(team);
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
  size_t             m_local_capacity = 1;
  size_t             m_nunits         = dash::Team::All().size();
  size_t             m_blocksz;
  dash::Team &       m_team           = dash::Team::All();

public:
  template<typename ... Args>
  Pattern(Args && ... args)
  : m_arguments(args...),
    m_distspec(m_arguments.distspec()), 
    m_teamspec(m_arguments.teamspec()), 
    m_memory_layout(m_arguments.sizespec()), 
    m_viewspec(m_arguments.viewspec()) {
    m_nunits = m_teamspec.size();

    checkValidDistEnum();
    checkTile();
  }

  Pattern(
    const SizeSpec_t & sizespec,
    const DistributionSpec_t & dist = DistributionSpec_t(), 
    const TeamSpec_t & teamorg      = TeamSpec_t::TeamSpec(),
    dash::Team & team               = dash::Team::All()) 
  : m_memory_layout(sizespec),
    m_distspec(dist),
    m_teamspec(teamorg),
    m_team(team) {
    m_nunits   = m_team.size();
    m_viewspec = ViewSpec_t(m_memory_layout);
    checkValidDistEnum();
    checkTile();
  }

  Pattern(
    const SizeSpec_t & sizespec,
    const DistributionSpec_t & dist = DistributionSpec_t(),
    dash::Team & team               = dash::Team::All())
  : m_memory_layout(sizespec),
    m_distspec(dist),
    m_teamspec(m_team) {
    m_team     = team;
    m_nunits   = m_team.size();
    m_viewspec = ViewSpec<NumDimensions>(m_memory_layout);
    checkValidDistEnum();
    checkTile();
  }

  template<typename ... values>
  long long atunit(values ... Values) const {
    assert(sizeof...(Values) == NumDimensions);
    std::array<long long, NumDimensions> inputindex = { Values... };
    return atunit_(inputindex, m_viewspec);
  }

  long long local_extent(size_t dim) const {
    assert(dim < NumDimensions && dim >= 0);
    return m_local_extent[dim];
  }

  long long local_size() const {
    return m_local_capacity;
  }

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

  long long num_units() const {
    return m_teamspec.size();
  }

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
   * The Team containing the units this Pattern's index range is mapped to.
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
   * Number of elements in the block in given dimension.
   *
   * \param  dim  The dimension in the pattern
   * \return  The blocksize in the given dimension
   */
  long long blocksize(size_t dim) const {
    // TODO
    return 0;
  }

  /**
   * Whether the given dimension offset involves any local part
   */
  bool is_local(
    size_t dim,
    size_t index,
    ViewSpec<NumDimensions> & viewspec) {
  }
  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  std::array<long long, NumDimensions> coords(long long offs) const {
    return m_memory_layout.coords(offs);
  }

private:
  long long modulo(const long long i, const long long k) const {
    long long res = i % k;
    if (res < 0)
      res += k;
    return res;
  }

  long long divCeil(const long long i, const long long k) const {
    if (i % k == 0)
      return i / k;
    else
      return i / k + 1;
  }

  long long divFloor(const long long i, const long long k) const {
    return i / k;
  }

  // check pattern constraints for tile
  void checkTile() const {
    int hastile = 0;
    int invalid = 0;
    for (int i = 0; i < NumDimensions-1; i++) {
      if (m_distspec.dim(i).type == DistEnum::disttype::TILE)
        hastile = 1;
      if (m_distspec.dim(i).type != m_distspec.dim(i+1).type)
        invalid = 1;
    }
    assert(!(hastile && invalid));
    if (hastile) {
      for (int i = 0; i < NumDimensions; i++) {
        assert(
          m_memory_layout.extent(i) % (m_distspec.dim(i).blocksz)
          == 0);
      }
    }
  }

  void checkValidDistEnum() {
    int n_validdist = 0;
    for (int i = 0; i < NumDimensions; i++) {
      if (m_distspec.dim(i).type != DistEnum::disttype::NONE)
        n_validdist++;
    }
    assert(n_validdist == m_teamspec.rank());
  }
};

#if 0
template<typename PatternIterator>
void forall(
  PatternIterator begin,
  PatternIterator end,
  ::std::function<void(long long)> func) {
  auto pattern = begin.container().pattern();
  for (long long i = 0; i < pattern.sizespec.size(); i++) {
    long long idx = pattern.unit_and_elem_to_index(
      pattern.team().myid(), i);
    if (idx < 0) {
      break;
    }
    func(idx);
  }
}
#endif

} // namespace dash

#endif // DASH__PATTERN_H_
