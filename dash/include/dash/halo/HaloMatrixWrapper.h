#ifndef DASH__HALO_HALOMATRIXWRAPPER_H
#define DASH__HALO_HALOMATRIXWRAPPER_H

#include <dash/dart/if/dart.h>

#include <dash/Pattern.h>
#include <dash/memory/GlobStaticMem.h>
#include <dash/Matrix.h>

#include <dash/halo/iterator/HaloMatrixIterator.h>

#include <type_traits>

namespace dash {

template <typename MatrixT, typename StencilSpecT>
class HaloMatrixWrapper {
private:
  using PatternT       = typename MatrixT::pattern_type;
  using size_type      = typename MatrixT::size_type;
  using index_type     = typename MatrixT::index_type;
  using ElementT       = typename MatrixT::value_type;
  using ViewSpecT      = ViewSpec<MatrixT::ndim(), index_type>;
  using SelfT          = HaloMatrixWrapper<MatrixT, StencilSpecT>;
  using HaloSpecT      = HaloSpec<MatrixT::ndim()>;
  using CycleSpecT     = CycleSpec<MatrixT::ndim()>;
  using HaloBlockT     = HaloBlock<ElementT, PatternT>;
  using HaloRegionT    = Region<ElementT, PatternT, MatrixT::ndim()>;
  using HaloMemoryT    = HaloMemory<HaloBlockT>;
  using CoordsT        = typename HaloMemoryT::CoordsT;
  using region_index_t = typename HaloBlockT::region_index_t;

