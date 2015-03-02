
#include <stdio.h>
#include <vector>
#include <list>

#include "Pattern.h"
#include "Team.h"

using namespace std;

int main(int argc, char* argv[])
{
	long long ext1 = 15;
	long long ext2 = 19;
	
	long long i=0;
	long long j=0;
	
	dash::Pattern<2> p1(ext1, ext2);
	
	printf("Unit layout: \n");
			
	for(i=0;i<ext1;i++)
	{
		for(j=0;j<ext2;j++)
		{
			printf("%d ", p1.atunit(i, j));
		}
		printf("\n");
	}		

	printf("Element layout\n");
	
	for(i=0;i<ext1;i++)
	{
		for(j=0;j<ext2;j++)
		{
			printf("%3d ", p1.at(i, j));
		}
		printf("\n");
	}	
	
  //  Pattern<3> pat(2, 4, 6, BLOCKED, BLOCKED);
}
