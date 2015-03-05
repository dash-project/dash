
#include <map>
#include <iostream>
#include <libdash.h>

using namespace std;
using namespace dash;

void test_fwd_mapping(size_t size);
void test_rev_mapping(size_t size);

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  if( myid==size-1 ) {
    test_fwd_mapping(11);
    test_rev_mapping(11);
  }

#if 0
  if( myid==size-1 ) {
    dash::Pattern<1> p1(n, BLOCKED);

    /*
    dash::Team& t1 = dash::TeamAll.split(1);
    dash::Pattern p2(n, CYCLIC, t1);
    dash::Pattern p3(n, BLOCKCYCLIC(4), t1 );
    */
  }
#endif
  
  dash::finalize();
}

void test_fwd_mapping(size_t size) 
{
  Pattern<1> p1(size); // team and blocking implicit
  Pattern<1> p2(size, BLOCKED );
  Pattern<1> p3(size, CYCLIC );
  Pattern<1> p4(size, BLOCKCYCLIC(1) );
  Pattern<1> p5(size, BLOCKCYCLIC(2) );
  Pattern<1> p6(size, BLOCKCYCLIC(size) );

  //  Pattern p6(EXTENT(-1, size-1, BLOCKED) );

  fprintf(stderr, "------------------------------------------------------------------------- \n");
  fprintf(stderr, " *** This is a test with %lld units and index space of %lld elements *** \n",
	  p1.nunits(), p1.nelem() );
  fprintf(stderr, " index -> b=block-id (unit-id, elem-id)\n");
  fprintf(stderr, "       p1          p2          p3          p4          p5          p6\n");
  fprintf(stderr, "------------------------------------------------------------------------- \n");
  for( long long i=-4; i<size+4; i++ ) {
    fprintf(stderr, "%3lld -> "
	    "b=%lld (%2lld,%2lld) "
	    "b=%lld (%2lld,%2lld) "
	    "b=%lld (%2lld,%2lld) "
	    "b=%lld (%2lld,%2lld) "
	    "b=%lld (%2lld,%2lld) "
	    "b=%lld (%2lld,%2lld) \n",
	    i, 
	    1, p1.index_to_unit(i), p1.index_to_elem(i),
	    1, p2.index_to_unit(i), p2.index_to_elem(i),
	    1, p3.index_to_unit(i), p3.index_to_elem(i),
	    1, p4.index_to_unit(i), p4.index_to_elem(i),
	    1, p5.index_to_unit(i), p5.index_to_elem(i),
	    1, p6.index_to_unit(i), p6.index_to_elem(i)
	    );
    long long m = i%size;
    if( m<0 ) m+=size;

    if( m==size-1 ) 
      fprintf(stderr, "------------------------------------------------------------------------- \n");
  }
  fprintf(stderr, "------------------------------------------------------------------------- \n");
  fprintf(stderr, "\n");
}

void test_rev_mapping(size_t size) 
{
  int i, j;

  Pattern<1> p1(size); // team and blocking implicit
  Pattern<1> p2(size, BLOCKED );
  Pattern<1> p3(size, CYCLIC );
  Pattern<1> p4(size, BLOCKCYCLIC(1) );
  Pattern<1> p5(size, BLOCKCYCLIC(2) );
  Pattern<1> p6(size, BLOCKCYCLIC(size) );

  std::map<Pattern<1>*, std::string>  pattern;
  pattern[&p1] = "default";
  pattern[&p2] = "BLOCKED";
  pattern[&p3] = "CYCLIC";
  pattern[&p4] = "BLOCKCYCLIC(1)";
  pattern[&p5] = "BLOCKCYCLIC(2)";
  pattern[&p6] = "BLOCKCYCLIC(size)";

  for( auto& it : pattern ) {
    Pattern<1> *pat = it.first;
    fprintf(stderr, "%s:\n", it.second.c_str());
    
    for( i=0; i<pat->nunits(); i++ ) {
      fprintf(stderr, "Unit %3d: ", i);
      for( j=0; j<size; j++ ) {
	long long res = pat->unit_and_elem_to_index(i,j);
	if( res<0 ) break;
	fprintf(stderr, "%d ", res);
      }
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "max_elem_per_unit   : %d\n", pat->max_elem_per_unit());
    fprintf(stderr, "max_blocks_per_unit : %d\n", 1);
    fprintf(stderr, "\n");
  }
}
