#ifndef DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__
#define DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__

#include <hdf5.h>
#include <hdf5_hl.h>
#include <algorithm>

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

  hid_t memspace;
  hid_t filespace = H5Dget_space(h5dset);

  // Create property list for collective writes
  hid_t plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
  
  // TODO: Optimize
  auto hyperslabs = _get_pattern_hdf_spec_boundary(container.pattern());
  hyperslabs.push_back(_get_pattern_hdf_spec(container.pattern()));
  
  // Sort hyperslabs by amount of data contrib.
  std::sort(hyperslabs.begin(), hyperslabs.end(), 
    [](const hdf5_hyperslab_spec<ndim> & a, const hdf5_hyperslab_spec<ndim> & b) -> bool
    { 
        return a.contrib_data > b.contrib_data;
    });
  
  for(auto & hs : hyperslabs){
    auto & ms_edge = hs.memory;
    auto & ts_edge = hs.dataset;
    memspace = H5Screate_simple(ndim, hs.data_extm.data(), NULL);
    H5Sselect_none(filespace);

    if(!hs.contrib_blocks){
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
