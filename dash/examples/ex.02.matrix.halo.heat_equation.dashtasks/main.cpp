#include <iostream>
#include <libdash.h>

//#define DEBUG

using namespace std;

using pattern_t    = dash::Pattern<2>;
using size_spec_t  = dash::SizeSpec<2>;
using dist_spec_t  = dash::DistributionSpec<2>;
using team_spec_t  = dash::TeamSpec<2>;
using matrix_t     = dash::Matrix<
                       double, 2,
                       typename pattern_t::index_type,
                       pattern_t>;
using StencilT     = dash::halo::StencilPoint<2>;
using StencilSpecT = dash::halo::StencilSpec<StencilT,4>;
using GlobBoundSpecT   = dash::halo::GlobalBoundarySpec<2>;
using HaloMatrixWrapperT = dash::halo::HaloMatrixWrapper<matrix_t>;

using array_t      = dash::Array<double>;

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

int main(int argc, char *argv[])
{

  if (argc < 3) {
    cerr << "Not enough arguments ./<prog> matrix_ext iterations" << endl;
    return 1;
  }
  auto matrix_ext = std::atoi(argv[1]);
  auto iterations = std::atoi(argv[2]);

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto ranks = dash::size();

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

  GlobBoundSpecT bound_spec(dash::halo::BoundaryProp::CYCLIC, dash::halo::BoundaryProp::CYCLIC);

  HaloMatrixWrapperT halomat(matrix, bound_spec, stencil_spec);
  HaloMatrixWrapperT halomat2(matrix2, bound_spec, stencil_spec);

  auto max_idx = dash::halo::RegionCoords<2>::NumRegionsMax;

  auto stencil_op = halomat.stencil_operator(stencil_spec);
  auto stencil_op2 = halomat2.stencil_operator(stencil_spec);

  auto* current_op = &stencil_op;
  auto* new_op = &stencil_op2;

  HaloMatrixWrapperT* current_halo = &halomat;
  HaloMatrixWrapperT* new_halo = &halomat2;



  constexpr double dx{ 1.0 };
  constexpr double dy{ 1.0 };
  constexpr double dt{ 0.05 };
  constexpr double k{ 1.0 };

  // initial total energy
  array_t energy{ranks};
  double initEnergy = calcEnergy(current_halo->matrix(), energy);

  const auto &lview = halomat.view_local();
  auto offset = lview.extent(1);
  long inner_start = offset + 1;
  long inner_end = lview.extent(0) * (offset - 1) - 1;

  current_halo->matrix().barrier();

  for (auto d = 0; d < iterations; ++d) {

    auto& current_matrix = current_halo->matrix();
    auto& new_matrix = new_halo->matrix();

    // dummy task to synchronize halo update on previous iter
    // TODO: implement CONCURRENT dependency to get rid of it
    dash::tasks::async("DUMMY",
      [](){
        // nothing to do
      },
      // dummy dependency to synchronize with previous iteration
      dash::tasks::out(*new_halo),
      dash::tasks::out(*current_halo)
    );

    // Update Halos asynchroniously
    //auto region_ptr = current_halo->halo_block().halo_region(idx);
    // region_ptr may be nullptr!
    // region_ptr represents the remote halo region
    //if (region_ptr != nullptr) {
      // halo_range.first is beginning of halo memory for that range
      //auto halo_range = current_halo->halo_memory().range_at(idx);
      dash::tasks::async("UPDATE_HALO",
        [current_halo, max_idx](){
          current_halo->update_async();
          //std::cout << "BEGIN UPDATE HALO @ALL" << std::endl;
          // TODO: dispatch this task on the internal dart_handle
          while (!current_halo->test()) dash::tasks::yield();
          //std::cout << "END   UPDATE HALO @ALL" << std::endl;
        },
        [&](auto deps){
          // dummy dependency to synchronize with previous iteration
          deps = dash::tasks::in(*new_halo);
          // dependency to synchronize with the boundary update tasks
          deps = dash::tasks::out(*current_halo);
          // neighbor halos
          for (int idx = 0; idx < max_idx; ++idx) {
            auto region_ptr = current_halo->halo_block().halo_region(idx);
            // region_ptr may be nullptr!
            // region_ptr represents the remote halo region
            if (region_ptr != nullptr) {
              deps = dash::tasks::in(region_ptr->begin());
            }
          }
        });
    //}

    // optimized calculation of inner matrix elements
    auto* new_begin = new_matrix.lbegin();
#if 0
    auto* current_begin = current_matrix.lbegin();
    for (auto i = inner_start; i < inner_end; i += offset) {
      auto* center = current_begin + i;
      auto* center_y_plus = center + offset;
      auto* center_y_minus = center - offset;
      for (auto j = 0; j < offset - 2;
           ++j, ++center, ++center_y_plus, ++center_y_minus) {
        /*auto dtheta =
            (*(center - 1) + *(center + 1) - 2 * (*center)) / (dx * dx) +
            (*(center_y_minus) + *(center_y_plus) - 2 * (*center)) / (dy * dy);
        *(new_begin + i + j) = *center + k * dtheta * dt;*/
        *(new_begin + i + j) = calc(center, center_y_minus, center_y_plus, center - 1, center + 1);
      }
    }
#endif

    //size_t num_inner_elems = current_op->inner.view().size();
    // TODO: taskloop breaks because StencilIterator::operator= is deleted!
#if 1
    dash::tasks::taskloop(
      current_op->inner.begin(), current_op->inner.end(),
      [current_op, &new_matrix, dx, dy](auto begin, auto end){
        current_op->inner.update(begin, end, new_matrix.lbegin(),
            [&](auto* center, auto* center_dst, auto offset, const auto& offsets) {
              //std::cout << "INNER " << dash::distance(begin, end) << std::endl;
              double dtheta =(center[offsets[0]] + center[offsets[1]] - 2 * *center) / (dx * dx) +
                      (center[offsets[2]] + center[offsets[3]] - 2 * *center) / (dy * dy);
              *center_dst = *center + k * dtheta * dt;
            });
      }, [&](auto begin, auto end, auto deps){
        // dummy dependency to synchronize with next iteration
        deps = dash::tasks::in(*new_halo);
        // NOTE: no OUT dependency here because we synchronize with the dummy task above
      });
#else
    size_t stepsize = 10;
    for (auto it = current_op->inner.begin(); it < current_op->inner.end(); it += stepsize) {
      dash::tasks::async([it, current_op, &new_matrix, stepsize](){
        current_op->inner.update(it, it+stepsize, new_matrix.lbegin(),
            [&](auto* center, auto* center_dst, auto offset, const auto& offsets) {
              std::cout << "INNER @" << it.rpos() << std::endl;
              double dtheta =(center[offsets[0]] + center[offsets[1]] - 2 * *center) / (dx * dx) +
                      (center[offsets[2]] + center[offsets[3]] - 2 * *center) / (dy * dy);
              *center_dst = *center + k * dtheta * dt;
            });
      });
    }
#endif // 0
#if 0
    // slow version
    auto it_end = current_op->inner.end();
    for(auto it = current_op->inner.begin(); it != it_end; ++it)
    {
      auto core = *it;
      auto dtheta = (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
                    (it.value_at(2) + it.value_at(3) - 2 * core) /(dy * dy);
      new_begin[it.lpos()] = core + k * dtheta * dt;
    }
#endif

    // Wait until all Halo updates ready
    //current_halo->wait();

    // Calculation of boundary Halo elements
    for (int dim = 0; dim < 2; ++dim) {
      {
        auto it_pair = current_op->boundary.iterator_at(dim, dash::halo::RegionPos::PRE);
        auto new_it_pair = new_op->boundary.iterator_at(dim, dash::halo::RegionPos::PRE);
        auto idx = dash::halo::RegionCoords<2>::index(dim, dash::halo::RegionPos::PRE);
        auto block = current_halo->halo_block();
        auto region_ptr = block.halo_region(idx);
        // halo_range.first is beginning of halo memory for that range
        //auto halo_range = current_halo->halo_memory().range_at(idx);

        // region_ptr may be nullptr!
        if (region_ptr != nullptr) {
          auto lbegin = new_matrix.lbegin();
          dash::tasks::async("UPDATE_BOUNDARY", [&, lbegin, it_pair, current_op, idx](){
            //std::cout << "BEGIN UPDATE BOUNDARY @" << idx << std::endl;
            current_op->boundary.update(
              it_pair.first, it_pair.second, lbegin,
              [&](auto& it) {
                auto core = *it;
                double dtheta =
                    (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
                    (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
                return core + k * dtheta * dt;
              });
          },
          // dummy dependency to synchronize with the halo update
          dash::tasks::in(*current_halo),
          // dummy dependency to synchronize with next iteration
          dash::tasks::in(*new_halo),
          dash::tasks::out(*new_it_pair.first));
        }
      }
      {
        auto it_pair = current_op->boundary.iterator_at(dim, dash::halo::RegionPos::POST);
        auto new_it_pair = new_op->boundary.iterator_at(dim, dash::halo::RegionPos::PRE);
        auto idx = dash::halo::RegionCoords<2>::index(dim, dash::halo::RegionPos::POST);
        auto block = current_halo->halo_block();
        auto region_ptr = block.halo_region(idx);
        // halo_range.first is beginning of halo memory for that range
        //auto halo_range = current_halo->halo_memory().range_at(idx);


        // region_ptr may be nullptr!
        if (region_ptr != nullptr) {

          auto lbegin = new_matrix.lbegin();
          dash::tasks::async("UPDATE_BOUNDARY", [&, lbegin, it_pair, current_op, idx](){
            //std::cout << "BEGIN UPDATE BOUNDARY @" << idx << std::endl;
            current_op->boundary.update(
              it_pair.first, it_pair.second, lbegin,
              [&](auto& it) {
                auto core = *it;
                double dtheta =
                    (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
                    (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
                return core + k * dtheta * dt;
              });
          },
          // dummy dependency to synchronize with the halo update
          dash::tasks::in(*current_halo),
          // dummy dependency to synchronize with next iteration
          dash::tasks::in(*new_halo),
          // output dependency
          dash::tasks::out(*new_it_pair.first));
        }
      }
    }

#if 0
    auto it_bend = current_op->boundary.end();
    for (auto it = current_op->boundary.begin(); it != it_bend; ++it) {
      auto core = *it;
      double dtheta =
          (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
          (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
      new_begin[it.lpos()] = core + k * dtheta * dt;
    }
#endif

    // swap current matrix and current halo matrix
    std::swap(current_halo, new_halo);
    std::swap(current_op, new_op);
    //current_matrix.barrier();
    dash::tasks::async_barrier();
  //dash::tasks::complete();
  }
  // wait for all tasks to complete
  dash::tasks::complete();
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
    cout.flush();
  }

  dash::finalize();

  return 0;
}
