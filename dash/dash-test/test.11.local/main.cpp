
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

    dash::Local_Ref<int, 2, 2> lref1 = mat1.local();
    dash::Local_Ref<double, 2, 2> lref2 = mat2.local();

    printf("1 local extent myid %d 1 %d 2 %d \n", myid, mat1.local().extent(0), mat1.local().extent(1));
    printf("2 local extent myid %d 1 %d 2 %d \n", myid, mat2.local().extent(0), mat2.local().extent(1));

    for( int i=0; i<mat1.local().extent(0); i++ ) {
//        if( !mat2.islocal(0, i) ) continue;
        for( int j=0; j<mat1.local().extent(1); j++ ) 
        {
//            if( !mat2.islocal(1, j) ) continue;
//            assert(mat1.islocal(0, i));
//           assert(mat1.islocal(1, j));
	    
			lref1(i, j)=myid;
			lref2(i, j)=100.0*((double)(i)+1)+10.0*((double)(j));

            fprintf(stdout, "I'm unit %03d, element %2d %2d is local to me\n",
                    myid, i, j);
        }
    }

    mat1.barrier();

    if( myid==size-1 ) {
        for( int i=0; i<mat1.extent(0); i++ ) {
	        for( int j=0; j<mat1.extent(1); j++ ) {
            int res = mat1(i, j);
            fprintf(stdout, "Owner of %2d %2d: %d \n", i, j, res);
    	    }
    	}
	}
    fflush(stdout);

    mat2.barrier();
    if( myid==size-1 ) {
        for( int i=0; i<mat2.extent(0); i++ ) {
	        for( int j=0; j<mat2.extent(1); j++ ) {
            double res = mat2(i, j);
            fprintf(stdout, "Value at %2d %2d: %f\n", i, j, res);
    	    }
    	}
	}

    fflush(stdout);  

    dash::finalize();
}
