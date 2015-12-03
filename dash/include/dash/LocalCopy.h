#ifndef DASH__LOCAL_COPY_H__
#define DASH__LOCAL_COPY_H__

#include <vector>
#include <list>

namespace dash
{

//
// An explicit local copy of remote data
//
template<typename T>
class LocalCopy
{
private:
  dash::GlobPtr<T>   _gptr;
  size_t             _nelem;
  std::vector<T>     _vector;
  T*                 _data;
  std::list<dart_handle_t> _handles;
   
public:
  LocalCopy() = delete;

  //
  // Construct a LocalCopy object by specifying a GlobPtr gptr and the
  // number of consecutive elements in the rage to get/put starting
  // from gptr. The local data is stored in a std::vector.
  //
  LocalCopy(const dash::GlobPtr<T>& gptr,
	    size_t nelem) :_gptr(gptr)
  {
    _nelem = nelem;
    _vector.resize(nelem);
    _data = _vector.data();
  }

  //
  // User provides a pointer to local storage
  //
  LocalCopy(const dash::GlobPtr<T>& gptr,
	    size_t nelem, T* data) :_gptr(gptr)
  {
    _nelem = nelem;
    _data  = data;
  }
  

  T& operator[](size_t idx) {
    return _data[idx];
  }

  // get nelem elements using one transfer operation, using
  // the blocking DART call
  void get() {
    dart_get_blocking((void*)_data, _gptr.dart_gptr(),
		      sizeof(T)*_nelem);
  }

  // put nelem elements using one transfer operation, using
  // the blocking DART call
  void put() {
    dart_put_blocking((void*)_data, _gptr.dart_gptr(),
		      sizeof(T)*_nelem);
  }
  
  // get nelem elements using one transfer operation, using
  // the handle-based DART call
  void async_get() {
    dart_handle_t hdl;
    dart_get_handle((void*)_data, _gptr.dart_gptr(),
		    sizeof(T)*_nelem, &hdl);
    _handles.push_back(hdl);
  }
  
  // put nelem elements using one transfer operation, using
  // the handle-based DART call
  void async_put() {
    dart_handle_t hdl;
    dart_put_handle((void*)_data, _gptr.dart_gptr(),
		    sizeof(T)*_nelem, &hdl);
    _handles.push_back(hdl);
  }

  void wait() {
    for( auto& el: _handles ) {
      dart_wait(el);
    }
  }
};


} // namespace dash


#endif // DASH__LOCAL_COPY_H__

