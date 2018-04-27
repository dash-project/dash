#ifndef DASH__HALO_HALOMATRIXWRAPPER_H
#define DASH__HALO_HALOMATRIXWRAPPER_H

#include <dash/dart/if/dart.h>

#include <dash/Matrix.h>
#include <dash/Pattern.h>
#include <dash/halo/StencilOperator.h>
#include <dash/memory/GlobStaticMem.h>

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

template <typename MatrixT>
class HaloMatrixWrapper {
private:
  using Pattern_t       = typename MatrixT::pattern_type;
  using pattern_index_t = typename Pattern_t::index_type;

  static constexpr auto NumDimensions = Pattern_t::ndim();

public:
  using Element_t = typename MatrixT::value_type;

  using ViewSpec_t      = ViewSpec<NumDimensions, pattern_index_t>;
  using GlobBoundSpec_t = GlobalBoundarySpec<NumDimensions>;
  using HaloBlock_t     = HaloBlock<Element_t, Pattern_t>;
  using HaloMemory_t    = HaloMemory<HaloBlock_t>;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;
  using region_index_t  = typename RegionCoords<NumDimensions>::region_index_t;

private:
  static constexpr auto MemoryArrange = Pattern_t::memory_order();

  using pattern_size_t        = typename Pattern_t::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using HaloSpec_t            = HaloSpec<NumDimensions>;
  using Region_t              = Region<Element_t, Pattern_t>;

public:
  /**
   * Constructor that takes \ref Matrix, a \ref GlobalBoundarySpec and a user
   * defined number of stencil specifications (\ref StencilSpec)
   */
  template <typename... StencilSpecT>
  HaloMatrixWrapper(MatrixT& matrix, const GlobBoundSpec_t& cycle_spec,
                    const StencilSpecT&... stencil_spec)
  : _matrix(matrix), _cycle_spec(cycle_spec), _halo_spec(stencil_spec...),
    _view_global(matrix.local.offsets(), matrix.local.extents()),
    _haloblock(matrix.begin().globmem(), matrix.pattern(), _view_global,
               _halo_spec, cycle_spec),
    _view_local(_haloblock.view_local()), _halomemory(_haloblock) {
    for(const auto& region : _haloblock.halo_regions()) {
      if(region.size() == 0)
        continue;
      // number of contiguous elements
      pattern_size_t num_blocks      = 1;
      pattern_size_t num_elems_block = 1;
      auto           rel_dim         = region.spec().relevant_dim();
      auto           level           = region.spec().level();
      auto*          off = &*(_halomemory.first_element_at(region.index()));
      auto           it  = region.begin();

      if(MemoryArrange == ROW_MAJOR) {
        if(level == 1) {  //|| (level == 2 && region.regionSpec()[0] != 1)) {
          for(auto i = rel_dim - 1; i < NumDimensions; ++i)
            num_elems_block *= region.view().extent(i);

          size_t region_size        = region.size();
          auto   ds_num_elems_block = dart_storage<Element_t>(num_elems_block);
          num_blocks                = region_size / num_elems_block;
          auto           it_dist    = it + num_elems_block;
          pattern_size_t stride =
            (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index)
                             : 1;
          auto            ds_stride = dart_storage<Element_t>(stride);
          dart_datatype_t stride_type;
          dart_type_create_strided(ds_num_elems_block.dtype, ds_stride.nelem,
                                   ds_num_elems_block.nelem, &stride_type);
          _dart_types.push_back(stride_type);

          _region_data.insert(std::make_pair(
            region.index(), Data{ region,
                                  [off, it, region_size, ds_num_elems_block,
                                   stride_type](dart_handle_t& handle) {
                                    dart_get_handle(off, it.dart_gptr(),
                                                    region_size, stride_type,
                                                    ds_num_elems_block.dtype,
                                                    &handle);
                                  },
                                  DART_HANDLE_NULL }));

        }
        // TODO more optimizations
        else {
          num_elems_block *= region.view().extent(NumDimensions - 1);
          size_t region_size         = region.size();
          auto   ds_num_elems_block  = dart_storage<Element_t>(num_elems_block);
          num_blocks                 = region_size / num_elems_block;
          auto                it_tmp = it;
          auto                start_index = it.lpos().index;
          std::vector<size_t> block_sizes(num_blocks);
          std::vector<size_t> block_offsets(num_blocks);
          std::fill(block_sizes.begin(), block_sizes.end(),
                    ds_num_elems_block.nelem);
          for(auto& index : block_offsets) {
            index =
              dart_storage<Element_t>(it_tmp.lpos().index - start_index).nelem;
            it_tmp += num_elems_block;
          }
          dart_datatype_t index_type;
          dart_type_create_indexed(
            ds_num_elems_block.dtype,
            num_blocks,            // number of blocks
            block_sizes.data(),    // size of each block
            block_offsets.data(),  // offset of first element of each block
            &index_type);
          _dart_types.push_back(index_type);
          _region_data.insert(std::make_pair(
            region.index(), Data{ region,
                                  [off, it, ds_num_elems_block, region_size,
                                   index_type](dart_handle_t& handle) {
                                    dart_get_handle(off, it.dart_gptr(),
                                                    region_size, index_type,
                                                    ds_num_elems_block.dtype,
                                                    &handle);
                                  },
                                  DART_HANDLE_NULL }));
        }
      } else {
        if(level == 1) {  //|| (level == 2 &&
                          // region.regionSpec()[NumDimensions - 1] != 1)) {
          for(auto i = 0; i < rel_dim; ++i)
            num_elems_block *= region.view().extent(i);

          size_t region_size        = region.size();
          auto   ds_num_elems_block = dart_storage<Element_t>(num_elems_block);
          num_blocks                = region_size / num_elems_block;
          auto           it_dist    = it + num_elems_block;
          pattern_size_t stride =
            (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index)
                             : 1;
          auto ds_stride = dart_storage<Element_t>(stride);

          dart_datatype_t stride_type;
          dart_type_create_strided(ds_num_elems_block.dtype, ds_stride.nelem,
                                   ds_num_elems_block.nelem, &stride_type);
          _dart_types.push_back(stride_type);

          _region_data.insert(std::make_pair(
            region.index(), Data{ region,
                                  [off, it, region_size, ds_num_elems_block,
                                   stride_type](dart_handle_t& handle) {
                                    dart_get_handle(off, it.dart_gptr(),
                                                    region_size, stride_type,
                                                    ds_num_elems_block.dtype,
                                                    &handle);
                                  },
                                  DART_HANDLE_NULL }));
        }
        // TODO more optimizations
        else {
          num_elems_block *= region.view().extent(0);
          size_t region_size         = region.size();
          auto   ds_num_elems_block  = dart_storage<Element_t>(num_elems_block);
          num_blocks                 = region_size / num_elems_block;
          auto                it_tmp = it;
          std::vector<size_t> block_sizes(num_blocks);
          std::vector<size_t> block_offsets(num_blocks);
          std::fill(block_sizes.begin(), block_sizes.end(),
                    ds_num_elems_block.nelem);
          auto start_index = it.lpos().index;
          for(auto& index : block_offsets) {
            index =
              dart_storage<Element_t>(it_tmp.lpos().index - start_index).nelem;
            it_tmp += num_elems_block;
          }

          dart_datatype_t index_type;
          dart_type_create_indexed(
            ds_num_elems_block.dtype,
            num_blocks,            // number of blocks
            block_sizes.data(),    // size of each block
            block_offsets.data(),  // offset of first element of each block
            &index_type);
          _dart_types.push_back(index_type);

          _region_data.insert(std::make_pair(
            region.index(), Data{ region,
                                  [off, it, index_type, region_size,
                                   ds_num_elems_block](dart_handle_t& handle) {
                                    dart_get_handle(off, it.dart_gptr(),
                                                    region_size, index_type,
                                                    ds_num_elems_block.dtype,
                                                    &handle);
                                  },
                                  DART_HANDLE_NULL }));
        }

        num_elems_block = region.view().extent(0);
      }
    }
  }

