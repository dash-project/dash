#ifndef DASH__HALO_HALOMATRIXWRAPPER_H
#define DASH__HALO_HALOMATRIXWRAPPER_H

#include <dash/dart/if/dart.h>

#include <dash/Matrix.h>
#include <dash/Pattern.h>
#include <dash/halo/StencilOperator.h>
#include <dash/halo/CoordinateAccess.h>
#include <dash/halo/Types.h>

#include <type_traits>
#include <vector>



namespace dash {

namespace halo {

/**
 * As known from classic stencil algorithms, *boundaries* are the outermost
 * elements within a block that are requested by neighoring units.
 * *Halos* represent additional outer regions of a block that contain ghost
 * cells with values copied from adjacent units' boundary regions.
 *
 * The \c HaloMatrixWrapper acts as a wrapper of the local blocks of the NArray
 * and extends these by boundary and halo regions. The HaloMatrixWrapper also
 * provides a function to create a \ref StencilOperator.
 *
 * Example for an outer block boundary iteration space (halo regions):
 *
 *            .--halo region 0   .-- halo region 1
 *           /                  /
 *       .-------..-------------------------. -.
 *       |  0  1 ||  0  1  2  3  4  5  6  7 |  |
 *       |  2  3 ||  8  9 10 11 12 13 14 15 |  |-- halo width in dimension 0
 *       '-------''-------------------------' -'
 *       .-------..-------------------------..-------.
 *       |  0  1 ||                         ||  0  1 |
 *       :  ...  ::       local block       ::  ...  : --- halo region 5
 *       |  6  7 ||                         ||  6  7 |
 *       '-------''-------------------------''-------'
 *           :    .-------------------------.:       :
 *           |    |  0  1  2  3  4  5  6  7 |'---.---'
 *           |    |  8  9 10 11 12 13 14 15 |    :
 *           |    `-------------------------'    '- halo width in dimension 1
 *           '                  \
 *     halo region 3             '- halo region 7
 */

template <typename MatrixT, SignalReady SigReady = SignalReady::OFF>
class HaloMatrixWrapper {
private:
  using Pattern_t       = typename MatrixT::pattern_type;
  using pattern_index_t = typename Pattern_t::index_type;

  static constexpr auto NumDimensions = Pattern_t::ndim();
  using GlobMem_t = typename MatrixT::GlobMem_t;

public:
  using Element_t = typename MatrixT::value_type;

  using ViewSpec_t      = ViewSpec<NumDimensions, pattern_index_t>;
  using GlobBoundSpec_t = GlobalBoundarySpec<NumDimensions>;
  using HaloBlock_t     = HaloBlock<Element_t, Pattern_t, GlobMem_t>;
  using HaloMemory_t    = HaloMemory<HaloBlock_t>;
  using HaloUpdateEnv_t = HaloUpdateEnv<HaloBlock_t, SigReady>;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;
  using region_index_t  = internal::region_index_t;
  using stencil_dist_t  = internal::spoint_value_t;

private:
  static constexpr auto MemoryArrange = Pattern_t::memory_order();

  using pattern_size_t        = typename Pattern_t::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using HaloSpec_t            = HaloSpec<NumDimensions>;
  using Region_t              = Region<Element_t, Pattern_t, typename MatrixT::GlobMem_t>;

public:
  /**
   * Constructor that takes \ref Matrix, a \ref GlobalBoundarySpec and a user
   * defined number of stencil specifications (\ref StencilSpec)
   */
  template <size_t NumStencilPointsFirst, typename... StencilSpecRestT>
  HaloMatrixWrapper(MatrixT& matrix, const GlobBoundSpec_t& glob_bnd_spec,
                    const StencilSpec<StencilPoint<NumDimensions>, NumStencilPointsFirst>& stencil_spec_first,
                    const StencilSpecRestT&... stencil_spec)
  : _matrix(matrix), _glob_bnd_spec(glob_bnd_spec),
    _halo_spec(stencil_spec_first, stencil_spec...),
    _view_global(matrix.local.offsets(), matrix.local.extents()),
    _haloblock(matrix.begin().globmem(), matrix.pattern(), _view_global,
               _halo_spec, glob_bnd_spec),
    _view_local(_haloblock.view_local()),
    //_halomemory(_haloblock),
    _halo_env(_haloblock, matrix.lbegin(), matrix.team(), matrix.pattern().teamspec()) {
  }

