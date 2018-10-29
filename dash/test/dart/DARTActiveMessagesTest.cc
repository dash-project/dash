
#include "DARTActiveMessagesTest.h"
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_active_messages.h>

typedef struct testdata {
  int       sender;
  uint64_t  payload;
} testdata_t;

static uint64_t volatile num_messages;

static void remote_fn_neighbor(void *data)
{
  testdata_t *testdata = static_cast<testdata_t*>(data);
  int expected_sender = (dash::myid() + dash::size() - 1) % dash::size();
  ++num_messages;
  ASSERT_EQ_U(expected_sender, testdata->sender);
  ASSERT_EQ_U(0xDEADBEEF, testdata->payload);
}

static void remote_fn(void *data)
{
  testdata_t *testdata = static_cast<testdata_t*>(data);
  ++num_messages;
  std::cout << "remote_fn: " << num_messages << std::endl;
  ASSERT_GT_U(testdata->sender, 0);
  ASSERT_EQ_U(0xDEADBEEF, testdata->payload);
}

template<typename T>
static void remote_fn_increment_gptr(void *data)
{
  T one = static_cast<T>(1);
  dart_gptr_t gptr = *static_cast<dart_gptr_t*>(data);
  dash::dart_storage<T> ds(1);
  dart_accumulate(gptr, &one, ds.nelem, ds.dtype, DART_OP_SUM);
  dart_flush_all(gptr);
}

TEST_F(DARTActiveMessagesTest, Neighbor)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }
  int ret;
  int neighbor = (dash::myid() + 1) % dash::size();
  num_messages = 0;

  dart_amsgq_t q;
  ret = dart_amsg_openq(
      sizeof(testdata_t), 1000, dash::Team::All().dart_id(), &q);
  ASSERT_EQ_U(ret, DART_OK);
  testdata_t data;
  data.sender  = dash::myid();
  data.payload = 0xDEADBEEF;
  do {
    ret = dart_amsg_trysend(
        neighbor, q, &remote_fn_neighbor, &data, sizeof(data));

    if (ret == DART_ERR_AGAIN) {
      dart_amsg_process(q);
    }
  } while(ret == DART_ERR_AGAIN);

  ASSERT_EQ_U(ret, DART_OK);

  // dart_amsg_process_blocking is sync'ing processes
  ret = dart_amsg_process_blocking(q, dash::Team::All().dart_id());
  ASSERT_EQ_U(ret, DART_OK);

  uint64_t _num_messages = num_messages;
  ASSERT_EQ_U(_num_messages, 1);

  ASSERT_EQ_U(dart_amsg_closeq(q), DART_OK);
}


TEST_F(DARTActiveMessagesTest, AllToOne)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }
  int ret;
  num_messages = 0;

  dart_amsgq_t q;
  ret = dart_amsg_openq(
      sizeof(testdata_t), 1000, dash::Team::All().dart_id(), &q);
  ASSERT_EQ_U(ret, DART_OK);
  testdata_t data;
  data.sender  = dash::myid();
  data.payload = 0xDEADBEEF;

  dash::barrier();

  if (dash::myid() > 0) {
    do {
      ret = dart_amsg_trysend(0, q, &remote_fn, &data, sizeof(data));
      if (ret == DART_ERR_AGAIN) {
        dart_amsg_process(q);
      }
    } while(ret == DART_ERR_AGAIN);
    ASSERT_EQ_U(ret, DART_OK);
  } else {
    while (num_messages != (dash::size() - 1)) {
      ret = dart_amsg_process(q);
      ASSERT_EQ_U(ret, DART_OK);
    }
  }
  ret = dart_amsg_process_blocking(q, dash::Team::All().dart_id());
  ASSERT_EQ_U(dart_amsg_closeq(q), DART_OK);
}


