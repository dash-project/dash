#ifndef DASH__IO__HDF5_INPUT_STREAM_H__
#define DASH__IO__HDF5_INPUT_STREAM_H__

#ifdef DASH_ENABLE_HDF5

#include <string>
#include <dash/Matrix.h>

namespace dash {
namespace io {

/**
 * DASH stream API to store an dash::Array or dash::Matrix
 * in an HDF5 file using parallel IO.
 *
 * All operations are collective.
 */

class HDF5InputStream {
  private:
    std::string                _filename;
    std::string                _dataset;
    hdf5_options							 _foptions;

  public:
    HDF5InputStream(std::string filename)
				: _filename(filename),
					_dataset("data"),
					_foptions(dash::io::StoreHDF::get_default_options())
		{ }

    friend HDF5InputStream & operator>> (
        HDF5InputStream & is,
        const HDF5dataset & tbl) {
        is._dataset = tbl._dataset;
        return is;
    }


		// IO Manipulators
		friend HDF5InputStream & operator>> (
			HDF5InputStream & is,
			HDF5setpattern_key pk) {
			is._foptions.pattern_metadata_key = pk._key;
			return is;
		}

		friend HDF5InputStream & operator>> (
			HDF5InputStream & is,
			HDF5restore_pattern rs) {
			is._foptions.restore_pattern = rs._restore;
			return is;
		}


    // Array Implementation
    template <
        typename value_t,
        typename index_t,
        class    pattern_t >
    friend HDF5InputStream & operator>> (
        HDF5InputStream & is,
        dash::Array < value_t,
        index_t,
        pattern_t > &array);

    // Matrix Implementation
    template <
        typename value_t,
        dim_t    ndim,
        typename index_t,
        class    pattern_t >
    friend HDF5InputStream & operator>> (
        HDF5InputStream & is,
        dash::Matrix < value_t,
        ndim,
        index_t,
        pattern_t > &matrix);
};

} // namespace io
} // namespace dash

#include <dash/io/internal/HDF5InputStream-inl.h>

#endif // DASH_ENABLE_HDF5

#endif // DASH__IO__HDF5_INPUT_STREAM_H__
