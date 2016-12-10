#include <dash/Init.h>
#include <dash/Matrix.h>
#include <dash/algorithm/Fill.h>
#include <dash/algorithm/Copy.h>

#include <fstream>
#include <string>
#include <iostream>

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

    file << "P2\n" << ext_x << " " << ext_y << std::endl;
    // Data
    // TODO: currently very inefficient
    for(long x=0; x<ext_x; ++x){
      for(long y=0; y<ext_y; ++y){
        int el = data[x][y];
        file << el << " ";
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

template<typename Array_t>
void smooth(Array_t & data_old, Array_t & data_new){
  // Todo: use stencil iterator
  auto pattern = data_old.pattern();
  auto local_gidx   = pattern.coords(pattern.global(0));

  auto lext_x = pattern.local_extent(1)-1;
  auto lext_y = pattern.local_extent(0)-1;

  auto olptr = data_old.lbegin();
  auto nlptr = data_new.lbegin();

  for( index_t x=1; x<lext_x; x++ ) {
    for( index_t y=1; y<lext_y; y++ ) {
      nlptr[y*lext_x+x] =
        ( 0.40 * olptr[y*lext_x+x] +
        0.15 * olptr[(y-1)*lext_x+x] +
        0.15 * olptr[(y+1)*lext_x+x] +
        0.15 * olptr[y*lext_x+x-1] +
        0.15 * olptr[y*lext_x+x+1]);
    }
  }
  // Boundary
  // TODO
}

int main(int argc, char* argv[])
{
  int sizex = 1000;
  int sizey = 1000;
  int niter = 20;

  dash::init(&argc, &argv);

  Array_t data_old(sizex, sizey, dash::BLOCKED, dash::BLOCKED);
  Array_t data_new(sizex, sizey, dash::BLOCKED, dash::BLOCKED);

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
    smooth(data_old, data_new);
    dash::barrier();
    std::copy(data_new.lbegin(), data_new.lend(), data_old.lbegin());
    dash::barrier();
  }

  write_pgm("testimg_output.pgm", data_new);
  dash::finalize();
}
