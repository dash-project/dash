#include "TaskingTest.h"

#include <dash/tasks/Tasks.h>
#include <dash/Array.h>
#include <dash/Matrix.h>
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


TEST_F(TaskingTest, RemoteDepsCentral)
{
  int num_iter = 10;
  dash::Array<int> array(dash::size());
  array.local[0] = 0;
  dash::barrier();

  for (int i = 0; i < num_iter; i++) {
    // have all units except for 0 skip the first round
    if (dash::myid() == 0 || i > 0) {
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
    dash::tasks::async_barrier();
  }

  dash::tasks::complete();

  ASSERT_EQ_U(num_iter, array.local[0]);
}


TEST_F(TaskingTest, RemoteDepsSweep)
{
  int num_iter = 10;
  int num_elems_per_unit = 10;
  dash::Matrix<int, 2> matrix(dash::size(), num_elems_per_unit);
  dash::fill(matrix.begin(), matrix.end(), 0);
  dash::barrier();

  auto myid     = dash::myid();
  auto neighbor = (myid + dash::size() - 1) % dash::size();

  /**
   * multi-iteration wrap-around sweep from top to bottom
   *
   * Input:
   *  0  0  0  0  0  0  0  0  0  0
   *  0  0  0  0  0  0  0  0  0  0
   *  0  0  0  0  0  0  0  0  0  0
   *
   * After first iteration:
   *  1  1  1  1  1  1  1  1  1  1
   *  2  2  2  2  2  2  2  2  2  2
   *  3  3  3  3  3  3  3  3  3  3
   *  4  4  4  4  4  4  4  4  4  4
   *
   * After second iteration: (notice the wrap-around)
   *  5  5  5  5  5  5  5  5  5  5
   *  6  6  6  6  6  6  6  6  6  6
   *  7  7  7  7  7  7  7  7  7  7
   *  8  8  8  8  8  8  8  8  8  8
   */
  for (int i = 0; i < num_iter; i++) {
    for (size_t unit = 0; unit < dash::size(); ++unit) {
      if (unit == myid) {
        for (int j = 0; j < num_elems_per_unit; j++) {
          dash::tasks::async(
            [=, &matrix](){
              int val = matrix[neighbor][j];
              ASSERT_EQ_U(i*dash::size() + myid, val);
              matrix[myid][j] = val + 1;
            },
            dash::tasks::in (matrix[neighbor][j]),
            dash::tasks::out(matrix[    myid][j])
          );
        }
      }
      // traditionally, this would be a dash::barrier() to wait for
      // neighboring unit to complete
      dash::tasks::async_barrier();
    }
  }

  dash::tasks::complete();

  for (int i = 0; i < num_elems_per_unit; ++i) {
    ASSERT_EQ_U((num_iter-1)*dash::size() + myid + 1, matrix.local[0][i]);
  }
}


TEST_F(TaskingTest, RemoteDepsDoubleBufferStencil)
{
  int num_iter = 10;
  int num_elems_per_unit = 10;
  using MatrixT = dash::Matrix<int, 2>;
  auto myid     = dash::myid();
  auto up       = (myid + dash::size() - 1) % dash::size();
  auto down       = (myid + 1) % dash::size();

  MatrixT matrix1(dash::size(), num_elems_per_unit);
  MatrixT matrix2(dash::size(), num_elems_per_unit);
  dash::fill(matrix1.begin(), matrix1.end(), myid);
  dash::fill(matrix2.begin(), matrix2.end(), myid);
  dash::barrier();

  for (int i = 0; i < num_iter; ++i) {
    // double buffer swap
    MatrixT& oldmat = (i%2) ? matrix1 : matrix2;
    MatrixT& newmat = (i%2) ? matrix2 : matrix1;
    for (int j = 0; j < num_elems_per_unit; ++j) {
      dash::tasks::async(
        [=, &oldmat, &newmat](){
          // check correct values in 5pt stencil

          // center
          ASSERT_EQ_U(i + myid, static_cast<int>(oldmat(myid, j)));
          // up
          if (myid > 0) {
            ASSERT_EQ_U(i + up, static_cast<int>(oldmat(up, j)));
          }
          // down
          if (myid < dash::size()-1) {
            ASSERT_EQ_U(i + down, static_cast<int>(oldmat(down, j)));
          }
          // left
          if (j > 0) {
            ASSERT_EQ_U(i + myid, static_cast<int>(oldmat(myid, j)));
          }
          // right
          if (j < num_elems_per_unit-1) {
            ASSERT_EQ_U(i + myid, static_cast<int>(oldmat(myid, j)));
          }

          // update value (ignoring stencil values for simplicity)
          auto value = oldmat(myid, j) + 1;
          ASSERT_EQ_U(i + myid + 1, value);
          newmat(myid, j) = value;
        },
        dash::tasks::in(oldmat(myid, j)),
        dash::tasks::in(oldmat(  up, j)),
        dash::tasks::in(oldmat(down, j)),
        (j > 0) ? dash::tasks::in(oldmat(myid, j-1)) : dash::tasks::none(),
        (j < num_elems_per_unit-1) ? dash::tasks::in(oldmat(myid, j+1))
                                   : dash::tasks::none(),
        dash::tasks::out(newmat(myid, j))
      );
    }
    dash::tasks::async_barrier();
  }
  dash::tasks::complete();

  for (int i = 0; i < num_elems_per_unit; ++i) {
    ASSERT_EQ_U(num_iter+myid, matrix2.local(0, i));
  }
}
