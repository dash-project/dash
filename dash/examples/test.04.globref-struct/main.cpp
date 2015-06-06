/* 
 * globref/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

#include <stddef.h>

#define SIZE 10

struct MyStruct
{
  char   a;
  int    b;
  double c;
};

std::ostream& operator<<(std::ostream& os, const MyStruct& s)
{
  os << "a:'"<<s.a<<"' b:"<<s.b<<" c:"<<s.c;
  return os;
}

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<MyStruct> arr(SIZE);

  if( myid==0 ) {
    auto r1 = arr[0];
    auto r2 = arr[1];
    // !! r1.a = 'x'; can't overload "." operator

    auto r1_char = r1.member<char>(offsetof(MyStruct,a));
    r1_char = 'c';

    auto r2_char = r2.member<char>(&MyStruct::a);
    r2_char = 'd';

    arr[2].member<char>(&MyStruct::a)='e';
    
    // template param. automatically deduced
    arr[3].member(&MyStruct::a)='f';

    arr[4].member(&MyStruct::c)=22.3;
  }
  
  arr.barrier();

  if(myid==size-1) {
    for( int i=0; i<arr.size(); i++ ) {
      std::cout<<arr[i]<<std::endl;
    }

  }
  
  dash::finalize();
}
