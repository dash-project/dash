#ifndef DASH__IO__HDF5__INTERNAL_IMPL_ND_BLOCK_H__
#define DASH__IO__HDF5__INTERNAL_IMPL_ND_BLOCK_H__

#include <hdf5.h>
#include <hdf5_hl.h>

namespace dash
{
namespace io
{
namespace hdf5
{

template<
  typename ElementT,
  typename PatternT,
  dim_t NDim,
  dim_t NViewDim >
void StoreHDF::_write_dataset_impl_nd_block(
              dash::MatrixRef< ElementT, NDim, NViewDim, PatternT > & container,
              const hid_t & h5dset,
              const hid_t & internal_type)
{
#if 0
  auto & pattern = container.pattern();
  auto first_pos = container.begin().pos();
  auto first_coords = pattern.coords(first_pos);

  auto last_pos = (container.end() - 1).pos();
  auto last_coords = pattern.coords(last_pos);

  auto local_elems = container.sub_local();
  // TODO
#endif
}

#endif // DASH__IO__HDF5__INTERNAL_IMPL_ND_BLOCK_H__

} // namespace hdf5
} // namespace io
} // namespace dash