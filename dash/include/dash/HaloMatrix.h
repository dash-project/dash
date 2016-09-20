#ifndef DASH__HALOMATRIX_H_INCLUDED
#define DASH__HALOMATRIX_H_INCLUDED

#include <dash/dart/if/dart.h>

//#include <dash/Team.h>
#include <dash/Pattern.h>
//#include <dash/GlobRef.h>
#include <dash/GlobMem.h>
//#include <dash/Allocator.h>
#include <dash/HaloMatrixIterator.h>
#include <dash/Halo.h>
#include <dash/Matrix.h>
#include <type_traits>

namespace dash {

template<typename MatrixT, typename HaloSpecT>
class HaloMatrix
{
  static_assert(MatrixT::ndim() == HaloSpecT::ndim(), "Number of dimensions of Matrix and HaloSpec not equal.");

public:
  using pattern_t  = typename MatrixT::pattern_type;
  using size_type  = typename MatrixT::size_type;
  using index_type = typename MatrixT::index_type;
  using value_t    = typename MatrixT::value_type;
  using ViewSpec_t = ViewSpec<MatrixT::ndim(),index_type>;
  using self_t     = HaloMatrix<MatrixT, HaloSpecT>;
  using HaloBlock_t = HaloBlock<value_t, pattern_t>;
  using HaloBlockView_t  = HaloBlockView<value_t, pattern_t>;
  using HaloMemory_t   = HaloMemory<HaloBlock_t>;

  using iterator             = HaloMatrixIterator<value_t, pattern_t, StencilViewScope::ALL>;
  using const_iterator       = const iterator;
  using iterator_inner       = HaloMatrixIterator<value_t, pattern_t, StencilViewScope::INNER>;
  using const_iterator_inner = const iterator_inner;
  using iterator_bnd         = HaloMatrixIterator<value_t, pattern_t, StencilViewScope::BOUNDARY>;
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
  HaloMatrix(MatrixT & matrix, HaloSpecT & halospec)
    : _matrix(matrix), _halospec(halospec),_view_local(matrix.local.extents()),
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

  void fillHalo(dim_t dim, HaloRegion halo_region)
  {
    const auto & region = _haloblock.halo_region(dim, halo_region);
    if(region.size() > 0 )
    {
      if(MemoryArrange == ROW_MAJOR)
      {
        if(dim == 0)
          dart_get_blocking(_halomemory.haloPos(dim, halo_region), region.begin().dart_gptr(),
              region.size() * sizeof(value_t));
        else
          copyHalos(dim, halo_region, region);
      }
      else
      {
        if(dim == NumDimensions)
          dart_get_blocking(_halomemory.haloPos(dim, halo_region), region.begin().dart_gptr(),
              region.size() * sizeof(value_t));
        else
          copyHalos(dim, halo_region, region);
      }
    }
  }

private:

  void copyHalos(dim_t dim, HaloRegion halo_region, const HaloBlockView_t & region)
  {
    //contiguous elements
    size_type cont_elems = region.region_view().extent(dim);
    size_type num_handle = region.size() / cont_elems;
    auto nbytes = cont_elems * sizeof(value_t);
    dart_handle_t *handle = (dart_handle_t*) malloc (sizeof (dart_handle_t) * num_handle);
    auto it = region.begin();

    auto off = _halomemory.haloPos(dim, halo_region);
    for(auto i = 0; i < num_handle; ++i, it += cont_elems)
      dart_get_handle (off + cont_elems * i, it.dart_gptr(), nbytes, &handle[i]);

    dart_waitall (handle, num_handle);

    for(auto i = 0; i < num_handle; ++i)
      free(handle[i]);
    free(handle);
  }

private:
  const MatrixT &         _matrix;
  const ViewSpec_t        _view_local;
  const ViewSpec_t        _view_global;
  const HaloBlock_t       _haloblock;
  const HaloSpecT &       _halospec;
  HaloMemory_t            _halomemory;

  iterator                _begin;
  iterator                _end;
  iterator_inner          _ibegin;
  iterator_inner          _iend;
  iterator_bnd            _bbegin;
  iterator_bnd            _bend;

};

}  // namespace dash

#endif  // DASH__HALOMATRIX_H_INCLUDED
