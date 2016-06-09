#ifndef DASH__IO__HDF5_STREAM_H__
#define DASH__IO__HDF5_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>

namespace dash {
namespace io {

typedef dash::io::StoreHDF::hdf5_file_options hdf5_file_options;

class HDF5Dataset {
  public:
    std::string _dataset;
  public:
    HDF5Dataset(std::string dataset) {
        _dataset = dataset;
    }
};

class HDF5Options {
  public:
    hdf5_file_options _foptions;
    std::string  _test;

  public:
    HDF5Options() {
    }

    HDF5Options(hdf5_file_options opts) {
        _foptions = opts;
    }
};

}
}

#include <dash/io/HDF5OutputStream.h>
#include <dash/io/HDF5InputStream.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5_STREAM_H__
