/**
 * \example ex.11.simple-stencil/main.cpp
 *
 * Stencil codes are iterative kernels on arrays of at least 2 dimensions
 * where the value of an array element at iteration i+1 depends on the values
 * of its neighbors in iteration i.
 *
 * Calculations of this kind are very common in scientific applications, e.g.
 * in iterative solvers and filters in image processing.
 *
 * This example implements a very simple blur filter. For simplicity
 * no real image is used, but an image containg circles is generated.
 * 
 * \TODO fix \cdash::copy problem
 */

#include <dash/Init.h>
#include <dash/Matrix.h>
#include <dash/Dimensional.h>
#include <dash/TeamSpec.h>
#include <dash/algorithm/Fill.h>

#include <fstream>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

using element_t = unsigned char;
using Array_t   = dash::NArray<element_t, 2>;
using index_t = typename Array_t::index_type;


void write_pgm(const std::string & filename, const Array_t & data){
  if(dash::myid() == 0){

    auto ext_x = data.extent(0);
    auto ext_y = data.extent(1);
    std::ofstream file;
    file.open(filename);

    file << "P2\n" << ext_x << " " << ext_y << "\n"
         << "255" << std::endl;
    // Data
//    std::vector<element_t> buffer(ext_x);

    for(long y=0; y<ext_y; ++y){
      auto first = data.begin()+ext_x*y; 
      auto last  = data.begin()+(ext_x*(y+1));

//      BUG!!!!
//      dash::copy(first, last, buffer.data());

      for(long x=0; x<ext_x; ++x){
//        file << buffer[x] << " ";
        file << setfill(' ') << setw(3)
             << static_cast<int>(data[x][y]) << " ";
      }
      file << std::endl;
    }
    file.close();
  }
  dash::barrier();
}

void set_pixel(Array_t & data, index_t x, index_t y){
  const element_t color = 1;
  auto ext_x = data.extent(0);
  auto ext_y = data.extent(1);

  x = (x+ext_x)%ext_x;
  y = (y+ext_y)%ext_y;

  data.at(x, y) = color;
}

void draw_circle(Array_t & data, index_t x0, index_t y0, int r){
  // Check who owns center, owner draws
  if(!data.at(x0, y0).is_local()){
    return;
  }

  int       f     = 1-r;
  int       ddF_x = 1;
  int       ddF_y = -2*r;
  index_t   x     = 0;
  index_t   y     = r;

  set_pixel(data, x0 - r, y0);
  set_pixel(data, x0 + r, y0);
  set_pixel(data, x0, y0 - r);
  set_pixel(data, x0, y0 + r);

  while(x<y){
    if(f>=0){
      y--;
      ddF_y+=2;
      f+=ddF_y;
    }
    ++x;
    ddF_x+=2;
    f+=ddF_x;
    set_pixel(data, x0+x, y0+y);
    set_pixel(data, x0-x, y0+y);
    set_pixel(data, x0+x, y0-y);
    set_pixel(data, x0-x, y0-y);
    set_pixel(data, x0+y, y0+x);
    set_pixel(data, x0-y, y0+x);
    set_pixel(data, x0+y, y0-x);
    set_pixel(data, x0-y, y0-x);
  }
}

