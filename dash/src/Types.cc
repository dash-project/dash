#include <dash/Types.h>
#include <dash/dart/if/dart_types.h>

namespace dash {

template<typename Type>
const dart_datatype_t dart_datatype<Type>::value   = DART_TYPE_UNDEFINED;

const dart_datatype_t dart_datatype<int>::value    = DART_TYPE_INT;
const dart_datatype_t dart_datatype<float>::value  = DART_TYPE_FLOAT;
const dart_datatype_t dart_datatype<double>::value = DART_TYPE_DOUBLE;

} // namespace dash
