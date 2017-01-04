#ifndef DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__
#define DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__

#include <hdf5.h>
#include <hdf5_hl.h>

namespace dash {
namespace io {
namespace hdf5 {

template < class Container_t >
void StoreHDF::_process_dataset_impl_zero_copy(
              StoreHDF::Mode io_mode,
              Container_t & container,
              const hid_t & h5dset,
              const hid_t & internal_type)
{
  using pattern_t = typename Container_t::pattern_type;
  constexpr auto ndim = pattern_t::ndim();

  DASH_LOG_DEBUG("Use zero_copy impl");

  // get hdf pattern layout
  auto hyperslabs_center = _get_pattern_hdf_spec(container.pattern());
  auto & ms = hyperslabs_center.memory;
  auto & ts = hyperslabs_center.dataset;

  // Create property list for collective writes
  hid_t plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  hid_t filespace = H5Dget_space(h5dset);
  hid_t memspace = H5Screate_simple(ndim, hyperslabs_center.data_extm.data(), NULL);

  if(hyperslabs_center.underfilled_blocks){
    // this unit holds only underfilled blocks
    H5Sselect_none(filespace);
  } else {
    H5Sselect_hyperslab(
                      memspace,
                      H5S_SELECT_SET,
                      ms.offset.data(),
                      ms.stride.data(),
                      ms.count.data(),
                      ms.block.data());
    H5Sselect_hyperslab(
                      filespace,
                      H5S_SELECT_SET,
                      ts.offset.data(),
                      ts.stride.data(),
                      ts.count.data(),
                      ts.block.data());
  }

  DASH_LOG_DEBUG("process completely filled blocks");
  if(io_mode == StoreHDF::Mode::WRITE){
    H5Dwrite(h5dset, internal_type, memspace, filespace,
           plist_id, container.lbegin());
  } else {
    H5Dread(h5dset, internal_type, memspace, filespace,
           plist_id, container.lbegin());
  }
  H5Sclose(memspace);
  
  // TODO: Optimize
  auto hyperslabs_edges = _get_pattern_hdf_spec_edges(container.pattern());
  for(auto & hs_edge : hyperslabs_edges){
    auto & ms_edge = hs_edge.memory;
    auto & ts_edge = hs_edge.dataset;
    memspace = H5Screate_simple(ndim, hs_edge.data_extm.data(), NULL);

    if(!hs_edge.underfilled_blocks){
      H5Sselect_none(filespace);
    } else {
      H5Sselect_hyperslab(
                      memspace,
                      H5S_SELECT_SET,
                      ms_edge.offset.data(),
                      ms_edge.stride.data(),
                      ms_edge.count.data(),
                      ms_edge.block.data());
      
      H5Sselect_hyperslab(
                      filespace,
                      H5S_SELECT_SET,
                      ts_edge.offset.data(),
                      ts_edge.stride.data(),
                      ts_edge.count.data(),
                      ts_edge.block.data());
    }

    DASH_LOG_DEBUG("write partially filled blocks");

    if(io_mode == StoreHDF::Mode::WRITE){
      H5Dwrite(h5dset, internal_type, memspace, filespace,
             plist_id, (container.lbegin()));
    } else {
      H5Dread(h5dset, internal_type, memspace, filespace,
             plist_id, (container.lbegin()));
    }
    H5Sclose(memspace);
  }
  H5Sclose(filespace);
  H5Pclose(plist_id);
}

} // namespace hdf5
} // namespace io
} // namespace dash

#endif // DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__
