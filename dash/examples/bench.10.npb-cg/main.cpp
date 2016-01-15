#include <random>
#include <iostream>
#include <cassert>
#include <unistd.h>

#include "../bench.h"

#define PAL_DASH
#include "../../include/dash/omp/PAL.h"


#define CLASS 'S'

#if CLASS=='S'
#define NA     1400
#define NONZER 7
#define SHIFT  10.0
#define NITER  15
#define RCOND  0.1
#define ZETAV  8.5971775078648
#endif

#if CLASS=='W'
#define NA     7000
#define NONZER 8
#define SHIFT  12.0
#define NITER  15
#define RCOND  0.1
#define ZETAV  10.362595087124
#endif

#if CLASS=='A'
#define NA     14000
#define NONZER 11
#define SHIFT  20.0
#define NITER  15
#define RCOND  0.1
#define ZETAV  17.130235054029
#endif

#if CLASS=='B'
#define NA     75000
#define NONZER 13
#define SHIFT  60.0
#define NITER  75
#define RCOND  0.1
#define ZETAV  22.712745482631
#endif

#if CLASS=='C'
#define NA     150000
#define NONZER 15
#define SHIFT  110.0
#define NITER  75
#define RCOND  0.1
#define ZETAV  28.973605592845
#endif

#if CLASS=='D'
#define NA     1500000
#define NONZER 21
#define SHIFT  500.0
#define NITER  100
#define RCOND  0.1
#define ZETAV  52.5145321058
#endif

#define CGITMAX  25
#define EPS      1.0e-10
#define NZ       NA*(NONZER+1)*(NONZER+1)+NA*(NONZER+2)
#define FIRSTROW 1
#define FIRSTCOL 1
#define LASTROW  NA
#define LASTCOL  NA


// Global Variables
PAL_SHARED_ARR_DEF(colidx, int, NZ+1);
PAL_SHARED_ARR_DEF(rowstr, int, NZ+1);

PAL_SHARED_ARR_DEF(iv, int, 2*NA+2);
PAL_SHARED_ARR_DEF(v, double, NA+2);

PAL_SHARED_ARR_DEF(arow, int, NZ+1);
PAL_SHARED_ARR_DEF(acol, int, NZ+1);
PAL_SHARED_ARR_DEF(aelt, double, NZ+1);

PAL_SHARED_ARR_DEF(a, double, NZ+1);
PAL_SHARED_ARR_DEF(p, double, NA+3);
PAL_SHARED_ARR_DEF(q, double, NA+3);
PAL_SHARED_ARR_DEF(r, double, NA+3);
PAL_SHARED_ARR_DEF(x, double, NA+3);
PAL_SHARED_ARR_DEF(z, double, NA+3);


#define MYDEBUG(cmd) cmd
//#define MYDEBUG(cmd)
#define square(x) ((x)*(x))


void alloc_arrays();
void makea();
void sparse(int nnza);
void sprnvc();
void vecset(int *nzv, int ival, double val);
void initx();
double findx();
double conj_grad();
double randlc();
double mflops(double total);
void print_header();
void print_result(double total);
void print_verify(double zeta);



