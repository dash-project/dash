#ifndef DASH__IO__INTERNAL__HDF5__OUTPUT_STREAM_INL_H__INCLUDED
#define DASH__IO__INTERNAL__HDF5__OUTPUT_STREAM_INL_H__INCLUDED


#include <dash/io/hdf5/OutputStream.h>
#include <dash/io/hdf5/StorageDriver.h>

#include <dash/Matrix.h>
#include <dash/Array.h>


namespace dash {
namespace io {
namespace hdf5 {

template < typename Container_t >
inline OutputStream & operator<< (
   OutputStream & os,
   Container_t  & container )
{
    if(os._launch_policy == dash::launch::async){
      os._store_object_impl_async(container);
    } else {
      os._store_object_impl(container);
    }
  // Append future data in this stream
  os._foptions.overwrite_file = false;
  return os;
}

} // namespace hdf5
} // namespace io
} // namespace dash

#endif // DASH__IO__INTERNAL__HDF5__OUTPUT_STREAM_INL_H__INCLUDED
