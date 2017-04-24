#ifndef DASH__HALOMATRIX_H_INCLUDED
#define DASH__HALOMATRIX_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Pattern.h>
#include <dash/memory/GlobStaticMem.h>
#include <dash/Matrix.h>

#include <dash/experimental/Halo.h>
#include <dash/experimental/iterator/HaloMatrixIterator.h>

#include <type_traits>

namespace dash {
namespace experimental {

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
      size_type cont_elems;
      size_type num_handle;

      if (MemoryArrange == ROW_MAJOR)
        cont_elems = region.region().extent(NumDimensions - 1);
      else
        cont_elems = region.region().extent(0);
      num_handle   = region.size() / cont_elems;

      auto           nbytes = cont_elems * sizeof(ElementT);
      dart_handle_t* handle = (dart_handle_t*)malloc(sizeof(dart_handle_t) * num_handle);
      for (auto i = 0; i < num_handle; ++i)
        handle[i] = nullptr;
      _region_data.insert(
          std::make_pair(region.index(), Data{region, handle, num_handle, cont_elems, nbytes}));
    }
  }

  ~HaloMatrixWrapper() {
    for (auto& region : _region_data)
      free(region.second.handle);
  }

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

  /*const HaloBlockView_t & haloRegion(dim_t dim, HaloRegion halo_region)
  {
    return _haloblock.halo_region(dim, halo_region);
  }*/

  const HaloBlockT& haloBlock() { return _haloblock; }

  void updateHalosAsync() {
    for (const auto& region : _region_data)
      updateHaloIntern(region.second, true);
  }

  void waitHalosAsync() {
    for (auto& region : _region_data)
      dart_waitall(region.second.handle, region.second.num_handles);
  }

  void updateHalos() {
    for (const auto& region : _region_data)
      updateHaloIntern(region.second, false);
  }

  void updateHalo(region_index_t index) {
    auto it_find = _region_data.find(index);
    if (it_find != _region_data.end())
      updateHaloIntern(it_find->second, false);
  }

  const ViewSpecT& getLocalView() { return _view_local; }

  template <typename FunctionT>
  void setFixedHalos(FunctionT f) {
    for (const auto& region : _haloblock.boundary_regions()) {
      if (region.border() && _cycle_spec[region.regionSpec().relevantDim() - 1] == Cycle::FIXED) {
        auto pos_ptr = _halomemory.haloPos(region.index());
        auto spec    = region.regionSpec();
        auto rel_ext = region.region().offsets();
        auto reg_ext = region.region().extents();
        for (auto d = 0; d < rel_ext.size(); ++d) {
          if (spec[d] == 0)
            rel_ext[d] = -reg_ext[d];
          else if (spec[d] == 1)
            rel_ext[d] = 0;
          else
            rel_ext[d] = reg_ext[d];
        }
        auto it = region.begin();
        for (auto i = 0; i < region.size(); ++i, ++it) {
          auto coords = it.gcoords();
          for (auto d = 0; d < coords.size(); ++d)
            coords[d] += rel_ext[d];
          *(pos_ptr + i) = f(coords);
        }
      }
    }
  }

private:
  struct Data {
    const HaloRegionT& region;
    dart_handle_t*     handle;
    size_type          num_handles;
    size_type          cont_elems;
    uint64_t           nbytes;
  };

  void updateHaloIntern(const Data& data, bool async) {
    if (data.region.border() &&
        _cycle_spec[data.region.regionSpec().relevantDim() - 1] == Cycle::FIXED)
      return;

    auto off = _halomemory.haloPos(data.region.index());
    auto it  = data.region.begin();

    for (auto i(0); i < data.num_handles; ++i, it += data.cont_elems) {
      dart_storage_t ds = dash::dart_storage<ElementT>(data.cont_elems);
      dart_get_handle(off + ds.nelem * i, it.dart_gptr(), ds.nelem, ds.dtype, &(data.handle[i]));
    }

    if (!async)
      dart_waitall(data.handle, data.num_handles);
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
} // namespace experimental

#endif // DASH__HALOMATRIX_H_INCLUDED
