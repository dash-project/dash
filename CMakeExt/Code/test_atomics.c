#include <stdint.h>
#include <stdatomic.h>

struct test128_t {
  uint64_t t1;
  uint64_t t2;
};

int main(int argc, char **argv)
{
  struct test128_t t1, t2;
  int res = atomic_compare_exchange_weak(&t1, &t2, t2);

  if (res != 1) return 1;

  return 0;
}
