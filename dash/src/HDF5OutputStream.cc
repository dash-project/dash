#ifdef DASH_ENABLE_HDF5

#include <dash/HDF5OutputStream.h>

#endif

namespace dash {
	HDF5OutputStream& operator<< (
				HDF5OutputStream& os,
				const HDF5Table& tbl)
		{
			os._table = tbl._table;
			return os;
		}

	HDF5OutputStream& operator<< (
			HDF5OutputStream& os,
			HDF5Options opts)
		{
			os._foptions = opts._foptions;
			//os._test = opts._test;
			return os;
		}
}