  /**
   * Constructor that takes \ref Matrix and a stencil point distance
   * to create a \ref HaloMatrixWrapper with a full stencil with the
   * given width.
   * The \ref GlobalBoundarySpec is set to default.
   */
  template <typename StencilPointT = StencilPoint<NumDimensions>>
  HaloMatrixWrapper(MatrixT& matrix, const GlobBoundSpec_t& glob_bnd_spec,
                    stencil_dist_t dist, std::enable_if_t<std::is_integral<stencil_dist_t>::value, std::nullptr_t> = nullptr )
  : HaloMatrixWrapper(matrix, glob_bnd_spec, StencilSpecFactory<StencilPointT>::full_stencil_spec(dist)) {
  }

  /**
   * Constructor that takes \ref Matrix and a stencil point distance
   * to create a \ref HaloMatrixWrapper with a full stencil with the
   * given width.
   * The \ref GlobalBoundarySpec is set to default.
   */
  template <typename StencilPointT = StencilPoint<NumDimensions>>
  HaloMatrixWrapper(MatrixT& matrix, stencil_dist_t dist, std::enable_if_t<std::is_integral<stencil_dist_t>::value, std::nullptr_t> = nullptr )
  : HaloMatrixWrapper(matrix, GlobBoundSpec_t(), StencilSpecFactory<StencilPointT>::full_stencil_spec(dist)) {
  }

  /**
   * Constructor that takes \ref Matrix and a user
   * defined number of stencil specifications (\ref StencilSpec).
   * The \ref GlobalBoundarySpec is set to default.
   */
  template <typename StencilPointT, std::size_t NumStencilPoints>
  HaloMatrixWrapper(MatrixT& matrix, const StencilSpec<StencilPointT,NumStencilPoints> stencil_spec)
  : HaloMatrixWrapper(matrix, GlobBoundSpec_t(), stencil_spec) {}

  /**
   * Constructor that takes \ref Matrix and a user
   * defined number of stencil specifications (\ref StencilSpec).
   * The \ref GlobalBoundarySpec is set to default.
   */
  template <typename... StencilSpecT, typename std::enable_if_t<sizeof...(StencilSpecT) >= 2, bool>>
  HaloMatrixWrapper(MatrixT& matrix, const StencilSpecT&... stencil_spec)
  : HaloMatrixWrapper(matrix, GlobBoundSpec_t(), stencil_spec...) {}

  HaloMatrixWrapper() = delete;

  /**
   * Returns the underlying \ref HaloBlock
   */
  const HaloBlock_t& halo_block() { return _haloblock; }

  /**
   * Initiates a blocking halo region update for all halo elements.
   */
  void update() {
    _halo_env.update();
  }

  /**
   * Initiates a blocking halo region update for all halo elements within the
   * the given region.
   */
  void update_at(region_index_t index) {
    _halo_env.update_at(index);
  }

  /**
   * Initiates an asychronous halo region update for all halo elements.
   */
  void update_async() {
    _halo_env.update_async();
  }

  /**
   * Initiates an asychronous halo region update for all halo elements within
   * the given region.
   */
  void update_async_at(region_index_t index) {
    _halo_env.update_async_at(index);
  }

  /**
   * Waits until all halo updates are finished. Only useful for asynchronous
   * halo updates.
   */
  void wait() {
    _halo_env.wait();
  }

  /**
   * Waits until the halo updates for the given halo region is finished.
   * Only useful for asynchronous halo updates.
   */
  void wait(region_index_t index) {
    _halo_env.wait(index);
  }

  /**
   * Returns the local \ref ViewSpec
   *
   */
  const ViewSpec_t& view_local() const { return _view_local; }

  /**
   * Returns the halo environment management object \ref HaloUpdateEnv
   */
  HaloUpdateEnv_t& halo_env() { return _halo_env; }

  /**
   * Returns the halo environment management object \ref HaloUpdateEnv
   */
  const HaloUpdateEnv_t& halo_env() const { return _halo_env; }

  /**
   * Returns the underlying NArray
   */
  MatrixT& matrix() { return _matrix; }

  /**
   * Returns the underlying NArray
   */
  const MatrixT& matrix() const { return _matrix; }

