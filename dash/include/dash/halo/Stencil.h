#ifndef DASH__HALO_STENCIL_H
#define DASH__HALO_STENCIL_H

#include <dash/halo/Types.h>

#include <dash/Dimensional.h>

namespace dash {

namespace halo {

using namespace internal;

/**
 * Stencil point with raletive coordinates for N dimensions
 * e.g. StencilPoint<2>(-1,-1) -> north west
 */
template <dim_t NumDimensions, typename CoeffT = double>
class StencilPoint : public Dimensional<int16_t, NumDimensions> {
public:
  using coefficient_t = CoeffT;

private:
  using Base_t = Dimensional<spoint_value_t, NumDimensions>;

public:
  // TODO constexpr
  /**
   * Default Contructor
   *
   * All stencil point values are 0 and default coefficient = 1.0.
   */
  StencilPoint() {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      this->_values[d] = 0;
    }
  }

  /**
   * Constructor
   *
   * Custom stencil point values for all dimensions and default
   * coefficient = 1.0.
   */
  template <typename... Values>
  constexpr StencilPoint(
    typename std::enable_if<sizeof...(Values) == NumDimensions - 1,
                            spoint_value_t>::type value,
    Values... values)
  : Base_t::Dimensional(value, (spoint_value_t) values...) {}

  /**
   * Constructor
   *
   * Custom values and custom coefficient.
   */
  template <typename... Values>
  constexpr StencilPoint(
    typename std::enable_if<sizeof...(Values) == NumDimensions - 1,
                            CoeffT>::type coefficient,
                            spoint_value_t value, Values... values)
  : Base_t::Dimensional(value, (spoint_value_t) values...),
    _coefficient(coefficient) {}

  // TODO as constexpr
  /**
   * Returns maximum distance to center over all dimensions
   */
  int max() const {
    int max = 0;
    for(dim_t d = 0; d < NumDimensions; ++d)
      max = std::max(max, (int) std::abs(this->_values[d]));
    return max;
  }

  /**
   * Returns coordinates adjusted by stencil point
   */
  template <typename ElementCoordsT>
  ElementCoordsT stencil_coords(ElementCoordsT& coords) const {
    return StencilPoint<NumDimensions, CoeffT>::stencil_coords(coords, this);
  }

  /**
   * Returns coordinates adjusted by a given stencil point
   */
  template <typename ElementCoordsT>
  static ElementCoordsT stencil_coords(
    ElementCoordsT                             coords,
    const StencilPoint<NumDimensions, CoeffT>& stencilp) {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      coords[d] += stencilp[d];
    }

    return coords;
  }

  /**
   * Returns coordinates adjusted by a stencil point and a boolean to indicate
   * a if the adjusted coordinate points to elements out of the given
   * \ref ViewSpecpossible (inside: true, else: false).
   */
  template <typename ElementCoordsT, typename ViewSpecT>
  std::pair<ElementCoordsT, bool> stencil_coords_check(
    ElementCoordsT coords, const ViewSpecT& view) const {
    bool halo = false;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      coords[d] += this->_values[d];
      if(coords[d] < view.offset(d) || coords[d] >= view.offset(d) + view.extent(d))
        halo = true;
    }

    return std::make_pair(coords, halo);
  }

  /**
   * Returns coordinates adjusted by a stencil point and a boolean to indicate
   * a if the adjusted coordinate points to elements out of the given
   * \ref ViewSpec:  possible (inside: true, else: false).
   * If one dimension points to an element outside the \ref ViewSpec this method
   * returns immediately the unfinished adjusted coordinate and true. Otherwise
   * the adjusted coordinate and false is returned,
   */
  template <typename ElementCoordsT, typename ViewSpecT>
  std::pair<ElementCoordsT, bool> stencil_coords_check_abort(
    ElementCoordsT coords, const ViewSpecT& view) const {
    for(dim_t d = 0; d < NumDimensions; ++d) {
      coords[d] += this->_values[d];
      if(coords[d] < view.offset(d) || coords[d] >= view.offset(d) + view.extent(d))
        return std::make_pair(coords, true);
    }

    return std::make_pair(coords, false);
  }

  /**
   * Returns the coefficient for this stencil point
   */
  CoeffT coefficient() const { return _coefficient; }

private:
  CoeffT _coefficient = 1.0;
};  // StencilPoint

template <dim_t NumDimensions, typename CoeffT>
std::ostream& operator<<(
  std::ostream& os, const StencilPoint<NumDimensions, CoeffT>& stencil_point) {
  os << "dash::halo::StencilPoint<" << NumDimensions << ">"
     << "(coefficient = " << stencil_point.coefficient() << " - points: ";
  for(auto d = 0; d < NumDimensions; ++d) {
    if(d > 0) {
      os << ",";
    }
    os << stencil_point[d];
  }
  os << ")";

  return os;
}

/**
 * A collection of stencil points (\ref Stencil)
 * e.g. StencilSpec<dash::StencilPoint<2>, 2,2>({StencilPoint<2>(-1,0),
 * StencilPoint<2>(1,0)}) -> north and south
 */
template <typename StencilPointT, std::size_t NumStencilPoints>
class StencilSpec {
private:
  using Self_t = StencilSpec<StencilPointT, NumStencilPoints>;
  static constexpr auto NumDimensions = StencilPointT::ndim();

public:
  using stencil_size_t   = std::size_t;
  using stencil_index_t  = std::size_t;
  using StencilArray_t   = std::array<StencilPointT, NumStencilPoints>;
  using StencilPoint_t   = StencilPointT;
  using DistanceDim_t = std::pair<spoint_value_t, spoint_value_t>;
  using DistanceAll_t = std::array<DistanceDim_t, NumDimensions>;

public:
  /**
   * Constructor
   *
   * Takes a list of \ref StencilPoint
   */
  constexpr StencilSpec(const StencilArray_t& specs) : _specs(specs) {}

