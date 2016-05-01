
#include <dash/io/StoreHDF.h>

#ifdef DASH_ENABLE_HDF5

#include <dash/Array.h>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <string>
#include <iostream>


namespace dash {
namespace io {

/*
 * \see https://www.hdfgroup.org/HDF5/doc/H5.user/Datatypes.html
 */
hid_t _convertType(int t) {
  return H5T_NATIVE_INT;
}
hid_t _convertType(double t) {
  return H5T_NATIVE_DOUBLE;
}

#if __OLD__

/**
 * write timing data to hdf5 file using parallel IO.
 */
template<
  typename value_t,
  typename index_t,
  class pattern_t >
void StoreHDF::write(
  dash::Array<value_t, index_t, pattern_t> & array,
  std::string filename,
  std::string table)
{

  auto    globalsize = array.size();
  auto    localsize   = array.pattern().local_size();
  auto    tilesize   = array.pattern().blocksize(0);
  auto    numunits   = array.pattern().num_units();
  auto    lbegindex   = array.pattern().lbegin();
  // global distance between two local tiles
  auto    tiledist   = numunits * tilesize;
  // Map native types to HDF5 types
  auto h5datatype = _convertType(array[0]);
  /*
  if(dash::myid() == 0){
    std::cout << "Array Config:" << std::endl
              << "Globalsize: " << globalsize << std::endl
              << "Localsize: " << localsize << std::endl
              << "Tilesize: " << tilesize << std::endl
              << "tiledist: " << tiledist << std::endl;
  }
  dash::barrier();
  std::cout << "UNIT: " << dash::myid()
            << ", lbegindex: " << lbegindex << std::endl;
  dash::barrier();
  */

  /* HDF5 definition */
  hid_t    file_id;
  hid_t    dataset;
  hid_t   timings_type;
  hid_t    plist_id; // property list identifier
  hid_t   filespace;
  hid_t    memspace;
  /* global or file data dims */
  hsize_t  data_dimsf[] = {(hsize_t) globalsize};
  /* local data dims */
  hsize_t data_dimsm[] = {(hsize_t) localsize};

  hid_t    attr_id;
  herr_t  status;

  // Hyperslab definition
  hsize_t  count[1];
  hsize_t stride[1];
  hsize_t  offset[1];
  hsize_t block[1];

  // setup mpi access
  plist_id = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);

  // HD5 create file
  file_id = H5Fcreate( filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, plist_id );

  // close property list
  H5Pclose(plist_id);

  // Create dataspace
  filespace      = H5Screate_simple(1, data_dimsf, NULL);
  memspace      = H5Screate_simple(1, data_dimsm, NULL);
  timings_type  = H5Tcopy(h5datatype);

  // Create dataset
  dataset = H5Dcreate(file_id, table.c_str(), timings_type, filespace,
      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  // Close global dataspace
  H5Sclose(filespace);

  // Select Hyperslabs in file
  count[0]  = (hsize_t) (localsize/tilesize); // tiles per node
  stride[0]  = (hsize_t) tiledist;   // space between two tiles of same node
  offset[0] = (hsize_t) lbegindex; // offset in positions
  block[0]  = (hsize_t) tilesize;   // size of a tile

  filespace = H5Dget_space(dataset);
  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

  // Create property list for collective writes
  plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  // Write data
  H5Dwrite(dataset, timings_type, memspace, filespace,
    plist_id, array.lbegin());

  // Add Attributes
  hid_t attrspace = H5Screate(H5S_SCALAR);
  long attr = (long) tilesize;
  hid_t attribute_id = H5Acreate(
    dataset, "DASH_TILESIZE", H5T_NATIVE_LONG,
    attrspace, H5P_DEFAULT, H5P_DEFAULT);
  H5Awrite(attribute_id, H5T_NATIVE_LONG, &attr);
  H5Aclose(attribute_id);
  H5Sclose(attrspace);

  // Close all
  H5Dclose(dataset);
  H5Sclose(filespace);
  H5Sclose(memspace);
  H5Tclose(timings_type);
  H5Fclose(file_id);
}

template<typename value_t>
void StoreHDF::read(
  dash::Array<value_t> &array,
  string filename,
  string table)
{

  long    globalsize;
  long     localsize;
  long    tilesize;
  int      numunits;
  long    lbegindex;
  long    tiledist;// global distance between two local tiles

  /* HDF5 definition */
  hid_t    file_id;
  hid_t    dataset;
  hid_t   timings_type;
  hid_t    plist_id; // property list identifier
  hid_t   filespace;
  hid_t    memspace;
  /* global data dims */
  hsize_t  data_dimsf[1];
  /* local data dims */
  hsize_t data_dimsm[1];

  herr_t  status;
  int      rank;
  // Map native types to HDF5 types
  hid_t h5datatype;

  // Hyperslab definition
  hsize_t  count[1];
  hsize_t stride[1];
  hsize_t  offset[1];
  hsize_t block[1];

  // setup mpi access
  plist_id = H5Pcreate(H5P_FILE_ACCESS);
  H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, MPI_INFO_NULL);

