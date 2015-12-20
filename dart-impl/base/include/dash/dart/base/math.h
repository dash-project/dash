#ifndef DART__BASE__MATH_H__
#define DART__BASE__MATH_H__

#define DART_MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define DART_MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

#endif  /* DART__BASE__MATH_H__ */
