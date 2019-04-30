
#include "DARTOnesidedTest.h"

#include <dash/Array.h>
#include <dash/Onesided.h>


TEST_F(DARTOnesidedTest, GetBlockingSingleBlock)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 1200;
  
  dart_gptr_t gptr;
  int *local_ptr;
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, dart_type, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = (dash::myid() * 1000) + i;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit];

  dash::barrier();

  // Global pointer of block to copy from:
  dart_unit_t neighbour = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbour;

  // Copy values:
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
  dart_get_blocking(
    buf,                                // lptr dest
    gptr,                               // gptr src
    num_elem_per_unit,
    dart_type,
    dart_type
  );

  LOG_MESSAGE("Validating values");
  value_t offset = ((dash::myid() + 1) % dash::size()) * 1000;
  for (size_t i = 0; i < num_elem_per_unit; ++i) {
    value_t expected = offset + i;
    ASSERT_EQ_U(expected, buf[i]);
  }

  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, PutBlockingSingleBlock)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 100;
  
  dart_gptr_t gptr;
  int *local_ptr;
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, dart_type, &gptr);

  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Zero put area
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = 0;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit];
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    buf[i] = (dash::myid() * 1000) + i;
  }

  dash::barrier();

  // Global pointer of block to copy to:
  dart_unit_t neighbour = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbour;
  // Put values:
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
  dart_put_blocking(
    gptr,   // gptr dest
    buf,    // lptr src
    num_elem_per_unit,
    dart_type,
    dart_type
  );
  // put from left neighbour into own memory is not controlled by put_blocking
  dash::barrier();
  LOG_MESSAGE("Validating values");
  value_t offset = ((dash::myid() - 1) % dash::size()) * 1000;
  for (size_t l = 0; l < num_elem_per_unit; ++l) {
    value_t expected = offset + l;
    ASSERT_EQ_U(expected, local_ptr[l]);
  }

  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, GetBlockingSingleBlockTeam)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

  auto& split_team = dash::Team::All().split(2);

  const size_t num_elem_per_unit = 120;
  
  dart_gptr_t gptr;
  int *local_ptr;
  dart_team_unit_t my_rel_id;
  dart_team_myid(split_team.dart_id(), &my_rel_id);
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    split_team.dart_id(), num_elem_per_unit, dart_type, &gptr);
  gptr.unitid = my_rel_id.id;
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = (my_rel_id.id * 1000) + i;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit];

  dash::barrier();

  // Global pointer of block to copy from:
  dart_unit_t neighbour = (my_rel_id.id + 1) % split_team.size();
  gptr.unitid = neighbour;

  // Copy values:
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
  dart_get_blocking(
    buf,                                // lptr dest
    gptr,                               // gptr start
    num_elem_per_unit,
    dart_type,
    dart_type
  );

  LOG_MESSAGE("Validating values");
  value_t offset = ((my_rel_id.id + 1) % split_team.size()) * 1000;
  for (size_t i = 0; i < num_elem_per_unit; ++i) {
    value_t expected = offset + i;
    ASSERT_EQ_U(expected, buf[i]);
  }

  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, GetSingleBlock)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 1200;
  
  dart_gptr_t gptr;
  int *local_ptr;
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, dart_type, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = (dash::myid() * 1000) + i;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit];
  dash::barrier();

  // Global id of neighbour to copy from:
  dart_unit_t neighbour = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbour;

  // Copy values:
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
  dart_get(
    buf,                                // lptr dest
    gptr,                               // gptr start
    num_elem_per_unit,
    dart_type,
    dart_type
  );
  
  dart_flush(gptr);
  LOG_MESSAGE("Validating values");
  value_t offset = ((dash::myid() + 1) % dash::size()) * 1000;
  for (size_t i = 0; i < num_elem_per_unit; ++i) {
    value_t expected = offset + i;
    ASSERT_EQ_U(expected, buf[i]);
  }
  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, PutSingleBlock)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 100;
  
  dart_gptr_t gptr;
  int *local_ptr;
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, dart_type, &gptr);

  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Zero put area
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = 0;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit];
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    buf[i] = (dash::myid() * 1000) + i;
  }

  dash::barrier();

  // Global pointer of block to copy to:
  dart_unit_t neighbour = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbour;
  // Copy values:
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
  dart_put(
    gptr,                     // gptr dest
    buf,                      // lptr src
    num_elem_per_unit,
    dart_type,
    dart_type
  );

  dart_flush(gptr);
  dash::barrier();
  LOG_MESSAGE("Validating values");
  value_t offset;
  if(dash::myid() == 0){
    offset = (dash::size() -1) * 1000;
  }
  else{
    offset = (dash::myid() -1) * 1000;
  }
  
  for (size_t i = 0; i < num_elem_per_unit; ++i) {
    value_t expected = offset + i;
    ASSERT_EQ_U(expected, local_ptr[i]);
  }
  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, PutHandleSingleRemote)
{
  // Handle variant of put, the handle contains src and dest seg_id as well as queue AFTER return
  // non blocking so must use wait at the end
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 100;
  
  dart_gptr_t gptr;
  int *local_ptr;
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, dart_type, &gptr);

  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Zero put area
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = 0;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit];
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    buf[i] = (dash::myid() * 1000) + i;
  }

  dash::barrier();

  // Global pointer of block to copy to:
  dart_unit_t neighbour = (dash::myid() + 1) % dash::size();
  gptr.unitid = neighbour;
  // Copy values:
  LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
  dart_handle_t handle;
  EXPECT_EQ_U(
        DART_OK,
    dart_put_handle(
      gptr,
      buf,
      num_elem_per_unit,
      dart_type,
      dart_type,
      &handle
    )
  );
  // wait for completion
  //sleep(3);
  dart_wait(&handle);
  dash::barrier();
  LOG_MESSAGE("Validating values");
  value_t offset = ((dash::myid() - 1) % dash::size()) * 1000;
  for(int i = 0; i < num_elem_per_unit; ++i)
  {
    auto expected =  offset + i;
    ASSERT_EQ_U(local_ptr[i], expected);
  }

  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, GetAllRemote)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 1000;
  
  dart_gptr_t gptr;
  int *local_ptr;
  if (dash::size() < 2) {
    return;
  }
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit, dart_type, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Assign initial values: [1000, 1001, 1002, ...] 
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = (dash::myid() * 1000) + i;
  }
  // Array to store local copy:
  auto *buf = new int[num_elem_per_unit * dash::size()];
  dash::barrier();

  LOG_MESSAGE("Requesting remote blocks");
  // Copy values from all non-local blocks to the local array:

  for (size_t u = 0; u < dash::size(); ++u) {
    if (u != static_cast<size_t>(dash::myid())) {
      LOG_MESSAGE("Requesting block from unit %zu", u);
      // Global pointer of block to copy from:
      gptr.unitid = u;
      LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
      EXPECT_EQ_U(
        DART_OK,
        dart_get(
          buf + (u * num_elem_per_unit),
          gptr,
          num_elem_per_unit,
          dart_type,
          dart_type
        )
      );
    }
  }
  // Wait for completion of get operations:
  LOG_MESSAGE("Waiting for completion of async requests");
  gptr.unitid = dash::myid();
  dart_flush_all(gptr);
  //sleep(5);
  LOG_MESSAGE("Validating values");
  value_t offset;
  for (size_t u = 0; u < dash::size(); ++u) {
    if (u != static_cast<size_t>(dash::myid())) {
      offset = u * 1000;
      for(size_t i = 0; i < num_elem_per_unit; ++i)
      {
        ASSERT_EQ_U(buf[(u * num_elem_per_unit) +i], (offset + i));
      }
    }
  }
  
  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
}

