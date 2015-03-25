
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
    int nelem = 11;

    dash::Pattern<2> pat(nelem, nelem);

    dash::Matrix<int, 2>     mat1(pat);
    dash::Matrix<double, 2>  mat2(pat);

    for( int i=0; i<nelem; i++ ) {
        if( !mat2.islocal(0, i) ) continue;
        for (int j=0; j<nelem; j++)
        {
            if( !mat2.islocal(1, j) ) continue;
            assert(mat1.islocal(0, i));
            assert(mat1.islocal(1, j));
	    printf("myid %d at %d %d \n", myid, i, j);
            mat1[i][j]=myid;
	    printf("after myid %d at %d %d \n", myid, i, j);
            mat2[i][j]=10.0*((double)(i)+1);
            fprintf(stdout, "I'm unit %03d, element %03d is local to me\n",
                    myid, i);
        }
    }

    mat1.barrier();

/*
    if( myid==size-1 ) {
        for( int i=0; i<arr1.size(); i++ ) {
            int res = arr1[i];
            fprintf(stdout, "Owner of %d: %d at %d atunit %d max %d nelem %d  \n", i, res, pat.index_to_elem(i), arr1.pattern().index_to_unit(i), arr1.pattern().max_elem_per_unit(), arr1.pattern().nelem());
        }
    }
    fflush(stdout);

    arr2.barrier();
    if( myid==size-1 ) {
        for( int i=0; i<arr2.size(); i++ ) {
            double res = arr2[i];
            fprintf(stdout, "Value at %d: %f\n", i, res);
        }
    }
    fflush(stdout); */

    dash::finalize();
}
