#ifndef DASH__PATTERN_BLOCK_VISUALIZER_H
#define DASH__PATTERN_BLOCK_VISUALIZER_H

#include <dash/dart/if/dart.h>

#include <sstream>
#include <array>
#include <iomanip>
#include <string>

#include <dash/tools/Colorspace.h>

namespace dash {
namespace tools {

/**
 *
 * Take a generic pattern instance and visualize it as an SVG image.
 * The visualization is limited to two dimensions at the moment, but
 * for higher-dimensional patterns any two dimensions can be specified
 * for visualization.
 */
template<typename PatternT>
class PatternBlockVisualizer
{
private:
  typedef PatternBlockVisualizer<PatternT> self_t;
  typedef typename PatternT::index_type    index_t;

private:
  const PatternT & _pattern;

  int _tileszx,
      _tileszy;
  int _gridx,
      _gridy;
  std::string _title;
  std::string _descr;
  int _fontsz;
  int _fontsz_title;
  int _fontsz_tiny;

  class RGB {
  private:
    unsigned _r, _g, _b;
  public:
    RGB(unsigned r, unsigned g, unsigned b) {
      _r = r;
      _g = g;
      _b = b;
    }
    std::string hex() const {
      std::ostringstream ss;
      ss << "#" << std::hex << std::uppercase << std::setw(2);
      ss << std::setfill('0') << _r << _g << _b;
      return ss.str();
    }
  };

public:
  PatternBlockVisualizer(const PatternT & pat,
                         const std::string & title = "",
                         const std::string & descr = "")
  : _pattern(pat), _title(title), _descr(descr)
  {
    _gridx        = _gridy   = 17;
    _tileszx      = _tileszy = 16;
    _fontsz_tiny  =  6;
    _fontsz       = 10;
    _fontsz_title = 12;
  }

  PatternBlockVisualizer() = delete;
  PatternBlockVisualizer(const self_t & other) = delete;

  void set_description(const std::string & str) {
    _descr = str;
  }
  void set_title(const std::string & str) {
    _title = str;
  }

  void draw_pattern(std::ostream & os,
                    std::array<index_t, PatternT::ndim()> coords,
                    int dimx, int dimy) {
    std::string title = _title;
    replace_all(title, "<", "&lt;");
    replace_all(title, ">", "&gt;");

    os << "<svg xmlns=\"http://www.w3.org/2000/svg\"";
    os << " xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";

    // typeset title line
    os << "<text x=\"10\" y=\"15\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz_title << "\">";
    os << title;
    os << "</text>\n";

    // draw the pane, including axes and key
    os << "<g transform=\"translate(10,20)\">\n";
    draw_pane(os, coords, dimx, dimy);
    os << "</g>\n";

    os << "</svg>" << std::endl;
  }

  void draw_pane(std::ostream & os,
                 std::array<index_t, PatternT::ndim()> coords,
                 int dimx, int dimy) {
    os << "<g transform=\"translate(10,10)\">" << std::endl;
    draw_axes(os, dimx, dimy);

    os << "<g transform=\"translate(4,4)\">" << std::endl;
    draw_tiles(os, coords, dimx, dimy);
    os << "</g>" << std::endl;

    os << "</g>" << std::endl;
  }

  void draw_axes(std::ostream & os,
                 int dimx, int dimy,
                 int offsx = 0, int offsy = 0) {
    int startx, starty;
    int endx, endy;

    startx = offsx;
    starty = offsy;
    endx = startx + (1 + _pattern.blockspec().extent(dimx)) * _gridx;
    endy = starty;

    os << "<line x1=\"" << startx << "\" y1=\"" << starty << "\"";
    os << " x2=\"" << endx << "\" y2=\"" << endy << "\"";
    os << " style=\"stroke:#808080;stroke-width:1\"/>";

    os << "<text x=\"" << endx / 3 << "\" y=\"" << starty - 1 << "\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz << "\"";
    //    os<<" transform=\"rotate(30 20,40)\" ";
    os << " >";
    os << "Dimension " << dimx << std::endl;
    os << "</text>" << std::endl;

    startx = offsx;
    starty = offsy;
    endx = startx;
    endy = starty + (1 + _pattern.blockspec().extent(dimy)) * _gridy;

    os << "<line x1=\"" << startx << "\" y1=\"" << starty << "\"";
    os << " x2=\"" << endx << "\" y2=\"" << endy << "\"";
    os << " style=\"stroke:#808080;stroke-width:1\"/>";

    os << "<text x=\"" << startx - 1 << "\" y=\"" << endy / 3 << "\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz << "\"";
    os << " transform=\"rotate(-90," << startx - 1 << "," << endy / 3 << ")\" ";
    os << " >";
    os << "Dimension " << dimy << std::endl;
    os << "</text>" << std::endl;
  }