TEST_F(DARTOnesidedTest, PutHandleAllRemote)
{
  // Handle variant of put, the handle contains src and dest seg_id as well as queue AFTER return
  // non blocking so must use wait at the end
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  const size_t num_elem_per_unit = 100;
  
  dart_gptr_t gptr;
  value_t *local_ptr;
  // Array of handles, one for each dart_put_handle:
  std::vector<dart_handle_t> handles;
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, num_elem_per_unit * dash::size(), dart_type, &gptr);

  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Zero put area
  for (int i = 0; i < num_elem_per_unit; ++i) {
    local_ptr[i] = 0;
  }
   
  auto buf = (dash::myid() + 1) * 1000;

  dash::barrier();

  // Copy values to all non-local blocks with the same offset:
  gptr.addr_or_offs.offset = static_cast<uint64_t>(dash::myid() * sizeof(value_t));
  for (size_t u = 0; u < dash::size(); ++u) {
    if (u != static_cast<size_t>(dash::myid())) {
      LOG_MESSAGE("Putting block to unit %zu", u);
      dart_handle_t handle;
      gptr.unitid = u;

      LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, num_elem_per_unit);
      EXPECT_EQ_U(
        DART_OK,
        dart_put_handle(
            gptr,
            &buf,
            1,
            dart_type,
            dart_type,
            &handle)
      );
      LOG_MESSAGE("dart_put_handle returned handle %p",
                  static_cast<void*>(handle));
      handles.push_back(handle);
    }
  }

  // Wait for completion of put operations:
  LOG_MESSAGE("Waiting for completion of async requests");
  dart_waitall(handles.data(), handles.size());
  dash::barrier();

  LOG_MESSAGE("Validating values");
  for(int j = 0; j < dash::size(); j++)
  {
    if(j != static_cast<size_t>(dash::myid()))
    {
        value_t expected  = (j + 1) * 1000;
        ASSERT_EQ_U(expected,  local_ptr[j]);
    }
    else
    {
       ASSERT_EQ_U(0, local_ptr[j]);
    }
    
  }

  // Wait for cleanup until all validation is finished
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  gptr.addr_or_offs.offset = 0;
  dart_team_memfree(gptr);
}