  /**
   * Constructor
   *
   * Takes all given \ref StencilPoint. The number of arguments has to be the
   * same as the given number of stencil points via the template argument.
   */
  template <typename... Values>
  constexpr StencilSpec(const StencilPointT& value, const Values&... values)
  : _specs{ { value, (StencilPointT) values... } } {
    static_assert(sizeof...(values) == NumStencilPoints - 1,
                  "Invalid number of stencil point arguments");
  }

  // TODO constexpr
  /**
   * Copy Constructor
   */
  StencilSpec(const Self_t& other) { _specs = other._specs; }

  /**
   * \return container storing all stencil points
   */
  constexpr const StencilArray_t& specs() const { return _specs; }

  /**
   * \return number of stencil points
   */
  static constexpr stencil_size_t num_stencil_points() {
    return NumStencilPoints;
  }

  /**
   * Returns the stencil point index for a given \ref StencilPoint
   *
   * \return The index and true if the given stecil point was found,
   *         else the index 0 and false.
   *         Keep in mind that index 0 is only a valid index, if the returned
   *         bool is true
   */
  const std::pair<stencil_index_t, bool> index(StencilPointT stencil) const {
    for(auto i = 0; i < _specs.size(); ++i) {
      if(_specs[i] == stencil)
        return std::make_pair(i, true);
    }

    return std::make_pair(0, false);
  }

  /**
   * Returns the minimal and maximal distances of all stencil points for all
   * dimensions. (minimum (first) <= 0 and maximum (second) >= 0)
   */
  DistanceAll_t minmax_distances() const {
    DistanceAll_t max_dist{};
    for(const auto& stencil_point : _specs) {
      for(auto d = 0; d < NumDimensions; ++d) {
        if(stencil_point[d] < max_dist[d].first) {
          max_dist[d].first = stencil_point[d];
          continue;
        }
        if(stencil_point[d] > max_dist[d].second)
          max_dist[d].second = stencil_point[d];
      }
    }

    return max_dist;
  }

  /**
   * Returns the minimal and maximal distances of all stencil points for the
   * given dimension. (minimum (first) <= 0 and maximum (second) >= 0)
   */
  DistanceDim_t minmax_distances(dim_t dim) const {
    DistanceDim_t max_dist{};
    for(const auto& stencil_point : _specs) {
      if(stencil_point[dim] < max_dist.first) {
        max_dist.first = stencil_point[dim];
        continue;
      }
      if(stencil_point[dim] > max_dist.second)
        max_dist.second = stencil_point[dim];
    }

    return max_dist;
  }
  /**
   * \return stencil point for a given index
   */
  constexpr const StencilPointT& operator[](stencil_index_t index) const {
    return _specs[index];
  }

private:
  StencilArray_t _specs{};
};  // StencilSpec

template <typename StencilPointT, std::size_t NumStencilPoints>
std::ostream& operator<<(
  std::ostream& os, const StencilSpec<StencilPointT, NumStencilPoints>& specs) {
  os << "dash::halo::StencilSpec<" << NumStencilPoints << ">"
     << "(";
  for(auto i = 0; i < NumStencilPoints; ++i) {
    if(i > 0) {
      os << ",";
    }
    os << specs[i];
  }
  os << ")";

  return os;
}

template <typename StencilPointT>
class StencilSpecFactory {
private:
  using stencil_dist_t = spoint_value_t;
  using StencilPerm_t  = std::vector<StencilPointT>;

  static constexpr auto NumDimensions = StencilPointT::ndim();

public:

  static constexpr decltype(auto) full_stencil_spec(stencil_dist_t dist) {
    using StencilSpec_t  = StencilSpec<StencilPointT, NumRegionsMax<NumDimensions>-1>;
    
    using StencilArray_t = typename StencilSpec_t::StencilArray_t;

    StencilPerm_t stencil_perms;
    StencilArray_t points;
    StencilPointT start_stencil;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      start_stencil[d] = std::abs(dist);
    }
    permutate_stencil_points(0, start_stencil, stencil_perms, dist);
    
    size_t count = 0;
    for(const auto& elem : stencil_perms) {
      bool center = true;
      for(dim_t d = 0; d < NumDimensions; ++d) {
        if(elem[d] != 0 ) {
          center = false;
          break;
        } 
      }
      if(!center) {
        points[count] = elem;
        ++count;
      }
    }

    return StencilSpec_t(points);
  }

private:
  static void permutate_stencil_points(dim_t dim_change, const StencilPointT& current_stencil, StencilPerm_t& perm_stencil, stencil_dist_t dist) {
    perm_stencil.push_back(current_stencil);

    for(dim_t d = dim_change; d < NumDimensions; ++d) {
      if(current_stencil[d] != 0) {
        auto new_stencil = current_stencil;
        new_stencil[d] = 0;
        permutate_stencil_points(d+1, new_stencil, perm_stencil, dist);
        new_stencil[d] = -dist;
        permutate_stencil_points(d+1, new_stencil, perm_stencil, dist);
      }
    }
  }

};

}  // namespace halo

}  // namespace dash

#endif // DASH__HALO_STENCIL_H