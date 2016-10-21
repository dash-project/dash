#ifndef DASH__IO__HDF5__INTERNAL__INPUT_STREAM_INL_H__INCLUDED
#define DASH__IO__HDF5__INTERNAL__INPUT_STREAM_INL_H__INCLUDED


#include <dash/io/hdf5/InputStream.h>
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
inline HDF5InputStream & operator>> (
    HDF5InputStream          & is,
    dash::Array< value_t,
                 index_t,
                 pattern_t > & array) {

    dash::io::hdf5::StoreHDF::read(
        array,
        is._filename,
        is._dataset,
        is._foptions);
    return is;
}

// Matrix implementation
template <
    typename value_t,
    dim_t    ndim,
    typename index_t,
    class    pattern_t >
inline HDF5InputStream & operator>> (
    HDF5InputStream           & is,
    dash::Matrix< value_t,
                  ndim,
                  index_t,
                  pattern_t > & matrix) {

    dash::io::hdf5::StoreHDF::read(
        matrix,
        is._filename,
        is._dataset,
        is._foptions);
    return is;
}

} // namespace hdf5
} // namespace io
} // namespace dash

#endif // DASH__IO__HDF5__INTERNAL__INPUT_STREAM_INL_H__INCLUDED
