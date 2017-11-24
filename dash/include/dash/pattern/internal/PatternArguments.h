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

  /// The extents of the pattern space in every dimension
  SizeSpec_t           _sizespec;
  /// The distribution type for every pattern dimension
  DistributionSpec_t   _distspec;
  /// The set of individual distributions provided
  std::array<Distribution, NumDimensions> _dists;
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
    parse<0, 0, 0, 0>(std::forward<Args>(args)...);
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
  /*
   * Match for up to \c NumDimensions extent value of type SizeType.
   *
   * \tparam ArgcSize     The number of arguments describing the extents of the
   *                      pattern parsed so far.
   *                      Set to -1 if a \c SizeSpec is encountered.
   * \tparam ArgcDist     The number of arguments describing the distribution
   *                      of the pattern parsed so far.
   *                      Set to -1 if a \c DistributionSpec is encountered.
   * \tparam ArgcTeam     The number of arguments describing the team passed to
   *                      the pattern parsed so far. Can be 0 or 1.
   * \tparam ArgcTeamSpec The number of arguments describing the team/unit
   *                      distribution (TeamSpec) of the pattern parsed so far.
   *                      Can be 0 or 1.
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec,
    typename ... Args>
  void parse(SizeType extent, Args && ... args) {
    static_assert(ArgcSize >= 0, "Cannot mix size and SizeSpec definition"
        "in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(extent)", extent);
    static_assert(ArgcSize < NumDimensions, "Number of size specifier exceeds"
        "the number of dimensions in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(extent)", extent);
    _sizespec.resize(ArgcSize, extent);
    parse<ArgcSize+1, ArgcDist, ArgcTeam,
      ArgcTeamSpec>(std::forward<Args>(args)...);
  }

  /*
   * Match for one optional parameter specifying the size (extents).
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec,
    typename ... Args>
  void parse(const SizeSpec_t & sizeSpec, Args && ... args) {
    static_assert(ArgcSize == 0, "Cannot mix size and SizeSpec definition"
        "in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(sizeSpec)");
    _sizespec = sizeSpec;
    parse<-1, ArgcDist, ArgcTeam,
      ArgcTeamSpec>(std::forward<Args>(args)...);
  }

  /*
   * Match for \c TeamSpec describing the distribution among the units
   * in the team.
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec,
    typename ... Args>
  void parse(const TeamSpec_t & teamSpec, Args && ... args) {
    static_assert(ArgcTeamSpec == 0,
        "Cannot specify TeamSpec twice in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(teamSpec)");
    // TODO: there is no way to check whether this TeamSpec
    //       was created from the team provided in the variadic arguments.
    _teamspec   = teamSpec;
    parse<ArgcSize, ArgcDist, ArgcTeam, 1>(std::forward<Args>(args)...);
  }

  /*
   * Match for one optional parameter specifying the team.
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec,
    typename ... Args>
  void parse(dash::Team & team, Args && ... args) {
    static_assert(ArgcTeam == 0,
        "Cannot specify Team twice in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(team)");
    // assign team, TeamSpec will be created when parsing is finished
    // and no TeamSpec has been found.
    _team     = &team;
    parse<ArgcSize, ArgcDist, 1,
      ArgcTeamSpec>(std::forward<Args>(args)...);
  }

  /*
   * Match for one optional parameter specifying the  distribution.
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec,
    typename ... Args>
  void parse(const DistributionSpec_t & ds, Args && ... args) {
    static_assert(ArgcDist == 0, "Cannot mix DistributionSpec and inidividual"
        "distributions in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(distSpec)");
    _distspec = ds;
    parse<ArgcSize, -1, ArgcTeam,
      ArgcTeamSpec>(std::forward<Args>(args)...);
  }

  /*
   * Match for up to \c NumDimensions optional parameters
   * specifying the distribution.
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec,
    typename ... Args>
  void parse(const Distribution & ds, Args && ... args) {
    static_assert(!(ArgcDist < 0), "Cannot mix DistributionSpec and "
        "inidividual distributions in variadic pattern constructor!");
    static_assert(ArgcDist < NumDimensions, "Number of distribution specifier"
        " exceeds the number of dimensions in variadic pattern constructor!");
    DASH_LOG_TRACE("PatternArguments.check(dist)");
    _dists[ArgcDist] = ds;
    parse<ArgcSize, ArgcDist+1, ArgcTeam,
      ArgcTeamSpec>(std::forward<Args>(args)...);
  }

  /**
   * Stop recursion when all arguments are processed.
   */
  template<
    int ArgcSize,
    int ArgcDist,
    int ArgcTeam,
    int ArgcTeamSpec>
  void parse() {
    static_assert(!(ArgcDist > 0 && ArgcDist != NumDimensions),
        "Incomplete distribution specification in "
        "variadic pattern constructor!");

    static_assert(!(ArgcSize > 0 && ArgcSize != NumDimensions),
        "Incomplete size specification in "
        "variadic pattern constructor!");

    if (ArgcDist > 0) {
      _distspec = DistributionSpec_t(_dists);
    }

    check_tile_constraints();

    /*
     * TODO: we have no way to statically check whether a TeamSpec was created
     *       using the team specified by the user.
     */
    /* Create a teamspec if none is provided */
    if (!ArgcTeamSpec) {
      create_team_spec();
    }
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
        DASH_ASSERT_MSG_ALWAYS(
          _sizespec.extent(i) % (_distspec.dim(i).blocksz) == 0,
          "Extent must match blocksize in each dimension!");
      }
    }
  }

  void create_team_spec() {
    // count the number of none-NONE dist specs
    int num_explicit_dist = 0;
    int explicit_dist_pos = -1;
    for (auto i = 0; i < NumDimensions; i++) {
      if (_distspec.dim(i).type != dash::internal::DIST_NONE) {
        ++num_explicit_dist;
        explicit_dist_pos = i;
      }
    }

    /* infer a teamspec if possible or error out */
    if (num_explicit_dist == NumDimensions) {
      // balance extents if all dimensions have an explicit distribution
      _teamspec = TeamSpec_t(this->team());
      _teamspec.balance_extents();
    } else if (num_explicit_dist == 1) {
      // create a TeamSpec with the specific dimension set
      std::array<Distribution, NumDimensions> dists;
      dists[explicit_dist_pos].type = _distspec.dim(explicit_dist_pos).type;
      DistributionSpec_t distspec(dists);
      _teamspec = TeamSpec_t(distspec, this->team());
    } else if (num_explicit_dist > 1) {
      DASH_ASSERT_MSG_ALWAYS(
        num_explicit_dist == 1 || num_explicit_dist == NumDimensions,
        "Cannot infer TeamSpec from mixed DistributionSpec"
      );
    }
  }
};


} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__PATTERN_ARGUMENTS_H_
