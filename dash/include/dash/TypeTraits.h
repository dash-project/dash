#ifndef DASH__TYPE_TRAITS_H_
#define DASH__TYPE_TRAITS_H_

#include <type_traits>

/// Forward decls

namespace dash {

template <class T>
class Atomic;

}  // namespace dash

namespace std {

template <class T>
struct add_const<dash::Atomic<T>> {
  using type = dash::Atomic<typename std::add_const<T>::type>;
};

}  // namespace std

#endif
