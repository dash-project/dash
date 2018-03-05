#ifndef DASH__HALO_HALOMATRIXWRAPPER_H
#define DASH__HALO_HALOMATRIXWRAPPER_H

#include <dash/dart/if/dart.h>

#include <dash/Matrix.h>
#include <dash/Pattern.h>
#include <dash/memory/GlobStaticMem.h>

#include <dash/halo/iterator/HaloMatrixIterator.h>

#include <type_traits>
#include <vector>

namespace dash {

/**
 * As known from classic stencil algorithms, *boundaries* are the outermost
 * elements within a block that are requested by neighoring units.
 * *Halos* represent additional outer regions of a block that contain ghost
 * cells with values copied from adjacent units' boundary regions.
 *
 * The \c HaloMatrixWrapper acts as a wrapper of the local blocks of the NArray
 * and extends these by boundary and halo regions. This wrapper provides
 * \ref HaloMatrixIterator for the inner block, the boundary elements and
 * both.
 *
 * The inner block iterator ensures that no stencil point accesses halo or not
 * existing elements. The stencil points of the boundary iterator point at least
 * to one halo element.
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
 *
 *
 * Example for an inner block boundary iteration space:
 *
 *                          boundary region 1
 *                                 :
 *                   .-------------'------------.
 *                   |                          |
 *           .-------.-------------------------.-------.
 *           |  0  1 |  2  3  4  5  6  7  8  9 | 10 11 |
 *           | 12 13 | 14 15 16 17 18 19 20 21 | 22 23 |
 *        .--:-------+-------------------------+-------:--.
 *        |  | 24 23 |                         | 34 35 |  |
 *      .-:  :  ...  :   inner block region    :  ...  :  :- boundary
 *      | |  | 60 62 |                         | 70 71 |  |  region 3
 *      | '--:-------+-------------------------+-------:--:
 *      |    | 72 73 | 74 75 76 77 78 79 80 81 | 82 83 |  :- boundary
 *      |    | 84 85 | 86 87 88 89 90 91 92 93 | 94 95 |  |  region 8
 *      |    `-------'-------------------------'-------'--'
 *      |            |                         |
 *      |            `------------.------------+
 *      :                         :
 *      boundary region 3   boundary region 8
 *
 */

template <typename MatrixT, typename StencilSpecT>
class HaloMatrixWrapper {
private:
  using Pattern_t             = typename MatrixT::pattern_type;
  using pattern_index_t       = typename Pattern_t::index_type;

  static constexpr auto NumDimensions = Pattern_t::ndim();

public:
  using Element_t = typename MatrixT::value_type;

  using iterator       = HaloMatrixIterator<Element_t, Pattern_t, StencilSpecT,
                                      StencilViewScope::ALL>;
  using const_iterator = const iterator;
  using iterator_inner = HaloMatrixIterator<Element_t, Pattern_t, StencilSpecT,
                                            StencilViewScope::INNER>;
  using const_iterator_inner = const iterator_inner;
  using iterator_bnd = HaloMatrixIterator<Element_t, Pattern_t, StencilSpecT,
                                          StencilViewScope::BOUNDARY>;
  using const_iterator_bnd = const iterator_bnd;

  using ViewSpec_t      = ViewSpec<NumDimensions, pattern_index_t>;
  using GlobBoundSpec_t = GlobalBoundarySpec<NumDimensions>;
  using HaloBlock_t     = HaloBlock<Element_t, Pattern_t>;
  using HaloMemory_t    = HaloMemory<HaloBlock_t>;
  using ElementCoords_t = std::array<pattern_index_t, NumDimensions>;
  using region_index_t  = typename RegionCoords<NumDimensions>::region_index_t;

private:
  static constexpr auto MemoryArrange = Pattern_t::memory_order();
  static constexpr auto NumStencilPoints = StencilSpecT::num_stencil_points();

