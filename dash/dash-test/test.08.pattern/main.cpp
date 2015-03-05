
#include <stdio.h>
#include <vector>
#include <list>

#include "Pattern.h"
#include "Team.h"

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

	long long ext1 = 15;
	long long ext2 = 19;
	
	long long i=0;
	long long j=0;
	
	dash::Pattern<1> p1(19, dash::BLOCKCYCLIC(2));
	
if(myid==0)
{
	printf("Unit layout: \n");
			
//	for(i=0;i<ext1;i++)
//	{
		for(j=0;j<ext2;j++)
		{
			printf("%d ", p1.atunit(j));
		}
		printf("\n");
//	}		

	printf("Element layout\n");
	
	for(i=0;i<size;i++)
	{
                printf("unit %d \n", i);
		for(j=0;j<ext2;j++)
		{
			printf("%3d ", p1.unit_and_elem_to_index(i,j));
		}
		printf("\n");
	}	
}

  dash::finalize();



	
  //  Pattern<3> pat(2, 4, 6, BLOCKED, BLOCKED);
}
