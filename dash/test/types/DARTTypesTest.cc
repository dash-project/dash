
#include <dash/Types.h>
#include <dash/algorithm/Operation.h>

#include "DARTTypesTest.h"

#include <array>


TEST_F(DARTTypesTest, DARTTypeConversions)
{
  typedef std::array<int, 4> undef_t;

  static_assert(dash::dart_datatype<unsigned char>::value == DART_TYPE_BYTE,
                "conversion dart_datatype<unsigned char> failed");
  static_assert(dash::dart_datatype<int>::value == DART_TYPE_INT,
                "conversion dart_datatype<int> failed");
  static_assert(dash::dart_datatype<double>::value == DART_TYPE_DOUBLE,
                "conversion dart_datatype<int> failed");
  static_assert(dash::dart_datatype<undef_t>::value == DART_TYPE_UNDEFINED,
                "conversion dart_datatype<int> failed");
}

TEST_F(DARTTypesTest, DARTPunnedTypeConversions)
{
  typedef struct {
    char c1;
    char c2;
    char c3;
    char c4;
  } size4_t;

  typedef struct {
    char data[8];
  } size8_t;

  static_assert(dash::dart_punned_datatype<unsigned char>::value
                  == DART_TYPE_BYTE,
                "conversion dart_datatype<unsigned char> failed");
  static_assert(dash::dart_punned_datatype<int>::value == DART_TYPE_INT,
                "conversion dart_punned_datatype<int> failed");
  static_assert(dash::dart_punned_datatype<double>::value == DART_TYPE_DOUBLE,
                "conversion dart_punned_datatype<double> failed");
  static_assert(dash::dart_punned_datatype<size4_t>::value == DART_TYPE_INT,
                "conversion dart_punned_datatype<size4_t> failed");
  static_assert(dash::dart_punned_datatype<size8_t>::value
                  == DART_TYPE_LONGLONG,
                "conversion dart_punned_datatype<size8_t> failed");
}

TEST_F(DARTTypesTest, DARTOperation)
{
  typedef struct {
    int x, y;
  } point_t;
  auto lambda = [](int x, int y){ return x+y; };
  static_assert(
    dash::internal::dart_reduce_operation<dash::plus<int>>::value
      == DART_OP_SUM,
    "dash::plus<int> should yield DART_OP_SUM");
  static_assert(
    dash::internal::dart_reduce_operation<dash::plus<point_t>>::value
      == DART_OP_UNDEFINED,
    "dash::plus<point_t> should yield DART_OP_UNDEFINED");
  static_assert(
    dash::internal::dart_reduce_operation<decltype(lambda)>::value
      == DART_OP_UNDEFINED,
    "dash::plus<decltype(lambda)> should yield DART_OP_UNDEFINED");
}
