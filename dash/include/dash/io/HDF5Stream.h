#ifndef DASH__IO__HDF5_STREAM_H__
#define DASH__IO__HDF5_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>

namespace dash {
namespace io {

typedef StoreHDF::hdf5_file_options hdf5_file_options;
typedef uint32_t hdf5_file_creation_options;

enum HDF5FileOptions : hdf5_file_creation_options { 
	Append = 1 << 0
};

class HDF5Dataset {
  public:
    std::string _dataset;
  public:
    HDF5Dataset(std::string dataset) {
        _dataset = dataset;
    }
};

class HDF5setpattern_key {
	public:
		std::string _key;
	public:
		HDF5setpattern_key(std::string name):
			_key(name){ }
};

class HDF5restore_pattern {
	public:
		bool _restore;
	public:
		HDF5restore_pattern(bool restore = true):
			_restore(restore){ }
	};

class HDF5store_pattern {
	public:
		bool _store;
	public:
		HDF5store_pattern(bool store = true):
			_store(store){ }
	};

class HDF5modify_dataset {
	public:
		bool _modify;
	public:
		HDF5modify_dataset(bool modify = true):
			_modify(modify){ }
	};

}
}

#include <dash/io/HDF5OutputStream.h>
#include <dash/io/HDF5InputStream.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5_STREAM_H__
