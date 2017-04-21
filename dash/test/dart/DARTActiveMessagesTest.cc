
#include "DARTActiveMessagesTest.h"
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_active_messages.h>

typedef struct testdata {
  int       sender;
  uint64_t  payload;
} testdata_t;

static uint64_t num_messages;

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
  ASSERT_GT_U(testdata->sender, 0);
  ASSERT_EQ_U(0xDEADBEEF, testdata->payload);
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
  } while(ret == DART_PENDING);

  ASSERT_EQ_U(ret, DART_OK);

  // dart_amsg_process_blocking is sync'ing processes
  ret = dart_amsg_process_blocking(q);
  ASSERT_EQ_U(ret, DART_OK);

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
  if (dash::myid() > 0) {
    do {
      ret = dart_amsg_trysend(0, q, &remote_fn, &data, sizeof(data));
    } while(ret == DART_PENDING);
    ASSERT_EQ_U(ret, DART_OK);
  } else {
    while (num_messages < (dash::size() - 1)) {
      ret = dart_amsg_process(q);
      ASSERT_EQ_U(ret, DART_OK);
    }
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
      sizeof(testdata_t), 1000, dash::Team::All().dart_id(), &q);
  ASSERT_EQ_U(ret, DART_OK);
  testdata_t data;
  data.sender  = dash::myid();
  data.payload = 0xDEADBEEF;
  if (dash::myid() > 0) {
    for (int i = 0; i < 200; i++) {
      do {
        ret = dart_amsg_trysend(0, q, &remote_fn, &data, sizeof(data));
      } while(ret == DART_PENDING);
      ASSERT_EQ_U(ret, DART_OK);
    }
  } else {
    // dart_amsg_process_blocking is sync'ing processes
    while (num_messages < ((dash::size() - 1)*200)) {
      ret = dart_amsg_process(q);
      ASSERT_EQ_U(ret, DART_OK);
    }
  }

  ASSERT_EQ_U(dart_amsg_closeq(q), DART_OK);
}

