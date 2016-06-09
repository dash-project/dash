#ifndef DASH__IO__INTERNAL__HDF5_INPUT_STREAM_H__
#define DASH__IO__INTERNAL__HDF5_INPUT_STREAM_H__


#include <dash/io/HDF5InputStream.h>
#include <dash/io/StoreHDF.h>

#include <dash/Matrix.h>



namespace dash {
namespace io {

// Array implementation
template <
    typename value_t,
    typename index_t,
    class    pattern_t >
inline HDF5InputStream & operator>> (
    HDF5InputStream & is,
    dash::Array< value_t,
    index_t,
    pattern_t > &array) {

    array.barrier();
    dash::io::StoreHDF::read(
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
    HDF5InputStream & is,
    dash::Matrix < value_t,
    ndim,
    index_t,
    pattern_t > &matrix) {

    matrix.barrier();
    dash::io::StoreHDF::read(
        matrix,
        is._filename,
        is._dataset,
        is._foptions);
    return is;
}

} // namespace io
} // namespace dash

#endif // DASH__IO__INTERNAL__HDF5_OUTPUT_STREAM_H__
