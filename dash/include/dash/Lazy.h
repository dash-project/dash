#ifndef DASH__LAZY_H__INCLUDED
#define DASH__LAZY_H__INCLUDED

#include <functional>


namespace dash {

/* TODO:
 * Should be
 * 
 *   template <typename T, bool evaluated>
 *   class Lazy;
 *
 * with:
 *
 *   constexpr Lazy<T, true>
 *   Lazy<T, false>::operator T() const {
 *     return Lazy<T, true>(gen_fun());
 *   }
 *
 * and:
 *
 *   constexpr Lazy<T, true>
 *   Lazy<T, true>::operator T() const {
 *     return _value;
 *   }
 * 
 *
 */

template <typename T>
class Lazy {

public:
  Lazy()
    : _initiator(default_initiator)
    , _initialized(false)
  { }

  Lazy(std::function<T()> initiator)
    : _initiator(initiator)
    , _initialized(false)
  { }
  
  T & get() {
    if (!_initialized) {
      _value       = _initiator();
      _initialized = true;
    }
    return _value;
  }
  operator T() {
    return get();
  }
  T & operator*() {
    return get();
  }

  Lazy<T> & operator=(const Lazy<T>& other) {
    _initiator   = other._initiator;
    _initialized = false;
    return *this;
  }

private:
  static T default_initiator() {
    return T();
  }
  std::function<T()> _initiator;
  T                  _value;
  bool               _initialized;
}; 

} // namespace dash

#endif // DASH__LAZY_H__INCLUDED
