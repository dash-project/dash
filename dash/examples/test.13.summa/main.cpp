
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include <list>

#include <dash/Pattern.h>
#include <dash/Matrix.h>
#include <dash/Team.h>

#if defined (__i386__)
static __inline__ unsigned long long GetCycleCount(void)
{
        unsigned long long int x;
        __asm__ volatile("rdtsc":"=A"(x));
        return x;
}
#elif defined (__x86_64__)
static __inline__ unsigned long long GetCycleCount(void)
{
        unsigned hi,lo;
        __asm__ volatile("rdtsc":"=a"(lo),"=d"(hi));
        return ((unsigned long long)lo)|(((unsigned long long)hi)<<32);
}
#endif

using namespace dash;

void Multiply(
  dash::MatrixRef<int, 2> A,
  dash::MatrixRef<int, 2> B,
  dash::MatrixRef<int, 2> C,
  unsigned m,
  unsigned n,
  unsigned p) {
	int i, j, k;
	for (i = 0; i < m; i++) {
		for (j = 0; j < p; j++) {
			int result = 0;
			for (k = 0; k < n; k++) {
				result = A[i][k] * B[k][j] + result;
			}
			C[i][j] = result;
		}
  }
}

void MatrixAdd(
  dash::LocalRef<int, 2> A,
  dash::LocalRef<int, 2> B,
  unsigned m,
  unsigned n) // the result remains in A 
{
	int i, j;
	for (i = 0; i < m; i++) {
		for (j = 0; j < n; j++) {
			A[i][j] = A[i][j] + B[i][j];
		}
  }
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  int myid = dash::myid();
  int size = dash::size();
  int nelem = 10;

	dash::TeamSpec<2> ts(2, 2);
	dash::SizeSpec<2> ss(nelem, nelem);
	dash::DistributionSpec<2> ds(dash::BLOCKED, dash::BLOCKED);

  dash::Pattern<2> pat(ss, ds, ts);

  dash::Matrix<int, 2>   matA(pat);
	dash::Matrix<int, 2> 	 matB(pat);
	dash::Matrix<int, 2> 	 matC(pat);

	dash::Matrix<int, 2> 	 tempC(pat);

  dash::LocalRef<int, 2> lrefa = matA.local();
  dash::LocalRef<int, 2> lrefb = matB.local();
  dash::LocalRef<int, 2> lrefc = matC.local();
  dash::LocalRef<int, 2> temp_lrefc = tempC.local();

	int p = 2; // 2*2=4 units
	int b = nelem/p;

	if(myid==0)
	{
		for(int i=0;i<nelem;i++)
			for(int j=0;j<nelem;j++)
				{
					matA[i][j] = i*10+j;
					matB[i][j] = j*10+i;
				}

		for(int i=0;i<nelem;i++)
		{
			for(int j=0;j<nelem;j++)
				{
					std::cout << matA[i][j] << " ";
				}
				std::cout << std::endl;	
		}

		for(int i=0;i<nelem;i++)
		{
			for(int j=0;j<nelem;j++)
				{
					std::cout << matB[i][j] << " ";
				}
				std::cout << std::endl;	
		} 
	}

	matA.barrier();
	
  unsigned long t1,t2;
  t1 = (unsigned long)GetCycleCount();

	for (int i=0;i<p;i++)
	{
		matA.barrier();		
		int rx = ts.coords(myid)[0];
		int ry = ts.coords(myid)[1];

		Multiply(
      matA.rows((rx)*b, b).cols((i)*b, b),
      matB.rows((i)*b, b).cols((ry)*b, b),
      tempC.rows((rx)*b, b).cols((ry)*b, b), b, b, b);
		MatrixAdd(lrefc, temp_lrefc, b, b);
	}

	matA.barrier();			
    t2 = (unsigned long)GetCycleCount();

	if(myid==0)
		for(int i=0;i<b;i++)
		{
			for(int j=0;j<b;j++)
				{
					std::cout << temp_lrefc[i][j] << " ";
				}
				std::cout << std::endl;		
		} 

	matA.barrier();			

	if(myid==1)
		for(int i=0;i<b;i++)
		{
			for(int j=0;j<b;j++)
				{
					std::cout << temp_lrefc[i][j] << " ";
				}
				std::cout << std::endl;		
		} 

	matA.barrier();			

	if(myid==2)
		for(int i=0;i<b;i++)
		{
			for(int j=0;j<b;j++)
				{
					std::cout << temp_lrefc[i][j] << " ";
				}
				std::cout << std::endl;		
		} 

	matA.barrier();			

	if(myid==3)
		for(int i=0;i<b;i++)
		{
			for(int j=0;j<b;j++)
				{
					printf("%5d ", temp_lrefc[i][j]);
				}
				std::cout << std::endl;		
		} 
	
	matA.barrier();	

	if(myid==0)
		for(int i=0;i<nelem;i++)
		{
			for(int j=0;j<nelem;j++)
				{
					printf("%5d ", (int)matC[i][j]);
				}
				std::cout << std::endl;		
		} 

    dash::finalize();
}
