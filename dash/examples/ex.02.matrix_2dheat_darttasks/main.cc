/*
 * 2heat_dash_local.cc
 *
 *  Created on: Aug 11, 2016
 *      Author: joseph
 */



#include <iostream>
#include <vector>
#include <type_traits>

#include <libdash.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart.h>

//#define HAVE_OMP 1

#define STR(x) #x
#define STRINGIFY(x) STR(x)
#define CONCATENATE(X,Y) X Y

#if HAVE_OMP
#define OMP(_s) \
    _Pragma( STRINGIFY( CONCATENATE( omp, _s ) ) )
#else
#define OMP(_s)
#endif

static struct Parms {
  float cx; // diffusivity
  float cy; // diffusivity
  int   nx; // grid size in X
  int   ny; // grid size in Y
  int   num_steps; // number of simulation steps
  int block_size_x;
  //} params = { 0.1, 0.1, 5000, 30000, 100, 100 };
  //} params = { 0.1, 0.1, 20, 20, 1000, 5 };
} params = { 0.1, 0.1, 2000, 2000, 1000, 5 };


template<typename ElemT, typename IndexT=int>
class Matrix {
private:
  std::vector<ElemT> _m;
  IndexT _nx, _ny;
public:

  typedef ElemT  value_type;
  typedef IndexT index_type;

  Matrix(IndexT nx, IndexT ny) : _nx(nx), _ny(ny){
    std::cout << "Allocating matrix with " << nx << "x" << ny << " elements" << std::endl;
    this->_m.resize(nx*ny);
  }
  ElemT& operator()(IndexT x, IndexT y){ return _m[x*_ny + _ny];}
  const ElemT& operator() (IndexT x, IndexT y) const { return _m[x*_ny + _ny];}

  ElemT& operator()(IndexT n) { return _m[n]; }
  const ElemT& operator()(IndexT n) const { return _m[n]; }

  IndexT nx() const {return _nx;}
  IndexT ny() const {return _ny;}

};


template<typename T>
class DoubleBuffer {
private:
  T _db[2];
  int _first, _second;
public:
  template<typename ... Args>
  DoubleBuffer(Args&... args) : _db({{args...}, {args...}}), _first(0), _second(1) {}

  void rotate() {
    std::swap(_first, _second);
  }

  T& first()  { return _db[_first]; }
  T& second() { return _db[_second]; }
};

template<typename MatrixT>
static void initialize_dist(MatrixT& mat);

template<typename MatrixT>
static void update_local(MatrixT& matnew, MatrixT& matold);


template<typename MatrixT>
static void update_local_blocked(MatrixT& matnew, MatrixT& matold);

template<typename MatrixT>
static void print2d(MatrixT& m);

static size_t myid;

typedef dash::util::Timer<dash::util::TimeMeasure::Clock> Timer;

int main(int argc, char **argv)
{
  int concurrency;
  dash::init_thread(&argc, &argv, &concurrency);

  if (concurrency != dash::DASH_THREAD_MULTIPLE) {
    std::cout << "ERROR: No support for multiple concurrent threads detected!" << std::endl;
    dash::finalize();
    exit(1);
  }

  dart_tasking_init();

  size_t team_size = dash::Team::All().size();
  myid = dash::Team::All().myid();

  double sigma = 0.0001;

  Timer::Calibrate(0);

  // create a teamspec that is automatically balanced in 2 dimensions
  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();
  // create the double buffer with two dash matrix objects distributed among the units
  // TODO: check whether tiling can improve cache re-use
  // NOTE: for some reason we need to create SizeSpec and DistribitionSpec before calling the ctor
  using Matrix2D = dash::Matrix<double, 2, dash::default_index_t, dash::Pattern<2>>;
  dash::SizeSpec<2> sspec(params.nx, params.ny);
  dash::DistributionSpec<2> dspec(dash::BLOCKED, dash::BLOCKED);
  DoubleBuffer<Matrix2D> mats(
      sspec, dspec,
      dash::Team::All(),
      teamspec_2d);

  if (myid == 0) {
    std::cout << "Initialization" << std::endl;
  }

  initialize_dist(mats.first());
  dash::barrier();
  if (myid == 0) {
    std::cout << "Initialization done." << std::endl;
  }

  std::cout << std::fixed << std::right << std::setprecision(2);

  if (params.nx <= 20 && params.ny <= 20 && myid == 0) {
    print2d(mats.first());
  }

  dash::barrier();

  std::cout << "[" << myid << "] Local extent: " << mats.second().local.extent(0) << "x" << mats.second().local.extent(1) << std::endl;
  std::cout << "[" << myid << "] Local offset: " << mats.second().local.offset(0) << "x" << mats.second().local.offset(1) << std::endl;

  for (int i = 0; i < params.num_steps; i++) {
    if (myid == 0) {
      std::cout << "Iteration " << i << std::endl;
    }
    mats.rotate();
    update_local_blocked(mats.first(), mats.second());
    mats.first().barrier();

    // check for equilibrium
    if (static_cast<double>(mats.first()(1, 1))
        - static_cast<double>(mats.first()(0, 0)) < sigma) {
      std::cout << "[" << dash::myid() << "] Equilibrium reached after " << i << " iterations ("
          << static_cast<double>(mats.first()(1, 1)) << " ~ "
          << static_cast<double>(mats.first()(0, 0)) << ") "
          << std::endl;
      break;
    }
  }

  dash::barrier();

  if (params.nx <= 20 && params.ny <= 20 && myid == 1) {
    print2d(mats.first());
  }

  dart_tasking_fini();
  dash::finalize();

  return 0;
}