  using pattern_size_t = typename Pattern_t::size_type;
  using signed_pattern_size_t = typename std::make_signed<pattern_size_t>::type;
  using HaloSpec_t     = HaloSpec<NumDimensions>;
  using Region_t       = Region<Element_t, Pattern_t, NumDimensions>;

public:
  HaloMatrixWrapper(MatrixT& matrix, const StencilSpecT& stencil_spec,
                    const GlobBoundSpec_t& cycle_spec = GlobBoundSpec_t())
  : _matrix(matrix), _stencil_spec(stencil_spec), _cycle_spec(cycle_spec),
    _halo_reg_spec(stencil_spec), _view_local(matrix.local.extents()),
    _view_global(ViewSpec_t(matrix.local.offsets(), matrix.local.extents())),
    _haloblock(matrix.begin().globmem(), matrix.pattern(), _view_global,
               _halo_reg_spec, cycle_spec),
    _halomemory(_haloblock), _begin(_haloblock, _halomemory, _stencil_spec, 0),
    _end(_haloblock, _halomemory, _stencil_spec,
         _haloblock.view_inner_with_boundaries().size()),
    _ibegin(_haloblock, _halomemory, _stencil_spec, 0),
    _iend(_haloblock, _halomemory, _stencil_spec,
          _haloblock.view_inner().size()),
    _bbegin(_haloblock, _halomemory, _stencil_spec, 0),
    _bend(_haloblock, _halomemory, _stencil_spec, _haloblock.boundary_size()) {

    for(const auto& region : _haloblock.halo_regions()) {
      if(region.size() == 0)
        continue;
      // number of contiguous elements
      pattern_size_t num_blocks      = 1;
      pattern_size_t num_elems_block = 1;
      auto           rel_dim         = region.spec().relevant_dim();
      auto           level           = region.spec().level();
      auto*          off             = _halomemory.pos_at(region.index());
      auto           it              = region.begin();

      if(MemoryArrange == ROW_MAJOR) {
        if(level == 1) {  //|| (level == 2 && region.regionSpec()[0] != 1)) {
          for(auto i = rel_dim - 1; i < NumDimensions; ++i)
            num_elems_block *= region.region().extent(i);

          size_t region_size      = region.size();
          auto ds_num_elems_block = dart_storage<Element_t>(num_elems_block);
          num_blocks              = region_size / num_elems_block;
          auto           it_dist  = it + num_elems_block;
          pattern_size_t stride =
            (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index)
                             : 1;
          auto     ds_stride = dart_storage<Element_t>(stride);
          HaloData halo_data;
          dart_datatype_t stride_type;
          dart_type_create_strided(
            ds_num_elems_block.dtype, ds_stride.nelem,
            ds_num_elems_block.nelem, &stride_type);
          _dart_types.push_back(stride_type);

          _region_data.insert(std::make_pair(
            region.index(), Data{ region,
                                  [off, it, region_size, ds_num_elems_block,
                                   stride_type](HaloData& data) {
                                    dart_get_handle(
                                      off, it.dart_gptr(), region_size,
                                      stride_type, ds_num_elems_block.dtype,
                                      &data.handle);
                                  },
                                  std::move(halo_data) }));

        }
        // TODO more optimizations
        else {
          num_elems_block *= region.region().extent(NumDimensions - 1);
          size_t region_size      = region.size();
          auto ds_num_elems_block = dart_storage<Element_t>(num_elems_block);
          num_blocks              = region_size / num_elems_block;
          auto     it_tmp         = it;
          HaloData halo_data;
          auto     start_index = it.lpos().index;
          std::vector<size_t> block_sizes(num_blocks);
          std::vector<size_t> block_offsets(num_blocks);
          std::fill(
            block_sizes.begin(), block_sizes.end(), ds_num_elems_block.nelem);
          for(auto& index : block_offsets) {
            index =
               dart_storage<Element_t>(it_tmp.lpos().index - start_index).nelem;
            it_tmp += num_elems_block;
          }
          dart_datatype_t index_type;
          dart_type_create_indexed(
            ds_num_elems_block.dtype,
            num_blocks,             // number of blocks
            block_sizes.data(),     // size of each block
            block_offsets.data(),   // offset of first element of each block
            &index_type);
          _dart_types.push_back(index_type);
          _region_data.insert(std::make_pair(
            region.index(),
            Data{ region,
                  [off, it, ds_num_elems_block,region_size, index_type]
                  (HaloData& data) {
                    dart_get_handle(
                      off, it.dart_gptr(), region_size, index_type,
                      ds_num_elems_block.dtype, &data.handle);
                  },
                  std::move(halo_data) }));
        }
      } else {
        if(level == 1) {  //|| (level == 2 &&
                          // region.regionSpec()[NumDimensions - 1] != 1)) {
          for(auto i = 0; i < rel_dim; ++i)
            num_elems_block *= region.region().extent(i);

          size_t region_size      = region.size();
          auto ds_num_elems_block = dart_storage<Element_t>(num_elems_block);
          num_blocks              = region_size / num_elems_block;
          auto           it_dist  = it + num_elems_block;
          pattern_size_t stride =
            (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index)
                             : 1;
          auto     ds_stride = dart_storage<Element_t>(stride);
          HaloData halo_data;

          dart_datatype_t stride_type;
          dart_type_create_strided(
            ds_num_elems_block.dtype, ds_stride.nelem,
            ds_num_elems_block.nelem, &stride_type);
          _dart_types.push_back(stride_type);

          _region_data.insert(std::make_pair(
            region.index(), Data{ region,
                                  [off, it, region_size, ds_num_elems_block,
                                   stride_type](HaloData& data) {
                                    dart_get_handle(
                                      off, it.dart_gptr(), region_size,
                                      stride_type, ds_num_elems_block.dtype,
                                      &data.handle);
                                  },
                                  std::move(halo_data) }));
        }
        // TODO more optimizations
        else {
          num_elems_block *= region.region().extent(0);
          size_t region_size      = region.size();
          auto ds_num_elems_block = dart_storage<Element_t>(num_elems_block);
          num_blocks              = region_size / num_elems_block;
          auto     it_tmp         = it;
          HaloData halo_data;
          std::vector<size_t> block_sizes(num_blocks);
          std::vector<size_t> block_offsets(num_blocks);
          std::fill(
              block_sizes.begin(), block_sizes.end(), ds_num_elems_block.nelem);
          auto     start_index = it.lpos().index;
          for(auto& index : block_offsets) {
            index =
              dart_storage<Element_t>(it_tmp.lpos().index - start_index).nelem;
            it_tmp += num_elems_block;
          }

          dart_datatype_t index_type;
          dart_type_create_indexed(
            ds_num_elems_block.dtype,
            num_blocks,             // number of blocks
            block_sizes.data(),     // size of each block
            block_offsets.data(),   // offset of first element of each block
            &index_type);
          _dart_types.push_back(index_type);

          _region_data.insert(std::make_pair(
            region.index(),
            Data{ region,
                  [off, it, index_type, region_size, ds_num_elems_block]
                  (HaloData& data) {
                    dart_get_handle(
                      off, it.dart_gptr(), region_size, index_type,
                      ds_num_elems_block.dtype, &data.handle);
                  },
                  std::move(halo_data) }));
        }

        num_elems_block = region.region().extent(0);
      }
    }
    set_stencil_offsets();
  }

