#ifndef DASH__IO__HDF5__STORAGEDRIVER_H__
#define DASH__IO__HDF5__STORAGEDRIVER_H__

#include <dash/internal/Config.h>

#ifdef DASH_ENABLE_HDF5

#ifndef DASH_MPI_IMPL_ID
#error "HDF5 module requires dart-mpi"
#endif

#include <dash/Exception.h>
#include <dash/Init.h>
#include <dash/Array.h>
#include <dash/Matrix.h>
#include <dash/Shared.h>

#include <sys/stat.h>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <list>
#include <array>
#include <string>
#include <sstream>
#include <typeinfo>
#include <type_traits>
#include <functional>
#include <utility>

#include <dash/dart/if/dart_io.h>

namespace dash {
namespace io {
namespace hdf5 {

/** forward declaration */
template <typename T>
hid_t get_h5_datatype();

/// Type of converter function from native type to hdf5 datatype
using type_converter_fun_type = std::function<hid_t()>;

/**
 * Options which can be passed to dash::io::StoreHDF::write
 * to specify how existing structures are treated and what
 * metadata is stored.
 *
 * Collective operation.
 */
struct hdf5_options {
  /// Overwrite HDF5 file if already existing
  bool overwrite_file = true;
  /**
   * Modify an already existing HDF5 dataset.
   * If the dataset is not existing, throws a runtime error
   */
  bool modify_dataset = false;
  /// Store dash pattern characteristics as metadata in HDF5 file
  bool store_pattern = true;
  /// Restore pattern from metadata if HDF5 file contains any.
  bool restore_pattern = true;
  /// Metadata attribute key in HDF5 file.
  std::string pattern_metadata_key = "DASH_PATTERN";
};

/**
 * DASH wrapper to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 * All operations are collective.
 */
class StoreHDF {
  /**
   * test at compile time if pattern is compatible
   * \return true if pattern is compatible
   */
  template <class pattern_t>
  static constexpr bool _compatible_pattern() {
    return dash::pattern_partitioning_traits<pattern_t>::type::rectangular &&
           dash::pattern_layout_traits<pattern_t>::type::linear &&
           !dash::pattern_mapping_traits<pattern_t>::type::shifted &&
           !dash::pattern_mapping_traits<pattern_t>::type::diagonal;
    // TODO: check if mapping is regular by checking pattern property
  }

  template <class ViewType>
  static constexpr bool _is_origin_view() {
    return dash::view_traits<ViewType>::is_origin::value;
  }