void smooth(Array_t & data_old, Array_t & data_new){
  // Todo: use stencil iterator
  auto pattern = data_old.pattern();

  auto gext_x = data_old.extent(0);
  auto gext_y = data_old.extent(1);

  auto lext_x = pattern.local_extent(0);
  auto lext_y = pattern.local_extent(1);
  auto local_beg_gidx = pattern.coords(pattern.global(0));
  auto local_end_gidx = pattern.coords(pattern.global(pattern.local_size()-1));

  auto olptr = data_old.lbegin();
  auto nlptr = data_new.lbegin();

  // Inner cell
  for( index_t x=1; x<lext_x-1; x++ ) {
    for( index_t y=1; y<lext_y-1; y++ ) {
      nlptr[x*lext_y+y] =
        ( 0.40 * olptr[x*lext_y+y] +
        0.15 * olptr[(x-1)*lext_y+y] +
        0.15 * olptr[(x+1)*lext_y+y] +
        0.15 * olptr[x*lext_y+y-1] +
        0.15 * olptr[x*lext_y+y+1]);
    }
  }
  // Boundary
  index_t begin_idx_x = (local_beg_gidx[0] == 0) ? 1 : 0;
  index_t end_idx_x   = (local_end_gidx[0] == gext_x-1) ? lext_x-2 : lext_x-1;
  index_t begin_idx_y = (local_beg_gidx[1] == 0) ? 1 : 0;
  index_t end_idx_y   = (local_end_gidx[1] == gext_y-1) ? lext_y-2 : lext_y-1;
  bool is_top    =(local_beg_gidx[1] == 0) ? true : false;
  bool is_bottom =(local_end_gidx[1] == (gext_y-1)) ? true : false;
  bool is_left   =(local_beg_gidx[0] == 0) ? true : false;
  bool is_right  =(local_end_gidx[0] == (gext_x-1)) ? true : false;

  if(!is_top){
    for( auto x=begin_idx_x; x<end_idx_x; ++x){
      nlptr[lext_y*x] =
        ( 0.40 * olptr[lext_y*x] +
        0.15 * data_old.at(local_beg_gidx[0] + x,   local_beg_gidx[1]-1) +
        0.15 * data_old.at(local_beg_gidx[0] + x,   local_beg_gidx[1]+1) +
        0.15 * data_old.at(local_beg_gidx[0] + x+1, local_beg_gidx[1]) +
        0.15 * data_old.at(local_beg_gidx[0] + x-1, local_beg_gidx[1]));
    }
  }
  if(!is_bottom){
    for( auto x=begin_idx_x; x<end_idx_x; ++x){
      nlptr[lext_y*x + end_idx_y] =
        ( 0.40 * olptr[lext_y*x + end_idx_y] +
        0.15 * data_old.at(local_beg_gidx[0] + x,   local_end_gidx[1]-1) +
        0.15 * data_old.at(local_beg_gidx[0] + x,   local_end_gidx[1]+1) +
        0.15 * data_old.at(local_beg_gidx[0] + x+1, local_end_gidx[1]) +
        0.15 * data_old.at(local_beg_gidx[0] + x-1, local_end_gidx[1]));
    }
  }
  if(!is_left){
    for( auto y=begin_idx_y; y<end_idx_y; ++y){
      nlptr[y] =
        ( 0.40 * olptr[y] +
        0.15 * data_old.at(local_beg_gidx[0]-1, local_beg_gidx[1] + y) +
        0.15 * data_old.at(local_beg_gidx[0]+1, local_beg_gidx[1] + y) +
        0.15 * data_old.at(local_beg_gidx[0],   local_beg_gidx[1] + y-1) +
        0.15 * data_old.at(local_beg_gidx[0],   local_beg_gidx[1] + y+1));
    }
  }
  if(!is_right){
    for( auto y=begin_idx_y; y<end_idx_y; ++y){
      nlptr[lext_y * end_idx_x + y] =
        ( 0.40 * olptr[lext_y * end_idx_x + y] +
        0.15 * data_old.at(local_end_gidx[0]-1, local_beg_gidx[1] + y) +
        0.15 * data_old.at(local_end_gidx[0]+1, local_beg_gidx[1] + y) +
        0.15 * data_old.at(local_end_gidx[0],   local_beg_gidx[1] + y-1) +
        0.15 * data_old.at(local_end_gidx[0],   local_beg_gidx[1] + y+1));
    }
  }
}

int main(int argc, char* argv[])
{
  int sizex = 1000;
  int sizey = 1000;
  int niter = 20;

  dash::init(&argc, &argv);
  
  // Prepare grid
  dash::TeamSpec<2> ts;
  dash::SizeSpec<2> ss(sizex, sizey);
  dash::DistributionSpec<2> ds(dash::BLOCKED, dash::BLOCKED);
  ts.balance_extents();

  dash::Pattern<2> pattern(ss, ds, ts);

  Array_t data_old(pattern);
  Array_t data_new(pattern);

  dash::fill(data_old.begin(), data_old.end(), 255);
  dash::fill(data_new.begin(), data_new.end(), 255);

  draw_circle(data_old, 0, 0, 40);
  draw_circle(data_old, 0, 0, 30);
  draw_circle(data_old, 100, 100, 10);
  draw_circle(data_old, 100, 100, 20);
  draw_circle(data_old, 100, 100, 30);
  draw_circle(data_old, 100, 100, 40);
  draw_circle(data_old, 100, 100, 50);

  dash::barrier();
  write_pgm("testimg_input.pgm", data_old);
  dash::barrier();

  for(int i=0; i<niter; ++i){
    // switch references
    auto & data_prev = i%2 ? data_new : data_old;
    auto & data_next = i%2 ? data_old : data_new;

    smooth(data_prev, data_next);
    dash::barrier();
  }

  // Assume niter is even
  write_pgm("testimg_output.pgm", data_new);
  dash::finalize();
}
