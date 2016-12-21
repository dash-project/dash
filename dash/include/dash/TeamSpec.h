#ifndef DASH__TEAM_SPEC_H_
#define DASH__TEAM_SPEC_H_

#include <dash/Types.h>
#include <dash/Dimensional.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/internal/Logging.h>

#include <array>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstring>
#include <type_traits>

namespace dash {

/**
 * Specifies the arrangement of team units in a specified number
 * of dimensions.
 * Size of TeamSpec implies the number of units in the team.
 *
 * Reoccurring units are currently not supported.
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<
  dim_t    MaxDimensions,
  typename IndexType = dash::default_index_t>
class TeamSpec :
  public CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>
{
private:
  typedef typename std::make_unsigned<IndexType>::type
    SizeType;
  typedef TeamSpec<MaxDimensions, IndexType>
    self_t;
  typedef CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>
    parent_t;

public:
  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) with all team units organized linearly in the first
   * dimension.
   */
  TeamSpec(
    Team & team = dash::Team::All())
  : _is_linear(true),
    _myid(team.myid())
  {
    DASH_LOG_TRACE_VAR("TeamSpec(t)", team.is_null());
    auto team_size = team.is_null() ? 0 : team.size();
    _rank = 1;
    this->_extents[0] = team_size;
    for (auto d = 1; d < MaxDimensions; ++d) {
      this->_extents[d] = 1;
    }
    this->resize(this->_extents);
  }

  /**
   * Constructor, creates an instance of TeamSpec with given extents
   * from a team (set of units) and a distribution spec.
   * The number of elements in the distribution different from NONE
   * must be equal to the rank of the extents.
   *
   * This constructor adjusts extents according to given distribution
   * spec if the passed team spec has been default constructed.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<2> ts(
   *     // default-constructed, extents: [Team::All().size(), 1]
   *     TeamSpec<2>(),
   *     // distributed in dimension 1 (y)
   *     DistSpec<2>(NONE, BLOCKED),
   *     Team::All().split(2));
   *   // Will be adjusted to:
   *   size_t units_x = ts.extent(0); // -> 1
   *   size_t units_y = ts.extent(1); // -> Team::All().size() / 2
   * \endcode
   */
  TeamSpec(
    const self_t & other,
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All())
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>(
      other.extents()),
      _myid(team.myid())
  {
    DASH_LOG_TRACE_VAR("TeamSpec(ts, dist, t)", team.is_null());
#if 0
    if (this->size() != team.size()) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Size of team "     << team.size()  << " differs from " <<
        "size of teamspec " << this->size() << " in TeamSpec()");
    }
