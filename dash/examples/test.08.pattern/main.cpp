
#include <stdio.h>
#include <vector>
#include <list>

#include <dash/Pattern.h>
#include <dash/Team.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

	long long ext1 = 10;
	long long ext2 = 10;
	
	long long i=0;
	long long j=0;
	
	dash::TeamSpec<2> ts(2, 2);
	dash::SizeSpec<2> ss(ext1, ext2);
	dash::DistributionSpec<2> ds(dash::BLOCKED, dash::BLOCKCYCLIC(3));

	dash::Pattern<2> p1(ss, ds, ts);
	
if(myid==0)
{
	printf("Unit layout: \n");
			
	for(i=0;i<ext1;i++)
	{
		for(j=0;j<ext2;j++)
		{
			printf("%d ", p1.unit_at(i, j));
		}
		printf("\n");
	}		

	printf("Element layout\n");
	
	for(i=0;i<ext1;i++)
	{
		for(j=0;j<ext2;j++)
		{
			printf("%3d ", p1.at(i,j));
		}
		printf("\n");
	}	
}

  dash::finalize();



	
  //  Pattern<3> pat(2, 4, 6, BLOCKED, BLOCKED);
}
