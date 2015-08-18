#ifndef DASH__INTERNAL__PATTERN_ARGUMENTS_H_
#define DASH__INTERNAL__PATTERN_ARGUMENTS_H_

#include <dash/Types.h>
#include <dash/Enums.h>
#include <dash/Team.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/Distribution.h>
#include <dash/Exception.h>
#include <dash/internal/Math.h>
#include <dash/internal/Logging.h>

namespace dash {
namespace internal {

/**
 * Extracting size-, distribution- and team specifications from
 * arguments passed to pattern varargs constructors.
 *
 * \see  Pattern<typename ... Args>::Pattern(Args && ... args)
 * \see  DashPatternConcept
 */
template<
  dim_t NumDimensions,
  typename IndexType = dash::default_index_t>
class PatternArguments {
private:
  /// Derive size type from given signed index / ptrdiff type
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef DistributionSpec<NumDimensions>
    DistributionSpec_t;
  typedef TeamSpec<NumDimensions, IndexType>
    TeamSpec_t;
  typedef SizeSpec<NumDimensions, SizeType>
    SizeSpec_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;

private:
  /// The extents of the pattern space in every dimension
  SizeSpec_t           _sizespec;
  /// The distribution type for every pattern dimension
  DistributionSpec_t   _distspec;
  /// The cartesian arrangement of the units in the team to which the
  /// patterns element are mapped
  TeamSpec_t           _teamspec;
  /// The view specification of the pattern, consisting of offset and
  /// extent in every dimension
  ViewSpec_t           _viewspec;
  /// Team containing all units to which pattern elements are mapped
  Team               * _team      = nullptr;
  /// Number of distribution specifying arguments in varargs
  int                  _argc_dist = 0;
  /// Number of size/extent specifying arguments in varargs
  int                  _argc_size = 0;
  /// Number of team specifying arguments in varargs
  int                  _argc_team = 0;

public:
  /**
   * Default constructor, used if no argument list is parsed.
   */
  PatternArguments() {
  }

  /**
   * Constructor, parses settings in argument list and checks for
   * constraints.
   */
  template<typename ... Args>
  PatternArguments(Args && ... args) {
    static_assert(
      sizeof...(Args) >= NumDimensions,
      "Invalid number of arguments for PatternArguments");
    // Parse argument list:
    check_recurse<0>(std::forward<Args>(args)...);
    // Validate number of arguments after parsing:
    if (_argc_size > 0 && _argc_size != NumDimensions) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Invalid number of size arguments for BlockPattern(...), " <<
        "expected " << NumDimensions << ", got " << _argc_size);
    }
    if (_argc_dist > 0 && _argc_dist != NumDimensions) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Invalid number of dist arguments for BlockPattern(...), " <<
        "expected " << NumDimensions << ", got " << _argc_dist);
    }
    check_tile_constraints();
  }

  bool is_tiled() const {
    for (auto d = 0; d < NumDimensions; ++d) {
      if (_distspec[d].is_tiled()) {
        return true;
      }
    }
    return false;
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
  Team & team() const {
    if (_team == nullptr) {
      return dash::Team::All();
    }
    return *_team;
  }

private:
  /// BlockPattern matching for extent value of type IndexType.
  template<int count>
  void check(SizeType extent) {
    DASH_LOG_TRACE("PatternArguments.check(extent)", extent);
    _argc_size++;
    _sizespec.resize(count, extent);
  }
  /// BlockPattern matching for up to \c NumDimensions optional 
  /// parameters specifying the distribution pattern.
  template<int count>
  void check(const TeamSpec_t & teamSpec) {
    DASH_LOG_TRACE("PatternArguments.check(teamSpec)");
    _argc_team++;
    _teamspec   = teamSpec;
  }
  /// BlockPattern matching for one optional parameter specifying the 
  /// team.
  template<int count>
  void check(dash::Team & team) {
    DASH_LOG_TRACE("PatternArguments.check(team)");
    if (_argc_team == 0) {
      _team     = &team;
      _teamspec = TeamSpec_t(_distspec, team);
    }
  }
  /// BlockPattern matching for one optional parameter specifying the 
  /// size (extents).
  template<int count>
  void check(const SizeSpec_t & sizeSpec) {
    DASH_LOG_TRACE("PatternArguments.check(sizeSpec)");
    _argc_size += NumDimensions;
    _sizespec   = sizeSpec;
  }
  /// BlockPattern matching for one optional parameter specifying the 
  /// distribution.
  template<int count>
  void check(const DistributionSpec_t & ds) {
    DASH_LOG_TRACE("PatternArguments.check(distSpec)");
    _argc_dist += NumDimensions;
    _distspec   = ds;
  }
  /// BlockPattern matching for up to NumDimensions optional parameters
  /// specifying the distribution.
  template<int count>
  void check(const Distribution & ds) {
    DASH_LOG_TRACE("PatternArguments.check(dist)");
    _argc_dist++;
    dim_t dim = count - NumDimensions;
    _distspec[dim] = ds;
  }
  /// Isolates first argument and calls the appropriate check() function
  /// on each argument via recursion on the argument list.
  template<int count, typename T, typename ... Args>
  void check_recurse(T && t, Args && ... args) {
    DASH_LOG_TRACE("PatternArguments.check(args) ",
                   "count", count,
                   "argc", sizeof...(Args));
    check<count>(std::forward<T>(t));
    if (sizeof...(Args) > 0) {
      check_recurse<count + 1>(std::forward<Args>(args)...);
    }
  }
  /// Terminator function for recursive argument parsing
  template<int count>
  void check_recurse() {
  }
  /// Check pattern constraints for tile
  void check_tile_constraints() const {
    bool has_tile = false;
    bool invalid  = false;
    for (auto i = 0; i < NumDimensions-1; i++) {
      if (_distspec.dim(i).type == dash::internal::DIST_TILE)
        has_tile = true;
      if (_distspec.dim(i).type != _distspec.dim(i+1).type)
        invalid = true;
    }
    assert(!(has_tile && invalid));
    if (has_tile) {
      for (auto i = 0; i < NumDimensions; i++) {
        assert(
          _sizespec.extent(i) % (_distspec.dim(i).blocksz)
          == 0);
      }
    }
  }
};

} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__PATTERN_ARGUMENTS_H_