template<typename MatrixT>
static void initialize(MatrixT& mat)
{
  typename MatrixT::size_type i, j;
  for (i = 0; i < mat.extent(0); i++)
    for (j = 0; j < mat.extent(1); j++) {
      mat[i][j] = static_cast<typename MatrixT::value_type>(i * (mat.extent(0) - i - 1) * j * (mat.extent(1) - j - 1))/(4*mat.extent(0)*mat.extent(1));
    }
}


template<typename MatrixT>
static void initialize_dist(MatrixT& mat)
{
  typename MatrixT::size_type startx = mat.local.offset(0);
  typename MatrixT::size_type starty = mat.local.offset(1);
  typename MatrixT::size_type endx = mat.local.offset(0) + mat.local.extent(0);
  typename MatrixT::size_type endy = mat.local.offset(1) + mat.local.extent(1);
  typename MatrixT::size_type i, j;
  for (i = startx; i < endx; i++)
    for (j = starty; j < endy; j++) {
      mat[i][j] = static_cast<typename MatrixT::value_type>(i
          * (mat.extent(0) - i - 1) * j * (mat.extent(1) - j - 1))
          / (4 * mat.extent(0) * mat.extent(1));
    }
}


template<typename MatrixT>
static void update_local_block(MatrixT& matnew, MatrixT& matold,
  typename MatrixT::index_type startx, typename MatrixT::index_type endx)
{
  typename MatrixT::index_type starty;
  typename MatrixT::index_type nx;
  typename MatrixT::index_type ny;

  //  auto start = Timer::Now();


  auto lmatnew = matnew.local;
  auto lmatold = matold.local;
  starty = 1;
  nx = endx;
  ny = lmatnew.extent(1) - 1;

  float cx = params.cx;
  float cy = params.cy;

  typename MatrixT::index_type i, j;

  // handle remaining cells
  for (i = startx; i < nx; i++) {
    for (j = starty; j < ny; j++) {
      //      lmatnew[i][j] = lmatold[i][j] +
      //          // heat transfer in *X direction*
      //          cx * (lmatold[i+1][j] + lmatold[i-1][j] - 2*lmatold[i][j]) +
      //          // heat transfer in *Y direction*
      //          cy * (lmatold[i][j+1] + lmatold[i][j-1] - 2*lmatold[i][j]);

      double lmatoldij = lmatold(i, j);
      lmatnew(i, j) = lmatoldij +
          // heat transfer in *X direction*
          cx * (lmatold(i + 1, j) + lmatold(i - 1, j) - 2 * lmatoldij) +
          // heat transfer in *Y direction*
          cy * (lmatold(i, j + 1) + lmatold(i, j - 1) - 2 * lmatoldij);
    }
  }

}


/**
 * Performs an update of the boundaries using global coordinates
 * All arguments should be global coordinates.
 */
template<typename MatrixT>
static void update_global_boundary(MatrixT& matnew, MatrixT& matold, int startx, int nx, int starty, int ny)
{
  float cx = params.cx;
  float cy = params.cy;
  typename MatrixT::index_type endx = startx + nx;
  typename MatrixT::index_type endy = starty + ny;
  typename MatrixT::index_type i, j;

  // handle remaining cells
  for (i = startx; i < endx; i++) {
    for (j = starty; j < endy; j++) {
      matnew(i, j) = matold(i, j);
      // left cell
      if (j > 0)
        matnew(i, j) += cy * (matold(i, j-1) - matold(i, j));
      // right cell
      if (j < matnew.extent(1) -1)
        matnew(i, j) += cy * (matold(i, j+1) - matold(i, j));
      // upper cell
      if (i > 0)
        matnew(i, j) += cx * (matold(i-1, j) - matold(i, j));
      // lower cell
      if (i < matnew.extent(0) -1)
        matnew(i, j) += cx * (matold(i+1, j) - matold(i, j));
    }
  }
}

