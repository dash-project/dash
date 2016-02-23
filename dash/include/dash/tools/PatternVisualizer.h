#ifndef DASH__PATTERN_VISUALIZER_H
#define DASH__PATTERN_VISUALIZER_H

#include <dash/dart/if/dart.h>

#include <sstream>
#include <string>
#include <iomanip>
#include <string>

namespace dash {
namespace tools {

/**
 *
 * Take a generic pattern instance and visualize it as an SVG image.
 * The visualization is limited to two dimensions at the moment, but
 * for higher-dimensional patterns any two dimensions can be specified
 * for visualization..
 */
template<typename PatternT>
class PatternVisualizer
{
private:
  typedef typename PatternT::index_type index_t;

private:
  const PatternT & _pattern;
  int _tileszx, _tileszy;
  int _gridx, _gridy;
  std::string _title;
  std::string _descr;
  int _fontsz;
  int _fontsz_title;

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
  PatternVisualizer(const PatternT & pat,
                    const std::string & title = "",
                    const std::string & descr = "")
  : _pattern(pat), _title(title), _descr(descr)
  {
    _gridx = _gridy = 12;
    _tileszx = _tileszy = 10;
    _fontsz = 9;
    _fontsz_title = 11;
  }

  PatternVisualizer() = delete;
  PatternVisualizer(const PatternVisualizer<PatternT> & other) = delete;

  void set_description(const std::string & str) {
    _descr = str;
  }
  void set_title (const std::string & str) {
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
    draw_memlayout(os, coords, dimx, dimy);
    draw_key(os, dimx, dimy, (2 + _pattern.extent(dimx))*_gridx, 0);
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
    endx = startx + (1 + _pattern.extent(dimx)) * _gridx;
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
    endy = starty + (1 + _pattern.extent(dimy)) * _gridy;

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

  void draw_key(std::ostream & os,
                int dimx, int dimy,
                int offsx = 0, int offsy = 0) {
    int startx, starty;

    startx = offsx;
    for (int unit = 0; unit < _pattern.num_units(); unit++) {
      startx = offsx;
      starty = offsy + (unit * _gridy);
      os << "<rect x=\"" << startx << "\" y=\"" << starty << "\" ";
      os << "height=\"" << _tileszy << "\" width=\"" << _tileszx << "\" ";
      os << tilestyle(unit);
      os << "> ";
      os << "</rect>" << std::endl;

      starty += _tileszy - 2;
      startx += _tileszx + 1;
      os << "<text x=\"" << startx << "\" y=\"" << starty << "\" ";
      os << " fill=\"grey\" font-size=\"" << _fontsz << "\"";
      os << " >";
      os << "Unit " << unit << std::endl;
      os << "</text>" << std::endl;
    }
  }

  void draw_tiles(std::ostream & os,
                  std::array<index_t, PatternT::ndim()> coords,
                  int dimx, int dimy) {
    for (int i = 0; i < _pattern.extent(dimx); i++) {
      for (int j = 0; j < _pattern.extent(dimy); j++) {
        os << "<rect x=\"" << (i * _gridx) << "\" y=\"" << (j * _gridy) << "\" ";
        os << "height=\"" << _tileszy << "\" width=\"" << _tileszx << "\" ";

        coords[dimx] = i;
        coords[dimy] = j;

        os << tilestyle(_pattern.unit_at(coords));
        os << "> ";
        os << "<!-- i=" << i << " j=" << j << "--> ";
        os << "</rect>" << std::endl;
      }
    }
  }

  void draw_memlayout(std::ostream & os,
                      std::array<index_t, PatternT::ndim()> coords,
                      int dimx, int dimy) {
    int startx, starty;
    int endx, endy;

    startx = starty = 0;
    for ( auto offset = 0; offset < _pattern.local_size(); offset++ ) {
      auto coords = _pattern.coords(_pattern.global(offset));

      endx = (coords[dimx] * _gridx) + _tileszx / 2;
      endy = (coords[dimy] * _gridy) + _tileszy / 2;

      if ( startx > 0 && starty > 0 ) {
        os << "<line x1=\"" << startx << "\" y1=\"" << starty << "\"";
        os << " x2=\"" << endx << "\" y2=\"" << endy << "\"";
        os << " style=\"stroke:#E0E0E0;stroke-width:1\"/>";
        os << " <!-- (" << offset << ") -->";
        os << std::endl;
      }
      startx = endx;
      starty = endy;
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

#endif // DASH__PATTERN_VISUALIZER_H
