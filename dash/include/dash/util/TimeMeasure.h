#ifndef DASH__UTIL__TIME_MEASURE_H_
#define DASH__UTIL__TIME_MEASURE_H_

namespace dash {
namespace util {

class TimeMeasure {
 public: 
   typedef enum {
     Counter = 0,
     Clock   = 1
   } MeasureMode; 

   typedef enum {
     Cycles = 0,
     Time   = 1
   } MeasureDomain; 
};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__TIME_MEASURE_H_
