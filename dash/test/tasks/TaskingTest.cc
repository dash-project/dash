#include "TaskingTest.h"

#include <dash/tasks/Tasks.h>
#include <dash/Array.h>
#include <dash/algorithm/Fill.h>

TEST_F(TaskingTest, SimpleTasks)
{
  // simple test without dependencies
  constexpr int num_iter = 100;
  int value = 0;

  for (int i = 0; i < num_iter; ++i) {
    dash::tasks::async([&](){
      ++value;
    });
  }

  dash::tasks::complete();

  ASSERT_EQ_U(num_iter, value);
}


TEST_F(TaskingTest, SimpleTasksReturn)
{
  // simple test with tasks returning values and direct task dependencies

  constexpr int num_iter = 10;
  int value = 0;
  std::vector<dash::tasks::TaskHandle<int>> handles;
  handles.reserve(num_iter);
  handles.push_back(
    dash::tasks::async_handle([&](){
      return value++;
    })
  );

  for (int i = 1; i < num_iter; ++i) {
    handles.push_back(
      dash::tasks::async_handle([&](){
        return value++;
      },
      dash::tasks::direct(handles[i-1])
    ));
  }

  ASSERT_EQ_U(handles.size(), num_iter);

  for (int i = 0; i < num_iter; ++i) {
    auto& handle = handles[i];
    if (!handle.test()) {
      handle.wait();
    }
    ASSERT_EQ_U(handle.get(), i);
  }

  ASSERT_EQ_U(num_iter, value);

}


TEST_F(TaskingTest, LocalDeps)
{
  constexpr int num_iter = 10;
  dash::Array<int> array(dash::size());
  dash::fill(array.begin(), array.end(), 0);
  dash::barrier();

  for (int i = 0; i < num_iter; ++i) {

    // first task to read the value
    dash::tasks::async(
      [&array, i](){
        ASSERT_EQ_U(i, static_cast<int>(array[dash::myid()]));
      },
      dash::tasks::in(array[dash::myid()])
    );

    // first task to read the value
    dash::tasks::async(
      [&array, i](){
        ASSERT_EQ_U(i, static_cast<int>(array[dash::myid()]));
        ++array[dash::myid()];
      },
      // we can also use local pointer instead of gptr
      dash::tasks::out(&array.local[0])
      //dash::tasks::out(array[dash::myid()])
    );
  }

  dash::tasks::complete();

  ASSERT_EQ_U(num_iter, array.local[0]);
}


TEST_F(TaskingTest, RemoteCentralDeps)
{
  int i;
  int num_iter = 10;
  dash::Array<int> array(dash::size());
  array.local[0] = 0;
  dash::barrier();

  for (i = 1; i <= num_iter; i++) {
    // have all units except for 0 skip the first round
    if (dash::myid() == 0 || i > 1) {
      dash::tasks::async(
        [&array, i](){
          int val = array[0];
          ASSERT_EQ_U(i, val);
          array[dash::myid()] = val + 1;
        },
        dash::tasks::in(array[0]),
        dash::tasks::out(array[dash::myid()])
      );
    }
  }

  dash::tasks::complete();

  ASSERT_EQ_U(num_iter, array.local[0]);
}
