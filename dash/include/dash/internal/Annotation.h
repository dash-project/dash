#ifndef DASH__INTERNAL__ANNOTATION_H__INCLUDED
#define DASH__INTERNAL__ANNOTATION_H__INCLUDED

namespace dash {

static void prevent_opt_elimination() {
  asm volatile("");
}

/**
 * Prevent elimination of variable in compiler optimizations.
 */
template <typename T>
void prevent_opt_elimination(T && var) {
  asm volatile("" : "+r" (var)); // noop-read
}

} // namespace dash

#endif // DASH__INTERNAL__ANNOTATION_H__INCLUDED