/**
 * C-style task invocation for global boundary update
 */
template<typename MatrixT>
struct update_boundaries_data {
  MatrixT& matnew;
  MatrixT& matold;
  update_boundaries_data(MatrixT& matnew, MatrixT& matold) : matnew(matnew), matold(matold) {}
};

template<typename MatrixT>
struct update_boundary_data {
  MatrixT& matnew;
  MatrixT& matold;
  int startx;
  int nx;
  int starty;
  int ny;
  update_boundary_data(MatrixT& matnew, MatrixT& matold, int startx, int nx, int starty, int ny)
  : matnew(matnew), matold(matold), startx(startx), nx(nx), starty(starty), ny(ny)
  {}
};

template<typename MatrixT>
static void update_boundary_taskfn(void *data)
{
  struct update_boundary_data<MatrixT> *task_data = (struct update_boundary_data<MatrixT>*)data;
  MatrixT& matnew = task_data->matnew;
  MatrixT& matold = task_data->matold;
  int startx      = task_data->startx;
  int nx          = task_data->nx;
  int starty      = task_data->starty;
  int ny          = task_data->ny;

  update_global_boundary(matnew, matold, startx, nx, starty, ny);
}

template<typename MatrixT>
static void update_boundaries_taskfn(void *data)
{
  /**
   * TODO: handle dependencies to locally updated blocks for eastern and western boundaries
   */
  struct update_boundaries_data<MatrixT> *task_data = (struct update_boundaries_data<MatrixT>*)data;
  MatrixT& matnew = task_data->matnew;
  MatrixT& matold = task_data->matold;
  // eastern boundary
  {
    typename MatrixT::index_type px = matnew.local.offset(0) + (matnew.local.extent(0) / 2);
    typename MatrixT::index_type py = matnew.local.offset(1) + matnew.local.extent(1);
    struct update_boundary_data<MatrixT> task_data(matnew, matold, matnew.local.offset(0) + 1,
        matnew.local.extent(0) - 1,
        matnew.local.offset(1) + matnew.local.extent(1) - 1, 1);

    int ndeps = 0;
    dart_task_dep_t *deps = malloc(sizeof(dart_task_dep_t) * (params.nx / params.block_size_x));

    if ((py + 1) < matnew.extent(1)) {
      // neighbor input dependency
      deps[ndeps].gptr = matold(px, py + 1).dart_gptr();
      deps[ndeps].type = DART_DEP_IN;
      ndeps++;
    }
    // local input dependencies
    // TODO!
    deps[ndeps].gptr = matold(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_IN;
    ndeps++;

    // output dependency
    deps[ndeps].gptr = matnew(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_OUT;
    ndeps++;
    dart_task_create(&update_boundary_taskfn<MatrixT>, (void*) &task_data, sizeof(task_data), deps, ndeps);
  }
  // western boundary
  {
    typename MatrixT::index_type px = matnew.local.offset(0) + (matnew.local.extent(0) / 2);
    typename MatrixT::index_type py = matnew.local.offset(1);
    struct update_boundary_data<MatrixT> task_data(matnew, matold, matnew.local.offset(0) + 1,
        matnew.local.extent(0) - 1, matnew.local.offset(1), 1);

    int ndeps = 0;
    dart_task_dep_t deps[3];

    if ((py - 1) > 0) {
      // neighbor input dependency
      deps[ndeps].gptr = matold(px, py - 1).dart_gptr();
      deps[ndeps].type = DART_DEP_IN;
      ndeps++;
    }
    // local input dependency
    deps[ndeps].gptr = matold(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_IN;
    ndeps++;

    // output dependency
    deps[ndeps].gptr = matnew(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_OUT;
    ndeps++;

    dart_task_create(&update_boundary_taskfn<MatrixT>, (void*) &task_data, sizeof(task_data), deps, ndeps);
  }
  // northern boundary
  {
    typename MatrixT::index_type px = matnew.local.offset(0);
    typename MatrixT::index_type py = matnew.local.offset(1) + (matnew.local.extent(1) / 2);
    struct update_boundary_data<MatrixT> task_data(matnew, matold, matnew.local.offset(0), 1,
        matnew.local.offset(1), matnew.local.extent(1));

    int ndeps = 0;
    dart_task_dep_t deps[3];

    if ((px - 1) > 0) {
      // neighbor input dependency
      deps[ndeps].gptr = matold(px - 1, py).dart_gptr();
      deps[ndeps].type = DART_DEP_IN;
      ndeps++;
    }
    // local input dependency
    deps[ndeps].gptr = matold(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_IN;
    ndeps++;

    // output dependency
    deps[ndeps].gptr = matnew(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_OUT;
    ndeps++;

    dart_task_create(&update_boundary_taskfn<MatrixT>, (void*) &task_data, sizeof(task_data), deps, ndeps);
  }
  // southern boundary
  {
    typename MatrixT::index_type px = matnew.local.offset(0) + matnew.local.extent(0);
    typename MatrixT::index_type py = matnew.local.offset(1) + (matnew.local.extent(1) / 2);
    struct update_boundary_data<MatrixT> task_data(matnew, matold,
        matnew.local.offset(0) + matnew.local.extent(0) - 1, 1,
        matnew.local.offset(1), matnew.local.extent(1));

    int ndeps = 0;
    dart_task_dep_t deps[3];

    if ((px + 1) > 0) {
      // neighbor input dependency
      deps[ndeps].gptr = matold(px + 1, py).dart_gptr();
      deps[ndeps].type = DART_DEP_IN;
      ndeps++;
    }
    // local input dependency
    deps[ndeps].gptr = matold(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_IN;
    ndeps++;

    // output dependency
    deps[ndeps].gptr = matnew(px, py).dart_gptr();
    deps[ndeps].type = DART_DEP_OUT;
    ndeps++;

    dart_task_create(&update_boundary_taskfn<MatrixT>, (void*) &task_data, sizeof(task_data), deps, ndeps);
  }
}


/**
 * C++ style invocation for local block update
 */
static void invoke_task(void *data)
{
  using FuncT = std::function<void()>;
  FuncT function(std::move(*static_cast<FuncT*>(data)));
  function();
}

template<typename MatrixT>
static void update_local_blocked(MatrixT& matnew, MatrixT& matold)
{
  /**
   * update boundaries
   */
  struct update_boundaries_data<MatrixT> task_data(matnew, matold);
  dart_task_create(&update_boundaries_taskfn<MatrixT>, (void*) &task_data, sizeof(task_data), NULL, 0);

  /**
   * update core blocks
   */
  typename MatrixT::index_type lastrow = matold.local.extent(0) - 1;
  typename MatrixT::index_type blocksize = params.block_size_x;
  for (typename MatrixT::index_type startx = 1; startx < matold.local.extent(0); startx += blocksize) {
    using FuncT = std::function<void()>;
    FuncT *task_fn =  new FuncT([=, &matnew, &matold]()->void
        {
          typename MatrixT::index_type endx;
          endx = startx + blocksize;
          if (endx > lastrow) {
            endx = lastrow;
          }
          update_local_block(matnew, matold, startx, endx);
        } );

    int ndeps = 0;
    dart_task_dep_t deps[4];

    deps[ndeps].gptr = matold.local(startx - 1, matnew.local.extent(1) / 2).dart_gptr();
    deps[ndeps].type = DART_DEP_IN;
    ndeps++;
    deps[ndeps].gptr = matold.local(startx + blocksize, matnew.local.extent(1) / 2).dart_gptr();
    deps[ndeps].type = DART_DEP_IN;
    ndeps++;
    deps[ndeps].gptr = matnew.local(startx, matnew.local.extent(1) / 2).dart_gptr();
    deps[ndeps].type = DART_DEP_OUT;
    ndeps++;
    deps[ndeps].gptr = matnew.local(startx + blocksize - 1, matnew.local.extent(1) / 2).dart_gptr();
    deps[ndeps].type = DART_DEP_OUT;
    ndeps++;

    // we cannot have dart copy the functor since  it would require deep copying :(
    dart_task_create(&invoke_task, (void*) task_fn, 0, deps, ndeps);
  }
  dart_task_complete();
}


// general algorithm taken from: http://www.cas.usf.edu/~cconnor/parallel/2dheat/2d_heat_equation.c
//void update(int start, int end, int ny, float *u1, float *u2)
//{
//  int ix, iy;
//  for (ix = start; ix <= end; ix++)
//    for (iy = 1; iy <= ny-2; iy++)
//      u2[ix*ny+iy] = u1[ix*ny+iy] +
//	params.cx * (u1[(ix+1)*ny+iy] +
//		    u1[(ix-1)*ny+iy] -
//		    2.0 * u1[ix*ny+iy]) +
//	params.cy * (u1[ix*ny+iy+1] +
//		    u1[ix*ny+iy-1] -
//		    2.0 * u1[ix*ny+iy]);
//}



template<typename MatrixT>
static void print2d(MatrixT & m)
{
  using ElemT = typename MatrixT::value_type;

  for ( auto i = 0; i < m.extent(0); ++i ) {
    for ( auto j = 0; j < m.extent(1); ++j ) {
      //      if ( std::is_same<ElemT, double>::value) {
      //        std::cout << std::fixed << std::right << std::setprecision(2);
      //      }
      std::cout << std::setw(6);
      std::cout << static_cast<ElemT>(m(i, j)) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
}