  ~HaloMatrixWrapper() {
    for(auto& dart_type : _dart_types) {
      dart_type_destroy(&dart_type);
    }
    _dart_types.clear();
  }

  /// returns the begin iterator for all relevant elements (inner + boundary)
  iterator begin() noexcept { return _begin; }

  /// returns the begin const iterator for all relevant elements
  /// (inner + boundary)
  const_iterator begin() const noexcept { return _begin; }

  /// returns the end iterator for all relevant elements (inner + boundary)
  iterator end() noexcept { return _end; }

  /// returns the end const iterator for all relevant elements (inner + boundary)
  const_iterator end() const noexcept { return _end; }

  /// returns the begin iterator for all inner elements
  iterator_inner ibegin() noexcept { return _ibegin; }

  /// returns the begin const iterator for all inner elements
  const_iterator_inner ibegin() const noexcept { return _ibegin; }

  /// returns the end iterator for all inner elements
  iterator_inner iend() noexcept { return _iend; }

  /// returns the end const iterator for all inner elements
  const_iterator_inner iend() const noexcept { return _iend; }

  /// returns the begin iterator for all boundary elements
  iterator_bnd bbegin() noexcept { return _bbegin; }

  /// returns the begin const iterator for all boundary elements
  const_iterator_bnd bbegin() const noexcept { return _bbegin; }

  /// returns the end iterator for all boundary elements
  iterator_bnd bend() noexcept { return _bend; }

  /// returns the end const iterator for all boundary elements
  const_iterator_bnd bend() const noexcept { return _bend; }

  /// returns the underlying \ref HaloBlock
  const HaloBlock_t& halo_block() { return _haloblock; }

  /**
   * Initiates a blocking halo region update for all halo elements.
   */
  void update() {
    for(auto& region : _region_data)
      update_halo_intern(region.second, false);
  }