  /**
   * Sets all global border halo elements. set_custom_halos calls FuntionT with
   * all global coordinates of type:
   * std::array<dash::default_index_t,Number Dimensions>.
   *
   * Every unit is called only with the related global coordinates.
   * E.g.:
   *
   *     .............
   *     : Border    | <- coordinates for example:
   *     : Unit 0    |    (-1,-1),(0,-1), (-2,5)
   *     :  .--------..--------.
   *     :  |        ||        |
   *     :  | Unit 0 || Unit 1 |
   *     :  |        ||        |
   *     '- :--------::--------:
   *        |        ||        |
   *        | Unit 2 || Unit 3 |
   *        |        ||        |
   *        '--------''--------'
   *
   */
  template <typename FunctionT>
  void set_custom_halos(FunctionT f) {
    using signed_extent_t = typename std::make_signed<pattern_size_t>::type;
    for(const auto& region : _haloblock.boundary_regions()) {
      if(!region.is_custom_region()) {
        continue;
      }

      const auto& spec    = region.spec();
      std::array<signed_extent_t, NumDimensions> coords_offset{};
      const auto& reg_ext = region.view().extents();
      for(auto d = 0; d < NumDimensions; ++d) {
        if(spec[d] == 0) {
          coords_offset[d] -= reg_ext[d];
          continue;
        }
        if(spec[d] == 2)
          coords_offset[d] = reg_ext[d];
      }

      auto range_mem = _halo_env.halo_memory().range_at(region.index());
      auto it_mem = range_mem.first;
      auto it_reg_end  = region.end();
      DASH_ASSERT_MSG(
          std::distance(range_mem.first, range_mem.second) == region.size(),
          "Range distance of the HaloMemory is unequal region size");
      const auto& pattern = _matrix.pattern();
      for(auto it = region.begin(); it != it_reg_end; ++it, ++it_mem) {
        auto coords = pattern.coords(it.rpos(), region.view());
        for(auto d = 0; d < NumDimensions; ++d) {
          coords[d] += coords_offset[d];
        }

        *it_mem = f(coords);
      }
    }
  }

  /**
   * Returns the halo value for a given global coordinate or nullptr if no halo
   * element exists. This also means that only a unit connected to the given
   * coordinate will return a halo value. All others will return nullptr.
   */
  Element_t* halo_element_at_global(ElementCoords_t coords) {
    const auto& offsets = _view_global.offsets();
    for(auto d = 0; d < NumDimensions; ++d) {
      coords[d] -= offsets[d];
    }

    return halo_element_at(coords);
  }

  /**
   * Returns the halo value for a given local coordinate or nullptr if no halo
   * element exists.
   */
  Element_t* halo_element_at_local(ElementCoords_t coords) {
    return halo_element_at(coords);
  }

  /**
   * Crates \ref StencilOperator for a given \ref StencilSpec.
   * Asserts whether the StencilSpec fits in the provided halo regions.
   */
  template <typename StencilSpecT>
  StencilOperator<HaloBlock_t, StencilSpecT> stencil_operator(
    const StencilSpecT& stencil_spec) {
    for(const auto& stencil : stencil_spec.specs()) {
      DASH_ASSERT_MSG(
        stencil.max()
          <= _halo_spec.extent(RegionSpec<NumDimensions>::index(stencil)),
        "Stencil point extent higher than halo region extent.");
    }

    return StencilOperator<HaloBlock_t, StencilSpecT>(
      &_haloblock, _matrix.lbegin(), &_halo_env.halo_memory(), stencil_spec);
  }

  CoordinateAccess<HaloBlock_t> coordinate_access() {
    return CoordinateAccess<HaloBlock_t>(&_haloblock, _matrix.lbegin(),&_halo_env.halo_memory());
  }

private:

  Element_t* halo_element_at(ElementCoords_t& coords) {
    auto        index     = _haloblock.index_at(_view_local, coords);
    const auto& spec      = _halo_spec.spec(index);
    auto& halo_memory     = _halo_env.halo_memory();
    auto        range_mem = halo_memory.range_at(index);
    if(spec.level() == 0 || range_mem.first == range_mem.second)
      return nullptr;

    if(!halo_memory.to_halo_mem_coords_check(index, coords))
      return nullptr;

    return &*(range_mem.first + halo_memory.offset(index, coords));
  }

private:
  MatrixT&                       _matrix;
  const GlobBoundSpec_t          _glob_bnd_spec;
  const HaloSpec_t               _halo_spec;
  const ViewSpec_t               _view_global;
  const HaloBlock_t              _haloblock;
  const ViewSpec_t&              _view_local;
  HaloUpdateEnv_t                _halo_env;
};

}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO_HALOMATRIXWRAPPER_H