  /**
   * Constructor that takes \ref Matrix and a user
   * defined number of stencil specifications (\ref StencilSpec).
   * The \ref GlobalBoundarySpec is set to default.
   */
  template <typename... StencilSpecT>
  HaloMatrixWrapper(MatrixT& matrix, const StencilSpecT&... stencil_spec)
  : HaloMatrixWrapper(matrix, GlobBoundSpec_t(), stencil_spec...) {}

  HaloMatrixWrapper() = delete;

  ~HaloMatrixWrapper() {
    for(auto& dart_type : _dart_types) {
      dart_type_destroy(&dart_type);
    }
    _dart_types.clear();
  }

  /**
   * Returns the underlying \ref HaloBlock
   */
  const HaloBlock_t& halo_block() { return _haloblock; }

  /**
   * Initiates a blocking halo region update for all halo elements.
   */
  void update() {
    for(auto& region : _region_data) {
      update_halo_intern(region.second);
    }
    wait();
  }

  /**
   * Initiates a blocking halo region update for all halo elements within the
   * the given region.
   */
  void update_at(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end()) {
      update_halo_intern(it_find->second);
      dart_wait_local(&it_find->second.handle);
    }
  }

  /**
   * Initiates an asychronous halo region update for all halo elements.
   */
  void update_async() {
    for(auto& region : _region_data) {
      update_halo_intern(region.second);
    }
  }

  /**
   * Initiates an asychronous halo region update for all halo elements within
   * the given region.
   */
  void update_async_at(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end()) {
      update_halo_intern(it_find->second);
    }
  }

  /**
   * Waits until all halo updates are finished. Only useful for asynchronous
   * halo updates.
   */
  void wait() {
    for(auto& region : _region_data) {
      dart_wait_local(&region.second.handle);
    }
  }

  /**
   * Waits until the halo updates for the given halo region is finished.
   * Only useful for asynchronous halo updates.
   */
  void wait(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end())
      dart_wait_local(it_find->second.handle);
  }

  /**
   * Returns the local \ref ViewSpec
   *
   */
  const ViewSpec_t& view_local() const { return _view_local; }

  /**
   * Returns the halo memory management object \ref HaloMemory
   */
  HaloMemory_t& halo_memory() { return _halomemory; }

  /**
   * Returns the halo memory management object \ref HaloMemory
   */
  const HaloMemory_t& halo_memory() const { return _halomemory; }

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
      if(region.is_custom_region()) {
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

        auto range_mem = _halomemory.range_at(region.index());
        auto it_mem = range_mem.first;
        auto it_reg_end  = region.end();
        DASH_ASSERT_MSG(
            std::distance(range_mem.first, range_mem.second) == region.size(),
            "Range distance of the HaloMemory is unequal region size");

        for(auto it = region.begin(); it != it_reg_end; ++it, ++it_mem) {
          auto coords = it.gcoords();
          for(auto d = 0; d < NumDimensions; ++d) {
            coords[d] += coords_offset[d];
          }

          *it_mem = f(coords);
        }
      }
    }
  }

  /**
   * Returns the halo value for a given global coordinate or nullptr if no halo
   * element exists. This also means that only a unit connected to the given
   * coordinate will return a halo value. All others will return nullptr.
   */
  Element_t* halo_element_at_global(const ElementCoords_t coords) {
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
  StencilOperator<Element_t, Pattern_t, StencilSpecT> stencil_operator(
    const StencilSpecT& stencil_spec) {
    for(const auto& stencil : stencil_spec.specs()) {
      DASH_ASSERT_MSG(
        stencil.max()
          <= _halo_spec.extent(RegionSpec<NumDimensions>::index(stencil)),
        "Stencil point extent higher than halo region extent.");
    }

    return StencilOperator<Element_t, Pattern_t, StencilSpecT>(
      &_haloblock, &_halomemory, stencil_spec, &_view_local);
  }

private:
  struct Data {
    const Region_t&                     region;
    std::function<void(dart_handle_t&)> get_halos;
    dart_handle_t                       handle;
  };

  void update_halo_intern(Data& data) {
    if(data.region.is_custom_region())
      return;

    data.get_halos(data.handle);
  }

  Element_t* halo_element_at(ElementCoords_t& coords) {
    auto        index     = _haloblock.index_at(_view_local, coords);
    const auto& spec      = _halo_spec.spec(index);
    auto        range_mem = _halomemory.range_at(index);
    if(spec.level() == 0 || range_mem.first == range_mem.second)
      return nullptr;

    if(!_halomemory.to_halo_mem_coords_check(index, coords))
      return nullptr;

    return &*(range_mem.first + _halomemory.offset(index, coords));
  }

private:
  MatrixT&                       _matrix;
  const GlobBoundSpec_t          _cycle_spec;
  const HaloSpec_t               _halo_spec;
  const ViewSpec_t               _view_global;
  const HaloBlock_t              _haloblock;
  const ViewSpec_t&              _view_local;
  HaloMemory_t                   _halomemory;
  std::map<region_index_t, Data> _region_data;
  std::vector<dart_datatype_t>   _dart_types;
};

}  // namespace halo

}  // namespace dash

#endif  // DASH__HALO_HALOMATRIXWRAPPER_H
