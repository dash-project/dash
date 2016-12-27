#ifndef DASH__IO__HDF5__STORAGEDRIVER_H__
#define DASH__IO__HDF5__STORAGEDRIVER_H__

#include <dash/internal/Config.h>

#ifdef DASH_ENABLE_HDF5

#include <dash/Exception.h>
#include <dash/Init.h>
#include <dash/Array.h>
#include <dash/Matrix.h>

#include <sys/stat.h>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <list>
#include <array>
#include <sstream>
#include <typeinfo>
#include <type_traits>
#include <functional>

#ifndef MPI_IMPL_ID
#pragma error "HDF5 module requires dart-mpi"
#endif

#include <dash/dart/if/dart_io.h>

namespace dash {
namespace io {
namespace hdf5 {

/** forward declaration */
template < typename T > hid_t get_h5_datatype();

/// Type of converter function from native type to hdf5 datatype
using type_converter_fun_type = std::function<hid_t()>;

/**
 * DASH wrapper to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 * All operations are collective.
 */
class StoreHDF {
public:
  /**
   * Options which can be passed to dash::io::StoreHDF::write
   * to specify how existing structures are treated and what
   * metadata is stored.
   *
   * Collective operation.
   */
  typedef struct hdf5_options_t {
    /// Overwrite HDF5 file if already existing
    bool          overwrite_file;
    /**
     * Modify an already existing HDF5 dataset.
     * If the dataset is not existing, throws a runtime error
     */
    bool          modify_dataset;
    /// Store dash pattern characteristics as metadata in HDF5 file
    bool          store_pattern;
    /// Restore pattern from metadata if HDF5 file contains any.
    bool          restore_pattern;
    /// Metadata attribute key in HDF5 file.
    std::string   pattern_metadata_key;
  } hdf5_options;

  /**
   * test at compile time if pattern is compatible
   * \return true if pattern is compatible
   */
private:
  template < class pattern_t >
  static constexpr bool _compatible_pattern()
  {
    return dash::pattern_partitioning_traits<pattern_t>::type::rectangular &&
           dash::pattern_layout_traits<pattern_t>::type::linear &&
           !dash::pattern_mapping_traits<pattern_t>::type::shifted &&
           !dash::pattern_mapping_traits<pattern_t>::type::diagonal;
    // TODO: check if mapping is regular by checking pattern property
  }

  static std::vector<std::string> _split_string(
                                    const std::string & str,
                                    const char delim)
  {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
      if(item != ""){
        elems.push_back(item);
      }
    }
    return elems;
  }

public:

