#ifndef TEST_H
#define TEST_H

#define CHECK(fncall) do {                       \
    int _retval;                                                     \
    if ((_retval = fncall) != DART_OK) {                             \
      fprintf(stderr, "ERROR %d calling: %s"                         \
          " at: %s:%i",                      \
              _retval, #fncall, __FILE__, __LINE__);                 \
      fflush(stderr);                                                \
    }                                                                \
  } while(0)

#endif /* TEST_H */
