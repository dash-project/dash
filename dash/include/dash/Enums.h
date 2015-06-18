#ifndef DASH__ENUMS_H_ 
#define DASH__ENUMS_H_

#include <dash/Exception.h>
#include <dash/internal/Math.h>
#include <cstddef>
#include <functional>

namespace dash {

typedef enum MemArrange {
  Undefined = 0,
  ROW_MAJOR,
  COL_MAJOR
} MemArrange;

} // namespace dash

#endif // DASH__ENUMS_H_