int main(int argc, char *argv[]) {
	int i, j, k, it;
	double start, stop, total, zeta, norm;

	TIMESTAMP(start)
	
	PAL_INIT
	alloc_arrays();

	PAL_SEQUENTIAL_BEGIN
	print_header();
	PAL_SEQUENTIAL_END

	
	zeta = randlc(); // init not-really-random generator
	srand((unsigned int)time(NULL)); // init C random generator

	PAL_SEQUENTIAL_BEGIN
	makea(); // generate sparse matrix
	initx(); // set x to (1..1)
	PAL_SEQUENTIAL_END
	
	// do one run for free
	norm = conj_grad();

	PAL_SEQUENTIAL_BEGIN
	zeta = findx(); // calculate x and zeta
	initx(); // set x to (1..1)
	PAL_SEQUENTIAL_END
	
	TIMESTAMP(stop)
	PAL_SEQUENTIAL_BEGIN
	printf("Initialization time: %f\n\n", stop - start);
	printf("%6s %21s %17s\n", "it", "||x||", "zeta");
	printf("%6d %21.13e %17.13f\n", 0, norm, zeta);
	total = 0.0;
	PAL_SEQUENTIAL_END
	
	// main iteration for inverse power method
	for (it = 1; it <= NITER; it++) {
		TIMESTAMP(start)
		norm = conj_grad();
		TIMESTAMP(stop)
		PAL_SEQUENTIAL_BEGIN
		zeta = findx(); // calculate x and zeta
		total += stop - start;
		printf("%6d %21.13e %17.13f\n", it, norm, zeta);
		PAL_SEQUENTIAL_END
	}

	PAL_SEQUENTIAL_BEGIN
	print_result(total);
	print_verify(zeta);
	PAL_SEQUENTIAL_END

	PAL_FINALIZE
	return 0;
}


void alloc_arrays() {
	PAL_SHARED_ARR_ALLOC(colidx, int, NZ+1);
	PAL_SHARED_ARR_ALLOC(rowstr, int, NZ+1);

	PAL_SHARED_ARR_ALLOC(iv, int, 2*NA+2);
	PAL_SHARED_ARR_ALLOC(v, double, NA+2);

	PAL_SHARED_ARR_ALLOC(arow, int, NZ+1);
	PAL_SHARED_ARR_ALLOC(acol, int, NZ+1);
	PAL_SHARED_ARR_ALLOC(aelt, double, NZ+1);

	PAL_SHARED_ARR_ALLOC(a, double, NZ+1);
	PAL_SHARED_ARR_ALLOC(p, double, NA+3);
	PAL_SHARED_ARR_ALLOC(q, double, NA+3);
	PAL_SHARED_ARR_ALLOC(r, double, NA+3);
	PAL_SHARED_ARR_ALLOC(x, double, NA+3);
	PAL_SHARED_ARR_ALLOC(z, double, NA+3);
}


void makea() {
	int i, iouter, nzv, nnza, ivelt, ivelt1, irow, jcol;
	double size, ratio, scale;

	size = 1.0;
	ratio = pow(RCOND, 1.0 / (float)NA);
	nnza = 0;

	// Initialize colidx(n+1 .. 2n) to zero.
	// Used by sprnvc to mark nonzero positions
	for (i = 1; i <= NA; i++) {
		colidx[NA+i] = 0;
	}

	for (int iouter = 1; iouter <= NA; iouter++) {
		sprnvc();
		nzv = NONZER;
		vecset(&nzv, iouter, .5);
		for (ivelt = 1; ivelt <= nzv; ivelt++) {
			jcol = PAL_SHARED_ARR_RVAL(iv, ivelt, int);
			if (jcol >= FIRSTCOL && jcol <= LASTCOL) {
				scale = size * PAL_SHARED_ARR_RVAL(v, ivelt, double);
				for (ivelt1 = 1; ivelt1 <= nzv; ivelt1++) {
					irow = PAL_SHARED_ARR_RVAL(iv, ivelt1, int);
					if (irow >= FIRSTROW && irow <= LASTROW) {
						nnza += 1;
						assert(nnza <= NZ && "space exceeded in makea");
						PAL_SHARED_ARR_SET(acol, nnza, jcol);
						PAL_SHARED_ARR_SET(arow, nnza, irow);
						PAL_SHARED_ARR_SET(aelt, nnza, scale * 
							PAL_SHARED_ARR_RVAL(v, ivelt1, double));
					}
				}
			}
		}
		size = size * ratio;
	}

	// add the identity * rcond to the generated matrix to bound
	// the smallest eigenvalue from below by rcond
	for (i = FIRSTROW; i <= LASTROW; i++) {
		if (i >= FIRSTCOL && i <= LASTCOL) {
			iouter = NA + i;
			nnza += 1;
			assert(nnza <= NZ && "space exceeded in makea");
			PAL_SHARED_ARR_SET(acol, nnza, i);
			PAL_SHARED_ARR_SET(arow, nnza, i);
			PAL_SHARED_ARR_SET(aelt, nnza, RCOND - SHIFT);
		}
	}

	sparse(nnza);
}