  /**
   * Initiates a blocking halo region update for all halo elements within the
   * the given region.
   */
  void update_at(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end())
      update_halo_intern(it_find->second, false);
  }

  /**
   * Initiates an asychronous halo region update for all halo elements.
   */
  void update_async() {
    for(auto& region : _region_data)
      update_halo_intern(region.second, true);
  }

  /**
   * Initiates an asychronous halo region update for all halo elements within the
   * given region.
   */
  void update_async_at(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end())
      update_halo_intern(it_find->second, true);
  }

  /**
   * Waits until all halo updates are finished. Only useful for asyncronous
   * halo updates.
   */
  void wait() {
    for(auto& region : _region_data)
      dart_wait_local(&region.second.halo_data.handle);
  }

  /**
   * Returns the local \ref ViewSpec
   *
   */
  const ViewSpec_t& view_local() const { return _view_local; }

  /**
   * Returns the stencil specification \ref StencilSpec
   */
  const StencilSpecT& stencil_spec() const { return _stencil_spec; }

  /**
   * Returns the halo memory management object \ref HaloMemory
   */
  HaloMemory_t& halo_memory() { return _halomemory; }

  /**
   * Returns the underlying NArray
   */
  MatrixT& matrix() { return _matrix; }

  /**
   * Sets all global border halo elements. set_fixed_halos calls FuntionT with
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
  void set_fixed_halos(FunctionT f) {
    using signed_extent_t = typename std::make_signed<pattern_size_t>::type;
    for(const auto& region : _haloblock.boundary_regions()) {
      auto rel_dim = region.spec().relevant_dim() - 1;
      if(region.is_fixed_region()) {
        auto*       pos_ptr = _halomemory.pos_at(region.index());
        const auto& spec    = region.spec();
        std::array<signed_extent_t, NumDimensions> coords_offset{};
        const auto& reg_ext = region.region().extents();
        for(auto d = 0; d < NumDimensions; ++d) {
          if(spec[d] == 0) {
            coords_offset[d] -= reg_ext[d];
            continue;
          }
          if(spec[d] == 2)
            coords_offset[d] = reg_ext[d];
        }

        auto it_reg_end = region.end();
        for(auto it = region.begin(); it != it_reg_end; ++it) {
          auto coords = it.gcoords();
          for(auto d = 0; d < NumDimensions; ++d)
            coords[d] += coords_offset[d];
          *(pos_ptr + it.rpos()) = f(coords);
        }
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
    auto index       = _haloblock.index_at(_view_local, coords);
    const auto& spec = _halo_reg_spec.spec(index);
    auto halomem_pos = _halomemory.pos_at(index);
    if(spec.level() == 0 || halomem_pos == nullptr)
      return nullptr;

    if(!_halomemory.to_halo_mem_coords_check(index, coords))
      return nullptr;

    return _halomemory.pos_at(index) + _halomemory.value_at(index, coords);
  }

  /**
   * Returns the halo value for a given local coordinate or nullptr if no halo
   * element exists.
   */
  Element_t* halo_element_at_local(ElementCoords_t coords) {
    auto index       = _haloblock.index_at(_view_local, coords);
    const auto& spec = _halo_reg_spec.spec(index);
    auto halomem_pos = _halomemory.pos_at(index);
    if(spec.level() == 0 || halomem_pos == nullptr)
      return nullptr;

    if(!_halomemory.to_halo_mem_coords_check(index, coords))
      return nullptr;

    return _halomemory.pos_at(index) + _halomemory.value_at(index, coords);
  }

  void set_value_at_inner_local(const ElementCoords_t& coords, Element_t value,
      Element_t coefficient_center,
      std::function<Element_t(const Element_t&, const Element_t&)> op =
        [](const Element_t& lhs , const Element_t& rhs) {return rhs;}) {
    auto* center = _matrix.lbegin();
    pattern_index_t offset = 0;

    if(MemoryArrange == ROW_MAJOR) {
      offset = coords[0];
      for(auto d = 1; d < NumDimensions; ++d)
        offset = offset * _view_local.extent(d) + coords[d];
    } else {
      offset = coords[NumDimensions - 1];
      for(auto d = NumDimensions - 2; d >= 0; --d)
        offset = offset * _view_local.extent(d) + coords[d];
    }
    center += offset;
    *center = op(*center, coefficient_center * value);
    for(auto i = 0; i < NumStencilPoints; ++i) {
      auto& stencil_point_value = center[_stencil_offsets[i]];
      stencil_point_value = op(stencil_point_value, _stencil_spec[i].coefficient() * value);
    }
  }

  void set_value_at_boundary_local(const ElementCoords_t& coords, Element_t value,
      Element_t coefficient_center,
      std::function<Element_t(const Element_t&, const Element_t&)> op =
        [](const Element_t& lhs , const Element_t& rhs) {return rhs;}) {
    auto* center = _matrix.lbegin();
    pattern_index_t offset = 0;

    if(MemoryArrange == ROW_MAJOR) {
      offset = coords[0];
      for(auto d = 1; d < NumDimensions; ++d)
        offset = offset * _view_local.extent(d) + coords[d];
    } else {
      offset = coords[NumDimensions - 1];
      for(auto d = NumDimensions - 2; d >= 0; --d)
        offset = offset * _view_local.extent(d) + coords[d];
    }
    center += offset;
    *center = op(*center, coefficient_center * value);
    for(auto i = 0; i < NumStencilPoints; ++i) {
      bool halo = false;
      for(auto d = 0; d < NumDimensions; ++d) {
        auto coord_value = coords[d] + _stencil_spec[i][d];
        if(coord_value < 0 || coord_value >= _view_local.extent(d)) {
          halo = true;
          break;
        }
      }
      if(halo)
        continue;
      auto& stencil_point_value = center[_stencil_offsets[i]];
     // if(dash::myid() == 1)
     //   std::cout << dash::myid() << " : " << coords[0] + _stencil_spec[i][0] << "," << coords[1] + _stencil_spec[i][1] << "," << coords[2] + _stencil_spec[i][2] << " -> " << _stencil_spec[i].coefficient()*value << std::endl;
      stencil_point_value = op(stencil_point_value, _stencil_spec[i].coefficient() * value);
    }
  }

  template <typename IteratorT, typename FunctionT>
  void compute(IteratorT inputBegin, IteratorT inputEnd, IteratorT outputBegin,
               FunctionT func) {
    while(inputBegin != inputEnd) {
      *outputBegin = func(inputBegin);
      ++inputBegin;
      ++outputBegin;
    }
  }

