
#include "DARTOnesidedTest.h"

#include <dash/Array.h>
#include <dash/Onesided.h>


TEST_F(DARTOnesidedTest, GetBlockingSingleBlock)
{
  typedef int value_t;
  const size_t block_size = 10;
  size_t num_elem_total   = dash::size() * block_size;
  dash::Array<value_t> array(num_elem_total, dash::BLOCKED);
  // Array to store local copy:
  int local_array[block_size];
  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (size_t l = 0; l < block_size; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  // Unit to copy values from:
  dart_unit_t unit_src  = (dash::myid() + 1) % dash::size();
  // Global start index of block to copy:
  int g_src_index       = unit_src * block_size;
  // Copy values:
  dash::dart_storage<value_t> ds(block_size);
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", ds.dtype, ds.nelem);
  dart_get_blocking(
    local_array,                                // lptr dest
    (array.begin() + g_src_index).dart_gptr(),  // gptr start
    ds.nelem,
    ds.dtype,
    ds.dtype
  );
  for (size_t l = 0; l < block_size; ++l) {
    value_t expected = array[g_src_index + l];
    ASSERT_EQ_U(expected, local_array[l]);
  }
}

TEST_F(DARTOnesidedTest, GetBlockingSingleBlockTeam)
{
  typedef int value_t;

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

  auto& split_team = dash::Team::All().split(2);
  const size_t block_size = 10;
  size_t num_elem_total   = split_team.size() * block_size;


  dash::Array<value_t> array(num_elem_total, dash::BLOCKED, split_team);
  // Array to store local copy:
  int local_array[block_size];
  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (size_t l = 0; l < block_size; ++l) {
    array.local[l] = (split_team.myid() + 1) * 1000 +
                       (split_team.dart_id() * 100) + l;
  }
  array.barrier();
  // Unit to copy values from:
  dart_unit_t unit_src  = (split_team.myid() + 1) % split_team.size();
  // Global start index of block to copy:
  int g_src_index       = unit_src * block_size;
  // Copy values:
  dash::dart_storage<value_t> ds(block_size);
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", ds.dtype, ds.nelem);
  dart_get_blocking(
    local_array,                                // lptr dest
    (array.begin() + g_src_index).dart_gptr(),  // gptr start
    ds.nelem,
    ds.dtype,
    ds.dtype
  );
  for (size_t l = 0; l < block_size; ++l) {
    value_t expected = array[g_src_index + l];
    ASSERT_EQ_U(expected, local_array[l]);
  }
}

TEST_F(DARTOnesidedTest, GetHandleAllRemote)
{
  typedef int value_t;
  const size_t block_size = 5000;
  size_t num_elem_copy    = (dash::size() - 1) * block_size;
  size_t num_elem_total   = dash::size() * block_size;
  dash::Array<value_t> array(num_elem_total, dash::BLOCKED);
  if (dash::size() < 2) {
    return;
  }
  // Array to store local copy:
  int * local_array = new int[num_elem_copy];
  // Array of handles, one for each dart_get_handle:
  std::vector<dart_handle_t> handles;
  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (size_t l = 0; l < block_size; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  LOG_MESSAGE("Requesting remote blocks");
  // Copy values from all non-local blocks:
  size_t block = 0;
  for (size_t u = 0; u < dash::size(); ++u) {
    if (u != static_cast<size_t>(dash::myid())) {
      LOG_MESSAGE("Requesting block %zu from unit %zu", block, u);
      dart_handle_t handle;

      dash::dart_storage<value_t> ds(block_size);
      LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", ds.dtype, ds.nelem);
      EXPECT_EQ_U(
        DART_OK,
        dart_get_handle(
            local_array + (block * block_size),
            (array.begin() + (u * block_size)).dart_gptr(),
            ds.nelem,
            ds.dtype,
            ds.dtype,
            &handle)
      );
      LOG_MESSAGE("dart_get_handle returned handle %p",
                  static_cast<void*>(handle));
      handles.push_back(handle);
      ++block;
    }
  }
  // Wait for completion of get operations:
  LOG_MESSAGE("Waiting for completion of async requests");
  dart_waitall_local(
    handles.data(),
    handles.size()
  );

  LOG_MESSAGE("Validating values");
  int l = 0;
  for (size_t g = 0; g < array.size(); ++g) {
    auto unit = array.pattern().unit_at(g);
    if (unit != dash::myid()) {
      value_t expected = array[g];
      ASSERT_EQ_U(expected, local_array[l]);
      ++l;
    }
  }
  delete[] local_array;
  ASSERT_EQ_U(num_elem_copy, l);
}


TEST_F(DARTOnesidedTest, StridedGetSimple) {
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

  dart_unit_t neighbor = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbor;

  for (int stride = 1; stride <= max_stride_size; stride++) {

    LOG_MESSAGE("Testing GET with stride %i", stride);

    dart_datatype_t new_type;
    dart_type_create_strided(DART_TYPE_INT, stride, 1, &new_type);

    // global-to-local strided-to-contig
    memset(buf, 0, sizeof(int)*num_elem_per_unit);
    dart_get_blocking(buf, gptr, num_elem_per_unit / stride,
                      new_type, DART_TYPE_INT);

    // the first elements should have a value
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

  dash::barrier();

  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);

  delete[] buf;

}


TEST_F(DARTOnesidedTest, StridedPutSimple) {
  constexpr size_t num_elem_per_unit = 120;
  constexpr size_t max_stride_size   = 5;

  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, DART_TYPE_INT, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);

  dart_unit_t neighbor = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbor;

  int *buf = new int[num_elem_per_unit];
  for (int i = 0; i < num_elem_per_unit; ++i) {
    buf[i] = i;
  }

  for (int stride = 1; stride <= max_stride_size; stride++) {

    LOG_MESSAGE("Testing PUT with stride %i", stride);

    memset(local_ptr, 0, sizeof(int)*num_elem_per_unit);

    dart_datatype_t new_type;
    dart_type_create_strided(DART_TYPE_INT, stride, 1, &new_type);

    dash::barrier();
    // local-to-global strided-to-contig
    dart_put_blocking(gptr, buf, num_elem_per_unit / stride,
                      new_type, DART_TYPE_INT);
    dash::barrier();

    // the first elements should have a value
    for (int i = 0; i < num_elem_per_unit / stride; ++i) {
      ASSERT_EQ_U(i*stride, local_ptr[i]);
    }

    // local-to-global contig-to-strided
    memset(local_ptr, 0, sizeof(int)*num_elem_per_unit);

    dash::barrier();
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


TEST_F(DARTOnesidedTest, BlockedStridedToStrided) {

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
  dash::barrier();

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

  dash::barrier();

  delete[] buf;
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
}


TEST_F(DARTOnesidedTest, IndexedGetSimple) {

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

  dart_get_blocking(buf, gptr, num_elems, DART_TYPE_INT, new_type);

  idx = 0;
  for (size_t i = 0; i < num_blocks; ++i) {
    for (size_t j = 0; j < blocklens[i]; ++j) {
      ASSERT_EQ_U(local_ptr[idx], buf[offsets[i] + j]);
      ++idx;
    }
  }

  dart_type_destroy(&new_type);

  dash::barrier();

  delete[] buf;
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
}

TEST_F(DARTOnesidedTest, IndexedPutSimple) {

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

  dart_datatype_t new_type;
  dart_type_create_indexed(DART_TYPE_INT, num_blocks, blocklens.data(),
                           offsets.data(), &new_type);

  dash::barrier();

  int *buf = new int[num_elem_per_unit];
  for (int i = 0; i < num_elem_per_unit; ++i) {
    buf[i] = i;
  }

  memset(local_ptr, 0, sizeof(int)*num_elem_per_unit);

  dash::barrier();

  // indexed-to-contig
  dart_put_blocking(gptr, buf, num_elems, new_type, DART_TYPE_INT);

  dash::barrier();

  size_t idx = 0;
  for (size_t i = 0; i < num_blocks; ++i) {
    for (size_t j = 0; j < blocklens[i]; ++j) {
      ASSERT_EQ_U(buf[offsets[i] + j], local_ptr[idx]);
      ++idx;
    }
  }

  // check we haven't copied more elements than requested
  for (size_t i = idx; i < num_elem_per_unit; ++i) {
    ASSERT_EQ_U(0, local_ptr[i]);
  }

  // contig-to-indexed
  memset(local_ptr, 0, sizeof(int)*num_elem_per_unit);

  dash::barrier();

  dart_put_blocking(gptr, buf, num_elems, DART_TYPE_INT, new_type);
  dash::barrier();

  idx = 0;
  for (size_t i = 0; i < num_blocks; ++i) {
    for (size_t j = 0; j < blocklens[i]; ++j) {
      ASSERT_EQ_U(buf[idx], local_ptr[offsets[i] + j]);
      ++idx;
    }
  }

  dart_type_destroy(&new_type);

  delete[] buf;
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
}


TEST_F(DARTOnesidedTest, IndexedToIndexedGet) {

  constexpr size_t num_elem_per_unit = 120;
  constexpr size_t num_blocks_to     = 10;
  constexpr size_t num_blocks_from   = 5;

  std::vector<size_t> blocklens_to(num_blocks_to);
  std::vector<size_t> offsets_to(num_blocks_to);
  std::vector<size_t> blocklens_from(num_blocks_from);
  std::vector<size_t> offsets_from(num_blocks_from);

  // set up offsets and block lengths
  size_t num_elems_to = 0;
  for (int i = 0; i < num_blocks_to; ++i) {
    blocklens_to[i] = (i+1);
    offsets_to[i]   = (i*5);
    num_elems_to   += blocklens_to[i];
  }

  size_t num_elems_from = 0;
  for (int i = 0; i < num_blocks_from; ++i) {
    blocklens_from[i] = (i+1)+8;
    offsets_from[i]   = (i*10);
    num_elems_from   += blocklens_from[i];
  }

  ASSERT_EQ_U(num_elems_from, num_elems_to);

  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, DART_TYPE_INT, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = i;
  }

  dart_datatype_t to_type;
  dart_type_create_indexed(DART_TYPE_INT, num_blocks_to,
                           blocklens_to.data(),
                           offsets_to.data(), &to_type);

  dart_datatype_t from_type;
  dart_type_create_indexed(DART_TYPE_INT, num_blocks_from,
                           blocklens_from.data(),
                           offsets_from.data(), &from_type);

  dash::barrier();

  int *buf = new int[num_elem_per_unit];
  memset(buf, 0, sizeof(int)*num_elem_per_unit);

  int *index_map_to = new int[num_elem_per_unit];
  memset(index_map_to, 0, sizeof(int)*num_elem_per_unit);

  int *index_map_from = new int[num_elem_per_unit];
  memset(index_map_from, 0, sizeof(int)*num_elem_per_unit);

  // populate the flat list of indices to copy from
  size_t idx = 0;
  for (size_t i = 0; i < num_blocks_from; ++i) {
    for (size_t j = 0; j < blocklens_from[i]; ++j) {
      index_map_from[idx] = offsets_from[i] + j;
      ++idx;
    }
  }

  // populate the mapping from target indices to values
  idx = 0;
  for (size_t i = 0; i < num_blocks_to; ++i) {
    for (size_t j = 0; j < blocklens_to[i]; ++j) {
      index_map_to[offsets_to[i] + j] = index_map_from[idx];
      ++idx;
    }
  }

  // indexed-to-indexed
  dart_get_blocking(buf, gptr, num_elems_to, from_type, to_type);

  for (size_t i = 0; i < num_elem_per_unit; ++i) {
    ASSERT_EQ_U(local_ptr[index_map_to[i]], buf[i]);
  }

  dart_type_destroy(&from_type);
  dart_type_destroy(&to_type);

  dash::barrier();

  delete[] buf;
  delete[] index_map_to;
  delete[] index_map_from;
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
}

