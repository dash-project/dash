#ifndef DASH__INTERNAL__PATTERN_ARGUMENTS_H_
#define DASH__INTERNAL__PATTERN_ARGUMENTS_H_

#include <utility>

#include <dash/Types.h>
#include <dash/Team.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>
#include <dash/TeamSpec.h>
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
public:
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
    // Parse argument list:
    check<0, 0, 0>(std::forward<Args>(args)...);
  }


public:


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
  template<int argc_size, int argc_dist, int has_team, typename ... Args>
  void check(SizeType extent, Args && ... args) {
    static_assert(argc_size >= 0, "Cannot mix size and SizeSpec definition"
        "in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(extent)", extent);
    static_assert(argc_size < NumDimensions, "Number of size specifier exceeds"
        "the number of dimensions in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(extent)", extent);
    _sizespec.resize(argc_size, extent);
    check<argc_size+1, argc_dist, has_team>(std::forward<Args>(args)...);
  }
  /// BlockPattern matching for up to \c NumDimensions optional 
  /// parameters specifying the distribution pattern.
  template<int argc_size, int argc_dist, int has_team, typename ... Args>
  void check(const TeamSpec_t & teamSpec, Args && ... args) {
    static_assert(has_team == 0,
        "Cannot mix Team and TeamSpec definition in variadic "
        "pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(teamSpec)");
    _teamspec   = teamSpec;
    check<argc_size, argc_dist, -1>(std::forward<Args>(args)...);
  }
  /// BlockPattern matching for one optional parameter specifying the 
  /// team.
  template<int argc_size, int argc_dist, int has_team, typename ... Args>
  void check(dash::Team & team, Args && ... args) {
    static_assert(!(has_team < 0),
        "Cannot mix Team and TeamSpec definition in variadic "
        "pattern constructor!");
    static_assert(has_team == 0,
        "Cannot specify Team twice in variadic "
        "pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(team)");
    _team     = &team;
    check<argc_size, argc_dist, 1>(std::forward<Args>(args)...);
  }
  /// BlockPattern matching for one optional parameter specifying the 
  /// size (extents).
  template<int argc_size, int argc_dist, int has_team, typename ... Args>
  void check(const SizeSpec_t & sizeSpec, Args && ... args) {
    static_assert(argc_size == 0, "Cannot mix size and SizeSpec definition"
        "in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(sizeSpec)");
    _sizespec   = sizeSpec;
    check<-1, argc_dist, has_team>(std::forward<Args>(args)...);
  }
  /// BlockPattern matching for one optional parameter specifying the 
  /// distribution.
  template<int argc_size, int argc_dist, int has_team, typename ... Args>
  void check(const DistributionSpec_t & ds, Args && ... args) {
    static_assert(argc_dist == 0, "Cannot mix DistributionSpec and inidividual"
        "distributions in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(distSpec)");
    _distspec   = ds;
    check<argc_size, -1, has_team>(std::forward<Args>(args)...);
  }
  /// BlockPattern matching for up to \c NumDimensions optional parameters
  /// specifying the distribution.
  template<int argc_size, int argc_dist, int has_team, typename ... Args>
  void check(const Distribution & ds, Args && ... args) {
    static_assert(!(argc_dist < 0), "Cannot mix DistributionSpec and "
        "inidividual distributions in variadic pattern constructor!");
    static_assert(argc_dist < NumDimensions, "Number of distribution specifier"
        " exceeds the number of dimensions in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(dist)");
    _distspec[argc_dist] = ds;
    check<argc_size, argc_dist+1, has_team>(std::forward<Args>(args)...);
  }

  /**
   * Stop recursion when all arguments are processed.
   */
  template<int argc_size, int argc_dist, int has_team>
  void check() {
    static_assert(!(argc_dist > 0 && argc_dist != NumDimensions),
        "Incomplete distribution specification in "
        "variadic pattern constructor!");

    static_assert(!(argc_size > 0 && argc_size != NumDimensions),
        "Incomplete size specification in "
        "variadic pattern constructor!");

    check_tile_constraints();
  }

  /// Check pattern constraints for tile
  void check_tile_constraints() const {
    bool has_tile = false;
    bool invalid  = false;
    for (auto i = 0; i < NumDimensions-1; i++) {
      if (_distspec.dim(i).type == dash::internal::DIST_TILE)
        has_tile = true;
      if (_distspec.dim(i).type != _distspec.dim(i+1).type)
        invalid  = true;
    }
    if (has_tile) {
      if (invalid) {
        DASH_THROW(dash::exception::InvalidArgument,
                   "Pattern arguments invalid: Mixed distribution types");
      }

      for (auto i = 0; i < NumDimensions; i++) {
        DASH_ASSERT_ALWAYS(
          _sizespec.extent(i) % (_distspec.dim(i).blocksz)
          == 0);
      }
    }
  }
};


} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__PATTERN_ARGUMENTS_H_
