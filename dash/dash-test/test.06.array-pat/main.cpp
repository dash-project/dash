/* 
 * array-pat/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <map>
#include <iostream>
#include <libdash.h>

using namespace std;
using namespace dash;

void test_pattern(size_t size);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  test_pattern(1000);

  dash::finalize();
}

void test_pattern(size_t size) 
{
  Pattern1D p1(size); // team and blocking implicit
  Pattern1D p2(size, BLOCKED );
  Pattern1D p3(size, CYCLIC );
  Pattern1D p4(size, BLOCKCYCLIC(1) );
  Pattern1D p5(size, BLOCKCYCLIC(2) );
  Pattern1D p6(size, BLOCKCYCLIC(size) );

  dash::Array<int> a1(p1);
  dash::Array<int> a2(p2);
  dash::Array<int> a3(p3);
  dash::Array<int> a4(p4);
  dash::Array<int> a5(p5);
  dash::Array<int> a6(p6);

  assert(a1.size()==size);
  assert(a2.size()==size);
  assert(a3.size()==size);
  assert(a4.size()==size);
  assert(a5.size()==size);
  assert(a6.size()==size);

  auto  myid = dash::myid();

  if(dash::myid()==0) {
    for(auto i=0; i<size; i++ ) {
      a1[i]=i; a2[i]=i; a3[i]=i; a4[i]=i; a5[i]=i; a6[i]=i;
    }
  }

  a1.barrier();
  a2.barrier();
  a3.barrier();
  a4.barrier();
  a5.barrier();
  a6.barrier();

  if(dash::myid()!=0) {
    for(auto i=0; i<size; i++ ) {
      assert(a1[i]==i);
      assert(a2[i]==i);
      assert(a3[i]==i);
      assert(a4[i]==i);
      assert(a5[i]==i);
      assert(a6[i]==i);
    }
  }
}
