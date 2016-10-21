#ifndef DASH__IO__INTERNAL__HDF5__OUTPUT_STREAM_INL_H__INCLUDED
#define DASH__IO__INTERNAL__HDF5__OUTPUT_STREAM_INL_H__INCLUDED


#include <dash/io/hdf5/OutputStream.h>
#include <dash/io/hdf5/StorageDriver.h>

#include <dash/Matrix.h>
#include <dash/Array.h>


namespace dash {
namespace io {
namespace hdf5 {

// Array implementation
template <
    typename value_t,
    typename index_t,
    class    pattern_t >
inline OutputStream & operator<< (
    OutputStream & os,
    dash::Array< value_t,
    index_t,
    pattern_t > &array) {

    array.barrier();
    dash::io::hdf5::StoreHDF::write(
        array,
        os._filename,
        os._dataset,
        os._foptions);

		// Append future data in this stream
		os._foptions.overwrite_file = false;
    return os;
}

// Matrix implementation
template <
    typename value_t,
    dim_t    ndim,
    typename index_t,
    class    pattern_t >
inline OutputStream & operator<< (
    OutputStream & os,
    dash::Matrix < value_t,
    ndim,
    index_t,
    pattern_t > &matrix) {

    matrix.barrier();
    dash::io::hdf5::StoreHDF::write(
        matrix,
        os._filename,
        os._dataset,
        os._foptions);

   // Append future data in this stream
	 os._foptions.overwrite_file = false;
   return os;
}

} // namespace hdf5
} // namespace io
} // namespace dash

#endif // DASH__IO__INTERNAL__HDF5__OUTPUT_STREAM_INL_H__INCLUDED
