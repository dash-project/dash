#ifndef DASH__HALOMATRIX_H_INCLUDED
#define DASH__HALOMATRIX_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Pattern.h>
#include <dash/GlobMem.h>
#include <dash/Matrix.h>

#include <dash/experimental/Halo.h>
#include <dash/experimental/iterator/HaloMatrixIterator.h>

#include <type_traits>


namespace dash {
namespace experimental {

template<typename MatrixT, typename HaloSpecT>
class HaloMatrix
{
  static_assert(MatrixT::ndim() == HaloSpecT::ndim(),
                "Number of dimensions of Matrix and HaloSpec not equal.");

public:
  using pattern_t            = typename MatrixT::pattern_type;
  using size_type            = typename MatrixT::size_type;
  using index_type           = typename MatrixT::index_type;
  using value_t              = typename MatrixT::value_type;
  using ViewSpec_t           = ViewSpec<MatrixT::ndim(),index_type>;
  using self_t               = HaloMatrix<MatrixT, HaloSpecT>;
  using HaloBlock_t          = HaloBlock<value_t, pattern_t>;
  using HaloBlockView_t      = typename HaloBlock_t::block_view_t;
  using HaloMemory_t         = HaloMemory<HaloBlock_t>;

  using iterator             = HaloMatrixIterator<
                                 value_t,
                                 pattern_t,
                                 StencilViewScope::ALL>;
  using const_iterator       = const iterator;
  using iterator_inner       = HaloMatrixIterator<
                                 value_t,
                                 pattern_t,
                                 StencilViewScope::INNER>;
  using const_iterator_inner = const iterator_inner;
  using iterator_bnd         = HaloMatrixIterator<
                                 value_t,
                                 pattern_t,
                                 StencilViewScope::BOUNDARY>;
  using const_iterator_bnd   = const iterator_inner;

private:
  static constexpr dim_t NumDimensions = pattern_t::ndim();
  static constexpr MemArrange MemoryArrange = pattern_t::memory_order();

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global matrix instances
   * that are declared before \c dash::Init().
   */
  //TODO adapt to more than one local block
  HaloMatrix(MatrixT & matrix, const HaloSpecT & halospec)
    : _matrix(matrix),
      _halospec(halospec),
      _view_local(matrix.local.extents()),
      _view_global(ViewSpec_t(matrix.local.offsets(), matrix.local.extents())),
      _haloblock(matrix.begin().globmem(), matrix.pattern(), _view_global, halospec),
      _halomemory(_haloblock),
      _begin(_haloblock, _halomemory, 0),
      _end(_haloblock, _halomemory, _haloblock.view_save().size()),
      _ibegin(_haloblock, _halomemory, 0),
      _iend(_haloblock, _halomemory, _haloblock.view_inner().size()),
      _bbegin(_haloblock, _halomemory, 0),
      _bend(_haloblock, _halomemory, _haloblock.boundary_size())
  {
    const auto & offsets = halospec.halo_offsets();

    for(auto d = 0; d < NumDimensions; ++d)
    {
      if(offsets[d].minus)
        initBlockViewData(d, HaloRegion::MINUS);

      if(offsets[d].plus)
        initBlockViewData(d, HaloRegion::PLUS);
    }
  }

  ~HaloMatrix()
  {
    for(auto & view : _blockview_data)
      free(view.second.handle);
  }

  iterator begin() noexcept
  {
    return _begin;
  }

  const_iterator begin() const noexcept
  {
    return _begin;
  }

  iterator end() noexcept
  {
    return _end;
  }

  const_iterator end() const noexcept
  {
    return _end;
  }

  iterator_inner ibegin() noexcept
  {
    return _ibegin;
  }

  const_iterator_inner ibegin() const noexcept
  {
    return _ibegin;
  }

  iterator_inner iend() noexcept
  {
    return _iend;
  }

  const_iterator_inner iend() const noexcept
  {
    return _iend;
  }