  static constexpr dim_t      NumDimensions = PatternT::ndim();
  static constexpr MemArrange MemoryArrange = PatternT::memory_order();

public:
  using iterator = HaloMatrixIterator<ElementT, PatternT, StencilSpecT, StencilViewScope::ALL>;
  using const_iterator = const iterator;
  using iterator_inner =
      HaloMatrixIterator<ElementT, PatternT, StencilSpecT, StencilViewScope::INNER>;
  using const_iterator_inner = const iterator_inner;
  using iterator_bnd =
      HaloMatrixIterator<ElementT, PatternT, StencilSpecT, StencilViewScope::BOUNDARY>;
  using const_iterator_bnd = const iterator_inner;

public:
  HaloMatrixWrapper(MatrixT& matrix, const StencilSpecT& stencil_spec,
                    const CycleSpecT& cycle_spec = CycleSpecT())
      : _matrix(matrix), _stencil_spec(stencil_spec), _cycle_spec(cycle_spec),
        _halo_reg_spec(stencil_spec), _view_local(matrix.local.extents()),
        _view_global(ViewSpecT(matrix.local.offsets(), matrix.local.extents())),
        _haloblock(matrix.begin().globmem(), matrix.pattern(), _view_global, _halo_reg_spec,
                   cycle_spec),
        _halomemory(_haloblock), _begin(_haloblock, _halomemory, _stencil_spec, 0),
        _end(_haloblock, _halomemory, _stencil_spec, _haloblock.view_safe().size()),
        _ibegin(_haloblock, _halomemory, _stencil_spec, 0),
        _iend(_haloblock, _halomemory, _stencil_spec, _haloblock.view_inner().size()),
        _bbegin(_haloblock, _halomemory, _stencil_spec, 0),
        _bend(_haloblock, _halomemory, _stencil_spec, _haloblock.boundary_size()) {

    for (const auto& region : _haloblock.halo_regions()) {
      if (region.size() == 0)
        continue;
      // number of contiguous elements
      size_type num_blocks = 1;
      size_type num_elems_block = 1;
      auto rel_dim = region.regionSpec().relevantDim();
      auto level = region.regionSpec().level();
      auto* off = _halomemory.haloPos(region.index());
      auto it  = region.begin();
      if (MemoryArrange == ROW_MAJOR){
        if(level == 1 ) { //|| (level == 2 && region.regionSpec()[0] != 1)) {
          for( auto i = rel_dim - 1; i < NumDimensions; ++i)
            num_elems_block *= region.region().extent(i);

          auto ds_num_elems_block = dart_storage<ElementT>(num_elems_block);
          num_blocks = region.size() / num_elems_block;
          auto it_dist = it + num_elems_block;
          size_type stride = (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index) : 1;
          auto ds_stride = dart_storage<ElementT>(stride);
          HaloData halo_data{nullptr, std::vector<int>(0)};

          _region_data.insert(std::make_pair(region.index(), Data{region,
              [off, it, num_blocks, ds_num_elems_block, ds_stride](HaloData& data) {
                dart_get_strided_handle(off, it.dart_gptr(), num_blocks, ds_num_elems_block.nelem,
                    ds_stride.nelem, ds_num_elems_block.dtype, STRIDED_TO_CONTIG, &data.handle);
              }, std::move(halo_data)}));

        }
        // TODO more optimizations
        else {
          num_elems_block *= region.region().extent(NumDimensions - 1);
          auto ds_num_elems_block = dart_storage<ElementT>(num_elems_block);
          num_blocks = region.size() / num_elems_block;
          auto it_tmp = it;
          HaloData halo_data{nullptr, std::vector<int>(num_blocks)};
          auto start_index = it.lpos().index;
          for(auto& index : halo_data.indexes) {
            index = static_cast<int>(dart_storage<ElementT>(it_tmp.lpos().index - start_index).nelem);
            it_tmp += num_elems_block;
          }
          _region_data.insert(std::make_pair(region.index(), Data{region,
              [off, it, num_blocks, ds_num_elems_block](HaloData& data) {
                dart_get_indexed_handle(off, it.dart_gptr(), num_blocks, ds_num_elems_block.nelem,
                  data.indexes.data(), ds_num_elems_block.dtype, STRIDED_TO_CONTIG, &data.handle);
              }, std::move(halo_data)}));
        }
      } else {
        if(level == 1 ) { //|| (level == 2 && region.regionSpec()[NumDimensions - 1] != 1)) {
          for( auto i = 0; i < rel_dim; ++i)
            num_elems_block *= region.region().extent(i);

          auto ds_num_elems_block = dart_storage<ElementT>(num_elems_block);
          num_blocks = region.size() / num_elems_block;
          auto it_dist = it + num_elems_block;
          size_type stride = (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index) : 1;
          auto ds_stride = dart_storage<ElementT>(stride);
          HaloData halo_data{nullptr, std::vector<int>(0)};

          _region_data.insert(std::make_pair(region.index(), Data{region,
              [off, it, num_blocks, ds_num_elems_block, ds_stride](HaloData& data) {
                dart_get_strided_handle(off, it.dart_gptr(), num_blocks, ds_num_elems_block.nelem,
                    ds_stride.nelem, ds_num_elems_block.dtype, STRIDED_TO_CONTIG, &data.handle);
              }, std::move(halo_data)}));
        }
        // TODO more optimizations
        else {
          num_elems_block *= region.region().extent(0);
          auto ds_num_elems_block = dart_storage<ElementT>(num_elems_block);
          num_blocks = region.size() / num_elems_block;
          auto it_tmp = it;
          HaloData halo_data{nullptr, std::vector<int>(num_blocks)};
          auto start_index = it.lpos().index;
          for(auto& index : halo_data.indexes) {
            index = static_cast<int>(dart_storage<ElementT>(
                  it_tmp.lpos().index - start_index).nelem);
            it_tmp += num_elems_block;
          }
          _region_data.insert(std::make_pair(region.index(), Data{region,
              [off, it, num_blocks, ds_num_elems_block](HaloData& data) {
                dart_get_indexed_handle(off, it.dart_gptr(), num_blocks, ds_num_elems_block.nelem,
                  data.indexes.data(), ds_num_elems_block.dtype, STRIDED_TO_CONTIG, &data.handle);
              }, std::move(halo_data)}));
        }

        num_elems_block = region.region().extent(0);
      }
    }
  }

  ~HaloMatrixWrapper() {}

  iterator begin() noexcept { return _begin; }

  const_iterator begin() const noexcept { return _begin; }

  iterator end() noexcept { return _end; }

  const_iterator end() const noexcept { return _end; }

  iterator_inner ibegin() noexcept { return _ibegin; }

  const_iterator_inner ibegin() const noexcept { return _ibegin; }

  iterator_inner iend() noexcept { return _iend; }

  const_iterator_inner iend() const noexcept { return _iend; }

  iterator_bnd bbegin() noexcept { return _bbegin; }

  const_iterator_bnd bbegin() const noexcept { return _bbegin; }

  iterator_bnd bend() noexcept { return _bend; }

  const_iterator_bnd bend() const noexcept { return _bend; }

  const HaloBlockT& haloBlock() { return _haloblock; }