  static std::vector<std::string> _split_string(const std::string& str,
                                                const char delim) {
    std::vector<std::string> elems;
    std::stringstream ss;
    ss.str(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
      if (item != "") {
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
  template <typename View_t>
  static void write(
      /// Import data in this Container
      View_t& array,
      /// Filename of HDF5 file including extension
      std::string filename,
      /// HDF5 Dataset in which the data is stored
      std::string datapath,
      /// options how to open and modify data
      hdf5_options foptions = hdf5_options(),
      /// \c std::function to convert native type into h5 type
      type_converter_fun_type to_h5_dt_converter = get_h5_datatype<
          typename dash::view_traits<View_t>::origin_type::value_type>) {
    using Container_t = typename dash::view_traits<View_t>::origin_type;
    using pattern_t = typename Container_t::pattern_type;
    using extent_t = typename pattern_t::size_type;
    using index_t = typename Container_t::index_type;
    using value_t = typename Container_t::value_type;

    constexpr auto ndim = pattern_t::ndim();

    // Check if container dims match pattern dims
    _verify_container_dims(array);
    static_assert(std::is_same<index_t, typename pattern_t::index_type>::value,
                  "Specified index_t differs from pattern_t::index_type");

    const dash::Team& team = array.team();

    // Map native types to HDF5 types
    auto h5datatype = to_h5_dt_converter();
    // for tracking opened groups
    std::list<hid_t> open_groups;
    // Split path in groups and dataset
    auto path_vec = _split_string(datapath, '/');
    auto dataset = path_vec.back();
    // remove dataset from path
    path_vec.pop_back();

    /* HDF5 definition */
    hid_t file_id;
    hid_t h5dset;
    hid_t internal_type;
    hid_t plist_id;  // property list identifier
    hid_t filespace;
    hid_t loc_id;

    // setup mpi access
    plist_id = H5Pcreate(H5P_FILE_ACCESS);
    DASH_ASSERT_RETURNS(dart__io__hdf5__prep_mpio(plist_id, team.dart_id()),
                        DART_OK);

    dash::Shared<int> f_exists;
    if (team.myid() == 0) {
      if (access(filename.c_str(), F_OK) != -1) {
        // check if file exists
        f_exists.set(static_cast<int>(H5Fis_hdf5(filename.c_str())));
      } else {
        f_exists.set(-1);
      }
    }
    team.barrier();

    if (foptions.overwrite_file || (f_exists.get() <= 0)) {
      // HD5 create file
      file_id =
          H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    } else {
      // Open file in RW mode
      file_id = H5Fopen(filename.c_str(), H5F_ACC_RDWR, plist_id);
    }

    // close property list
    H5Pclose(plist_id);

    // Traverse path
    loc_id = file_id;
    for (std::string elem : path_vec) {
      if (H5Lexists(loc_id, elem.c_str(), H5P_DEFAULT)) {
        // open group
        DASH_LOG_DEBUG("Open Group", elem);
        loc_id = H5Gopen2(loc_id, elem.c_str(), H5P_DEFAULT);
      } else {
        // create group
        DASH_LOG_DEBUG("Create Group", elem);
        loc_id = H5Gcreate2(loc_id, elem.c_str(), H5P_DEFAULT, H5P_DEFAULT,
                            H5P_DEFAULT);
      }
      if (loc_id != file_id) {
        open_groups.push_back(loc_id);
      }
    }

    // view extents are relevant (instead of pattern extents)
    auto filespace_extents = _get_container_extents(array);

    // Create dataspace
    filespace = H5Screate_simple(ndim, filespace_extents.extent, NULL);
    internal_type = H5Tcopy(h5datatype);

    if (foptions.modify_dataset) {
      // Open dataset in RW mode
      h5dset = H5Dopen(loc_id, dataset.c_str(), H5P_DEFAULT);
    } else {
      // Create dataset
      h5dset = H5Dcreate(loc_id, dataset.c_str(), internal_type, filespace,
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }

    // Close global dataspace
    H5Sclose(filespace);

    // ----------- prepare and write dataset --------------

    _write_dataset_impl(array, h5dset, internal_type);

    // ----------- end prepare and write dataset --------------

    // Add Attributes
    if (foptions.store_pattern && _is_origin_view<Container_t>()) {
      DASH_LOG_DEBUG("store pattern in hdf5 file");
      _store_pattern(array, h5dset, foptions);
    }

    // Close all
    H5Dclose(h5dset);
    H5Tclose(internal_type);

    std::for_each(open_groups.rbegin(), open_groups.rend(),
                  [](hid_t& group_id) { H5Gclose(group_id); });

    H5Fclose(file_id);

    team.barrier();
  }

  /**
   * Read an HDF5 dataset into a dash container using parallel IO
   * if the matrix is already allocated, the sizes have to match
   * the HDF5 dataset sizes and all data will be overwritten.
   * Otherwise the matrix will be allocated.
   *
   * Collective operation.
   */
  template <typename Container_t>
  typename std::enable_if<
      _compatible_pattern<typename Container_t::pattern_type>() &&
          _is_origin_view<Container_t>(),
      void>::
      type static read(
          /// Import data in this Container
          Container_t& matrix,
          /// Filename of HDF5 file including extension
          std::string filename,
          /// HDF5 Dataset in which the data is stored
          std::string datapath,
          /// options how to open and modify data
          hdf5_options foptions = hdf5_options(),
          /// \c std::function to convert native type into h5 type
          type_converter_fun_type to_h5_dt_converter =
              get_h5_datatype<typename Container_t::value_type>) {
    using pattern_t = typename Container_t::pattern_type;
    using extent_t = typename pattern_t::size_type;
    using index_t = typename Container_t::index_type;
    using value_t = typename Container_t::value_type;

    constexpr auto ndim = pattern_t::ndim();

    static_assert(std::is_same<index_t, typename pattern_t::index_type>::value,
                  "Specified index_t differs from pattern_t::index_type");

    // HDF5 definition
    hid_t file_id;
    hid_t h5dset;
    hid_t internal_type;
    hid_t plist_id;  // property list identifier
    hid_t filespace;
    // global data dims
    hsize_t data_dimsf[ndim];
    herr_t status;
    // Map native types to HDF5 types
    hid_t h5datatype;
    // rank of hdf5 dataset
    int rank;

    // Check if matrix is already allocated
    bool is_alloc = (matrix.size() != 0);

    // Setup MPI IO
    plist_id = H5Pcreate(H5P_FILE_ACCESS);
    if (is_alloc) {
      DASH_ASSERT_RETURNS(
          dart__io__hdf5__prep_mpio(plist_id, matrix.team().dart_id()),
          DART_OK);
    } else {
      DASH_ASSERT_RETURNS(
          dart__io__hdf5__prep_mpio(plist_id, dash::Team::All().dart_id()),
          DART_OK);
    }

    // HD5 create file
    file_id = H5Fopen(filename.c_str(), H5P_DEFAULT, plist_id);

    // close property list
    H5Pclose(plist_id);

    // Create dataset
    h5dset = H5Dopen(file_id, datapath.c_str(), H5P_DEFAULT);

    // Get dimensions of data
    filespace = H5Dget_space(h5dset);
    rank = H5Sget_simple_extent_ndims(filespace);

    DASH_ASSERT_EQ(rank, ndim,
                   "Data dimension of HDF5 dataset does not match matrix "
                   "dimension");

    status = H5Sget_simple_extent_dims(filespace, data_dimsf, NULL);

    std::array<extent_t, ndim> size_extents;

    // set matrix size according to hdf5 dataset dimensions
    for (int i = 0; i < ndim; ++i) {
      size_extents[i] = data_dimsf[i];
    }

    // Check if file contains DASH metadata and recreate the pattern
    auto pat_key = foptions.pattern_metadata_key.c_str();

    if (!is_alloc                       // not allocated
        && foptions.restore_pattern     // pattern should be restored
        && H5Aexists(h5dset, pat_key))  // hdf5 contains pattern
    {
      _restore_pattern(matrix, h5dset, foptions);
    } else if (is_alloc) {
      DASH_LOG_DEBUG("Matrix already allocated");
      // Check if matrix extents match data extents
      auto container_extents = _get_container_extents(matrix);
      for (int i = 0; i < ndim; ++i) {
        DASH_ASSERT_EQ(size_extents[i], container_extents.extent[i],
                       "Container extents do not match data extents");
      }
    } else {
      // Auto deduce pattern
      const pattern_t pattern(dash::SizeSpec<ndim, typename pattern_t::size_type>(size_extents),
                              dash::DistributionSpec<ndim>(),
                              dash::TeamSpec<ndim, typename pattern_t::index_type>(), dash::Team::All());

      matrix.allocate(pattern);
    }

    h5datatype = to_h5_dt_converter();
    internal_type = H5Tcopy(h5datatype);

    // ----------- prepare and read dataset ------------------

    _read_dataset_impl(matrix, h5dset, internal_type);

    // ----------- end prepare and read dataset --------------

    // Close all
    H5Dclose(h5dset);
    H5Tclose(internal_type);
    H5Fclose(file_id);

    matrix.team().barrier();
  }

  template <class Container_t>
  typename std::enable_if<
      !(_compatible_pattern<typename Container_t::pattern_type>() &&
        _is_origin_view<Container_t>()),
      void>::
      type static read(
          /// Import data in this Container
          Container_t& matrix,
          /// Filename of HDF5 file including extension
          std::string filename,
          /// HDF5 Dataset in which the data is stored
          std::string datapath,
          /// options how to open and modify data
          hdf5_options foptions = hdf5_options(),
          /// \c std::function to convert native type into h5 type
          type_converter_fun_type to_h5_dt_converter =
              get_h5_datatype<typename Container_t::value_type>) {}

 public:
  /**
   * hdf5 pattern specification for parallel IO
   */
  template <dim_t ndim>
  struct hdf5_pattern_spec {
    std::array<hsize_t, ndim> count{{0}};
    std::array<hsize_t, ndim> stride{{0}};
    std::array<hsize_t, ndim> offset{{0}};
    std::array<hsize_t, ndim> block{{0}};

    hdf5_pattern_spec() = default;
  };

  /**
   * hdf5 pattern specification for parallel IO
   */
  template <dim_t ndim>
  struct hdf5_filespace_spec {
    hsize_t extent[ndim] = {0};
  };

  template <dim_t ndim>
  struct hdf5_hyperslab_spec {
    hdf5_pattern_spec<ndim> memory;
    hdf5_pattern_spec<ndim> dataset;
    std::array<hsize_t, ndim> data_extf{{0}};
    std::array<hsize_t, ndim> data_extm{{0}};
    // approx. amount of data this unit contribs in this hyperslab
    size_t contrib_data = 0;
    bool contrib_blocks = false;

    // cannot be defaulted as otherwise const construction is not
    // possible in clang
    hdf5_hyperslab_spec() {};
  };

 private:
  enum class Mode : uint16_t {
    READ = 0x1,
    WRITE = 0x2
  };

  template <class BlockSpec_t, typename index_t>
  index_t static inline _blockspec_at(const BlockSpec_t& lblockspec,
                                      const std::array<index_t, 1>& coords) {
    return coords[0];
  }

  template <class BlockSpec_t, typename index_t, std::size_t ndim>
  index_t static inline _blockspec_at(const BlockSpec_t& lblockspec,
                                      const std::array<index_t, ndim>& coords) {
    return lblockspec.at(coords);
  }

  /**
   * Get a hdf5 slab representing a part of the dash pattern.
   * Which part is determined by the additional parameter dimensions.
   */
  template <class pattern_t>
  const static inline hdf5_hyperslab_spec<pattern_t::ndim()> _get_hdf_slab_body(
      /// The dash pattern to convert to hdf5 slabs
      const pattern_t& pattern,
      /// The dimensions in which the underilled blocks are considered
      const std::vector<dim_t> dimensions) {
    using index_t = typename pattern_t::index_type;
    constexpr auto ndim = pattern_t::ndim();
    hdf5_hyperslab_spec<ndim> hs;
    auto & ms = hs.memory;
    auto & ts = hs.dataset;

    const auto & lblockspec = pattern.local_blockspec();

    // get the index of the start block of the current slab
    // if local space is not empty
    if (lblockspec.size() == 0) {
      return hdf5_hyperslab_spec<ndim>();
    }
    std::array<index_t, ndim> coords{{0}};
    index_t lblckidx;
    for (auto d : dimensions) {
      coords[d] = lblockspec.extent(d) - 1;
    }
    lblckidx = _blockspec_at(lblockspec, coords);

    hs.contrib_blocks = true;
    // setup extends per dimension
    for (int i = 0; i < ndim; ++i) {
      auto tilesize = pattern.blocksize(i);
      auto num_tiles = pattern.local_extent(i) / tilesize;

      // if the current dimension is in dimensions look at the undefilled blocks
      if (std::find(dimensions.begin(), dimensions.end(), i) !=
          dimensions.end()) {
        // only look at underfilled blocks in this dimension
        ts.count[i] = 1;
        ts.block[i] = pattern.local_block(lblckidx).extent(i);
        if (pattern.local_extent(i) == num_tiles * tilesize) {
          // not underfilled on this unit
          return hdf5_hyperslab_spec<ndim>();
        }
      } else {
        // look at not undefilled blocks in this dimension
        if (num_tiles == 0) {
          // only underfilled blocks in this dimension
          return hdf5_hyperslab_spec<ndim>();
        }
        ts.count[i] = num_tiles;
        ts.block[i] = tilesize;
      }
      ts.offset[i] = pattern.local_block(lblckidx).offset(i);

      if (num_tiles > 1) {
        ts.stride[i] = pattern.teamspec().extent(i) * ts.block[i];
      } else {
        ts.stride[i] = 1;
      }

      ms.count[i] = ts.count[i];
      ms.block[i] = ts.block[i];
      ms.offset[i] = pattern.local_block_local(lblckidx).offset(i);
      ms.stride[i] = tilesize;

      hs.data_extm[i] = pattern.local_extent(i);
      hs.contrib_data *= ts.count[i] * ts.block[i];

      DASH_LOG_DEBUG("dimensions", dimensions);
      DASH_LOG_DEBUG("ts.count", i, ts.count[i]);
      DASH_LOG_DEBUG("ts.offset", i, ts.offset[i]);
      DASH_LOG_DEBUG("ts.block", i, ts.block[i]);
      DASH_LOG_DEBUG("ts.stride", i, ts.stride[i]);
      DASH_LOG_DEBUG("ms.count", i, ms.count[i]);
      DASH_LOG_DEBUG("ms.block", i, ms.block[i]);
      DASH_LOG_DEBUG("ms.offset", i, ms.offset[i]);
      DASH_LOG_DEBUG("ms.stride", i, ms.stride[i]);
    }
    hs.contrib_blocks = true;
    return hs;
  }

  /**
   * Convert a dash pattern into a hdf5 pattern in form of a list of hdf5 slabs.
   */
  template <dim_t ndim, MemArrange Arr, typename index_t>
  const static inline std::vector<hdf5_hyperslab_spec<ndim>>
      /// The dash pattern to convert to hdf5 slabs
      _get_hdf_slabs(const dash::Pattern<ndim, Arr, index_t>& pattern) {
    std::vector<hdf5_hyperslab_spec<ndim>> specs_hyperslab;
    std::vector<dim_t> dimensions;

    // iteration over powerset of {0, ..., ndim-1}
    specs_hyperslab.push_back(_get_hdf_slab_body(pattern, dimensions));
    // initialize dimensions so that it is equal to {0} in the first iteration
    dimensions.push_back(-1);
    while (dimensions.size() > 0) {
      ++dimensions.back();
      if (ndim == dimensions.back()) {
        dimensions.pop_back();
        continue;
      } else if (pattern.underfilled_blocksize(dimensions.back()) == 0) {
        continue;
      }  // else
      auto& current_hs = _get_hdf_slab_body(pattern, dimensions);
      if (current_hs.contrib_blocks) {
        specs_hyperslab.push_back(current_hs);
      }
      dimensions.push_back(dimensions.back());
    }

    return specs_hyperslab;
  }

  template <class pattern_t>
  const static inline std::vector<hdf5_hyperslab_spec<pattern_t::ndim()>>
  _get_hdf_slabs(const pattern_t& pattern) {
    return std::vector<hdf5_hyperslab_spec<pattern_t::ndim()>>(
        1, _get_hdf_slab_body(pattern, {}));
  }

  template <typename value_t>
  const static inline hdf5_filespace_spec<1> _get_container_extents(
      dash::Array<value_t>& array) {
    hdf5_filespace_spec<1> fs;
    fs.extent[0] = array.size();
    return fs;
  }

  template <class Container_t>
  const static inline hdf5_filespace_spec<Container_t::pattern_type::ndim()>
  _get_container_extents(Container_t& container) {
    constexpr auto ndim = Container_t::pattern_type::ndim();
    hdf5_filespace_spec<ndim> fs;
    for (int i = 0; i < ndim; ++i) {
      fs.extent[i] = container.extent(i);
    }
    return fs;
  }

#if 0
  // new implementation using view traits
  template < typename ViewType >
  const static inline hdf5_filespace_spec<dash::ndim(ViewType)>
  _get_container_extents(ViewType & container){
    constexpr auto ndim = dash::ndim(ViewType);
    hdf5_filespace_spec<ndim> fs;
    for(int i=0; i<ndim; ++i){
      fs.extent[i] = dash::extent<i>(container);
    }
    return fs;
  }
#endif

  template <dim_t ndim, typename value_t, typename index_t, typename pattern_t>
  static inline void _verify_container_dims(
      const Matrix<value_t, ndim, index_t, pattern_t>& container) {
    static_assert(ndim == pattern_t::ndim(),
                  "Pattern dimension has to match matrix dimension");
  }
  template <class Container_t>
  static inline void _verify_container_dims(const Container_t& container) {
    return;
  }

  template <typename Container_t>
  typename std::enable_if<
      _is_origin_view<Container_t>(),
      void>::type static _store_pattern(Container_t& container, hid_t h5dset,
                                        hdf5_options& foptions) {
    using pattern_t = typename Container_t::pattern_type;
    using extent_t = typename pattern_t::size_type;
    constexpr auto ndim = pattern_t::ndim();

    auto& pattern = container.pattern();

    auto pat_key = foptions.pattern_metadata_key.c_str();
    extent_t pattern_spec[ndim * 4];

    // Delete old attribute when overwriting dataset
    if (foptions.modify_dataset) {
      H5Adelete(h5dset, pat_key);
    }
    // Structure is
    // sizespec, teamspec, blockspec, blocksize
    for (int i = 0; i < ndim; ++i) {
      pattern_spec[i] = pattern.sizespec().extent(i);
      pattern_spec[i + ndim] = pattern.teamspec().extent(i);
      pattern_spec[i + (ndim * 2)] = pattern.blockspec().extent(i);
      pattern_spec[i + (ndim * 3)] = pattern.blocksize(i);
    }

    hsize_t attr_len[] = {static_cast<hsize_t>(ndim * 4)};
    hid_t attrspace = H5Screate_simple(1, attr_len, NULL);
    hid_t attribute_id = H5Acreate(h5dset, pat_key, H5T_NATIVE_LONG, attrspace,
                                   H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite(attribute_id, H5T_NATIVE_LONG, &pattern_spec);
    H5Aclose(attribute_id);
    H5Sclose(attrspace);
  }

  template <typename Container_t>
  typename std::enable_if<
      !_is_origin_view<Container_t>(),
      void>::type static _store_pattern(Container_t& container, hid_t h5dset,
                                        hdf5_options& foptions) {}

  template <typename Container_t>
  typename std::enable_if<
      _is_origin_view<Container_t>(),
      void>::type static _restore_pattern(Container_t& container, hid_t h5dset,
                                          hdf5_options& foptions) {
    using pattern_t = typename Container_t::pattern_type;
    using extent_t = typename pattern_t::size_type;
    constexpr auto ndim = pattern_t::ndim();

    std::array<extent_t, ndim> size_extents;
    std::array<extent_t, ndim> team_extents;
    std::array<dash::Distribution, ndim> dist_extents;
    extent_t hdf_dash_pattern[ndim * 4];

    hsize_t attr_len[] = {ndim * 4};
    hid_t attrspace = H5Screate_simple(1, attr_len, NULL);
    hid_t attribute_id =
        H5Aopen(h5dset, foptions.pattern_metadata_key.c_str(), H5P_DEFAULT);

    H5Aread(attribute_id, H5T_NATIVE_LONG, hdf_dash_pattern);
    H5Aclose(attribute_id);
    H5Sclose(attrspace);

    for (int i = 0; i < ndim; ++i) {
      size_extents[i] = static_cast<extent_t>(hdf_dash_pattern[i]);
      team_extents[i] = static_cast<extent_t>(hdf_dash_pattern[i + ndim]);
      dist_extents[i] = dash::TILE(hdf_dash_pattern[i + (ndim * 3)]);
    }
    DASH_LOG_DEBUG("Created pattern according to metadata");

    const pattern_t pattern(dash::SizeSpec<ndim, typename pattern_t::size_type>(size_extents),
                            dash::DistributionSpec<ndim>(dist_extents),
                            dash::TeamSpec<ndim, typename pattern_t::index_type>(team_extents),
                            dash::Team::All());

    // Allocate DASH Matrix
    container.allocate(pattern);
  }

  template <typename Container_t>
  typename std::enable_if<
      !_is_origin_view<Container_t>(),
      void>::type static _restore_pattern(Container_t& container, hid_t h5dset,
                                          hdf5_options& foptions) {}

  // -------------------------------------------------------------------------
  // ------------ write dataset implementation specialisations ---------------
  // -------------------------------------------------------------------------

  /**
   * Switches between different write implementations based on pattern
   * and container types.
   *
   * Specializes for cases which can be zero-copy implemented
   */
  template <class Container_t>
  typename std::enable_if<
      _is_origin_view<Container_t>() &&
          _compatible_pattern<typename Container_t::pattern_type>(),
      void>::type static _write_dataset_impl(Container_t& container,
                                             const hid_t& h5dset,
                                             const hid_t& internal_type) {
    _process_dataset_impl_zero_copy(StoreHDF::Mode::WRITE, container, h5dset,
                                    internal_type);
  }

  /**
  * Switches between different write implementations based on pattern
  * and container types.
  *
  * Specializes for cases which need buffering
  */
  template <class Container_t>
  typename std::enable_if<
      !(_is_origin_view<Container_t>() &&
        _compatible_pattern<typename Container_t::pattern_type>()),
      void>::type static _write_dataset_impl(Container_t& container,
                                             const hid_t& h5dset,
                                             const hid_t& internal_type) {
    _write_dataset_impl_buffered(container, h5dset, internal_type);
  }

  template <class Container_t>
  static void _process_dataset_impl_zero_copy(StoreHDF::Mode io_mode,
                                              Container_t& container,
                                              const hid_t& h5dset,
                                              const hid_t& internal_type);

  template <class Container_t>
  static void _write_dataset_impl_buffered(Container_t& container,
                                           const hid_t& h5dset,
                                           const hid_t& internal_type);

  template <
      typename ElementT,
      typename PatternT,
      dim_t NDim,
      dim_t NViewDim,
      typename LocalMemT>
  static void _write_dataset_impl_nd_block(
      dash::MatrixRef<ElementT, NDim, NViewDim, PatternT, LocalMemT>& container,
      const hid_t& h5dset,
      const hid_t& internal_type);

  // --------------------------------------------------------------------------
  // --------------------- READ specializations -------------------------------
  // --------------------------------------------------------------------------

  /**
   * Switches between different read implementations based on pattern
   * and container types.
   *
   * Specializes for cases which can be zero-copy implemented
   */
  template <class Container_t>
  typename std::enable_if<
      _compatible_pattern<typename Container_t::pattern_type>() &&
          _is_origin_view<Container_t>(),
      void>::type static inline _read_dataset_impl(Container_t& container,
                                                   const hid_t& h5dset,
                                                   const hid_t& internal_type) {
    _process_dataset_impl_zero_copy(StoreHDF::Mode::READ, container, h5dset,
                                    internal_type);
  }
};

}  // namespace hdf5
}  // namespace io
}  // namespace dash

#include <dash/io/hdf5/internal/DriverImplZeroCopy.h>
#include <dash/io/hdf5/internal/DriverImplBuffered.h>
#include <dash/io/hdf5/internal/DriverImplNdBlock.h>

#include <dash/io/hdf5/internal/StorageDriver-inl.h>

#endif  // DASH_ENABLE_HDF5

#endif  // DASH__IO__HDF5__STORAGEDRIVER_H__
