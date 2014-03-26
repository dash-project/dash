
#include <stdio.h>
#include <iostream>
#include <libdash.h>

using namespace std;

typedef struct some_struct
{
  int a;
  char b;
  double c;
} ss;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();
  
  cout<<"Hello world from unit "<<myid<<" of "<<size
      <<endl;

  dart_gptr_t gptr;

  dart_team_memalloc_aligned(DART_TEAM_ALL, 
			     100*sizeof(ss), &gptr);


  dash::SymmetricAlignedAccess<ss> acc(DART_TEAM_ALL,
					gptr,
					100,
					0 );

  if( myid==1 ) {
    for( int i=0; i<100*size; i++ ) {
      ss val;
      val.c=(double)myid;
      acc.put_value(val);
      acc.increment();
    }
  }
  
  dart_barrier(DART_TEAM_ALL);

  if( myid==0 ) {
    for( int i=0; i<100*size; i++ ) {
      ss val;
      acc.get_value(&val);
      fprintf(stderr, "i: %d Read %5.2f\n", i, val.c);

      acc.increment();

    }
  }

  dash::finalize();
}

