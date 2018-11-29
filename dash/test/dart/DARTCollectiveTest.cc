
#include "DARTCollectiveTest.h"

#include <dash/dart/if/dart.h>


TEST_F(DARTCollectiveTest, Send_Recv) {
  // we need an even amount of participating units
  const int units = (_dash_size / 2) * 2;

  std::vector<int> data(units);
  for(int i = 0; i < units; ++i) {
    data[i] = i;
  }

  // only use non-excess units
  if(_dash_id < units) {
    // every other unit sends data to the next unit
    if(_dash_id % 2 == 0) {
      dart_unit_t send_to = _dash_id + 1;
      dart_send(&data[_dash_id], 1, DART_TYPE_INT, 0, send_to);
    } else {
      int recv;
      dart_unit_t recv_from = _dash_id - 1;
      dart_recv(&recv, 1, DART_TYPE_INT, 0, recv_from);
      ASSERT_EQ(recv, data[recv_from]);
    }
  }
}

TEST_F(DARTCollectiveTest, Sendrecv) {
  const int units = (_dash_size / 2) * 2;

  std::vector<int> data(units);
  for(int i = 0; i < units; ++i) {
    data[i] = i;
  }

  if(_dash_id < units) {
    int recv;
    dart_unit_t partner;
    if(_dash_id % 2 ==0) {
       partner = _dash_id + 1;
     } else {
       partner = _dash_id - 1;
     }
    // each pair of units send data to each other
    dart_sendrecv(&data[_dash_id], 1, DART_TYPE_INT, 0, partner,
        &recv, 1, DART_TYPE_INT, 0, partner);
    ASSERT_EQ(recv, data[partner]);
  }
}


TEST_F(DARTCollectiveTest, MinMax) {

  using elem_t = int;
  elem_t lmin = dash::myid();
  elem_t lmax = dash::myid() + dash::size();

  std::array<elem_t, 2> min_max_in{lmin, lmax};
  std::array<elem_t, 2> min_max_out{};
  dart_allreduce(
      &min_max_in,                        // send buffer
      &min_max_out,                       // receive buffer
      2,                                  // buffer size
      dash::dart_datatype<elem_t>::value,  // data type
      DART_OP_MINMAX,                     // operation
      dash::Team::All().dart_id()         // team
      );

  ASSERT_EQ_U(min_max_out[DART_OP_MINMAX_MAX], 2*dash::size()-1);
  ASSERT_EQ_U(min_max_out[DART_OP_MINMAX_MIN], 0);

}

TEST_F(DARTCollectiveTest, MinMaxInt64t) {

  using elem_t = int64_t;

  if (dash::size() != 4) {
    SKIP_TEST_MSG("Exactly 4 units required");
  }

  std::array<elem_t, 4> lmin = {-930, -989, -951, -909};
  std::array<elem_t, 4> lmax = {946, 933, 969, 882};

  std::array<elem_t, 2> min_max_in{lmin[dash::myid()], lmax[dash::myid()]};

  std::array<elem_t, 2> min_max_out{};
  dart_allreduce(
      &min_max_in,                        // send buffer
      &min_max_out,                       // receive buffer
      2,                                  // buffer size
      dash::dart_datatype<elem_t>::value,  // data type
      DART_OP_MINMAX,                     // operation
      dash::Team::All().dart_id()         // team
      );

  LOG_MESSAGE("global min: %ld, global max: %ld", min_max_out[DART_OP_MINMAX_MIN], min_max_out[DART_OP_MINMAX_MAX]);

  ASSERT_EQ_U(min_max_out[DART_OP_MINMAX_MAX], *std::max_element(std::begin(lmax), std::end(lmax)));
  ASSERT_EQ_U(min_max_out[DART_OP_MINMAX_MIN], *std::min_element(std::begin(lmin), std::end(lmin)));

}

template<typename T>
static void reduce_max_fn(
  const void   *invec_,
        void   *inoutvec_,
        size_t  len,
        void   *user_data)
{
  const T *cutoff = static_cast<T*>(user_data);
  const auto *invec    = static_cast<const T *>(invec_);
  auto *      inoutvec = static_cast<T *>(inoutvec_);
  for (size_t i = 0; i < len; ++i) {
    if (inoutvec[i] > *cutoff) {
      inoutvec[i] = *cutoff;
    }
    if (invec[i] > inoutvec[i] && invec[i] <= *cutoff) {
      inoutvec[i] = invec[i];
    }
  }
}

TEST_F(DARTCollectiveTest, CustomReduction) {

  using elem_t = int;
  elem_t value = dash::myid();
  elem_t max;

  dart_operation_t new_op;
  elem_t cutoff = dash::size() / 2;
  ASSERT_EQ_U(
    DART_OK,
    dart_op_create(
      &reduce_max_fn<elem_t>, &cutoff, true,
      dash::dart_datatype<elem_t>::value, false, &new_op)
  );
  ASSERT_NE_U(new_op, DART_OP_UNDEFINED);
  ASSERT_EQ_U(DART_OK,
    dart_allreduce(
      &value,                               // send buffer
      &max,                                 // receive buffer
      1,                                    // buffer size
      dash::dart_datatype<elem_t>::value,   // data type
      new_op,                               // operation
      dash::Team::All().dart_id()           // team
      ));

  ASSERT_EQ_U(dash::size() / 2, max);

  dart_op_destroy(&new_op);
}

template<typename T>
struct value_at{
  T value{};
  dash::global_unit_t unit;
};

template<typename T>
static void max_value_at_fn(
  const void   *invec_,
        void   *inoutvec_,
        size_t  len,
        void   *)
{
  using value_at_t = struct value_at<T>;
  const auto *invec    = static_cast<const value_at_t *>(invec_);
  auto *      inoutvec = static_cast<value_at_t *>(inoutvec_);
  ASSERT_EQ_U(1, len);
  if (invec->value > inoutvec->value) {
    inoutvec->value = invec->value;
    inoutvec->unit  = invec->unit;
  }
}

TEST_F(DARTCollectiveTest, MaxElementAt) {

  using elem_t = int;
  using value_at_t = struct value_at<elem_t>;
  elem_t value = (dash::size()*10+dash::myid());

  dart_datatype_t new_type;
  dart_type_create_custom(sizeof(value_at_t), &new_type);

  dart_operation_t new_op;
  ASSERT_EQ_U(
      DART_OK,
      dart_op_create(
          &max_value_at_fn<elem_t>, nullptr, true, new_type, false, &new_op));
  value_at_t lmax = {value, dash::myid()};
  value_at_t gmax;
  ASSERT_NE_U(new_op, DART_OP_UNDEFINED);
  ASSERT_EQ_U(DART_OK,
    dart_allreduce(
      &lmax,                                // send buffer
      &gmax,                                // receive buffer
      1,                                    // buffer size
      new_type,                             // data type
      new_op,                               // operation
      dash::Team::All().dart_id()           // team
      ));

  ASSERT_EQ_U(dash::size()*10+(dash::size()-1), gmax.value);
  ASSERT_EQ_U((dash::size()-1), gmax.unit);

  dart_type_destroy(&new_type);
  dart_op_destroy(&new_op);

}
