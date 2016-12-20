#ifndef DASH__IO__HDF5__INPUT_STREAM_H__
#define DASH__IO__HDF5__INPUT_STREAM_H__

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

using IOStreamMode = dash::io::IOStreamMode<dash::io::IOSBaseMode>;

class InputStream
: public ::dash::io::IOSBase<IOStreamMode> {
  private:
    std::string                _filename;
    std::string                _dataset;
    type_converter             _converter;
    hdf5_options               _foptions = StoreHDF::get_default_options();
    bool                       _use_cust_conv = false;
    dash::launch               _launch_policy;
    
    std::vector<std::shared_future<void> > _async_ops;

  public:
    InputStream(
        dash::launch lpolicy,
        std::string filename)
        : _filename(filename),
          _dataset("data"),
          _launch_policy(lpolicy)
    { }

    InputStream(std::string  filename)
        : InputStream(dash::launch::sync, filename)
    { }
    
    /**
     * Synchronizes with the data source.
     * If async IO is used, waits until all data is read
     */
    InputStream flush(){
        for(auto & fut : _async_ops){
            fut.wait();
        }
        return *this;
    }

    // IO Manipulators

    friend InputStream & operator>> (
        InputStream   & is,
        const dataset   tbl) {
        is._dataset = tbl._dataset;
        return is;
    }

    friend InputStream & operator>> (
      InputStream    & is,
      setpattern_key   pk) {
      is._foptions.pattern_metadata_key = pk._key;
      return is;
    }

    friend InputStream & operator>> (
      InputStream     & is,
      restore_pattern   rs) {
      is._foptions.restore_pattern = rs._restore;
      return is;
    }

    friend InputStream & operator>> (
      InputStream          & is,
      const type_converter   conv) {
      is._converter = conv;
      is._use_cust_conv = true;
      return is;
    }

    template < typename Container_t >
    friend InputStream & operator>> (
        InputStream & is,
        Container_t & matrix);
    
private:
    template< typename Container_t>
    void _load_object_impl(Container_t & container){
      if(_use_cust_conv){
        StoreHDF::read(
          container,
          _filename,
          _dataset,
          _foptions,
          _converter);
      } else {
        StoreHDF::read(
          container,
          _filename,
          _dataset,
          _foptions);
      }
    }
};

} // namespace hfd5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/InputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__INPUT_STREAM_H__
