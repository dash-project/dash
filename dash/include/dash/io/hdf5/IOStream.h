#ifndef DASH__IO__HDF5__IOSTREAM_h
#define DASH__IO__HDF5__IOSTREAM_h

#include <dash/io/IOStream.h>

namespace dash {
namespace io   {
namespace hdf5 {

// No subclassing necessary
using DeviceMode = dash::io::IOSBaseMode;
using StreamMode = dash::io::IOStreamMode<DeviceMode>;

} // namespace hdf5 
} // namespace io
} // namespace dash

#include <dash/io/hdf5/IOManip.h>
#include <dash/io/hdf5/InputStream.h>
#include <dash/io/hdf5/OutputStream.h>

#endif // DASH__IO__HDF5__IOSTREAM_h
