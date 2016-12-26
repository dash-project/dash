#ifndef DASH__IO__HDF5__IO_MANIP_H__
#define DASH__IO__HDF5__IO_MANIP_H__

#ifdef DASH_ENABLE_HDF5

#include <string>

namespace dash {
namespace io {
namespace hdf5 {

typedef StoreHDF::hdf5_options hdf5_options;

/**
 * Stream manipulator class to specify
 * the hdf5 dataset
 */
class dataset {
public:
  std::string _dataset;
public:
  dataset(std::string dataset) {
    _dataset = dataset;
  }
};

/**
 * Stream manipulator class to set the dash
 * pattern key of the dataset.
 */
class setpattern_key {
public:
  std::string _key;
public:
  setpattern_key(std::string name)
  : _key(name)
  { }
};

/**
 * Stream manipulator class to set whether
 * the pattern should be restored from the
 * hdf5 dataset metadata or not.
 */
class restore_pattern {
public:
  bool _restore;
public:
  restore_pattern(bool restore = true)
  : _restore(restore)
  { }
};

/**
 * Stream manipulator class to set whether
 * the pattern should be stored as metadata
 * of the hdf5 dataset or not.
 */
class store_pattern {
public:
  bool _store;
public:
  store_pattern(bool store = true)
  : _store(store)
  { }
};

/**
 * Stream manipulator class to set whether
 * the selected dataset should be overwritten.
 * The element type and the extents in each dimension
 * have to match the extends of the dataset
 */
class modify_dataset {
public:
  bool _modify;
public:
  modify_dataset(bool modify = true)
  : _modify(modify)
  { }
};

class type_converter {
private:
    type_converter_fun_type _converter;
public:
    type_converter(type_converter_fun_type fun = get_h5_datatype<UNSPECIFIED_TYPE>)
    : _converter(fun) { }

    operator type_converter_fun_type(){
        return _converter;
    }
};

} // namespace hdf5
} // namespace io
} // namespace dash

#include <dash/io/hdf5/IOStream.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5__IO_MANIP_H__