#endif
    // Test if other teamspec has been default-constructed and has
    // to be rearranged for a distribution with higher rank:
    if (other._is_linear && distribution.rank() > 1) {
      // Set extent of teamspec in the dimension the distribution is
      // different from NONE:
      if (distribution.is_tiled()) {
        bool major_tiled_dim_set = false;
        for (auto d = 0; d < MaxDimensions; ++d) {
          this->_extents[d] = 1;
          if (!major_tiled_dim_set &&
              distribution[d].type == dash::internal::DIST_TILE) {
            this->_extents[d] = team.size();
            major_tiled_dim_set = true;
          }
        }
      } else {
        for (auto d = 0; d < MaxDimensions; ++d) {
          if (distribution[d].type == dash::internal::DIST_NONE) {
            this->_extents[d] = 1;
          } else {
            // Use size of given team; possibly different from size
            // of default-constructed team spec:
            this->_extents[d] = team.size();
          }
        }
      }
    }
    update_rank();
    DASH_LOG_TRACE_VAR("TeamSpec(ts, dist, t)", this->_extents);
    this->resize(this->_extents);
    DASH_LOG_TRACE_VAR("TeamSpec(ts, dist, t)", this->size());
  }

  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) and a distribution spec.
   * All but one element in the distribution spec must be \c NONE.
   */
  TeamSpec(
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All())
  : _myid(team.myid())
  {
    DASH_LOG_TRACE_VAR("TeamSpec(dist, t)", team.is_null());
    bool distrib_dim_set = false;
    if (distribution.is_tiled()) {
      bool major_tiled_dim_set = false;
      for (auto d = 0; d < MaxDimensions; ++d) {
        this->_extents[d] = 1;
        if (!major_tiled_dim_set &&
            distribution[d].type == dash::internal::DIST_TILE) {
          this->_extents[d] = team.size();
          major_tiled_dim_set = true;
        }
      }
    } else {
      for (auto d = 0; d < MaxDimensions; ++d) {
        if (distribution[d].type == dash::internal::DIST_NONE) {
          this->_extents[d] = 1;
        } else {
          this->_extents[d] = team.size();
          if (distrib_dim_set) {
            DASH_THROW(
              dash::exception::InvalidArgument,
              "TeamSpec(DistributionSpec, Team) only allows "
              "one distributed dimension");
          }
          distrib_dim_set = true;
        }
      }
    }
    update_rank();
    DASH_LOG_TRACE_VAR("TeamSpec(dist, t)", this->_extents);
    this->resize(this->_extents);
    DASH_LOG_TRACE_VAR("TeamSpec(dist, t)", this->size());
  }

  /**
   * Constructor, initializes new instance of TeamSpec with
   * extents specified in argument list.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts(1,2,3); // extents 1x2x3
   * \endcode
   */
  template<typename ... Types>
  TeamSpec(SizeType value, Types ... values)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(value, values...),
      _myid(dash::Team::All().myid())
  {
    update_rank();
    this->resize(this->_extents);
    DASH_LOG_TRACE_VAR("TeamSpec(size,...)", this->_extents);
  }

  /**
   * Constructor, initializes new instance of TeamSpec with
   * extents specified in array by dimension.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts({ 1,2,3 }); // extents 1x2x3
   * \endcode
   */
  TeamSpec(const std::array<SizeType, MaxDimensions> & extents)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(extents),
      _myid(dash::Team::All().myid())
  {
    update_rank();
    this->resize(this->_extents);
    DASH_LOG_TRACE_VAR("TeamSpec({extents})", this->_extents);
  }

  /**
   * Copy constructor.
   */
  TeamSpec(
    /// Teamspec instance to copy
    const self_t & other)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(other.extents()),
    _rank(other._rank),
    _myid(other._myid)
  { }

  void balance_extents()
  {
    DASH_LOG_TRACE_VAR("TeamSpec.balance_extents()", this->_extents);
    DASH_LOG_TRACE_VAR("TeamSpec.balance_extents()", size());
    SizeType num_units = 1;
    for (auto d = 0; d < MaxDimensions; ++d) {
      num_units *= this->_extents[d];
      this->_extents[d] = 1;
    }
    _is_linear = false;

    // Find best surface-to-volume for two-dimensional team:
    auto teamsize_prime_factors = dash::math::factorize(num_units);
    SizeType surface = 0;
    for (auto it : teamsize_prime_factors) {
      DASH_LOG_TRACE("TeamSpec.balance_extents",
                     "factor:", it.first, "x", it.second);
      for (auto i = 1; i < it.second + 1; ++i) {
        SizeType extent_x = it.first * this->_extents[0];
        SizeType extent_y = num_units / extent_x;
        SizeType surface_new = (2 * extent_x) + (2 * extent_y);
        DASH_LOG_TRACE("TeamSpec.balance_extents", "Testing extents",
                       extent_x, "x", extent_y, " - surface:", surface_new);
        if (surface == 0 || surface_new < surface) {
          surface           = surface_new;
          this->_extents[0] = extent_x;
          this->_extents[1] = extent_y;
        }
      }
    }
    this->resize(this->_extents);
    update_rank();
    DASH_LOG_TRACE_VAR("TeamSpec.balance_extents ->", this->_extents);
  }

  /**
   * Resolve unit id at given offset in Cartesian team grid relative to the
   * active unit's position in the team.
   *
   * Example:
   *
   * \code
   *   TeamSpec<2> teamspec(7,4);
   *   // west neighbor is offset -1 in column dimension:
   *   team_unit_t neighbor_west = teamspec.neigbor({ 0, -1 });
   *   // second south neighbor is offset -2 in row dimension:
   *   team_unit_t neighbor_west = teamspec.neigbor({ -2, 0 });
   * \endcode
   *
   * \returns  The unit id at given offset in the team grid, relative to the
   *           active unit's position in the team, or DART_UNDEFINED_UNIT_ID
   *           if the offset is out of bounds.
   */
  team_unit_t neighbor(std::initializer_list<int> offsets) const
  {
    auto neighbor_coords = this->coords(_myid);
    dim_t d = 0;
    for (auto offset_d : offsets) {
      neighbor_coords[d] += offset_d;
      if (neighbor_coords[d] < 0 ||
          neighbor_coords[d] >= this->_extents[d]) {
        return UNDEFINED_TEAM_UNIT_ID;
      }
      ++d;
    }
    return this->at(neighbor_coords);
  }

  /**
   * Resolve unit id at given offset in Cartesian team grid relative to the
   * active unit's position in the team.
   * Offsets wrap around in every dimension as in a torus topology.
   *
   * Example:
   *
   * \code
   *   // assuming dash::myid() == 1, i.e. team spec coordinates are (0,1)
   *   TeamSpec<2> teamspec(2,2);
   *   // west neighbor is offset -1 in column dimension:
   *   team_unit_t neighbor_west = teamspec.neigbor_periodic({ 0, -1 });
   *   // -> unit 0
   *   // second south neighbor at offset -2 in row dimension wraps around
   *   // to row coordinate 0:
   *   team_unit_t neighbor_west = teamspec.neigbor_periodic({ -2, 0 });
   *   // -> unit 1
   * \endcode
   *
   * \returns  The unit id at given offset in the team grid, relative to the
   *           active unit's position in the team.
   *           If an offset is out of bounds, it is wrapped around in the
   *           respective dimension as in a torus topology.
   */
  team_unit_t periodic_neighbor(std::initializer_list<int> offsets) const
  {
    auto neighbor_coords = this->coords(_myid);
    dim_t d = 0;
    for (auto offset_d : offsets) {
      neighbor_coords[d] += offset_d;
      if (neighbor_coords[d] < 0 ||
          neighbor_coords[d] >= this->_extents[d]) {
        neighbor_coords[d] %= this->_extents[d];
      }
      ++d;
    }
    return at(neighbor_coords);
  }

  /**
   * Whether the given index lies in the cartesian sub-space specified by a
   * dimension and offset in the dimension.
   */
  bool includes_index(
    IndexType index,
    dim_t dimension,
    IndexType dim_offset) const
  {
    if (_rank == 1) {
      // Shortcut for trivial case
      return (index >= 0 && index < size());
    }
    return parent_t::includes_index(index, dimension, dim_offset);
  }

  /**
   * The number of units (extent) available in the given dimension.
   *
   * \param    dimension  The dimension
   * \returns  The number of units in the given dimension
   */
  SizeType num_units(dim_t dimension) const
  {
    return parent_t::extent(dimension);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename... Args>
  void resize(SizeType arg, Args... args)
  {
    static_assert(
      sizeof...(Args) == MaxDimensions-1,
      "Invalid number of arguments");
    std::array<SizeType, MaxDimensions> extents =
      {{ arg, (SizeType)(args)... }};
    resize(extents);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename SizeType_>
  void resize(const std::array<SizeType_, MaxDimensions> & extents)
  {
    _is_linear = false;
    parent_t::resize(extents);
    update_rank();
  }

  /**
   * Change the extent of the cartesian space in the given dimension.
   */
  void resize(dim_t dim, SizeType extent)
  {
    this->_extents[dim] = extent;
    resize(this->_extents);
  }

  /**
   * The actual number of dimensions with extent greater than 1 in
   * this team arragement, that is the dimension of the vector space
   * spanned by the team arrangement's extents.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts(1,2,3); // extents 1x2x3
   *   ts.rank(); // returns 2, as one dimension has extent 1
   * \endcode
   */
  dim_t rank() const
  {
    return _rank;
  }

private:
  void update_rank()
  {
    _rank = 0;
    for (auto d = 0; d < MaxDimensions; ++d) {
      if (this->_extents[d] > 1) {
        ++_rank;
      }
    }
    if (_rank == 0) _rank = 1;
  }

protected:
  /// Actual number of dimensions of the team layout specification.
  dim_t _rank       = 0;
  /// Whether the team spec is linear
  bool  _is_linear  = false;
  /// Unit id of active unit
  team_unit_t _myid;

}; // class TeamSpec

} // namespace dash

#endif // DASH__TEAM_SPEC_H_
