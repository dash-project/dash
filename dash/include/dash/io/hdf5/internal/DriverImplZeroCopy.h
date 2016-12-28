#ifndef DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__
#define DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__

#include <hdf5.h>
#include <hdf5_hl.h>

namespace dash {
namespace io {
namespace hdf5 {

template < class Container_t >
void StoreHDF::_write_dataset_impl_zero_copy(
              Container_t & container,
              const hid_t & h5dset,
              const hid_t & internal_type)
{
  using pattern_t = typename Container_t::pattern_type;
  constexpr auto ndim = pattern_t::ndim();
  hdf5_pattern_spec<ndim> ts;

  // get hdf pattern layout
  ts = _get_pattern_hdf_spec(container.pattern());

  // Create property list for collective writes
  hid_t plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  hid_t filespace = H5Dget_space(h5dset);
  hid_t memspace = H5Screate_simple(ndim, ts.data_dimsm, NULL);

  H5Sselect_hyperslab(
                      filespace,
                      H5S_SELECT_SET,
                      ts.offset,
                      ts.stride,
                      ts.count,
                      ts.block);

  // Write completely filled blocks
  H5Dwrite(h5dset, internal_type, memspace, filespace,
           plist_id, container.lbegin());
  H5Sclose(memspace);

  // write underfilled blocks
  // get hdf pattern layout
  auto ts_rem = _get_pattern_hdf_spec_underfilled(container.pattern());
  memspace = H5Screate_simple(ndim, ts_rem.data_dimsm, NULL);

  H5Sselect_hyperslab(
                      filespace,
                      H5S_SELECT_SET,
                      ts_rem.offset,
                      ts_rem.stride,
                      ts_rem.count,
                      ts_rem.block);

  H5Dwrite(h5dset, internal_type, memspace, filespace,
           plist_id, container.lbegin());

  H5Sclose(memspace);
  H5Sclose(filespace);
  H5Pclose(plist_id);
}

#endif // DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__

} // namespace hdf5
} // namespace io
} // namespace dash