
#include <gtest/gtest.h>
#include <string.h>

#include "DARTDatatypesTest.h"

#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_types.h>

TEST_F(DARTDatatypesTest, StridedSimple) {
  constexpr size_t num_elem_per_unit = 100;
  constexpr size_t stride_size = 2;

  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, DART_TYPE_INT, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = i;
  }

  dash::barrier();

  /**
   * Create a strided type and fetch elements from our neighbor.
   * Data:   0 1 2 3 ... 10 11 12 ... 90 91 92 93 ... 99
   * Result: 0 10 20 ... 90
   */
  dart_datatype_t new_type;
  dart_type_create_strided(DART_TYPE_INT, stride_size, 1, &new_type);

  dart_unit_t neighbor = (dash::myid() + 1) % dash::size();

  // global-to-local strided-to-contig
  int *buf = new int[num_elem_per_unit];
  memset(buf, 0, sizeof(int)*num_elem_per_unit);
  gptr.unitid = neighbor;
  dart_get_blocking(buf, gptr, num_elem_per_unit / stride_size,
                    new_type, DART_TYPE_INT);

  // the first 50 elements should have a value
  for (int i = 0; i < num_elem_per_unit / stride_size; ++i) {
    ASSERT_EQ_U(i*stride_size, buf[i]);
  }

  // global-to-local strided-to-contig
  memset(buf, 0, sizeof(int)*num_elem_per_unit);

  dart_get_blocking(buf, gptr, num_elem_per_unit / stride_size,
                    DART_TYPE_INT, new_type);

  // every other element should have a value
  for (int i = 0; i < num_elem_per_unit; ++i) {
    if (i%2 == 0) {
      ASSERT_EQ_U(i/2, buf[i]);
    } else {
      ASSERT_EQ_U(0, buf[i]);
    }
  }

  dash::barrier();

  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);

  delete[] buf;

  dart_type_destroy(&new_type);

}