TEST_F(DARTActiveMessagesTest, AllToOneBlock)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }
  int ret;
  num_messages = 0;

  dart_amsgq_t q;
  ret = dart_amsg_openq(
      sizeof(testdata_t), 1000, dash::Team::All().dart_id(), &q);
  ASSERT_EQ_U(ret, DART_OK);
  testdata_t data;
  data.sender  = dash::myid();
  data.payload = 0xDEADBEEF;

  dash::barrier();

  if (dash::myid() > 0) {
    do {
      ret = dart_amsg_trysend(0, q, &remote_fn, &data, sizeof(data));
      if (ret == DART_ERR_AGAIN) {
        dart_amsg_process(q);
      }
    } while(ret == DART_ERR_AGAIN);
    ASSERT_EQ_U(ret, DART_OK);
  }
  ret = dart_amsg_process_blocking(q, dash::Team::All().dart_id());
  ASSERT_EQ_U(ret, DART_OK);
  if (dash::myid() == 0) {
    uint64_t _num_messages = num_messages;
    ASSERT_EQ_U(_num_messages, (dash::size() - 1));
  }

  ASSERT_EQ_U(dart_amsg_closeq(q), DART_OK);
}



TEST_F(DARTActiveMessagesTest, Overload)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }
  int ret;
  num_messages = 0;

  dart_amsgq_t q;
  ret = dart_amsg_openq(
      sizeof(testdata_t), 100, dash::Team::All().dart_id(), &q);
  ASSERT_EQ_U(ret, DART_OK);
  testdata_t data;
  data.sender  = dash::myid();
  data.payload = 0xDEADBEEF;
  if (dash::myid() > 0) {
    for (int i = 0; i < 200; i++) {
      do {
        ret = dart_amsg_trysend(0, q, &remote_fn, &data, sizeof(data));
        if (ret == DART_ERR_AGAIN) {
          dart_amsg_process(q);
        }
      } while(ret == DART_ERR_AGAIN);
      ASSERT_EQ_U(ret, DART_OK);
    }
  }
  ret = dart_amsg_process_blocking(q, dash::Team::All().dart_id());
  ASSERT_EQ_U(ret, DART_OK);
  if (dash::myid() == 0) {
    uint64_t _num_messages = num_messages;
    ASSERT_EQ_U(_num_messages, ((dash::size() - 1)*200));
  }

  ASSERT_EQ_U(dart_amsg_closeq(q), DART_OK);
}

TEST_F(DARTActiveMessagesTest, Broadcast)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units required");
  }

  using value_t = int;
  static_assert(
    dash::dart_datatype<value_t>::value != DART_TYPE_UNDEFINED,
    "Only basic DART types allowed!");

  dart_amsgq_t q;
  dart_gptr_t gptr;
  dash::dart_storage<value_t> ds(1);
  dart_ret_t ret = dart_amsg_openq(
      sizeof(gptr), 1000, DART_TEAM_ALL, &q);
  ASSERT_EQ_U(DART_OK, ret);

  if (dash::myid() == 0) {
    dart_memalloc(ds.nelem, ds.dtype, &gptr);
    value_t zero = static_cast<value_t>(0);
    dart_put_blocking(gptr, &zero, 1, ds.dtype, ds.dtype);
    dart_amsg_bcast(
      DART_TEAM_ALL, q, &remote_fn_increment_gptr<value_t>, &gptr, sizeof(gptr));
  }
  ret = dart_amsg_process_blocking(q, DART_TEAM_ALL);
  // dart_amsg_process_blocking guarantees that all messages have been exchanged
  // but not that they are all processed.
  dart_barrier(DART_TEAM_ALL);
  ASSERT_EQ_U(DART_OK, ret);

  if (dash::myid() == 0) {
    value_t expected = static_cast<value_t>(dash::size() - 1);
    value_t actual;
    dart_get_blocking(&actual, gptr, 1, ds.dtype, ds.dtype);
    ASSERT_EQ_U(expected, actual);
    dart_memfree(gptr);
  }

}