void sparse(int nnza) {
	int i, j, jajp1, nza, k, nzrow, nrows;
	double xi;

	nrows = LASTROW - FIRSTROW + 1;
	// count the number of triples in each row
	for (j = 1; j <= NA; j++) {
		PAL_SHARED_ARR_SET(rowstr, j, 0);
		PAL_SHARED_ARR_SET(iv, j, 0);
	}
	PAL_SHARED_ARR_SET(rowstr, NA+1, 0);
	for (nza = 1; nza <= nnza; nza++) {
		j = (PAL_SHARED_ARR_RVAL(arow, nza, int) - FIRSTROW + 1) + 1;
		PAL_SHARED_ARR_ADD(rowstr, j, 1);
	}
	PAL_SHARED_ARR_SET(rowstr, 1, 1);
	for (j = 2; j <= nrows + 1; j++) {
		PAL_SHARED_ARR_ADD(rowstr, j, PAL_SHARED_ARR_RVAL(rowstr, j-1, int));
	}
	// rowstr(j) now is the location of the first nonzero of row j of a
	// do a bucket sort of the triples on the row index
	for (nza = 1; nza <= nnza; nza++) {
		j = PAL_SHARED_ARR_RVAL(arow, nza, int) - FIRSTROW + 1;
		k = PAL_SHARED_ARR_RVAL(rowstr, j, int);
		PAL_SHARED_ARR_SET(a, k, PAL_SHARED_ARR_RVAL(aelt, nza, double));
		PAL_SHARED_ARR_SET(colidx, k, PAL_SHARED_ARR_RVAL(acol, nza, int));
		PAL_SHARED_ARR_ADD(rowstr, j, 1);
	}
	// rowstr(j) now points to the first element of row j+1
	for (j = nrows - 1; j >= 0; j--) {
		PAL_SHARED_ARR_SET(rowstr, j+1, PAL_SHARED_ARR_RVAL(rowstr, j, int));
	}
	PAL_SHARED_ARR_SET(rowstr, 1, 1);
	// generate the actual output rows by adding elements
	nza = 0;
	for (i = 1; i <= NA; i++) {
		PAL_SHARED_ARR_SET(v, i, 0.0);
		PAL_SHARED_ARR_SET(iv, i, 0);
	}

	jajp1 = PAL_SHARED_ARR_RVAL(rowstr, 1, int);

	for (j = 1; j <= nrows; j++) {
		nzrow = 0;
		// loop over the jth row of a
		for (k = jajp1; k <= (PAL_SHARED_ARR_RVAL(rowstr, j+1, int) - 1); k++) {
			i = PAL_SHARED_ARR_RVAL(colidx, k, int);
			PAL_SHARED_ARR_ADD(v, i, PAL_SHARED_ARR_RVAL(a, k, double));
			if ((PAL_SHARED_ARR_RVAL(iv, i, int) == 0) && 
				(PAL_SHARED_ARR_RVAL(v, i, double) != 0)) {
				PAL_SHARED_ARR_SET(iv, i, 1);
				nzrow += 1;
				PAL_SHARED_ARR_SET(iv, NA+nzrow, i);
			}
		}
		// extract the nonzeros of this row
		for (k = 1; k <= nzrow; k++) {
			i = PAL_SHARED_ARR_RVAL(iv, NA+k, int);
			PAL_SHARED_ARR_SET(iv, i, 0);
			xi = PAL_SHARED_ARR_RVAL(v, i, double);
			PAL_SHARED_ARR_SET(v, i, 0);
			if (xi != 0) {
				nza += 1;
				PAL_SHARED_ARR_SET(a, nza, xi);
				PAL_SHARED_ARR_SET(colidx, nza, i);
			}
		}
		jajp1 = PAL_SHARED_ARR_RVAL(rowstr, j+1, int);
		PAL_SHARED_ARR_SET(rowstr, j+1, nza + PAL_SHARED_ARR_RVAL(rowstr, 1, int));
	}
}


