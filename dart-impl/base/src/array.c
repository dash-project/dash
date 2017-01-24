
#include <dash/dart/base/array.h>

int dart__base__intsunique(
  int * values,
  int   num_values)
{
  int last_unique = 0;
  if (num_values >= 2) {
    for (int i = 1; i < num_values; i++) {
      if (values[i] != values[last_unique]) {
        ++last_unique;
        if (i != last_unique) {
          values[last_unique] = values[i];
        }
      }
    }
  }
  return last_unique + 1;
}