  /**
   * Store all dash::Matrix values in an HDF5 file using parallel IO.
   *
   * Collective operation.
   *
   * \param  array     Array to store
   * \param  filename  Filename of HDF5 file including extension
   * \param  datapath   HDF5 Dataset in which the data is stored
   * \param  foptions
   */
 template <typename Container_t>
   typename std::enable_if <
    _compatible_pattern<typename Container_t::pattern_type>(),
    void>::type
  static write(
      /// Import data in this Container
      Container_t &  array,
      /// Filename of HDF5 file including extension
      std::string    filename,
      /// HDF5 Dataset in which the data is stored
      std::string    datapath,
      /// options how to open and modify data
      hdf5_options   foptions = _get_fdefaults(),
      /// \cstd::function to convert native type into h5 type
      type_converter_fun_type  to_h5_dt_converter =
                        get_h5_datatype<typename Container_t::value_type>)
  {
    using pattern_t = typename Container_t::pattern_type;
    using extent_t  = typename pattern_t::size_type;
    using index_t   = typename Container_t::index_type;
    using value_t   = typename Container_t::value_type;

    constexpr auto ndim = pattern_t::ndim();

    // Check if container dims match pattern dims
    _verify_container_dims(array);
    static_assert(std::is_same<index_t, typename pattern_t::index_type>::value,
                  "Specified index_t differs from pattern_t::index_type");

    auto pattern             = array.pattern();
    const dash::Team & team  = array.team();

    // Map native types to HDF5 types
    auto h5datatype  = to_h5_dt_converter();
    // for tracking opened groups
    std::list<hid_t> open_groups;
    // Split path in groups and dataset
    auto path_vec    = _split_string(datapath, '/');
    auto dataset     = path_vec.back();
    // remove dataset from path
    path_vec.pop_back();

    /* HDF5 definition */
    hid_t   file_id;
    hid_t   h5dset;
    hid_t   internal_type;
    hid_t   plist_id; // property list identifier
    hid_t   filespace;
    hid_t   memspace;
    hid_t   loc_id;

    hdf5_pattern_spec<ndim> ts;

    // get hdf pattern layout
    ts = _get_pattern_hdf_spec(pattern);

    // setup mpi access
    plist_id = H5Pcreate(H5P_FILE_ACCESS);
    dart__io__hdf5__prep_mpio(plist_id, team.dart_id());

    dash::Shared<int> f_exists;
    if (team.myid() == 0) {
      if (access( filename.c_str(), F_OK ) != -1) {
        // check if file exists
        f_exists.set(static_cast<int> (H5Fis_hdf5( filename.c_str())));
      } else {
        f_exists.set(-1);
      }
    }
    team.barrier();

    if (foptions.overwrite_file || (f_exists.get() <= 0)) {
      // HD5 create file
      file_id = H5Fcreate(filename.c_str(),
                          H5F_ACC_TRUNC,
                          H5P_DEFAULT,
                          plist_id);
    } else {
      // Open file in RW mode
      file_id = H5Fopen(filename.c_str(), H5F_ACC_RDWR, plist_id );
    }

    // close property list
    H5Pclose(plist_id);

    // Traverse path
    loc_id = file_id;
    for(std::string elem : path_vec){
          if(H5Lexists(loc_id, elem.c_str(), H5P_DEFAULT)){
            // open group
            DASH_LOG_DEBUG("Open Group", elem);
            loc_id = H5Gopen2(loc_id, elem.c_str(), H5P_DEFAULT);
          } else {
            // create group
            DASH_LOG_DEBUG("Create Group", elem);
            loc_id = H5Gcreate2(loc_id, elem.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
          }
          if(loc_id != file_id){
            open_groups.push_front(loc_id);
          }
    }

    // Create dataspace
    filespace     = H5Screate_simple(ndim, ts.data_dimsf, NULL);
    memspace      = H5Screate_simple(ndim, ts.data_dimsm, NULL);
    internal_type = H5Tcopy(h5datatype);

    if(foptions.modify_dataset){
      // Open dataset in RW mode
      h5dset = H5Dopen(loc_id, dataset.c_str(), H5P_DEFAULT);
    } else {
      // Create dataset
      h5dset = H5Dcreate(loc_id, dataset.c_str(), internal_type, filespace,
                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }

    // Close global dataspace
    H5Sclose(filespace);

    filespace = H5Dget_space(h5dset);

    H5Sselect_hyperslab(
      filespace,
      H5S_SELECT_SET,
      ts.offset,
      ts.stride,
      ts.count,
      ts.block);

    // Create property list for collective writes
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

    // Write completely filled blocks
    H5Dwrite(h5dset, internal_type, memspace, filespace,
             plist_id, array.lbegin());

    // write underfilled blocks
    // get hdf pattern layout
    ts = _get_pattern_hdf_spec_underfilled(pattern);
    memspace      = H5Screate_simple(ndim, ts.data_dimsm, NULL);
    H5Sselect_hyperslab(
      filespace,
      H5S_SELECT_SET,
      ts.offset,
      ts.stride,
      ts.count,
      ts.block);

    H5Dwrite(h5dset, internal_type, memspace, filespace,
             plist_id, array.lbegin());

    // Add Attributes
    if (foptions.store_pattern) {
      DASH_LOG_DEBUG("store pattern in hdf5 file");
      auto pat_key = foptions.pattern_metadata_key.c_str();
      extent_t pattern_spec[ndim * 4];

      // Delete old attribute when overwriting dataset
      if(foptions.modify_dataset){
        H5Adelete(h5dset, pat_key);
      }
      // Structure is
      // sizespec, teamspec, blockspec, blocksize
      for (int i = 0; i < ndim; ++i) {
        pattern_spec[i]              = pattern.sizespec().extent(i);
        pattern_spec[i + ndim]       = pattern.teamspec().extent(i);
        pattern_spec[i + (ndim * 2)] = pattern.blockspec().extent(i);
        pattern_spec[i + (ndim * 3)] = pattern.blocksize(i);
      }

      hsize_t attr_len[] = { static_cast<hsize_t> (ndim * 4) };
      hid_t attrspace = H5Screate_simple(1, attr_len, NULL);
      hid_t attribute_id = H5Acreate(
                             h5dset, pat_key, H5T_NATIVE_LONG,
                             attrspace, H5P_DEFAULT, H5P_DEFAULT);
      H5Awrite(attribute_id, H5T_NATIVE_LONG, &pattern_spec);
      H5Aclose(attribute_id);
      H5Sclose(attrspace);
    }

    // Close all
    H5Pclose(plist_id);
    H5Dclose(h5dset);
    H5Sclose(filespace);
    H5Sclose(memspace);
    H5Tclose(internal_type);
    for(auto group_id : open_groups){
      H5Gclose(group_id);
    }
    H5Fclose(file_id);
  }

  /**
   * Read an HDF5 dataset into a dash::Matrix using parallel IO
   * if the matrix is already allocated, the sizes have to match
   * the HDF5 dataset sizes and all data will be overwritten.
   * Otherwise the matrix will be allocated.
   *
   * Collective operation.
   */
 template <typename Container_t>
   typename std::enable_if <
    _compatible_pattern<typename Container_t::pattern_type>(),
    void
  >::type
  static read(
      /// Import data in this Container
      Container_t &  matrix,
      /// Filename of HDF5 file including extension
      std::string    filename,
      /// HDF5 Dataset in which the data is stored
      std::string    datapath,
      /// options how to open and modify data
      hdf5_options   foptions = _get_fdefaults(),
      /// \cstd::function to convert native type into h5 type
      type_converter_fun_type  to_h5_dt_converter =
                        get_h5_datatype<typename Container_t::value_type>)
  {
    using pattern_t = typename Container_t::pattern_type;
    using extent_t  = typename pattern_t::size_type;
    using index_t   = typename Container_t::index_type;
    using value_t   = typename Container_t::value_type;

    constexpr auto ndim = pattern_t::ndim();

    static_assert(std::is_same<index_t,
                               typename pattern_t::index_type>::value,
                  "Specified index_t differs from pattern_t::index_type");

    // HDF5 definition
    hid_t   file_id;
    hid_t   h5dset;
    hid_t   internal_type;
    hid_t   plist_id; // property list identifier
    hid_t   filespace;
    hid_t   memspace;
    // global data dims
    hsize_t data_dimsf[ndim];
    herr_t  status;
    // Map native types to HDF5 types
    hid_t   h5datatype;
    // rank of hdf5 dataset
    int     rank;
    hdf5_pattern_spec<ndim> ts;

    // Check if matrix is already allocated
    bool is_alloc = (matrix.size() != 0);

    // Setup MPI IO
    plist_id = H5Pcreate(H5P_FILE_ACCESS);
    if(is_alloc){
      dart__io__hdf5__prep_mpio(plist_id, matrix.team().dart_id());
    } else {
      dart__io__hdf5__prep_mpio(plist_id, dash::Team::All().dart_id());
    }

    // HD5 create file
    file_id = H5Fopen(filename.c_str(), H5P_DEFAULT, plist_id );

    // close property list
    H5Pclose(plist_id);

    // Create dataset
    h5dset = H5Dopen(file_id, datapath.c_str(), H5P_DEFAULT);

    // Get dimensions of data
    filespace     = H5Dget_space(h5dset);
    rank          = H5Sget_simple_extent_ndims(filespace);

    DASH_ASSERT_EQ(rank, ndim,
                   "Data dimension of HDF5 dataset does not match matrix "
                   "dimension");

    status        = H5Sget_simple_extent_dims(filespace, data_dimsf, NULL);

    std::array<extent_t, ndim>           size_extents;
    std::array<extent_t, ndim>           team_extents;
    std::array<dash::Distribution, ndim> dist_extents;
    extent_t hdf_dash_pattern[ndim * 4];

    // set matrix size according to hdf5 dataset dimensions
    for (int i = 0; i < ndim; ++i) {
      size_extents[i] = data_dimsf[i];
    }

    // Check if file contains DASH metadata and recreate the pattern
    auto pat_key   = foptions.pattern_metadata_key.c_str();

    if (!is_alloc                        // not allocated
        && foptions.restore_pattern      // pattern should be restored
        && H5Aexists(h5dset, pat_key)) { // hdf5 contains pattern
      hsize_t attr_len[]  = { ndim * 4};
      hid_t attrspace      = H5Screate_simple(1, attr_len, NULL);
      hid_t attribute_id  = H5Aopen(h5dset, pat_key, H5P_DEFAULT);

      H5Aread(attribute_id, H5T_NATIVE_LONG, hdf_dash_pattern);
      H5Aclose(attribute_id);
      H5Sclose(attrspace);

      for (int i = 0; i < ndim; ++i) {
        size_extents[i]  = static_cast<extent_t> (hdf_dash_pattern[i]);
        team_extents[i]  = static_cast<extent_t> (hdf_dash_pattern[i + ndim]);
        dist_extents[i]  = dash::TILE(hdf_dash_pattern[i + (ndim * 3)]);
      }
      DASH_LOG_DEBUG("Created pattern according to metadata");

      const pattern_t pattern(
        dash::SizeSpec<ndim>(size_extents),
        dash::DistributionSpec<ndim>(dist_extents),
        dash::TeamSpec<ndim>(team_extents),
        dash::Team::All());

      // Allocate DASH Matrix
      matrix.allocate(pattern);

    } else if (is_alloc) {
      DASH_LOG_DEBUG("Matrix already allocated");
      // Check if matrix extents match data extents
      auto pattern_extents = matrix.pattern().extents();
      for (int i = 0; i < ndim; ++i) {
        DASH_ASSERT_EQ(
          size_extents[i],
          pattern_extents[i],
          "Matrix extents do not match data extents");
      }
    } else {
      // Auto deduce pattern
      const pattern_t pattern(
        dash::SizeSpec<ndim>(size_extents),
        dash::DistributionSpec<ndim>(),
        dash::TeamSpec<ndim>(),
        dash::Team::All());

      matrix.allocate(pattern);
    }

    h5datatype = to_h5_dt_converter();
    internal_type = H5Tcopy(h5datatype);

    // setup extends per dimension
    auto pattern = matrix.pattern();
    //DASH_LOG_DEBUG("Pattern", pattern);
    ts = _get_pattern_hdf_spec(pattern);

    // Create dataspace
    memspace = H5Screate_simple(ndim, ts.data_dimsm, NULL);

    H5Sselect_hyperslab(
      filespace,
      H5S_SELECT_SET,
      ts.offset,
      ts.stride,
      ts.count,
      ts.block);

    // Create property list for collective reads
    plist_id = H5Pcreate(H5P_DATASET_XFER);
    H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
    
    // read data
    H5Dread(h5dset, internal_type, memspace, filespace,
            plist_id, matrix.lbegin());

    // read underfilled blocks
    // get hdf pattern layout
    ts = _get_pattern_hdf_spec_underfilled(pattern);
    memspace      = H5Screate_simple(ndim, ts.data_dimsm, NULL);

    H5Sselect_hyperslab(
      filespace,
      H5S_SELECT_SET,
      ts.offset,
      ts.stride,
      ts.count,
      ts.block);

    // Read underfilled blocks by pattern
    H5Dread(h5dset, internal_type, memspace, filespace,
            plist_id, matrix.lbegin());

    // Close all
    H5Pclose(plist_id);
    H5Dclose(h5dset);
    H5Sclose(filespace);
    H5Sclose(memspace);
    H5Tclose(internal_type);
    H5Fclose(file_id);
  }

public:
  /**
   * Default file options.
   *
   * \return hdf5_options struct
   */
  static inline hdf5_options get_default_options()
  {
    return _get_fdefaults();
  }

  /**
   * hdf5 pattern specification for parallel IO
   */
  template <
    dim_t ndim >
  struct hdf5_pattern_spec {
    hsize_t data_dimsf[ndim];
    hsize_t data_dimsm[ndim];
    hsize_t count[ndim];
    hsize_t stride[ndim];
    hsize_t offset[ndim];
    hsize_t block[ndim];
  };

private:

  static inline hdf5_options _get_fdefaults()
  {
    hdf5_options fopt;
    fopt.overwrite_file       = true;
    fopt.modify_dataset       = false;
    fopt.store_pattern        = true;
    fopt.restore_pattern      = true;
    fopt.pattern_metadata_key = "DASH_PATTERN";

    return fopt;
  }

  /**
   * convert a dash pattern into a hdf5 pattern
   * \param pattern_t pattern
   * \return hdf5_pattern_spec<ndim>
   */
  template < class pattern_t >
  const static inline hdf5_pattern_spec<pattern_t::ndim()>
    _get_pattern_hdf_spec(const pattern_t & pattern)
  {
    constexpr auto ndim = pattern_t::ndim();
    hdf5_pattern_spec<ndim> ts;
    // setup extends per dimension
    for (int i = 0; i < ndim; ++i) {
      auto tilesize    = pattern.blocksize(i);
      ts.data_dimsf[i] = pattern.extent(i);
      ts.data_dimsm[i] = (pattern.local_extent(i) / tilesize) * tilesize;
      // number of tiles in this dimension
      ts.count[i]      = pattern.local_extent(i) / tilesize;
      ts.offset[i]     = pattern.local_block(0).offset(i);
      ts.block[i]      = pattern.blocksize(i);
      ts.stride[i]     = pattern.teamspec().extent(i) * ts.block[i];
    }
    return ts;
  }

  /**
   * get the layout of the last underfilled block of a dash::BlockPattern
   * if the calling unit does not have any underfilled blocks, a zero size
   * block is returned.
   * \param pattern_t pattern
   * \return hdf5_pattern_spec<ndim>
   */
  template < class pattern_t >
  const static inline hdf5_pattern_spec<pattern_t::ndim()>
    _get_pattern_hdf_spec_underfilled(const pattern_t & pattern)
  {
    constexpr auto ndim = pattern_t::ndim();
    hdf5_pattern_spec<ndim> ts;

    for (int i = 0; i < ndim ; i++) {
      auto tilesize    = pattern.blocksize(i);
      auto localsize   = pattern.local_extent(i);
      auto localblocks = localsize / tilesize;
      auto lfullsize   = localblocks * tilesize;

      ts.data_dimsf[i] = pattern.extent(i);
      ts.data_dimsm[i] = localsize - lfullsize;
      ts.stride[i]     = tilesize;
      if (localsize != lfullsize) {
        ts.count[i]    = 1;
        ts.offset[i]   = pattern.local_block(localblocks).offset(i);
        ts.block[i]    = localsize - lfullsize;
      } else {
        ts.count[i]    = 0;
        ts.offset[i]   = 0;
        ts.block[i]    = 0;
      }
    }
    return ts;
  }

  
  template <
    dim_t ndim,
    typename value_t,
    typename index_t,
    typename pattern_t>
  static inline void _verify_container_dims(const Matrix<value_t, ndim, index_t, pattern_t> & container)
  {
    static_assert(ndim == pattern_t::ndim(),
                  "Pattern dimension has to match matrix dimension");
  }
  template < class Container_t >
  static inline void _verify_container_dims(const Container_t & container)
  {return;}


};

} // namespace hdf5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/StorageDriver-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__STORAGEDRIVER_H__