void sprnvc() {
	double vecelt, vecloc;
	int nzrow = 0, nzv = 0, idx, ii, nn1 = 1;
	while (nn1 < NA) nn1 *= 2;

	while (nzv < NONZER) {
		vecelt = randlc(); // = (double)rand()/(double)RAND_MAX;
		vecloc = randlc();
		idx = (int)(vecloc * nn1) + 1;
		if (idx > NA) continue;
		// better than the last three lines but cannot be verified
		// idx = rand() % NA + 1;
		if (PAL_SHARED_ARR_RVAL(colidx, NA+idx, int) == 0) {
			PAL_SHARED_ARR_SET(colidx, NA+idx, 1);
			nzrow += 1;
			PAL_SHARED_ARR_SET(colidx, nzrow, idx);
			nzv += 1;
			PAL_SHARED_ARR_SET(v, nzv, vecelt);
			PAL_SHARED_ARR_SET(iv, nzv, idx);
		}
	}

	for (ii = 1; ii <= nzrow; ii++) {
		idx = PAL_SHARED_ARR_RVAL(colidx, ii, int);
		PAL_SHARED_ARR_SET(colidx, NA+idx, 0);
	}
}


void vecset(int *nzv, int ival, double val) {
	int set = 0;
	for (int k = 1; k <= *nzv; k++) {
		if (PAL_SHARED_ARR_RVAL(iv, k, int) == ival) {
			PAL_SHARED_ARR_SET(v, k, val);
			set = 1;
		}
	}
	if (!set) {
		*nzv += 1;
		PAL_SHARED_ARR_SET(v, *nzv, val);
		PAL_SHARED_ARR_SET(iv, *nzv, ival);
	}
}


void initx() {
	int i;
	for (i = 1; i <= NA + 1; i++) {
		x[i] = 1.0;
	}
	//PAL_PARALLEL_BEGIN()
	//PAL_FOR_WAIT_BEGIN(1, NA+1, 1, i, int)
	//	PAL_SHARED_ARR_SET(x, i, 1.0);
	//PAL_FOR_WAIT_END
	//PAL_PARALLEL_END
}


double findx() {
	int j;
	double zeta, tnorm1, tnorm2;
	// zeta = shift + 1/(x.z)
	tnorm1 = 0.0;
	tnorm2 = 0.0;
	for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
		tnorm1 += PAL_SHARED_ARR_RVAL(x, j, double) * 
			PAL_SHARED_ARR_RVAL(z, j, double);
		tnorm2 += square(PAL_SHARED_ARR_RVAL(z, j, double));
	}
	tnorm2 = 1.0 / sqrt(tnorm2);
	zeta = SHIFT + 1.0 / tnorm1;
	// normalize z to obtain x
	for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
		PAL_SHARED_ARR_SET(x, j, tnorm2 * PAL_SHARED_ARR_RVAL(z, j, double));
	}
	return zeta;
}


