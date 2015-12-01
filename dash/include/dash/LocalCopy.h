#ifndef DASH__LOCAL_COPY_H__
#define DASH__LOCAL_COPY_H__

namespace dash
{

template<typename T>
class LocalCopy
{
private:
  dash::GlobPtr<T>   _gptr;
  size_t             _nelem;
  std::vector<T>     _data;
  
public:
  LocalCopy(const dash::GlobPtr<T>& gptr, size_t nelem) :
    _gptr(gptr)
  {
    _nelem = nelem;
    _data.resize(nelem);
  }

  T& operator[](size_t idx) {
    return _data[idx];
  }

  void get() {
    dart_get_blocking(_data.data(),
		      _gptr.dart_gptr(), sizeof(T)*_nelem);
  }
  
};


} // namespace dash


#endif // DASH__LOCAL_COPY_H__

