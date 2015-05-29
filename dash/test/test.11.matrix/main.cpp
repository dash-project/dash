
#include <stdio.h>
#include <vector>
#include <list>

#include "Pattern.h"
#include "Matrix.h"

#include "Team.h"

using namespace std;

int main(int argc, char* argv[])
{
    dash::init(&argc, &argv);

    int myid = dash::myid();
    int size = dash::size();
    int nelem = 5;

    dash::Pattern<2> pat(nelem, nelem);

    dash::Matrix<int, 2>     mat1(pat);
    dash::Matrix<double, 2>  mat2(pat);

    for( int i=0; i<nelem; i++ ) {
        if( !mat2.isLocal(0, i) ) continue;
        for (int j=0; j<nelem; j++)
        {
            if( !mat2.isLocal(1, j) ) continue;
            assert(mat1.isLocal(0, i));
            assert(mat1.isLocal(1, j));
	    
            mat1.at(i,j)=myid;

            mat2.at(i,j)=100.0*((double)(i)+1)+10.0*((double)(j));
            fprintf(stdout, "I'm unit %03d, element %2d %2d is local to me\n",
                    myid, i, j);
        }
    }

    mat1.barrier();

    if( myid==size-1 ) {
        for( int i=0; i<mat1.extent(0); i++ ) {
	        for( int j=0; j<mat1.extent(1); j++ ) {
            int res = mat1.at(i,j);
            fprintf(stdout, "Owner of %2d %2d: %d \n", i, j, res);
    	    }
    	}
	}
    fflush(stdout);

    mat2.barrier();
    if( myid==size-1 ) {
        for( int i=0; i<mat2.extent(0); i++ ) {
	        for( int j=0; j<mat2.extent(1); j++ ) {
            double res = mat2.at(i,j);
            fprintf(stdout, "Value at %2d %2d: %f\n", i, j, res);
    	    }
    	}
	}

    fflush(stdout);  

    mat2.barrier();

    int nelem2 = 4;

	dash::TeamSpec<2> ts(2, 2);
	dash::SizeSpec<2> ss(nelem2, nelem2);
	dash::DistributionSpec<2> ds(dash::BLOCKED, dash::BLOCKED);

    dash::Pattern<2> pat2(ss, ds, ts);

    dash::Matrix<int, 2>     matA(pat2);

    cout<<matA.extent(0)<<" "<<matA.extent(1)<<endl;
	if(myid==0)
	{
		for(int i=0;i<nelem2;i++)
			for(int j=0;j<nelem2;j++)
				{
				  matA(i,j)= 10*i+j;
				  //matA[i][j] = 10*i+j;
				}

		for(int i=0;i<nelem2;i++)
		{
			for(int j=0;j<nelem2;j++)
				{
				  cout<<matA(i,j)<<" ";
				  //cout << matA[i][j] << " ";
				}
				cout << endl;	
		}
	}

    dash::finalize();
}
