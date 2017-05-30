#ifndef DASH__IO__HDF5__INPUT_STREAM_H__
#define DASH__IO__HDF5__INPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>
#include <future>

#include <dash/Matrix.h>
#include <dash/Array.h>

namespace dash {
namespace io {
namespace hdf5 {

/**
 * DASH stream API to load an dash container or view
 * from an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */

using IOStreamMode = dash::io::IOStreamMode<dash::io::IOSBaseMode>;

class InputStream : public ::dash::io::IOSBase<IOStreamMode> {
 private:

  using self_t = InputStream;

  std::string _filename;
  std::string _dataset;
  type_converter _converter;
  hdf5_options _foptions = hdf5_options();
  bool _use_cust_conv = false;
  dash::launch _launch_policy;

  std::vector<std::shared_future<void> > _async_ops;

 public:
  /**
   * Creates an HDF5 input stream using a launch policy
   *
   * Support of \ref dash::launch::async is still highly experimental and requires
   * thread support in MPI. If multi-threaded access is not supported,
   * blocking I/O is used as fallback. To wait for outstanding IO operations
   * use \c flush(). Until the stream is not flushed, no write accesses to the
   * container, as well as no barriers are allowed.
   * Otherwise the behavior is undefined.
   */
  InputStream(dash::launch lpolicy, std::string filename)
      : _filename(filename), _dataset("data"), _launch_policy(lpolicy) {
    if (_launch_policy == dash::launch::async && !dash::is_multithreaded()) {
      _launch_policy = dash::launch::sync;
      DASH_LOG_WARN(
          "Requested ASIO but DART does not support "
          "multi-threaded access. Blocking IO is used"
          "as fallback");
    }
  }

  /**
   * Creates an HDF5 input stream using blocking IO.
   *
   * The stream takes an arbitrary number of modifiers and objects,
   * where the objects are stored in the order of passing it to the stream.
   *
   * The interface follows roughly the STL stream concept.
   *
   * Example:
   * \code
   *  dash::Array<double> array_a;
   *  dash::Array<double> array_b;
   *
   *  InputStream is("file.hdf5");
   *  is >> dataset("dataset") >> array_a
   *     >> dataset("seconddata") >> array_b;
   * \endcode
   */
  explicit InputStream(std::string filename)
      : InputStream(dash::launch::sync, filename) {}

  ~InputStream() {
    if (!_async_ops.empty()) {
      _async_ops.back().wait();
    }
  }

  InputStream()                     = delete;
  InputStream(const self_t & other) = delete;
  InputStream(self_t && other)      = default;

  self_t & operator= (const self_t & other) = delete;
  self_t & operator= (self_t && other)      = default;

  /**
   * Synchronizes with the data source.
   * If \ref dash::launch::async is used, waits until all data is read
   */
  InputStream & flush() {
    DASH_LOG_DEBUG("flush input stream", _async_ops.size());
    if (!_async_ops.empty()) {
      _async_ops.back().wait();
    }
    DASH_LOG_DEBUG("input stream flushed");
    return *this;
  }

  // IO Manipulators

  /// set name of dataset
  friend InputStream& operator>>(InputStream& is, const dataset tbl) {
    is._dataset = tbl._dataset;
    return is;
  }

  /// set metadata key at which the pattern will be stored
  friend InputStream& operator>>(InputStream& is, setpattern_key pk) {
    is._foptions.pattern_metadata_key = pk._key;
    return is;
  }

  /// set whether pattern layout should be restored from metadata
  friend InputStream& operator>>(InputStream& is, restore_pattern rs) {
    is._foptions.restore_pattern = rs._restore;
    return is;
  }

  /// custom type converter function to convert native type to HDF5 type
  friend InputStream& operator>>(InputStream& is, const type_converter conv) {
    is._converter = conv;
    is._use_cust_conv = true;
    return is;
  }

  /// kicker which loads an container using the specified stream properties.
  template <typename Container_t>
  friend InputStream& operator>>(InputStream& is, Container_t& matrix);

 private:
  template <typename Container_t>
  void _load_object_impl(Container_t& container) {
    if (_use_cust_conv) {
      StoreHDF::read(container, _filename, _dataset, _foptions, _converter);
    } else {
      StoreHDF::read(container, _filename, _dataset, _foptions);
    }
  }

  template <typename Container_t>
  void _load_object_impl_async(Container_t& container) {
    auto pos = _async_ops.size();

    // copy state of stream
    auto s_filename = _filename;
    auto s_dataset = _dataset;
    auto s_foptions = _foptions;
    auto s_use_cust_conv = _use_cust_conv;
    type_converter_fun_type s_converter = _converter;

    // pass pos by value as it might be out of scope when function is called
    std::shared_future<void> fut = std::async(
        std::launch::async, [&, pos, s_filename, s_dataset, s_foptions,
                             s_converter, s_use_cust_conv]() {
          if (pos != 0) {
            // wait for previous tasks
            auto last_task = _async_ops[pos - 1];
            DASH_LOG_DEBUG("waiting for future", pos);
            last_task.wait();
          }
          DASH_LOG_DEBUG("execute async io task");

          if (s_use_cust_conv) {
            StoreHDF::read(container, s_filename, s_dataset, s_foptions,
                           s_converter);
          } else {
            StoreHDF::read(container, s_filename, s_dataset, s_foptions);
          }
          DASH_LOG_DEBUG("execute async io task done");
        });
    _async_ops.push_back(fut);
  }
};

}  // namespace hfd5
}  // namespace io
}  // namespace dash

#include <dash/io/hdf5/internal/InputStream-inl.h>

#endif  // DASH_ENABLE_HDF5

#endif  // DASH__IO__HDF5__INPUT_STREAM_H__
