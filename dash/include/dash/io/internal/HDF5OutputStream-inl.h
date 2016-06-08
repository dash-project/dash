#ifndef DASH__IO__INTERNAL__HDF5_OUTPUT_STREAM_H__
#define DASH__IO__INTERNAL__HDF5_OUTPUT_STREAM_H__


#include <dash/io/HDF5OutputStream.h>
#include <dash/io/StoreHDF.h>

#include <dash/Matrix.h>



namespace dash {
namespace io {

// Array implementation
template <
    typename value_t,
    typename index_t,
    class    pattern_t >
inline HDF5OutputStream & operator<< (
    HDF5OutputStream & os,
    dash::Array< value_t,
    index_t,
    pattern_t > &array) {

    // make stream immudataset
    os._flushed = true;

    array.barrier();
    dash::io::StoreHDF::write(
        array,
        os._filename,
        os._dataset,
        os._foptions);
    return os;
}


template <
    typename value_t,
    typename index_t,
    class    pattern_t >
inline HDF5OutputStream & operator>> (
    HDF5OutputStream & os,
    dash::Array< value_t,
    index_t,
    pattern_t > &array) {

    // make stream immudataset
    os._flushed = true;

    array.barrier();
    dash::io::StoreHDF::read(
        array,
        os._filename,
        os._dataset,
        os._foptions);
    return os;
}


// Matrix implementation
template <
    typename value_t,
    dim_t    ndim,
    typename index_t,
    class    pattern_t >
inline HDF5OutputStream & operator<< (
    HDF5OutputStream & os,
    dash::Matrix < value_t,
    ndim,
    index_t,
    pattern_t > &matrix) {

    // make stream immudataset
    os._flushed = true;

    matrix.barrier();
    dash::io::StoreHDF::write(
        matrix,
        os._filename,
        os._dataset,
        os._foptions);
    return os;
}


template <
    typename value_t,
    dim_t    ndim,
    typename index_t,
    class    pattern_t >
inline HDF5OutputStream & operator>> (
    HDF5OutputStream & os,
    dash::Matrix < value_t,
    ndim,
    index_t,
    pattern_t > &matrix) {

    // make stream immudataset
    os._flushed = true;

    matrix.barrier();
    dash::io::StoreHDF::read(
        matrix,
        os._filename,
        os._dataset,
        os._foptions);
    return os;
}

} // namespace io
} // namespace dash

#endif // DASH__IO__INTERNAL__HDF5_OUTPUT_STREAM_H__