  iterator_bnd bbegin() noexcept
  {
    return _bbegin;
  }

  const_iterator_bnd bbegin() const noexcept
  {
    return _bbegin;
  }

  iterator_bnd bend() noexcept
  {
    return _bend;
  }

  const_iterator_bnd bend() const noexcept
  {
    return _bend;
  }

  const HaloBlockView_t & haloRegion(dim_t dim, HaloRegion halo_region)
  {
    return _haloblock.halo_region(dim, halo_region);
  }

  const HaloBlock_t & haloBlock()
  {
    return _haloblock;
  }

  void updateHalosAsync()
  {
    for(auto & view : _blockview_data)
      updateHaloIntern(view.first.first, view.first.second, true);
  }

  void waitHalosAsync()
  {
    for(auto & view : _blockview_data)
      dart_waitall(view.second.handle, view.second.num_handles);
  }

  void updateHalos()
  {
    for(auto & view : _blockview_data)
      updateHaloIntern(view.first.first, view.first.second, false);
  }

  void updateHalo(dim_t dim, HaloRegion region)
  {
    updateHaloIntern(dim, region, false);
  }

  const ViewSpec_t & getLocalView()
  {
    return _view_local;
  }

private:
  void initBlockViewData(dim_t dim, HaloRegion region)
  {
    const auto blockview = _haloblock.halo_region(dim, region);
    if(blockview.size() == 0)
      return;

    // number of contiguous elements
    size_type cont_elems;
    size_type num_handle;
    if((MemoryArrange == ROW_MAJOR && dim == 0) ||
       (MemoryArrange == COL_MAJOR && dim == NumDimensions - 1))
    {
      cont_elems = blockview.size();
      num_handle = 1;
    }
    else
    {
      cont_elems = blockview.region_view().extent(dim);
      num_handle = blockview.size() / cont_elems;
    }

    auto nbytes = cont_elems * sizeof(value_t);
    dart_handle_t * handle = (dart_handle_t*) malloc (sizeof (dart_handle_t) * num_handle);
    for(auto i = 0; i < num_handle; ++i)
      handle[i] = nullptr;
    _blockview_data.insert(std::make_pair(
          std::move(std::make_pair(dim, region)), Data{std::move(blockview), handle, num_handle, cont_elems, nbytes}));
  }

  void updateHaloIntern(dim_t dim, HaloRegion region, bool async)
  {
    auto it_find = _blockview_data.find(std::make_pair(dim, region));
    if(it_find != _blockview_data.end())
    {
      auto & data = it_find->second;
      auto off = _halomemory.haloPos(dim, region);
      auto it = data.blockview.begin();
      for(auto i = 0; i < data.num_handles; ++i, it += data.cont_elems){
        dart_storage_t ds = dash::dart_storage<value_t>(data.cont_elems);
        dart_get_handle (off + ds.nelem * i, it.dart_gptr(), ds.nelem, ds.dtype, &(data.handle[i]));
      }
      if(!async)
        dart_waitall(data.handle, data.num_handles);
    }
  }

private:
  MatrixT &               _matrix;
  const HaloSpecT &       _halospec;
  const ViewSpec_t        _view_local;
  const ViewSpec_t        _view_global;
  const HaloBlock_t       _haloblock;
  HaloMemory_t            _halomemory;

  struct Data
  {
    const HaloBlockView_t blockview;
    dart_handle_t *       handle;
    size_type             num_handles;
    size_type             cont_elems;
    std::uint64_t         nbytes;
  };
  std::map<std::pair<dim_t, HaloRegion>, Data> _blockview_data;

  iterator                _begin;
  iterator                _end;
  iterator_inner          _ibegin;
  iterator_inner          _iend;
  iterator_bnd            _bbegin;
  iterator_bnd            _bend;

};

}  // namespace dash
}  // namespace experimental

#endif  // DASH__HALOMATRIX_H_INCLUDED
