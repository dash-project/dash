
#include <gtest/gtest.h>
#include <string.h>

#include "DARTDatatypesTest.h"

#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_types.h>

TEST_F(DARTDatatypesTest, StridedGetSimple) {
  constexpr size_t num_elem_per_unit = 120;
  constexpr size_t max_stride_size   = 5;

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
  int *buf = new int[num_elem_per_unit];

  for (int stride = 1; stride <= max_stride_size; stride++) {

    LOG_MESSAGE("Testing GET with stride %i", stride);

    dart_datatype_t new_type;
    dart_type_create_strided(DART_TYPE_INT, stride, 1, &new_type);

    dart_unit_t neighbor = (dash::myid() + 1) % dash::size();

    // global-to-local strided-to-contig
    memset(buf, 0, sizeof(int)*num_elem_per_unit);
    gptr.unitid = neighbor;
    dart_get_blocking(buf, gptr, num_elem_per_unit / stride,
                      new_type, DART_TYPE_INT);

    // the first 50 elements should have a value
    for (int i = 0; i < num_elem_per_unit / stride; ++i) {
      ASSERT_EQ_U(i*stride, buf[i]);
    }

    // global-to-local strided-to-contig
    memset(buf, 0, sizeof(int)*num_elem_per_unit);

    dart_get_blocking(buf, gptr, num_elem_per_unit / stride,
                      DART_TYPE_INT, new_type);

    // every other element should have a value
    for (int i = 0; i < num_elem_per_unit; ++i) {
      if (i%stride == 0) {
        ASSERT_EQ_U(i/stride, buf[i]);
      } else {
        ASSERT_EQ_U(0, buf[i]);
      }
    }
    dart_type_destroy(&new_type);
  }

  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);

  delete[] buf;

}


TEST_F(DARTDatatypesTest, StridedPutSimple) {
  constexpr size_t num_elem_per_unit = 120;
  constexpr size_t max_stride_size   = 5;

  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, DART_TYPE_INT, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  memset(local_ptr, 0, sizeof(int)*num_elem_per_unit);

  dart_unit_t neighbor = (dash::myid() + 1) % dash::size();

  int *buf = new int[num_elem_per_unit];
  for (int i = 0; i < num_elem_per_unit; ++i) {
    buf[i] = i;
  }
  gptr.unitid = neighbor;

  for (int stride = 1; stride <= max_stride_size; stride++) {

    LOG_MESSAGE("Testing PUT with stride %i", stride);

    dash::barrier();
    dart_datatype_t new_type;
    dart_type_create_strided(DART_TYPE_INT, stride, 1, &new_type);

    // local-to-global strided-to-contig
    dart_put_blocking(gptr, buf, num_elem_per_unit / stride,
                      new_type, DART_TYPE_INT);

    dash::barrier();

    // the first 50 elements should have a value
    for (int i = 0; i < num_elem_per_unit / stride; ++i) {
      ASSERT_EQ_U(i*stride, local_ptr[i]);
    }

    // local-to-global strided-to-contig
    memset(local_ptr, 0, sizeof(int)*num_elem_per_unit);

    dart_put_blocking(gptr, buf, num_elem_per_unit / stride,
                      DART_TYPE_INT, new_type);

    dash::barrier();

    // every other element should have a value
    for (int i = 0; i < num_elem_per_unit; ++i) {
      if (i%stride == 0) {
        ASSERT_EQ_U(i/stride, local_ptr[i]);
      } else {
        ASSERT_EQ_U(0, local_ptr[i]);
      }
    }

    dart_type_destroy(&new_type);
  }

  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);

  delete[] buf;
}


TEST_F(DARTDatatypesTest, BlockedStridedToStrided) {

  constexpr size_t num_elem_per_unit = 120;
  constexpr size_t from_stride       = 5;
  constexpr size_t from_block_size   = 2;
  constexpr size_t to_stride         = 2;

  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, DART_TYPE_INT, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = i;
  }

  // global-to-local strided-to-contig
  int *buf = new int[num_elem_per_unit];
  memset(buf, 0, sizeof(int)*num_elem_per_unit);

  dart_datatype_t to_type;
  dart_type_create_strided(DART_TYPE_INT, to_stride, 1, &to_type);
  dart_datatype_t from_type;
  dart_type_create_strided(DART_TYPE_INT, from_stride,
                           from_block_size, &from_type);

  // strided-to-strided get
  dart_get_blocking(buf, gptr, num_elem_per_unit / from_stride * from_block_size,
                    from_type, to_type);

  int value = 0;
  for (int i = 0;
       i < num_elem_per_unit/from_stride*to_stride*from_block_size;
       ++i) {
    if (i%to_stride == 0) {
      ASSERT_EQ_U(value, buf[i]);
      // consider the block size we used as source
      // if
      if ((value%from_stride) < (from_block_size-1)) {
        // expect more elements with incremented value
        ++value;
      } else {
        value += from_stride - (from_block_size  - 1);
      }
    } else {
      ASSERT_EQ_U(0, buf[i]);
    }
  }

  dart_type_destroy(&from_type);
  dart_type_destroy(&to_type);

  delete[] buf;
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
}


TEST_F(DARTDatatypesTest, IndexedGetSimple) {

  constexpr size_t num_elem_per_unit = 120;
  constexpr size_t num_blocks        = 5;

  std::vector<size_t> blocklens(num_blocks);
  std::vector<size_t> offsets(num_blocks);

  // set up offsets and block lengths
  size_t num_elems = 0;
  for (int i = 0; i < num_blocks; ++i) {
    blocklens[i] = (i+1);
    offsets[i]   = (i*10);
    num_elems   += blocklens[i];
  }

  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, DART_TYPE_INT, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = i;
  }

  dart_datatype_t new_type;
  dart_type_create_indexed(DART_TYPE_INT, num_blocks, blocklens.data(),
                           offsets.data(), &new_type);

  dash::barrier();

  int *buf = new int[num_elem_per_unit];
  memset(buf, 0, sizeof(int)*num_elem_per_unit);

  // indexed-to-contig
  dart_get_blocking(buf, gptr, num_elems, new_type, DART_TYPE_INT);

  size_t idx = 0;
  for (size_t i = 0; i < num_blocks; ++i) {
    for (size_t j = 0; j < blocklens[i]; ++j) {
      ASSERT_EQ_U(local_ptr[offsets[i] + j], buf[idx]);
      ++idx;
    }
  }

  // check we haven't copied more elements than requested
  for (size_t i = idx; i < num_elem_per_unit; ++i) {
    ASSERT_EQ_U(0, buf[i]);
  }


  // contig-to-indexed
  memset(buf, 0, sizeof(int)*num_elem_per_unit);

  // indexed-to-contig
  dart_get_blocking(buf, gptr, num_elems, DART_TYPE_INT, new_type);

  idx = 0;
  for (size_t i = 0; i < num_blocks; ++i) {
    for (size_t j = 0; j < blocklens[i]; ++j) {
      ASSERT_EQ_U(local_ptr[idx], buf[offsets[i] + j]);
      ++idx;
    }
  }

  dart_type_destroy(&new_type);

  delete[] buf;
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
}