double conj_grad() {
	int i, j, k, cgit;
	double rnorm = 0.0, rho = 0.0, rho0 = 0.0;

	PAL_PARALLEL_BEGIN(shared(rho))
	PAL_FOR_WAIT_BEGIN(1, NA+1, 1, j, int)
	//for (j = 1; j <= NA + 1; j++) {
		PAL_SHARED_ARR_SET(q, j, 0.0);
		PAL_SHARED_ARR_SET(z, j, 0.0);
		PAL_SHARED_ARR_SET(r, j, PAL_SHARED_ARR_RVAL(x, j, double));
		PAL_SHARED_ARR_SET(p, j, PAL_SHARED_ARR_RVAL(x, j, double));
	//}
	PAL_FOR_WAIT_END
	//PAL_PARALLEL_END

	// rho = r.r
	PAL_FOR_REDUCE_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int, PAL_RED_OP_PLUS, rho, double)
	//for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
		rho += square(PAL_SHARED_ARR_RVAL(r, j, double));
	//}
	PAL_FOR_REDUCE_END
	PAL_PARALLEL_END

	//fprintf(stderr, "rho=%.15e\n", rho);

	// The conj grad iteration loop
	for (cgit = 1; cgit <= CGITMAX; cgit++) {
		double sum = 0.0;
		rho0 = rho;
		rho = 0.0;

		// q = A.p
		PAL_PARALLEL_BEGIN(shared(rho,rho0,sum))
		double alpha = 0.0, beta = 0.0;
		PAL_FOR_WAIT_BEGIN(1, LASTROW - FIRSTROW + 1, 1, j, int)
		//for (j = 1; j <= LASTROW - FIRSTROW + 1; j++) {
			double e = 0.0;
			for (k = PAL_SHARED_ARR_RVAL(rowstr, j, double); k <= PAL_SHARED_ARR_RVAL(rowstr, j+1, double) - 1; k++) {
				e += PAL_SHARED_ARR_RVAL(a, k, double) * PAL_SHARED_ARR_RVAL(p, PAL_SHARED_ARR_RVAL(colidx, k, int), double);
			}
			PAL_SHARED_ARR_SET(q, j, e);
		//}
		PAL_FOR_WAIT_END

		// obtain p.q
		PAL_FOR_REDUCE_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int, PAL_RED_OP_PLUS, sum, double)
		//for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
			sum += PAL_SHARED_ARR_RVAL(p, j, double) * PAL_SHARED_ARR_RVAL(q, j, double);
		//}
		PAL_FOR_REDUCE_END

		// obtain alpha = rho / (p.q)
		alpha = rho0 / sum;

		// obtain z = z + alpha * p  and  r = r - alpha * q
		PAL_FOR_WAIT_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int)
		//for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
			PAL_SHARED_ARR_ADD(z, j, alpha * PAL_SHARED_ARR_RVAL(p, j, double));
			PAL_SHARED_ARR_SET(r, j, PAL_SHARED_ARR_RVAL(r, j, double) - alpha * PAL_SHARED_ARR_RVAL(q, j, double));
		//}
		PAL_FOR_WAIT_END
		

		// rho = r.r
		PAL_FOR_REDUCE_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int, PAL_RED_OP_PLUS, rho, double)
		//for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
			rho += square(PAL_SHARED_ARR_RVAL(r, j, double));
		//}
		PAL_FOR_REDUCE_END

		beta = rho / rho0;

		// p = r + beta * p
		PAL_FOR_WAIT_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int)
		//for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
			PAL_SHARED_ARR_SET(p, j, PAL_SHARED_ARR_RVAL(r, j, double) + beta * PAL_SHARED_ARR_RVAL(p, j, double));
		//}
		PAL_FOR_WAIT_END
		PAL_PARALLEL_END
		
		//fprintf(stderr, "%d| rho=%.15e, sum=%.15e\n", PAL_THREAD_NUM, rho, sum);
	}

	PAL_PARALLEL_BEGIN(shared(rnorm))
	// compute residual norm explicitly:  ||r|| = ||x - A.z||
	// obtain A.z
	PAL_FOR_WAIT_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int)
	//for (j = 1; j <= LASTROW - FIRSTROW + 1; j++) {
		double e = 0.0;
		for (k = PAL_SHARED_ARR_RVAL(rowstr, j, int); k <= PAL_SHARED_ARR_RVAL(rowstr, j+1, int) - 1; k++) {
			e += PAL_SHARED_ARR_RVAL(a, k, double) * PAL_SHARED_ARR_RVAL(z, PAL_SHARED_ARR_RVAL(colidx, k, int), double);
		}
		PAL_SHARED_ARR_SET(r, j, e);
	//}
	PAL_FOR_WAIT_END
	//PAL_PARALLEL_END

	PAL_FOR_REDUCE_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int, PAL_RED_OP_PLUS, rnorm, double)
	//PAL_FOR_WAIT_BEGIN(1, LASTCOL - FIRSTCOL + 1, 1, j, int)
	//for (j = 1; j <= LASTCOL - FIRSTCOL + 1; j++) {
		rnorm += square(PAL_SHARED_ARR_RVAL(x, j, double) - PAL_SHARED_ARR_RVAL(r, j, double));
	//}
	//PAL_FOR_WAIT_END
	PAL_FOR_REDUCE_END

	PAL_PARALLEL_END

	return sqrt(rnorm);
}