  // HD5 create file
  file_id = H5Fopen(filename.c_str(), H5P_DEFAULT, plist_id );

  // close property list
  H5Pclose(plist_id);

  // Create dataset
  dataset = H5Dopen(file_id, table.c_str(), H5P_DEFAULT);

  // Get dimensions of data
  filespace     = H5Dget_space(dataset);
  rank          = H5Sget_simple_extent_ndims(filespace);
  if(rank != 1){
    std::cerr << "Data dimension is not 1" << std::endl;
    return NULL; // QUIT
  }
  status        = H5Sget_simple_extent_dims(filespace, data_dimsf, NULL);

  if(dash::myid() == 0){
    std::cout << "Dataset dimension: " << data_dimsf[0] << std::endl;
  }

  // Initialize DASH Array
  // no explicit pattern specified / try to load pattern from hdf5 file
  if(H5Aexists(dataset, "DASH_TILESIZE")){
    hid_t attrspace      = H5Screate(H5S_SCALAR);
    hid_t attribute_id  = H5Aopen(dataset, "DASH_TILESIZE", H5P_DEFAULT);
    H5Aread(attribute_id, H5T_NATIVE_LONG, &tilesize);
    H5Aclose(attribute_id);
    H5Sclose(attrspace);
  } else {
    tilesize = 1; // dash::CYCLIC
  }

  // Allocate DASH Array
  array.allocate(data_dimsf[0], dash::TILE(tilesize));
  h5datatype     = _convertType(array[0]); // hack

  // Calculate chunks
  globalsize    = array.size();
  localsize      = array.pattern().local_size();
  tilesize      = array.pattern().blocksize(0);
  numunits      =  array.pattern().num_units();
  lbegindex      = array.pattern().lbegin();
  tiledist      = numunits * tilesize;
  data_dimsm[0]  = localsize;

  // Create HDF5 memspace
  memspace      = H5Screate_simple(1,data_dimsm,NULL);
  timings_type  = H5Tcopy(h5datatype);

  // Select Hyperslabs in file
  count[0]  = (hsize_t) (localsize/tilesize); // tiles per node
  stride[0]  = (hsize_t) tiledist;   // space between two tiles of same node
  offset[0] = (hsize_t) lbegindex; // offset in positions
  block[0]  = (hsize_t) tilesize;   // size of a tile

  H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, stride, count, block);

  // Create property list for collective reads
  plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

  // read data
  H5Dread(dataset, timings_type, memspace, filespace,
    plist_id, array.lbegin());

  // Close all
  H5Dclose(dataset);
  H5Sclose(filespace);
  H5Sclose(memspace);
  H5Tclose(timings_type);
  H5Fclose(file_id);
};

#endif // __OLD__

} // namespace io
} // namespace dash

#endif // DASH_ENABLE_HDF5

