#include <iostream>
#include <libdash.h>
#include <chrono>
//#include "minimon.h"

#define DEBUG

using namespace std;

using PatternT    = dash::Pattern<2>;
using MatrixT     = dash::Matrix<double, 2, typename PatternT::index_type, PatternT>;
using StencilT     = dash::halo::StencilPoint<2>;
using StencilSpecT = dash::halo::StencilSpec<StencilT,4>;
using GlobBoundSpecT     = dash::halo::GlobalBoundarySpec<2>;
using HaloMatrixWrapperT = dash::halo::HaloMatrixWrapper<MatrixT>;

using array_t      = dash::Array<double>;

using time_point_t = std::chrono::time_point<std::chrono::system_clock>;
using time_diff_t  = std::chrono::duration<double>;

//MiniMon minimon{};

constexpr double dx = 1.0;
constexpr double dy = 1.0;
constexpr double dt = 0.05;
constexpr double k = 1.0;

struct locked_cout_t {

  template<typename T>
  locked_cout_t& operator<<(const T& value) {
    if (owner != dash::tasks::threadnum()) {
      mtx.lock();
      owner = dash::tasks::threadnum();
    }
    std::cout << value;

    return *this;
  }

  // terminator, unlocks the mutex on std::endl
  locked_cout_t& operator<<(std::ostream& (*pf)(std::ostream&)) {
    std::cout << pf;

    owner = -1;
    mtx.unlock();
  }

private:
  static std::mutex mtx;
  int owner = -1;
};

std::mutex locked_cout_t::mtx;

static locked_cout_t locked_cout;

std::fstream *thread_streams;

#define DEBUGOUT (thread_streams[dash::tasks::threadnum()])

void print_matrix(const MatrixT& matrix) {
  auto rows = matrix.extent(0);
  auto cols = matrix.extent(1);
  cout << fixed;
  cout.precision(4);
  cout << "Matrix:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c)
      cout << " " << setw(4) << (double) matrix[r][c] << " ";
    cout << endl;
  }
}

double calcEnergy(const MatrixT& m, array_t &a) {
  *a.lbegin() = std::accumulate( m.lbegin(), m.lend(), 0.0);
  a.barrier();

  double energy = 0.0;
  if(dash::myid() == 0)
    energy = std::accumulate(a.begin(), a.end(), 0.0);
  a.barrier();

  return energy;
}

template<typename PtrT, typename IteratorT>
static bool in_range(PtrT* ptr, const std::pair<IteratorT, IteratorT>& range) {
  return (ptr >= range.first && ptr < range.second);
}

#if 0
// TODO: needs fixing!
template<typename StencilOpT, typename DependencyIteratorT>
static
typename DependencyIteratorT::value_t&
dependency_in_dim(
  int dim,
  int chunk_size,
  const std::array<int, 2>& range,
  const StencilOpT& op,
  dash::halo::RegionPos rpos)
{
  auto end_coords = (op.inner.end() - 1).coords();
  bool needs_boundary = (rpos == dash::halo::RegionPos::PRE) ? range[0] == 0 : range[]
  if (coords[dim] == 0) {
    // dependency to the left boundary
    // TODO: check that dimension is correct
    return *(op.boundary.iterator_at(dim, rpos).first);
  } else {
    // dependency to left inner block
    auto sign = (rpos == dash::halo::RegionPos::PRE) ? -1 : 1;
    return op.inner.at({coords[0] + (sign*(!dim)*chunk_size),
                        coords[1] + (sign*( dim)*chunk_size)});
  }
}
#endif

