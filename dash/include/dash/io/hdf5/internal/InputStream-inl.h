#ifndef DASH__IO__HDF5__INTERNAL__INPUT_STREAM_INL_H__INCLUDED
#define DASH__IO__HDF5__INTERNAL__INPUT_STREAM_INL_H__INCLUDED


#include <dash/io/hdf5/InputStream.h>
#include <dash/io/hdf5/StorageDriver.h>

#include <dash/Matrix.h>
#include <dash/Array.h>


namespace dash {
namespace io {
namespace hdf5 {

template < typename Container_t >
inline InputStream & operator>> (
    InputStream & is,
    Container_t & container)
{
    if(is._launch_policy == dash::launch::async){
      is._load_object_impl_async(container);
    } else {
      is._load_object_impl(container);
    }
    return is;
}

} // namespace hdf5
} // namespace io
} // namespace dash

#endif // DASH__IO__HDF5__INTERNAL__INPUT_STREAM_INL_H__INCLUDED
