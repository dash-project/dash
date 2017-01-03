#ifndef DASH__IO__HDF5__OUTPUT_STREAM_H__
#define DASH__IO__HDF5__OUTPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>
#include <future>

#include <dash/Matrix.h>
#include <dash/Array.h>

#include <dash/LaunchPolicy.h>

#include <chrono>
#include <thread>

namespace dash {
namespace io {
namespace hdf5 {

/**
 * DASH stream API to store a dash container or view
 * in an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */

class OutputStream
: public ::dash::io::IOSBase<StreamMode> {

  typedef OutputStream                   self_t;
  typedef dash::io::IOSBase<StreamMode>  base_t;
  typedef StreamMode                     mode_t;

private:
  std::string                _filename;
  std::string                _dataset;
  type_converter             _converter;
  hdf5_options               _foptions = StoreHDF::hdf5_options();
  bool                       _use_cust_conv = false;
  dash::launch               _launch_policy;
  
  std::vector<std::shared_future<void> > _async_ops;

public:
  /**
   * Creates an HDF5 output stream using a launch policy
   * 
   * Support of \cdash::launch::async is still highly experimental and requires
   * thread support in MPI. To wait for outstanding IO operations use
   * \cflush(). Until the stream is not flushed, no write accesses to the
   * container, as well as no barriers are allowed.
   * Otherwise the behavior is undefined.
   */
    OutputStream(
        /// 
        dash::launch lpolicy,
        std::string filename,
        /// device opening flags: \see dash::io::IOSBaseMode
        mode_t open_mode = DeviceMode::no_flags)
        : _filename(filename),
          _dataset("data"),
          _launch_policy(lpolicy)
    {
        if((open_mode & DeviceMode::app)){
          _foptions.overwrite_file = false;
        }    
    }
    
    /**
     * Creates an HDF5 output stream using blocking IO.
     * 
     * The stream takes an arbitrary number of modifiers and objects,
     * where the objects are stored in the order of passing it to the stream.
     * 
     * The interface follows roughly the STL stream concept.
     * 
     * Example:
     * \code
     *  dash::Array<double> array_a(1000);
     *  dash::Array<double> array_b(500);
     * 
     *  OutputStream os("file.hdf5");
     *  os << dataset("dataset") << array_a
     *     << dataset("seconddata") << array_b;
     * \endcode
     */ 
    OutputStream(
        std::string filename,
        mode_t open_mode = DeviceMode::no_flags)
        : OutputStream(dash::launch::sync, filename, open_mode)
    { }
    
    ~OutputStream(){
      if(!_async_ops.empty()){
        _async_ops.back().wait();
      }
    }
    
    /**
     * Synchronizes with the data sink.
     * If \cdash::launch::async is used, waits until all data is written
     */
    OutputStream flush(){
      DASH_LOG_DEBUG("flush output stream", _async_ops.size());
      if(!_async_ops.empty()){
        _async_ops.back().wait();
      }
      DASH_LOG_DEBUG("output stream flushed");
      return *this;
    }

    // IO Manipulators

    /// set name of dataset
    friend OutputStream & operator<<(
        OutputStream  & os,
        const dataset   tbl)
    {
        os._dataset = tbl._dataset;
        return os;
    }

    /// set metadata key at which the pattern will be stored
    friend OutputStream & operator<<(
        OutputStream         & os,
        const setpattern_key   pk)
    {
        os._foptions.pattern_metadata_key = pk._key;
        return os;
    }

    /// set whether pattern layout should be stored as metadata
    friend OutputStream & operator<<(
        OutputStream         & os,
        const store_pattern    sp) {
        os._foptions.store_pattern = sp._store;
        return os;
    }

    /// modify an already existing dataset instead of overwriting it
    friend OutputStream & operator<<(
        OutputStream         & os,
        const modify_dataset   md)
    {
        os._foptions.modify_dataset = md._modify;
        return os;
    }

    /// custom type converter function to convert native type to HDF5 type
    friend OutputStream & operator<< (
      OutputStream         & os,
      const type_converter   conv) {
      os._converter = conv;
      os._use_cust_conv = true;
      return os;
    }

    /// kicker which stores an container using the specified stream properties.
    template < typename Container_t >
    friend OutputStream & operator<<(
        OutputStream & os,
        Container_t  & container);

private:
    template< typename Container_t >
    void _store_object_impl(Container_t & container){
      if(_use_cust_conv){
        StoreHDF::write(
          container,
          _filename,
          _dataset,
          _foptions,
          _converter);
      } else {
        StoreHDF::write(
          container,
          _filename,
          _dataset,
          _foptions);
      }
    }
    
    template< typename Container_t >
    void _store_object_impl_async(Container_t & container){
      auto pos = _async_ops.size();
      
      // copy state of stream
      auto s_filename      = _filename;
      auto s_dataset       = _dataset;
      auto s_foptions      = _foptions;
      auto s_use_cust_conv = _use_cust_conv; 
      type_converter_fun_type s_converter = _converter;
      
      // pass pos by value as it might be out of scope when function is called
      std::shared_future<void> fut = 
              std::async(std::launch::async,
                        [&,pos, s_filename, s_dataset, s_foptions, s_converter, s_use_cust_conv](){
        if(pos != 0){
          // wait for previous tasks
          auto last_task = _async_ops[pos-1];
          DASH_LOG_DEBUG("waiting for future", pos);
          last_task.wait();
        }
        DASH_LOG_DEBUG("execute async io task");
        
        if(s_use_cust_conv){
          StoreHDF::write(
            container,
            s_filename,
            s_dataset,
            s_foptions,
            s_converter);
        } else {
          StoreHDF::write(
            container,
            s_filename,
            s_dataset,
            s_foptions);
        }
        DASH_LOG_DEBUG("execute async io task done");
        });
      _async_ops.push_back(fut);
    }
};

} // namespace hdf5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/internal/OutputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__OUTPUT_STREAM_H__