TEST_F(DARTOnesidedTest, GetHandleAllRemote)
{
  using value_t = int;
  dart_datatype_t dart_type = DART_TYPE_INT;

  dart_gptr_t gptr;
  int *local_ptr;
  if (dash::size() < 2) {
    return;
  }
  // create memory that is accessible for all team members
  dart_team_memalloc_aligned(
    DART_TEAM_ALL, dash::size(), dart_type, &gptr);
  gptr.unitid = dash::myid();
  dart_gptr_getaddr(gptr, (void**)&local_ptr);
  // Assign initial values: [1000, 1001, 1002, ...] 
  local_ptr[0] = 1000 * (dash::myid() + 1);

  // Array to store local copys:
  auto *buf = new value_t[dash::size()];
  for(int i = 0; i < dash::size(); i++)
  {
    buf[i] = 0;
  }
  // Array of handles, one for each dart_get_handle:
  std::vector<dart_handle_t> handles;
  dash::barrier();
  
  LOG_MESSAGE("Requesting remote blocks");
  // Copy values from all non-local blocks:
  for (size_t u = 0; u < dash::size(); ++u) {
    if (u != static_cast<size_t>(dash::myid())) {
      LOG_MESSAGE("Requesting block from unit %zu", u);
      dart_handle_t handle;
      gptr.unitid = u;
      LOG_MESSAGE("DART storage: dtype:%ld nelem:%zu", dart_type, 1);
      EXPECT_EQ_U(
        DART_OK,
        dart_get_handle(
            &(buf[u]),
            gptr,
            1,
            dart_type,
            dart_type,
            &handle)
      );
      LOG_MESSAGE("dart_get_handle returned handle %p",
                  static_cast<void*>(handle));
      handles.push_back(handle);
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
  for (size_t g = 0; g < dash::size(); ++g) {
    if (g != dash::myid()) {
      ASSERT_EQ_U( (g + 1) * 1000, buf[g]);
    }
    else
    {
       ASSERT_EQ_U(0, buf[g]);
    }
  }
  
  dash::barrier();
  // clean-up
  gptr.unitid = 0;
  dart_team_memfree(gptr);
  delete[] buf;
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
  auto *buf = new int[num_elem_per_unit];

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

    // global-to-local contig-to-strided
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

  auto *buf = new int[num_elem_per_unit];
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
  auto *buf = new int[num_elem_per_unit];
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

  auto *buf = new int[num_elem_per_unit];
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

  auto *buf = new int[num_elem_per_unit];
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

  auto *buf = new int[num_elem_per_unit];
  memset(buf, 0, sizeof(int)*num_elem_per_unit);

  auto *index_map_to = new int[num_elem_per_unit];
  memset(index_map_to, 0, sizeof(int)*num_elem_per_unit);

  auto *index_map_from = new int[num_elem_per_unit];
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

