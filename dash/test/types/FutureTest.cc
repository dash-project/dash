
#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "FutureTest.h"

#include <dash/Future.h>


TEST_F(FutureTest, DefaultCtor)
{
  dash::Future<int> fut;
  ASSERT_EQ_U(false, fut.valid());
}

TEST_F(FutureTest, ReadyVal)
{
  int value = 42;
  dash::Future<int> fut(value);
  ASSERT_EQ_U(true, fut.valid());
  ASSERT_EQ_U(true, fut.test());
  ASSERT_EQ_U(value, fut.get());
}

TEST_F(FutureTest, GetFunc)
{
  int value = 42;

  dash::Future<int> fut([=](){ return value; });

  ASSERT_EQ_U(true, fut.valid());
  ASSERT_EQ_U(true, fut.test());
  ASSERT_EQ_U(value, fut.get());
}

TEST_F(FutureTest, WaitGetFunc)
{
  int value = 42;

  dash::Future<int> fut([=](){ return value; });

  ASSERT_EQ_U(true, fut.valid());
  fut.wait();
  ASSERT_EQ_U(value, fut.get());
}


TEST_F(FutureTest, GetTestDestroyFunc)
{
  int value = 42;
  bool destructor_called = false;
  {
    dash::Future<int> fut(
      [=](){ return value; },
      [=](int *val){
        // the first call returns false
        static bool ready = false;
        bool result = ready;
        if (!ready) {
          ready = true;
        } else {
          *val = value;
        }
        return result;
      },
      [&](){
        destructor_called = true;
      });

    ASSERT_EQ_U(true, fut.valid());
    ASSERT_EQ_U(false, fut.test());
    // the second call to ready() should return true
    ASSERT_EQ_U(true, fut.test());
    ASSERT_EQ_U(value, fut.get());
  }
  ASSERT_EQ_U(true, destructor_called);
}


TEST_F(FutureTest, VoidFunc)
{
  bool destructor_called = false;
  bool get_called        = false;
  bool test_called       = false;
  {
    dash::Future<void> fut(
      [&](){ get_called  = true; },
      [&](){ test_called = true; return false;},
      [&](){
        destructor_called = true;
      });

    ASSERT_EQ_U(true, fut.valid());
    ASSERT_EQ_U(false, fut.test());
    fut.get();
  }
  ASSERT_EQ_U(true, get_called);
  ASSERT_EQ_U(true, test_called);
  ASSERT_EQ_U(true, destructor_called);
}