double randlc() {
	double r23, r46, t23, t46, t1, t2, t3, t4, a1, a2, x1, x2, z;
	double a = 1220703125.0;
	static double x = 314159265.0;
	r23 = pow(0.5, 23.0);
	r46 = r23 * r23;
	t23 = pow(2.0, 23.0);
	t46 = t23 * t23;
	// break A into two parts such that A = 2^23 * A1 + A2.
	t1 = r23 * a;
	a1 = (int)t1;
	a2 = a - t23 * a1;
	// break X into two parts such that X = 2^23 * X1 + X2, compute
	// Z = A1 * X2 + A2 * X1  (mod 2^23), and then
	// X = 2^23 * Z + A2 * X2  (mod 2^46).
	t1 = r23 * x;
	x1 = (int)t1;
	x2 = x - t23 * x1;
	t1 = a1 * x2 + a2 * x1;
	t2 = (int)(r23 * t1);
	z = t1 - t23 * t2;
	t3 = t23 * z + a2 * x2;
	t4 = (int)(r46 * t3);
	x = t3 - t46 * t4;
	return r46 * x;
}


double mflops(double total) {
	double mflops = 0.0;
	if (total != 0.0) {
		mflops = (float)(2 * NA) * (3. + (float)(NONZER * (NONZER + 1)) + 25. * (5. + (float)(NONZER * (NONZER + 1))) + 3.);
		mflops *= NITER / (total * 1000000.0);
	}
	return mflops;
}


void print_header() {
	printf("---------------------------------------------------\n");
	printf("NAS Parallel Benchmarks (NPB3.2-OMP) - CG Benchmark\n");
	printf("---------------------------------------------------\n");
	printf("Class:      %6c\n", CLASS);
	printf("Size:       %6d\n", NA);
	printf("Iterations: %6d\n", NITER);
	printf("Threads:    %6d\n", PAL_MAX_THREADS);
	printf("---------------------------------------------------\n");
}


void print_result(double total) {
	printf("---------------------------------------------------\n");
	printf("Benchmark time:      %f\n", total);
	printf("MFLOPS:              %.3f\n", mflops(total));
}


void print_verify(double zeta) {
	double err;
	err = zeta - ZETAV;
	if (err < 0.0) err *= -1.0;
	if (err < EPS) {
		printf("Verification:        successful\n");
		printf("Zeta:                %16.13f\n", zeta);
		printf("Correct Zeta:        %16.13f\n", ZETAV);
		printf("Error:               %16.13f\n", err);
	} else {
		printf("Verification:        FAILED\n");
		printf("Zeta:                %16.13f\n", zeta);
		printf("Correct Zeta:        %16.13f\n", ZETAV);
		printf("Error:               %16.13f\n", err);
	}
	printf("---------------------------------------------------\n");
}
