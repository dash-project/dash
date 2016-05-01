#ifndef DART__BASE__MACRO_H_
#define DART__BASE__MACRO_H_

/**
 * Macro parameter value string expansion.
 */
#define dart__tostr(s) #s
/**
 * Macro parameter string expansion.
 */
#define dart__toxstr(s) dart__tostr(s)
/**
 * Mark variable as intentionally or potentially unused to avoid compiler
 * warning from unused variable.
 */
#define dart__unused(x) (void)(x)

#endif /* DART__BASE__MACRO_H_ */
