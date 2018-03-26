#include <iostream>
#include <libdash.h>

using namespace std;

using pattern_t    = dash::Pattern<2>;
using size_spec_t  = dash::SizeSpec<2>;
using dist_spec_t  = dash::DistributionSpec<2>;
using team_spec_t  = dash::TeamSpec<2>;
using matrix_t     = dash::Matrix<
                       double, 2,
                       typename pattern_t::index_type,
                       pattern_t>;
using StencilT     = dash::StencilPoint<2>;
using StencilSpecT = dash::StencilSpec<StencilT,4>;
using GlobBoundSpecT   = dash::GlobalBoundarySpec<2>;
using HaloMatrixWrapperT = dash::HaloMatrixWrapper<matrix_t>;

using array_t      = dash::Array<double>;

#define DEBUG

void print_matrix(const matrix_t& matrix) {
  auto rows = matrix.extent(0);
  auto cols = matrix.extent(1);
  cout << fixed;
  cout.precision(4);
  cout << "Matrix:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c)
      cout << " " << setw(3) << (double) matrix[r][c];
    cout << endl;
  }
}

double calcEnergy(const matrix_t& m, array_t &a) {
  *a.lbegin() = std::accumulate( m.lbegin(), m.lend(), 0.0);
  a.barrier();

  if(dash::myid() == 0)
    return std::accumulate(a.begin(), a.end(), 0.0);
  else
    return 0.0;
}

