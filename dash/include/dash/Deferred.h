#ifndef DASH__DEFERRED_H__INCLUDED
#define DASH__DEFERRED_H__INCLUDED

#include <functional>


namespace dash {

/* TODO:
 * Should be
 * 
 *   template <typename T, bool evaluated>
 *   class Deferred;
 *
 * with:
 *
 *   constexpr Deferred<T, true>
 *   Deferred<T, false>::operator T() const {
 *     return Deferred<T, true>(gen_fun());
 *   }
 *
 * and:
 *
 *   constexpr Deferred<T, true>
 *   Deferred<T, true>::operator T() const {
 *     return _value;
 *   }
 * 
 *
 */

template <typename T>
class Deferred {

  static T default_gen() {
    return T();
  }

  std::function<T()> _gen;
  T                  _value;
  bool               _initialized;

public:
  Deferred()
    : _gen(default_gen)
    , _initialized(false)
  { }

  Deferred(std::function<T()> gen)
    : _gen(gen)
    , _initialized(false)
  { }
  
  T & get() {
    if (!_initialized) {
      _value       = _gen();
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

  Deferred<T> & operator=(const Deferred<T>& other) {
    _gen   = other._gen;
    _initialized = false;
    return *this;
  }
}; 

} // namespace dash

#endif // DASH__DEFERRED_H__INCLUDED