  void draw_tiles(std::ostream & os,
                  std::array<index_t, PatternT::ndim()> coords,
                  int dimx, int dimy)
  {
    std::array<index_t, PatternT::ndim()> block_coords;
    std::array<index_t, PatternT::ndim()> block_begin_coords;
    auto blockspec = _pattern.blockspec();
    for (int i = 0; i < blockspec.extent(dimx); i++) {
      for (int j = 0; j < blockspec.extent(dimy); j++) {
        block_coords[dimx] = i;
        block_coords[dimy] = j;
        auto block_idx = _pattern.blockspec().at(block_coords);
        auto block     = _pattern.block(block_idx);
        block_begin_coords[dimx] = block.offset(dimx);
        block_begin_coords[dimy] = block.offset(dimy);
        auto unit      = _pattern.unit_at(block_begin_coords);

        int i_grid = i * _gridx;
        int j_grid = j * _gridy;
        int t_x    = i_grid + (_tileszx / 5);
        int t_y    = j_grid + (_tileszy / 2) + (_fontsz_tiny / 2);

        os << "<rect x=\"" << i_grid << "\" y=\"" << j_grid << "\" ";
        os << "height=\"" << _tileszy << "\" width=\"" << _tileszx << "\" ";
        os << tilestyle(unit);
        os << "> ";
        os << "<!-- i=" << i << " j=" << j << "--> ";
        os << "</rect>" << std::endl;
        os << "<text " << "x=\"" << t_x << "\" "
                       << "y=\"" << t_y << "\" "
                       << "fill=\"black\" "
                       << "font-size=\"" << _fontsz_tiny << "\" >";
        os << unit;
        os << "</text>" << std::endl;
      }
    }
  }

private:
  RGB color(dart_unit_t unit) {
    float min = 0;
    float max = _pattern.num_units();
    float nx  = _pattern.teamspec().extent(1);
    float ny  = _pattern.teamspec().extent(0);
    auto  unit_coord = _pattern.teamspec().coords(unit);

    // unit id to color wavelength:
    float unit_h_perc = static_cast<float>(unit) / max;
    float unit_s_perc = static_cast<float>(unit_coord[0]) / ny;
    float unit_v_perc = static_cast<float>(unit_coord[1]) / nx;

    dash::tools::color::hsv hsv;
    hsv.h = 360.0 * unit_h_perc;
    hsv.s = 0.3 + 0.5 * unit_s_perc;
    hsv.v = 0.3 + 0.4 * unit_v_perc;

    auto rgb = dash::tools::color::hsv2rgb(hsv);
    int  r   = static_cast<int>(rgb.r * 255);
    int  g   = static_cast<int>(rgb.g * 255);
    int  b   = static_cast<int>(rgb.b * 255);

    return RGB(r,g,b);
  }

  std::string tilestyle(dart_unit_t unit)
  {
    std::ostringstream ss;
    ss << "style=\"fill:" << color(unit).hex() << ";";
    ss << "stroke-width:0\"";
    return ss.str();
  }

  bool replace_string(std::string & str,
                      const std::string & from,
                      const std::string & to)
  {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
      return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
  }

  bool replace_all(std::string & str,
                   const std::string & from,
                   const std::string & to)
  {
    while (replace_string(str, from, to)) {};
    return true;
  }
};

} // namespace tools
} // namespace dash

#endif // DASH__PATTERN_BLOCK_VISUALIZER_H