int main(int argc, char *argv[])
{
  //minimon.enter();


  if (argc < 4) {
    cerr << "Not enough arguments ./<prog> matrix_ext iterations chunk_size" << endl;
    return 1;
  }

  auto matrix_ext = std::atoi(argv[1]);
  auto iterations = std::atoi(argv[2]);
  auto chunk_size = std::atoi(argv[3]);

  //minimon.enter();
  dash::init(&argc, &argv);
  //minimon.leave("initialize");

  auto myid = dash::myid();
  auto ranks = dash::size();

  thread_streams = new std::fstream[dash::tasks::numthreads()];

  for (int i = 0; i < dash::tasks::numthreads(); ++i) {
    std::stringstream fname;
    fname << "halo.";
    fname << dash::myid();
    fname << ".";
    fname << i;
    fname << ".log";
    thread_streams[i] = std::fstream(fname.str().c_str(), std::ios_base::out);
  }

  dash::DistributionSpec<2> dist(dash::BLOCKED, dash::BLOCKED);
  dash::TeamSpec<2> tspec;
  //tspec.balance_extents();
  PatternT pattern(dash::SizeSpec<2>(matrix_ext, matrix_ext),dist, tspec, dash::Team::All());
  MatrixT src_matrix(pattern);
  MatrixT dst_matrix(pattern);
  // Stencil points for North, South, West, and East, Center is defined automatically
  StencilSpecT stencil_spec( StencilT(-1,0), StencilT(1,0), StencilT( 0,-1), StencilT(0,1) );
  // Periodic/cyclic global boundary values for both dimensions
  GlobBoundSpecT bound_spec(dash::halo::BoundaryProp::CYCLIC,dash::halo::BoundaryProp::CYCLIC);
  // HaloWrapper for source and destination partitions
  HaloMatrixWrapperT src_halo(src_matrix, bound_spec, stencil_spec);
  HaloMatrixWrapperT dst_halo(dst_matrix, bound_spec, stencil_spec);
  auto src_halo_ptr = &src_halo;
  auto dst_halo_ptr = &dst_halo;
  // Stencil specific operator for both partitions
  auto src_stencil_op = src_halo.stencil_operator(stencil_spec);
  auto dst_stencil_op = dst_halo.stencil_operator(stencil_spec);
  auto src_op_ptr = &src_stencil_op;
  auto dst_op_ptr = &dst_stencil_op;

  // neighboring boundary regions for each boundary region
  const std::map<int, std::vector<int>> boundary_boundary_dependencies = {
                                                {0, {0, 1, 3}}, // NW
                                                {1, {0, 1, 2}}, // N
                                                {2, {1, 2, 5}}, // NE
                                                {3, {0, 3, 6}}, // W
                                                {4, {}},
                                                {5, {2, 5, 8}}, // E
                                                {6, {3, 6, 7}}, // SW
                                                {7, {6, 7, 8}}, // S
                                                {8, {5, 7, 8}}};// SE

  // dependencies of a boundary on halo memory
  const std::map<int, std::vector<int>> boundary_halo_dependencies = {
                                                {0, {1, 3}},  // NW
                                                {1, {1}},     // N
                                                {2, {1, 5}},  // NE
                                                {3, {3}},     // W
                                                {4, {}},
                                                {5, {5}},     // E
                                                {6, {3, 7}},  // SW
                                                {7, {7}},     // S
                                                {8, {7, 5}}}; // SE

  // boundaries on the remote side required for each halo transfer
  // TODO: combine all corner elements into one task
  DEBUGOUT << "pattern.local_extent(0): " << pattern.local_extent(0) << std::endl;
  const std::map<int, std::vector<long>> halo_boundary_offsets = {
                                                {1, {0, 1, pattern.local_extent(1)-1}}, // N depends on the row and both corners
                                                {3, {0, 1, pattern.local_extent(0)-1}}, // W depends on the column and both corners
                                                {5, {0, 1, pattern.local_extent(0)-1}}, // E depends on the column and both corners
                                                {7, {0, 1, pattern.local_extent(1)-1}}, // S depends on the row and both corners
  };

  constexpr const auto max_idx = dash::halo::RegionCoords<2>::NumRegionsMax;

  constexpr const std::array<dash::halo::RegionPos, 2> region_positions(
                                          {dash::halo::RegionPos::PRE,
                                           dash::halo::RegionPos::POST});


  auto ptr_matrix = src_matrix.lbegin();
  auto ptr_matrix2 = dst_matrix.lbegin();
  if(myid == 0) {
    for(auto i = 0; i < src_matrix.local.extent(0); ++i) {
      for(auto j = 0; j < src_matrix.local.extent(1); ++j, ++ptr_matrix, ++ptr_matrix2) {
        if(i < 100  && j < 100) {
          //*ptr_matrix = i + (j / 100.0);
          *ptr_matrix = 1;
        } else {
          *ptr_matrix = 0;
          *ptr_matrix2 = 0;
        }
      }
    }
  } else {
    for(auto i = 0; i < src_matrix.local.extent(0); ++i) {
      for(auto j = 0; j < src_matrix.local.extent(1); ++j, ++ptr_matrix, ++ptr_matrix2) {
        *ptr_matrix = 0;
        *ptr_matrix2 = 0;
      }
    }
  }

/*  if (myid == 0) {
    for(auto i = 0; i < matrix.local.extent(0); i++) {
      for(auto j = 0; j < matrix.local.extent(1); j++) {
        if(i < 100 && j < 100) {
          matrix.local[i][j] = 1;
          matrix2.local[i][j] = 1;
        } else {
          matrix.local[i][j] = 0;
          matrix2.local[i][j] = 0;
        }
      }
    }
  } else {
    for(auto i = 0; i < matrix.local.extent(0); i++) {
      for(auto j = 0; j < matrix.local.extent(1); j++) {
        matrix.local[i][j] = 0;
        matrix2.local[i][j] = 0;
      }
    }
  }
*/
  src_matrix.barrier();

#ifdef DEBUG
  if (myid == 0)
    print_matrix(src_matrix);
#endif

  //minimon.enter();

  //minimon.leave("Halo initialize");

  // initial total energy
  array_t energy(ranks);
  //minimon.enter();
  double initEnergy = calcEnergy(src_halo_ptr->matrix(), energy);
  //minimon.leave("calc energy");

  const auto &lview = src_halo_ptr->view_local();
  auto offset = lview.extent(1);
  long inner_start = offset + 1;
  long inner_end = lview.extent(0) * (offset - 1) - 1;

  src_halo_ptr->matrix().barrier();

  //minimon.enter();
  dash::tasks::async_fence();

  for (auto iter = 0; iter < iterations; ++iter) {
    auto dst_matrix_lbegin = dst_halo_ptr->matrix().lbegin();

    DEBUGOUT << "iter = " << iter << std::endl;

    //minimon.enter();

    /* Update Halos asynchronously */

    for (int idx = 0; idx < max_idx; ++idx) {
      auto region_ptr  = src_op_ptr->halo_block().halo_region(idx);

      // region_ptr may be nullptr!
      // region_ptr represents the remote halo region
      if (region_ptr != nullptr) {

        dash::tasks::async("UPDATE_HALO",
          [src_halo_ptr, idx, iter, src_op_ptr](){
            DEBUGOUT << "{" << iter << "} Starting update of halo region "
                      << idx << std::endl;
            src_halo_ptr->update_async_at(idx);
            //src_halo_ptr->update_at(idx);
            // TODO: dispatch this task on the internal dart_handle
            //while (!src_halo_ptr->test(idx)) dash::tasks::yield();
            dart_handle_t handle = src_halo_ptr->handle_at(idx);
            dart_task_wait_handle(&handle, 1);
            DEBUGOUT << "{" << iter << "} Finished update of halo region "
                      << idx << std::endl;
            auto range = src_op_ptr->halo_memory().range_at(idx);
            for (auto iter = range.first; iter < range.second; iter++){
              DEBUGOUT << *iter << " ";
            }
            DEBUGOUT << std::endl;
          },
          [&](auto deps){
            // local halo memory
            deps = dash::tasks::out(*src_op_ptr->halo_memory().range_at(idx).first);

            /* dependency to remote boundaries */
            auto region_begin = region_ptr->begin();
            for (int offset : halo_boundary_offsets.find(idx)->second) {
              DEBUGOUT  << "HALO transfer for region " << idx
                        << " depends on boundary " << max_idx - idx - 1
                        << " at offset " << offset
                        << " (" << region_begin + offset << ")" << std::endl;
              auto iter = region_begin + offset;
              deps = dash::tasks::in(*iter);
            }
          });
      }
    }

#if 0
    for (int dim = 0; dim < 2; ++dim) {
      for (auto rpos : region_positions)
      {
        auto idx         = dash::halo::RegionCoords<2>::index(dim, rpos);
        auto region_ptr  = src_op_ptr->halo_block().halo_region(idx);

        // region_ptr may be nullptr!
        // region_ptr represents the remote halo region
        if (region_ptr != nullptr) {

          std::cout << "Creating UPDATE task for region " << idx << " in " << iter << " with IN " << region_ptr->begin() << std::endl;

          dash::tasks::async("UPDATE_HALO",
            [src_halo_ptr, idx, iter, src_op_ptr](){
              std::cout << "{" << iter << "} Starting update of halo region " << idx << std::endl;
              //src_halo_ptr->update_async_at(idx);
              src_halo_ptr->update_at(idx);
              // TODO: dispatch this task on the internal dart_handle
              //while (!src_halo_ptr->test(idx)) dash::tasks::yield();
              std::cout << "{" << iter << "} Finished update of halo region " << idx << std::endl;
              auto range = src_op_ptr->halo_memory().range_at(idx);
              for (auto iter = range.first; iter < range.second; iter++){
                std::cout << *iter << " ";
              }
              std::cout << std::endl;
            },
            [&](auto deps){
              // neighbor halo
              deps = dash::tasks::in(region_ptr->begin());
              // local halo memory
              deps = dash::tasks::out(*src_op_ptr->halo_memory().range_at(idx).first);
            });
        }
      }
    }
#endif // 0

    //minimon.leave("async");

    //minimon.enter();
    // Calculation of all inner partition elements

    // the coordinate of the first element in the inner part
    const auto coords_begin =  src_op_ptr->inner.begin().coords();
    // the coordinates of the _last_ element in the inner part
    const auto coords_last   = (src_op_ptr->inner.end() - 1).coords();
    // Y-Direction: slower index, top to bottom
    auto begin_y = coords_begin[0];
    while (begin_y <= coords_last[0]) {
      auto end_y = begin_y + chunk_size - 1;
      if(end_y > coords_last[0])
        end_y = coords_last[0];
      // X-Direction: fastest running index, left to right
      auto begin_x = coords_begin[1];
      while (begin_x <= coords_last[1]) {
        auto end_x   = begin_x + chunk_size - 1;
        if(end_x > coords_last[1])
          end_x = coords_last[1];
        dash::tasks::async("UPDATE_INNER",
          [=](){
            src_op_ptr->inner.update_blocked({begin_y, begin_x},{end_y, end_x}, dst_matrix_lbegin,
              [&, iter, begin_x, begin_y](auto* center, auto* center_dst, auto offset, const auto& offsets) {
                double dtheta = (center[offsets[0]] + center[offsets[1]] - 2 * *center) / (dx * dx) +
                              (center[offsets[2]] + center[offsets[3]] - 2 * *center) / (dy * dy);
                *center_dst = *center + k * dtheta * dt;


                DEBUGOUT << "{" << iter << "} Computing value at " << offset << "{" << begin_y << ", " << begin_x << "} : {" << *center << " ";
                for (int i = 0; i <= 3; ++i) {
                  DEBUGOUT << center[offsets[i]] << " ";
                }
                DEBUGOUT << "} -> " << *center_dst << std::endl;
                for (int i = 0; i <= 3; ++i) {
                  if (center[offsets[i]] != 1.0) {
                    DEBUGOUT << "value in dimension " << i << " not one: " << center[offsets[i]] << ", offset " << offset + offsets[i] << std::endl;
                  }
                }
              } );
          },
          [&](auto deps){
            // same-block dependencies
            deps = dash::tasks::out(dst_op_ptr->inner.at({begin_y, begin_x}));
            deps = dash::tasks::in (src_op_ptr->inner.at({begin_y, begin_x}));

            // dependencies in X direction
            if (begin_x == coords_begin[1]) {
              // dependency to the left boundary
              // TODO: check that dimension is correct
              deps = dash::tasks::in(*src_op_ptr->boundary.iterator_at(3).first);
            } else {
              // dependency to left inner block
              deps = dash::tasks::in( src_op_ptr->inner.at({begin_y, begin_x - chunk_size}));
            }

            if (end_x == coords_last[1]) {
              // dependency to the right boundary
              // TODO: check that dimension is correct
              deps = dash::tasks::in(*src_op_ptr->boundary.iterator_at(5).first);
              auto coords = src_op_ptr->boundary.iterator_at(5).first.coords();
              DEBUGOUT << "Block at {" << begin_y << ", " << begin_x << "} depends on boundary 5 {" << coords[0] << ", " << coords[1] << "}" << std::endl;
            } else {
              // dependency to right inner block
              deps = dash::tasks::in( src_op_ptr->inner.at({begin_y, begin_x + chunk_size}));
            }


            // dependencies in Y direction
            if (begin_y == coords_begin[0]) {
              // dependency to the upper boundary
              // TODO: check that dimension is correct
              deps = dash::tasks::in(*src_op_ptr->boundary.iterator_at(1).first);
            } else {
              // dependency to upper inner block
              deps = dash::tasks::in( src_op_ptr->inner.at({begin_y - chunk_size, begin_x}));
            }

            if (end_y == coords_last[0]) {
              // dependency to the lower boundary
              // TODO: check that dimension is correct
              deps = dash::tasks::in(*src_op_ptr->boundary.iterator_at(7).first);
            } else {
              // dependency to lower inner block
              deps = dash::tasks::in( src_op_ptr->inner.at({begin_y + chunk_size, begin_x}));
            }

          }
        );
        begin_x += chunk_size;
      }
      begin_y += chunk_size;
    }

    //minimon.leave("inner");

    // BOUNDARY
    for (int idx = 0; idx < max_idx; ++idx) {
      // Region index 4 is the inner area
      if (idx == 4) continue;
      DEBUGOUT << "Creating boundary update task for region " << idx << " in " << iter << std::endl;
      dash::tasks::async("UPDATE_BOUNDARY",
        [&, dst_matrix_lbegin, src_op_ptr, iter, idx](){
          auto src_it_pair = src_op_ptr->boundary.iterator_at(idx);
          src_op_ptr->boundary.update(
            src_it_pair.first, src_it_pair.second, dst_matrix_lbegin,
            [=](auto& it) {
              auto core = *it;
              double dtheta =
                  (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
                  (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
              auto res = core + k * dtheta * dt;


              DEBUGOUT << "{" << iter << "} Computing BOUNDARY value in region " << idx << " at " << it.lpos() << ": { " << core << " ";
              for (int i = 0; i <= 3; ++i) {
                DEBUGOUT << it.value_at(i) << " ";
              }
              DEBUGOUT << "} -> " << res << std::endl;



              return res;
            });
      },
      [&](auto deps){
        /* output dependency on first element of boundary */
        deps = dash::tasks::out(*dst_op_ptr->boundary.iterator_at(idx).first);

        // dependencies to other boundaries
        for (int bidx : boundary_boundary_dependencies.find(idx)->second) {
          deps = dash::tasks::in(*src_op_ptr->boundary.iterator_at(bidx).first);
        }

        // dependencies of the boundary on the halo
        for (int bidx : boundary_halo_dependencies.find(idx)->second) {
          deps = dash::tasks::in(*src_op_ptr->halo_memory().range_at(bidx).first);
        }

        // corner tasks don't have dependencies to inner blocks
        if (idx == 0 || idx == 2 || idx == 6 || idx == 8) return;

        /* dependency on blocks along the boundary */
        // TODO: check that directions are correct
        auto coords = src_op_ptr->boundary.iterator_at(idx).first.coords();
        int dim  = (coords[1] == 0 || (coords[1] == matrix_ext - 1)) ? 0 : 1;
        dash::halo::RegionPos rpos = (coords[1] == 0 || coords[1] == 0) ?
                                                  dash::halo::RegionPos::PRE :
                                                  dash::halo::RegionPos::POST;

        // the position of the first element in the block
        auto blockpos = 0;
        // the position of the first element in the last block in this dimension
        auto end   = coords_begin[dim] + (( coords_last[dim] - coords_begin[dim]) / chunk_size) * chunk_size;
        DEBUGOUT << "dim " << dim << ", rpos " << rpos << ", end " << end << ", coords_end[dim] " << coords_last[dim] << ", coords_begin[dim] " << coords_begin[dim] << std::endl;
        while (blockpos <= coords_last[dim]) {
          // for left/upper boundary: walk down with second index 0
          if (dash::halo::RegionPos::PRE == rpos) {
            deps = dash::tasks::in(
              src_op_ptr->inner.at({(dim == 0) ? blockpos : 0,
                                    (dim == 1) ? blockpos : 0}));
          }
          // for right/lower boundary: walk down with second index pointing to last block
          if (dash::halo::RegionPos::POST == rpos) {
            deps = dash::tasks::in(
              src_op_ptr->inner.at({(dim == 0) ? blockpos : end,
                                    (dim == 1) ? blockpos : end}));
          }
          blockpos += chunk_size;
        }
      });
    }

#if 0
    // Calculation of boundary Halo elements
    for (int dim = 0; dim < 2; ++dim) {
      for (auto rpos : region_positions)
      {
        auto idx         = dash::halo::RegionCoords<2>::index(dim, rpos);
        auto region_ptr  = src_op_ptr->halo_block().halo_region(idx);

        // region_ptr may be nullptr!
        if (region_ptr != nullptr) {
          std::cout << "Creating boundary update task for region " << idx << " in " << iter << std::endl;
          dash::tasks::async("UPDATE_BOUNDARY",
            [&, dst_matrix_lbegin, dim, rpos, src_op_ptr, iter, idx](){
              auto src_it_pair = src_op_ptr->boundary.iterator_at(dim, rpos);
              src_op_ptr->boundary.update(
                src_it_pair.first, src_it_pair.second, dst_matrix_lbegin,
                [=](auto& it) {
                  auto core = *it;
                  double dtheta =
                      (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
                      (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
                  auto res = core + k * dtheta * dt;


                  std::cout << "{" << iter << "} Computing BOUNDARY value in region " << idx << " at " << it.lpos() << ": { " << core << " ";
                  for (int i = 0; i <= 3; ++i) {
                    std::cout << it.value_at(i) << " ";
                  }
                  std::cout << "} -> " << res << std::endl;



                  return res;
                });
          },
          [&](auto deps){
            auto dst_it_pair = dst_op_ptr->boundary.iterator_at(dim, rpos);
            /* output dependency on first element of boundary */
            deps = dash::tasks::out(*dst_it_pair.first);

            /* dependency on blocks along the boundary */

            // the position of the first element in the block
            auto blockpos = 0;
            // the position of the first element in the last block in this dimension
            auto end   = coords_begin[dim] + (( coords_last[dim] - coords_begin[dim]) / chunk_size) * chunk_size;
            std::cout << "dim " << dim << ", rpos " << rpos << ", end " << end << ", coords_end[dim] " << coords_last[dim] << ", coords_begin[dim] " << coords_begin[dim] << std::endl;
            while (blockpos <= coords_last[dim]) {
              // for left/upper boundary: walk down with second index 0
              if (dash::halo::RegionPos::PRE == rpos) {
                deps = dash::tasks::in(
                  src_op_ptr->inner.at({(dim == 0) ? blockpos : 0,
                                        (dim == 1) ? blockpos : 0}));
              }
              // for right/lower boundary: walk down with second index pointing to last block
              if (dash::halo::RegionPos::POST == rpos) {
                deps = dash::tasks::in(
                  src_op_ptr->inner.at({(dim == 0) ? blockpos : end,
                                        (dim == 1) ? blockpos : end}));
              }
              blockpos += chunk_size;
            }

            /* Boundaries in one dimension depend on the two boundaries in
             * the other dimension
             */
            for (auto rpos : region_positions)
            {
              int other_dim = !dim;
              auto boundary_it_pair  = src_op_ptr->boundary.iterator_at(other_dim, rpos);
              deps = dash::tasks::in(*boundary_it_pair.first);
            }

            /* Depend on the halo transfer in this dimension / direction */
            deps = dash::tasks::out(*src_op_ptr->halo_memory().range_at(idx).first);

            /* Boundaries in dimension 0 handle the corners and thus depend on
             * the halos of dimension 1
             */
            if (dim == 0) {
              for (auto rpos : region_positions)
              {
                auto _idx = dash::halo::RegionCoords<2>::index(1, rpos);
                /* boundaries in dimension X rely on the two halos in dimension Y */
                deps = dash::tasks::out(*src_op_ptr->halo_memory().range_at(_idx).first);
              }
            }
          });
        }
      }
    }
#endif // 0

    //minimon.leave("boundary");
    // swap source and destination partitions and operators
    std::swap(src_halo_ptr, dst_halo_ptr);
    std::swap(src_op_ptr, dst_op_ptr);

    //src_halo_ptr->matrix().barrier(); // barrier to keep iterations in sync
    dash::tasks::async_fence();
  }
  dash::tasks::complete();
  //minimon.leave("calc total");
  // final total energy
  //minimon.enter();
  double endEnergy = calcEnergy(src_halo_ptr->matrix(), energy);
  //minimon.leave("calc energy");

#ifdef DEBUG
  if (myid == 0)
    print_matrix(src_halo_ptr->matrix());
#endif


  for (int i = 0; i < dash::tasks::numthreads(); ++i) {
    thread_streams[i].close();
  }

  delete[] thread_streams;

  // Output
  if (myid == 0) {
    cout << fixed;
    cout.precision(5);
    cout << "InitEnergy=" << initEnergy << endl;
    cout << "EndEnergy=" << endEnergy << endl;
    cout << "DiffEnergy=" << endEnergy - initEnergy  << endl;
    cout << "Matrixspec: " << matrix_ext << " x " << matrix_ext << endl;
    cout << "Iterations: " << iterations << endl;
    cout.flush();
  }

  dash::Team::All().barrier();
  //minimon.leave("total");

  dash::finalize();

  return 0;
}
