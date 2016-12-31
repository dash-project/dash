#ifndef DASH__IO__HDF5__INTERNAL_IMPL_ND_BLOCK_H__
#define DASH__IO__HDF5__INTERNAL_IMPL_ND_BLOCK_H__

#include <hdf5.h>
#include <hdf5_hl.h>

#include <vector>


namespace dash {
namespace io {
namespace hdf5 {

/**
 * Concept:
 *  ______________
 * |  .  .  .  .  |             
 * |..............|
 * |  .  .__._ .  |
 * |....|.....|...|
 * |  . |.  . |.  |
 * |....|_._._|...|
 * |______________|
 *
 * perform three collective writes:
 *
 * 1. top left underfilled blocks
 * 2. fully filled middle blocks
 * 3. down right underfilled blocks
 */
template<
  typename ElementT,
  typename PatternT,
  dim_t ndim,
  dim_t NViewDim >
void StoreHDF::_write_dataset_impl_nd_block(
  dash::MatrixRef< ElementT, ndim, NViewDim, PatternT > & container,
  const hid_t                                           & h5dset,
  const hid_t                                           & internal_type)
{
  auto & pattern       = container.pattern();
  auto   local_elems   = container.sub_local();
  auto   tilesize      = pattern.blocksize(0);
  auto   written_elems = 0;
  
  hdf5_pattern_spec<ndim>   ts;
  hdf5_filespace_spec<ndim> fs = _get_container_extents(container);

  hid_t filespace  = H5Dget_space(h5dset);
  
  // TODO: Currently only the 1-D Case is implemented
  
  /* --------------------- step 1 -------------------------*/
  
  // returns first local range
  auto lrange = dash::local_range(local_elems.begin(), local_elems.end());
  auto lrange_size = std::distance(lrange.begin(), lrange.end());
  
  ts.data_dimsm[0] = lrange_size;
  
  // Create property list for collective writes
  hid_t plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
  
  hid_t memspace  = H5Screate_simple(ndim, ts.data_dimsm, NULL);
  
  // In the 1D case at most 1 unit has a left sided underfilled block
  if(lrange_size > 0 && lrange_size != tilesize){
    // Unit has local elements in an underfilled block
    ts.block[0]      = lrange_size;
    ts.count[0]      = 1;
    ts.offset[0]     = 0;
    // greater than max blocksize, to make hdf5 happy
    ts.stride[0]  = tilesize;
  } else {
    ts.count[0]    = 0;
    ts.offset[0]   = 0;
    ts.block[0]    = 0;
  }

  H5Sselect_hyperslab(
    filespace,
    H5S_SELECT_SET,
    ts.offset,
    ts.stride,
    ts.count,
    ts.block);

  H5Dwrite(h5dset, internal_type, memspace, filespace,
           plist_id, lrange.begin);
  
  written_elems = ts.block[0];
  H5Sclose(memspace);
  
  /* --------------------- step 2 ------------------------ */
  // Get pointer to first element in local range
  auto lbegin_giter = local_elems.begin();
  const ElementT * lbegin_lptr = lbegin_giter.local();
  
  if(lrange_size != tilesize){
    auto second_lrange = dash::local_range(
                                local_elems.begin()+lrange_size,
                                local_elems.end());
    if(std::distance(second_lrange.begin, second_lrange.end) != tilesize){
      // no fully filled block at this unit
      ts.block[0]  = 0;
      ts.count[0]  = 0;
      ts.offset[0] = 0;
    } else {
      ts.block[0]  = tilesize;
      ts.count[0]  = (local_elems.size()-written_elems) / tilesize;
      ts.stride[0] = pattern.teamspec().extent(0) * ts.block[0];
      lbegin_lptr = second_lrange.begin;
    }
  } else {
    lbegin_lptr = lbegin_giter.local();
    if(local_elems.size() < tilesize){
      // SKIP
      // Could be optimized by writing remaining data in this step
      ts.block[0]  = 0;
      ts.count[0]  = 0;
      ts.offset[0] = 0;
    } else {
      ts.block[0]  = tilesize;
      ts.count[0]  = local_elems.size() / tilesize;
      ts.stride[0] = pattern.teamspec().extent(0) * ts.block[0];
    }
  }
  
  H5Sselect_hyperslab(
    filespace,
    H5S_SELECT_SET,
    ts.offset,
    ts.stride,
    ts.count,
    ts.block);

  H5Dwrite(h5dset, internal_type, memspace, filespace,
           plist_id, lbegin_lptr);
  written_elems += ts.count*ts.block;
  
  /* --------------------- step 3 ------------------------ */
  
  auto remaining_elems = local_elems.size() - written_elems;
  if(remaining_elems == 0){
      ts.block[0]  = 0;
      ts.count[0]  = 0;
      ts.offset[0] = 0;
  } else {
    lbegin_giter = local_elems.begin()+written_elems;
    lbegin_lptr  = lbegin_giter.local();
    ts.offset[0] = lbegin_giter.pos();
    ts.count[0]  = 1;
    ts.block[0]  = container.size() - lbegin_giter.pos();
  }
  H5Sselect_hyperslab(
    filespace,
    H5S_SELECT_SET,
    ts.offset,
    ts.stride,
    ts.count,
    ts.block);

  H5Dwrite(h5dset, internal_type, memspace, filespace,
           plist_id, lbegin_lptr);
}

} // namespace hdf5
} // namespace io
} // namespace dash

#endif // DASH__IO__HDF5__INTERNAL_IMPL_ND_BLOCK_H__