private:
  struct HaloData {
    dart_handle_t       handle = DART_HANDLE_NULL;
  };

  struct Data {
    const Region_t&                region;
    std::function<void(HaloData&)> get_halos;
    HaloData                       halo_data;
  };

  void update_halo_intern(Data& data, bool async) {
    auto rel_dim = data.region.spec().relevant_dim() - 1;
    if(data.region.is_fixed_region())
      return;

    data.get_halos(data.halo_data);

    if(!async)
      dart_wait_local(&data.halo_data.handle);
  }

  void set_stencil_offsets() {
    for(auto i = 0; i < NumStencilPoints; ++i) {
      signed_pattern_size_t offset = 0;
      if(MemoryArrange == ROW_MAJOR) {
        offset = _stencil_spec[i][0];
        for(auto d = 1; d < NumDimensions; ++d)
          offset = _stencil_spec[i][d] + offset * _view_local.extent(d);
      } else {
        offset = _stencil_spec[i][NumDimensions - 1];
        for(auto d = NumDimensions - 2; d >= 0; --d)
          offset = _stencil_spec[i][d] + offset * _view_local.extent(d);
      }
      _stencil_offsets[i] = offset;
    }
  }

private:
  MatrixT&                       _matrix;
  const StencilSpecT&            _stencil_spec;
  const GlobBoundSpec_t          _cycle_spec;
  const HaloSpec_t               _halo_reg_spec;
  const ViewSpec_t               _view_local;
  const ViewSpec_t               _view_global;
  const HaloBlock_t              _haloblock;
  HaloMemory_t                   _halomemory;
  std::map<region_index_t, Data> _region_data;
  std::vector<dart_datatype_t>   _dart_types;
  std::array<signed_pattern_size_t, NumStencilPoints> _stencil_offsets;

  iterator       _begin;
  iterator       _end;
  iterator_inner _ibegin;
  iterator_inner _iend;
  iterator_bnd   _bbegin;
  iterator_bnd   _bend;
};

}  // namespace dash

#endif  // DASH__HALO_HALOMATRIXWRAPPER_H