  void updateHalosAsync() {
    for (auto& region : _region_data)
      updateHaloIntern(region.second, true);
  }

  void waitHalosAsync() {
    for (auto& region : _region_data)
      dart_wait_local(&region.second.halo_data.handle);
  }

  void updateHalos() {
    for (auto& region : _region_data)
      updateHaloIntern(region.second, false);
  }

  void updateHalo(region_index_t index) {
    auto it_find = _region_data.find(index);
    if (it_find != _region_data.end())
      updateHaloIntern(it_find->second, false);
  }

  const ViewSpecT& getLocalView() const { return _view_local; }

  const StencilSpecT& stencilSpec() const { return _stencil_spec; }

  const HaloMemoryT& haloMemory() const { return _halomemory; }

  MatrixT& matrix() { return _matrix; }

  template <typename FunctionT>
  void setFixedHalos(FunctionT f) {
    using signed_extent_t = typename std::make_signed<size_type>::type;
    for (const auto& region : _haloblock.boundary_regions()) {
      auto rel_dim = region.regionSpec().relevantDim() - 1;
      if (region.borderRegion() && region.borderDim(rel_dim)
          && _cycle_spec[rel_dim] == Cycle::FIXED) {
        auto* pos_ptr = _halomemory.haloPos(region.index());
        const auto& spec    = region.regionSpec();
        std::array<signed_extent_t, NumDimensions> coords_offset{};
        const auto& reg_ext = region.region().extents();
        for (auto d = 0; d < NumDimensions; ++d) {
          if (spec[d] == 0) {
            coords_offset[d] -= reg_ext[d];
            continue;
          }
          if (spec[d] == 2)
            coords_offset[d] = reg_ext[d];
        }

        auto it_reg_end = region.end();
        for (auto it = region.begin(); it != it_reg_end; ++it) {
          auto coords = it.gcoords();
          for (auto d = 0; d < NumDimensions; ++d)
            coords[d] += coords_offset[d];
          *(pos_ptr + it.rpos()) = f(coords);
        }
      }
    }
  }

  ElementT* haloElementAt(CoordsT coords) {
    const auto& offsets = _view_global.offsets();
    for(auto d = 0; d < NumDimensions; ++d) {
      coords[d] -= offsets[d];
    }
    auto index = _haloblock.indexAt(_view_local, coords);
    auto spec = _halo_reg_spec.spec(index);
    auto halomem_pos = _halomemory.haloPos(index);
    if(spec.level() == 0 || halomem_pos == nullptr)
      return nullptr;

    if(!_halomemory.toHaloMemoryCoordsWithCheck(index, coords))
      return nullptr;

    return _halomemory.haloPos(index) + _halomemory.haloValueAt(index, coords);
  }

  template<typename IteratorT, typename FunctionT>
  void compute(IteratorT inputBegin, IteratorT inputEnd, IteratorT outputBegin, FunctionT func) {
    while(inputBegin != inputEnd) {
      *outputBegin = func(inputBegin);
      ++inputBegin;
      ++outputBegin;
    }
  }

private:
  struct HaloData {
    dart_handle_t     handle;
    std::vector<int>  indexes;
  };

  struct Data {
    const HaloRegionT&             region;
    std::function<void(HaloData&)> get_halos;
    HaloData                       halo_data;
  };

  void updateHaloIntern(Data& data, bool async) {
    auto rel_dim = data.region.regionSpec().relevantDim() - 1;
    if (data.region.borderRegion() && data.region.borderDim(rel_dim)
        && _cycle_spec[rel_dim] == Cycle::FIXED) {

      return;
    }
    data.get_halos(data.halo_data);

    if (!async)
      dart_wait_local(&data.halo_data.handle);
  }

private:
  MatrixT&            _matrix;
  const StencilSpecT& _stencil_spec;
  const CycleSpecT    _cycle_spec;
  const HaloSpecT     _halo_reg_spec;
  const ViewSpecT     _view_local;
  const ViewSpecT     _view_global;
  const HaloBlockT    _haloblock;
  HaloMemoryT         _halomemory;
  std::map<region_index_t, Data> _region_data;

  iterator       _begin;
  iterator       _end;
  iterator_inner _ibegin;
  iterator_inner _iend;
  iterator_bnd   _bbegin;
  iterator_bnd   _bend;
};

} // namespace dash

#endif // DASH__HALO_HALOMATRIXWRAPPER_H
