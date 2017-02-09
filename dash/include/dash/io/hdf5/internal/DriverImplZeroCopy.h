#ifndef DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__
#define DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__

#include <hdf5.h>
#include <hdf5_hl.h>
#include <vector>
#include <numeric>
#include <algorithm>

namespace dash {
namespace io {
namespace hdf5 {

template <class Container_t>
void StoreHDF::_process_dataset_impl_zero_copy(StoreHDF::Mode io_mode,
                                               Container_t& container,
                                               const hid_t& h5dset,
                                               const hid_t& internal_type) {
  using pattern_t = typename Container_t::pattern_type;
  constexpr auto ndim = pattern_t::ndim();

  DASH_LOG_DEBUG("Use zero_copy impl");

  hid_t memspace;
  hid_t filespace = H5Dget_space(h5dset);

  // Create property list for collective writes
  hid_t plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  // TODO: Optimize
  auto hyperslabs = _get_hdf_slabs(container.pattern());

  // hyperslab data can be quite large => sort indices only
  std::vector<int> hs_index_set(hyperslabs.size());
  std::iota(hs_index_set.begin(), hs_index_set.end(), 0);

  // Sort hyperslabs by amount of data contrib.
  std::sort(hs_index_set.begin(), hs_index_set.end(),
            [&hyperslabs](const int & a, const int & b)->bool {
    return hyperslabs[a].contrib_data > hyperslabs[b].contrib_data;
  });

  // get maximum number of hyperslabs
  // optimization to avoid storing empty only hyperslabs
  int hs_count_local = hyperslabs.size();
  int hs_count_max = 0;

  DASH_ASSERT_RETURNS(dart_allreduce(&hs_count_local, &hs_count_max, 1,
                                     dart_datatype<int>::value, DART_OP_MAX,
                                     container.team().dart_id()),
                      DART_OK);

  const hdf5_hyperslab_spec<ndim> hs_empty;
  for (int hs_idx = 0; hs_idx < hs_count_max; ++hs_idx) {
    auto& hs = (hs_idx < hs_count_local) ? hyperslabs[hs_idx] : hs_empty;
    auto& ms_edge = hs.memory;
    auto& ts_edge = hs.dataset;
    memspace = H5Screate_simple(ndim, hs.data_extm.data(), NULL);
    H5Sselect_none(filespace);

    if (!hs.contrib_blocks) {
      H5Sselect_none(filespace);
    } else {
      H5Sselect_hyperslab(memspace, H5S_SELECT_SET, ms_edge.offset.data(),
                          ms_edge.stride.data(), ms_edge.count.data(),
                          ms_edge.block.data());

      H5Sselect_hyperslab(filespace, H5S_SELECT_SET, ts_edge.offset.data(),
                          ts_edge.stride.data(), ts_edge.count.data(),
                          ts_edge.block.data());
    }

    if (io_mode == StoreHDF::Mode::WRITE) {
      H5Dwrite(h5dset, internal_type, memspace, filespace, plist_id,
               (container.lbegin()));
    } else {
      H5Dread(h5dset, internal_type, memspace, filespace, plist_id,
              (container.lbegin()));
    }
    H5Sclose(memspace);
  }
  H5Sclose(filespace);
  H5Pclose(plist_id);
}

}  // namespace hdf5
}  // namespace io
}  // namespace dash

#endif  // DASH__IO__HDF5__INTERNAL_IMPL_ZEROCOPY_H__
