#ifndef DASH__PATTERN_BLOCK_VISUALIZER_H
#define DASH__PATTERN_BLOCK_VISUALIZER_H

#include <dash/dart/if/dart.h>

#include <sstream>
#include <array>
#include <iomanip>
#include <string>

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

  struct sizes {
    int tileszx,
        tileszy;
    int blockszx,
        blockszy;
    int grid_base;
    int gridx,
        gridy;
  };

private:
  const PatternT & _pattern;

  int _tile_base_size;
  int _block_base_size;
  std::string _title;
  std::string _descr;
  int _fontsz_tiny,
      _fontsz,
      _fontsz_title;

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
    _tile_base_size = 10;
    _block_base_size = 26;
    _fontsz_tiny  =  8;
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
                    std::array<index_t, PatternT::ndim()> coords = {},
                    int dimx = 1, int dimy = 0) {
    sizes sz;
    sz.tileszx   = sz.tileszy  = _tile_base_size;
    sz.blockszx  = sz.blockszy = _block_base_size;
    sz.grid_base = _block_base_size + 2;

    // Adjust tile sizes proportional to block regions:
    float block_format = static_cast<float>(_pattern.blocksize(dimy)) /
                         static_cast<float>(_pattern.blocksize(dimx));
    if (block_format < 1) {
      block_format = 1.0 / block_format;
      sz.blockszx *= block_format;
    } else {
      sz.blockszy *= block_format;
    }
    sz.gridx        = sz.blockszx + 2;
    sz.gridy        = sz.blockszy + 2;

    std::string title = _title;
    replace_all(title, "<", "&lt;");
    replace_all(title, ">", "&gt;");

    os << "<svg xmlns=\"http://www.w3.org/2000/svg\"";
    os << " xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";

    // typeset title line
    os << "<text font-family=\"Verdana\" x=\"10\" y=\"15\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz_title << "\">";
    os << title;
    os << "</text>\n";

    // draw the pane, including axes and key
    os << "<g transform=\"translate(10,30)\">\n";
    draw_pane(os, sz, coords, dimx, dimy);
    os << "</g>\n";

    os << "</svg>" << std::endl;
  }

  void draw_pane(std::ostream & os,
                 const sizes & sz,
                 std::array<index_t, PatternT::ndim()> coords,
                 int dimx, int dimy) {
    os << "<g transform=\"translate(10,10)\">" << std::endl;
    draw_axes(os, sz, dimx, dimy);

    os << "<g transform=\"translate(4,4)\">" << std::endl;
    draw_blocks(os, sz, coords, dimx, dimy);
    os << "</g>" << std::endl;

    draw_key(os, sz, dimx, dimy, _pattern.blockspec().extent(dimx)*sz.gridx + 2*(sz.grid_base + 2), 0);
    os << "</g>" << std::endl;
  }

  void draw_axes(std::ostream & os,
                 const sizes & sz,
                 int dimx, int dimy,
                 int offsx = 0, int offsy = 0) {

    int startx = offsx;
    int starty = offsy;
    int lenx   = _pattern.blockspec().extent(dimx) * sz.gridx + sz.grid_base;
    int leny   = _pattern.blockspec().extent(dimy) * sz.gridy + sz.grid_base;

    os << "<defs>\n";
    os << "<marker id=\"arrowhead\" orient=\"auto\"";
    os << " markerWidth=\"6\" markerHeight=\"6\"";
    os << " refX=\"0\" refY=\"0\"";
    os << " viewBox=\"-10 -15 30 30\">\n";
    os << "<path d=\"";
    os << "M " << -10 << " " << -15 << " ";
    os << "L " <<  20 << " " <<   0 << " ";
    os << "L " << -10 << " " <<  15 << " ";
    os << "L " <<   0 << " " <<   0 << " ";
    os << "z ";
    os << "\"";
    os << " style=\"fill:#808080;stroke:#808080;stroke-width:1\"";
    os << "/>";
    os << "</marker>\n";
    os << "</defs>\n";

    os << "<path d=\"";
    os << "M " << startx << " " << starty << " ";
    os << "h " <<   lenx << " ";
    os << "\"";
    os << " style=\"fill:none;stroke:#808080;stroke-width:1\"";
    os << " marker-end=\"url(#arrowhead)\"";
    os << "/>";

    os << "<text font-family=\"Verdana\" x=\"" << startx + lenx/3 << "\" y=\"" << starty - _fontsz/2 << "\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz << "\" >";
    os << "Dimension " << dimx << std::endl;
    os << "</text>" << std::endl;

    os << "<path d=\"";
    os << "M " << startx << " " << starty << " ";
    os << "v " <<   leny << " ";
    os << "\"";
    os << " style=\"fill:none;stroke:#808080;stroke-width:1\"";
    os << " marker-end=\"url(#arrowhead)\"";
    os << "/>";

    os << "<text font-family=\"Verdana\" x=\"" << startx - _fontsz/2 << "\" y=\"" << starty + leny/3 << "\" ";
    os << " transform=\"rotate(-90," << startx - _fontsz/2 << "," << starty + leny/3 << ")\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz << "\" >";
    os << "Dimension " << dimy << std::endl;
    os << "</text>" << std::endl;
  }

  void draw_key(std::ostream & os,
                const sizes & sz,
                int dimx, int dimy,
                int offsx = 0, int offsy = 0) {
    int startx, starty;

    startx = offsx;
    for (int unit = 0; unit < _pattern.num_units(); unit++) {
      startx = offsx;
      starty = offsy + (unit * (sz.tileszy + 2));
      os << "<rect x=\"" << startx << "\" y=\"" << starty << "\" ";
      os << "height=\"" << sz.tileszy << "\" width=\"" << sz.tileszx << "\" ";
      os << tilestyle(unit);
      os << "> ";
      os << "</rect>" << std::endl;

      starty += sz.tileszy - 2;
      startx += sz.tileszx + 1;
      os << "<text font-family=\"Verdana\" x=\"" << startx << "\" y=\"" << starty << "\" ";
      os << " fill=\"grey\" font-size=\"" << _fontsz_tiny << "\"";
      os << " >";
      os << "Unit " << unit << std::endl;
      os << "</text>" << std::endl;
    }
  }





  void draw_blocks(std::ostream & os,
                   const sizes & sz,
                   std::array<index_t, PatternT::ndim()> coords,
                   int dimx, int dimy) {
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

        int i_grid = i * sz.gridx;
        int j_grid = j * sz.gridy;

        os << "<rect x=\"" << i_grid << "\" y=\"" << j_grid << "\" ";
        os << "height=\"" << sz.blockszy << "\" width=\"" << sz.blockszx << "\" ";
        os << tilestyle(unit);
        os << "> ";
        os << "<!-- i=" << i << " j=" << j << "--> ";
        os << "</rect>" << std::endl;
      }
    }
  }

private:
  RGB color(dart_unit_t unit) {
    unsigned r = 0, g = 0, b = 0;
    switch (unit % 8) {
    case 0:
      r = 0x00;
      g = 0x72;
      b = 0xBD;
      break;
    case 1:
      r = 0xD9;
      g = 0x53;
      b = 0x19;
      break;
    case 2:
      r = 0xEB;
      g = 0xB1;
      b = 0x20;
      break;
    case 3:
      r = 0x7E;
      g = 0x2F;
      b = 0x8E;
      break;
    case 4:
      r = 0x77;
      g = 0xAC;
      b = 0x30;
      break;
    case 5:
      r = 0x4D;
      g = 0xBE;
      b = 0xEE;
      break;
    case 6:
      r = 0xA2;
      g = 0x14;
      b = 0x2F;
      break;
    case 7:
      r = 0x33;
      g = 0x6F;
      b = 0x45;
      break;
    }

    r += 20 * (unit / 8);
    g += 20 * (unit / 8);
    b += 20 * (unit / 8);

    return RGB(r % 255, g % 255, b % 255);
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
