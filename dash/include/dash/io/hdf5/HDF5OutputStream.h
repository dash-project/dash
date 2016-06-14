#ifndef DASH__IO__HDF5__HDF5_OUTPUT_STREAM_H__
#define DASH__IO__HDF5__HDF5_OUTPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>

#include <dash/Matrix.h>
#include <dash/Array.h>


namespace dash {
namespace io {
namespace hdf5 {

/**
 * DASH stream API to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */
class HDF5OutputStream {

public:
  /**
   * Options which can be passed to dash::io::HDF5OutputStream::write
   * to specify how existing structures are treated and what
   * metadata is stored.
   */
  typedef struct hdf5_writemode_t
  {
    /// Overwrite HDF5 file if already existing
    bool          overwrite_file;

    /// Modify an already existing HDF5 dataset.
    /// If the dataset is not existing, throws a runtime error
    bool          modify_dataset;

    /// Store dash pattern characteristics as metadata in HDF5 file
    bool          store_pattern;

    /// Restore pattern from metadata if HDF5 file contains any.
    bool          restore_pattern;

    /// Metadata attribute key in HDF5 file.
    std::string   pattern_metadata_key;

  } hdf5_writemode;

private:

  std::string                _filename;
  std::string                _dataset;
  hdf5_writemode             _foptions;

  public:
    HDF5OutputStream(
        std::string filename,
        hdf5_file_options fcopts = 0)
        : _filename(filename),
          _dataset("data"),
          _foptions(StoreHDF::get_default_options())
    {
      if((fcopts & HDF5FileOptions::Append) != 0){
          _foptions.overwrite_file = false;
        }
    }

    // IO Manipulators

    friend HDF5OutputStream & operator<<(
        HDF5OutputStream & os,
        const dataset    & tbl)
    {
        os._dataset = tbl._dataset;
        return os;
    }

    friend HDF5OutputStream & operator<<(
        HDF5OutputStream & os,
        setpattern_key     pk)
    {
        os._foptions.pattern_metadata_key = pk._key;
        return os;
    }

    friend HDF5OutputStream & operator<<(
        HDF5OutputStream & os,
        store_pattern      sp) {
        os._foptions.store_pattern = sp._store;
        return os;
    }

    friend HDF5OutputStream & operator<<(
        HDF5OutputStream & os,
        modify_dataset     md)
    {
        os._foptions.modify_dataset = md._modify;
        return os;
    }


    // Array Implementation
    template <
        typename value_t,
        typename index_t,
        class    pattern_t >
    friend HDF5OutputStream & operator<<(
        HDF5OutputStream         & os,
        dash::Array< value_t,
                     index_t,
                     pattern_t > & array);

    // Matrix Implementation
    template <
        typename value_t,
        dim_t    ndim,
        typename index_t,
        class    pattern_t >
    friend HDF5OutputStream & operator<<(
        HDF5OutputStream          & os,
        dash::Matrix< value_t,
                      ndim,
                      index_t,
                      pattern_t > & matrix);

};

} // namespace hdf5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/HDF5OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__HDF5_OUTPUT_STREAM_H__