template<typename BoundaryIterator, typename ValueT>
static void compute_boundary_range(
  BoundaryIterator begin,
  BoundaryIterator end,
  ValueT          *new_begin,
  ValueT           dx,
  ValueT           dy,
  ValueT           dt,
  ValueT           k)
{
  for (auto it = begin; it != end; ++it) {
    auto core = *it;
    double dtheta =
        (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
        (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
    new_begin[it.lpos()] = core + k * dtheta * dt;
  }
}

template<typename HaloT, typename HaloRegionT>
static dart_handle_t update_halo_async(HaloT& current_halo, const HaloRegionT* region)
{
  if (region->size() == 0) return DART_HANDLE_NULL;

  typedef typename HaloT::Element_t value_t;
  auto& halo_memory = current_halo.halo_memory();
  auto& halo_block  = current_halo.halo_block();
  // number of contiguous elements
  size_t num_elems_block         = 1;
  auto           rel_dim         = region->spec().relevant_dim();
  auto           level           = region->spec().level();
  auto*          off             = halo_memory.pos_at(region->index());
  auto           it              = region->begin();

  for(auto i = rel_dim - 1; i < 2; ++i)
    num_elems_block *= region->region().extent(i);

  size_t region_size        = region->size();
  auto   ds_num_elems_block = dash::dart_storage<value_t>(num_elems_block);
  size_t num_blocks         = region_size / num_elems_block;
  auto   it_dist            = it + num_elems_block;
  size_t stride =
    (num_blocks > 1) ? std::abs(it_dist.lpos().index - it.lpos().index)
                      : 1;
  dart_datatype_t dart_type = ds_num_elems_block.dtype;

  if (stride > 1) {
    auto            ds_stride = dash::dart_storage<value_t>(stride);
    dart_type_create_strided(ds_num_elems_block.dtype, ds_stride.nelem,
                              ds_num_elems_block.nelem, &dart_type);
  }
  dart_handle_t handle;
  dart_get_handle(off, it.dart_gptr(),
                  region_size, dart_type,
                  ds_num_elems_block.dtype,
                  &handle);

  if (stride > 1) {
    dart_type_destroy(&dart_type);
  }

  return handle;
}

static dart_ret_t wait_yield(dart_handle_t *handle, int num_handles)
{
  dart_ret_t ret;
  do {
    int flag;
    ret = dart_testall_local(handle, num_handles, &flag);
    if (flag || ret != DART_OK) break;
#pragma omp taskyield
  } while (1);

  return ret;
}

int main(int argc, char *argv[])
{

  if (argc < 3) {
    cerr << "Not enough arguments ./<prog> matrix_ext iterations" << endl;
    return 1;
  }
  using HaloBlockT = dash::HaloBlock<double,pattern_t>;
  using HaloMemT = dash::HaloMemory<HaloBlockT>;

  auto matrix_ext = std::atoi(argv[1]);
  auto iterations = std::atoi(argv[2]);

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto ranks = dash::size();

  typedef dash::util::Timer<
    dash::util::TimeMeasure::Clock
  > Timer;

  Timer::Calibrate(0);

  dist_spec_t dist(dash::BLOCKED, dash::BLOCKED);
  team_spec_t tspec(ranks,1);

  tspec.balance_extents();

  pattern_t pattern(size_spec_t(matrix_ext, matrix_ext), dist, tspec,
                    dash::Team::All());

  matrix_t matrix(pattern);
  matrix_t matrix2(pattern);

  if (myid == 0) {
    std::fill(matrix.lbegin(), matrix.lend(), 1.0);
    std::fill(matrix2.lbegin(), matrix2.lend(), 1.0);
  } else {
    std::fill(matrix.lbegin(), matrix.lend(), 0.0);
    std::fill(matrix2.lbegin(), matrix2.lend(), 0.0);
  }

  matrix.barrier();

#ifdef DEBUG
  if (myid == 0)
    print_matrix(matrix);
#endif

  StencilSpecT stencil_spec( StencilT(-1, 0), StencilT(1, 0), StencilT( 0, -1), StencilT(0, 1));

  GlobBoundSpecT bound_spec(dash::BoundaryProp::CYCLIC, dash::BoundaryProp::CYCLIC);

  HaloMatrixWrapperT halomat(matrix, bound_spec, stencil_spec);
  HaloMatrixWrapperT halomat2(matrix2, bound_spec, stencil_spec);

  auto stencil_op = halomat.stencil_operator(stencil_spec);
  auto stencil_op2 = halomat2.stencil_operator(stencil_spec);

  decltype(stencil_op)* current_op = &stencil_op;
  decltype(stencil_op2)* new_op = &stencil_op2;

  HaloMatrixWrapperT* current_halo = &halomat;
  HaloMatrixWrapperT* new_halo = &halomat2;



  double dx{ 1.0 };
  double dy{ 1.0 };
  double dt{ 0.05 };
  double k{ 1.0 };

  // initial total energy
  array_t energy(ranks);
  double initEnergy = calcEnergy(current_halo->matrix(), energy);

  const auto &lview = halomat.view_local();
  auto offset = lview.extent(1);
  long inner_start = offset + 1;
  long inner_end = lview.extent(0) * (offset - 1) - 1;

  current_halo->matrix().barrier();

  Timer timer;

#pragma omp parallel
#pragma omp single
{
  for (auto d = 0; d < iterations; ++d) {

    auto& current_matrix = current_halo->matrix();
    auto& new_matrix = new_halo->matrix();

    // Update Halos asynchroniously
    //current_halo->update_async();

    // manually start transfers of halos
    // halo region coordinate run from (0,0) at top left to (2,2) at bottom right
    // we need (0, 1), (1, 0), (1, 2), (2, 1) [top, left, right, bottom]
    auto& halo_memory = current_halo->halo_memory();
    auto& halo_block  = current_halo->halo_block();

    // optimized calculation of inner matrix elements
    auto* current_begin = current_matrix.lbegin();
    auto* new_begin = new_matrix.lbegin();

    // slow version
    auto it_end = current_op->iend();
    //for(auto it = current_op->ibegin(); it != it_end; ++it)
    auto& inner_view = halo_block.view_inner();
#pragma omp taskloop
    for (auto row = inner_view.offset(0); row < inner_view.extent(0); row++)
    {
      auto* up_lbegin   = current_matrix.local[row-1].lbegin();
      auto* down_lbegin = current_matrix.local[row+1].lbegin();
      auto* cur_lbegin  = current_matrix.local[row].lbegin();
      auto* new_lbegin  = new_matrix.local[row].lbegin();
      for (auto col = inner_view.offset(1); col < inner_view.extent(1); col++)
      {
        auto core   = cur_lbegin[col];
        auto dtheta = (up_lbegin[col] + down_lbegin[col] - 2 * core) / (dx * dx) +
                      (cur_lbegin[col-1] + cur_lbegin[col+1] - 2 * core) /(dy * dy);
        new_lbegin[col] = core + k * dtheta * dt;
      }
    }

    // Wait until all Halo updates ready
    //current_halo->wait();
    //dart_waitall(handles.data(), handles.size());

    // Calculation of boundary Halo elements

    const auto& bnd_elems = halo_block.boundary_elements();

    // upper boundary
    auto up_it      = current_op->bbegin();
    auto up_it_bend = up_it + bnd_elems[0].size();
#pragma omp task shared(halo_block)
{
    // we need to transfer halo region 1  here
    dart_handle_t handle[3] = {DART_HANDLE_NULL, DART_HANDLE_NULL, DART_HANDLE_NULL};
    handle[0] = update_halo_async(*current_halo, halo_block.halo_region(0));
    handle[1] = update_halo_async(*current_halo, halo_block.halo_region(1));
    handle[2] = update_halo_async(*current_halo, halo_block.halo_region(2));
    wait_yield(handle, 3);
    compute_boundary_range(up_it, up_it_bend, new_begin, dx, dy, dt, k);
}

    // lower boundary (follows upper boundary iteration)
    auto down_it      = up_it_bend;
    auto down_it_bend = down_it + bnd_elems[1].size();
#pragma omp task shared(halo_block)
{
    // we need to transfer halo region 7 here
    dart_handle_t handle[3] = {DART_HANDLE_NULL, DART_HANDLE_NULL, DART_HANDLE_NULL};
    //handle[0] = update_halo_async(*current_halo, halo_block.halo_region(6));
    handle[1] = update_halo_async(*current_halo, halo_block.halo_region(7));
    //handle[2] = update_halo_async(*current_halo, halo_block.halo_region(8));
    wait_yield(handle, 3);
    compute_boundary_range(down_it, down_it_bend, new_begin, dx, dy, dt, k);
}

    // left boundary (follows lower boundary iteration)
    auto left_it      = down_it_bend;
    auto left_it_bend = left_it + bnd_elems[2].size();
#pragma omp task shared(halo_block, current_halo)
{
    // TODO: we need to transfer halo region 3 here
    dart_handle_t handle;
    handle = update_halo_async(*current_halo, halo_block.halo_region(3));
    wait_yield(&handle, 1);
    compute_boundary_range(left_it, left_it_bend, new_begin, dx, dy, dt, k);
}

    // right boundary (follows lower boundary iteration)
    auto right_it      = left_it_bend;
    auto right_it_bend = right_it + bnd_elems[3].size();
#pragma omp task shared(halo_block)
{
    // TODO: we need to transfer halo region 5 here
    dart_handle_t handle;
    handle = update_halo_async(*current_halo, halo_block.halo_region(5));
    wait_yield(&handle, 1);
    compute_boundary_range(right_it, right_it_bend, new_begin, dx, dy, dt, k);
}

    // wait for all tasks to finish in this iteration
#pragma omp taskwait

    // swap current matrix and current halo matrix
    std::swap(current_halo, new_halo);
    std::swap(current_op, new_op);
    current_matrix.barrier();
  }

} // pragma omp single

  dash::barrier();
  auto elapsed = timer.Elapsed();

  // final total energy
  double endEnergy = calcEnergy(current_halo->matrix(), energy);

#ifdef DEBUG
  if (myid == 0)
    print_matrix(current_halo->matrix());
#endif

  // Output
  if (myid == 0) {
    cout << fixed;
    cout.precision(5);
    cout << "InitEnergy=" << initEnergy << endl;
    cout << "EndEnergy=" << endEnergy << endl;
    cout << "DiffEnergy=" << endEnergy - initEnergy << endl;
    cout << "Matrixspec: " << matrix_ext << " x " << matrix_ext << endl;
    cout << "Iterations: " << iterations << endl;
    cout << "Time: " << timer.Elapsed() / 1E6 << " s" << endl;
    cout.flush();
  }

  dash::finalize();

  return 0;
}
