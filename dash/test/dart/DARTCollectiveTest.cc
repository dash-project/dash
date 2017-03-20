
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
