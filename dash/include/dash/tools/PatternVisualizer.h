#ifndef DASH__PATTERN_VISUALIZER_H
#define DASH__PATTERN_VISUALIZER_H

#include <dash/dart/if/dart.h>

#include <sstream>
#include <array>
#include <iomanip>
#include <string>

#if 0
#define PATTERN_VISUALIZER_HSV
#endif

#ifdef PATTERN_VISUALIZER_HSV
#include <dash/tools/Colorspace.h>
#endif

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
class PatternVisualizer
{
private:
  typedef PatternVisualizer<PatternT>      self_t;
  typedef typename PatternT::index_type    index_t;

  struct sizes {
    int tileszx,
        tileszy;
    int blockszx,
        blockszy;
    int grid_width,
        grid_height;
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
      ss << "#" << std::hex << std::uppercase << std::setfill('0');
      ss << std::setw(2) << _r << std::setw(2) << _g << std::setw(2) << _b;
      return ss.str();
    }
  };

public:
  /**
   * Constructs the Pattern Visualizer with a pattern instance.
   *
   * The pattern instance is constant. For a different pattern a new
   * PatternVisualizer has to be constructed.
   */
  PatternVisualizer(const PatternT & pat,
                    /// An optional title
                    const std::string & title = "",
                    /// An optional describtion, currently not used
                    const std::string & descr = "")
  : _pattern(pat), _title(title), _descr(descr)
  {
    _tile_base_size  = 10;
    _block_base_size = 26;
    _fontsz_tiny     =  9;
    _fontsz          = 10;
    _fontsz_title    = 12;
  }

  PatternVisualizer() = delete;
  PatternVisualizer(const self_t & other) = delete;

  /**
   * Sets a description for the pattern.
   * Currently not used.
   */
  void set_description(const std::string & str) {
    _descr = str;
  }
  /**
   * Sets the title displayed above the pattern
   */
  void set_title(const std::string & str) {
    _title = str;
  }

  /**
   * Outputs the pattern as a svg over the given output stream.
   * This method should only be called by a single unit.
   */
  void draw_pattern(
      std::ostream & os,
      /// If this option is false every tile of the pattern is displayed seperate.
      /// If this option is true only the blocks (group of elements) get displayed.
      bool blocked_display = false,
      /// For higher dimensional patterns, defines which slice gets displayed
      std::array<index_t, PatternT::ndim()> coords = {},
      /// Defines which dimension gets displayed in x-direction
      int dimx = 1,
      /// Defines which dimension gets displayed in y-direction
      int dimy = 0) {
    sizes sz;
    sz.tileszx   = sz.tileszy = _tile_base_size;
    sz.blockszx  = sz.blockszy = _block_base_size;
    if(!blocked_display) {
        sz.grid_width  = _pattern.extent(dimx);
        sz.grid_height = _pattern.extent(dimy);
        sz.grid_base   = _tile_base_size + 2;
        sz.gridx       = sz.gridy   = sz.grid_base;
    } else {
        sz.grid_width  = _pattern.blockspec().extent(dimx);
        sz.grid_height = _pattern.blockspec().extent(dimy);
        sz.grid_base   = _block_base_size + 2;

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
    }

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
    os << "<g transform=\"translate(10,30)\">\n";
    draw_pane(os, blocked_display, sz, coords, dimx, dimy);
    os << "</g>\n";

    os << "</svg>" << std::endl;
  }

  /**
   * Draws a pane (svg group) containing axes, key, tiles/blocks
   * For the non blocked display (tiles) the local blocks and
   * the memory layout gets drawn, too.
   */
  void draw_pane(std::ostream & os, bool blocked_display,
                 const sizes & sz,
                 std::array<index_t, PatternT::ndim()> coords,
                 int dimx, int dimy) {
    os << "<g transform=\"translate(10,10)\">" << std::endl;
    draw_axes(os, sz, dimx, dimy);

    os << "<g transform=\"translate(4,4)\">" << std::endl;
    if(!blocked_display) {
        draw_local_blocks(os, sz, coords, dimx, dimy);
        draw_tiles(os, sz, coords, dimx, dimy);
        draw_local_memlayout(os, sz, dimx, dimy);
    } else {
        draw_blocks(os, sz, dimx, dimy);
    }
    os << "</g>" << std::endl;

    draw_key(os, sz, sz.grid_width * sz.gridx + 2*sz.grid_base, 0);
    os << "</g>" << std::endl;
  }

  /**
   * Draws the axes labeled with their dedicated dimension
   */
  void draw_axes(std::ostream & os,
                 const sizes & sz,
                 int dimx, int dimy,
                 int offsx = 0, int offsy = 0) {

    int startx = offsx;
    int starty = offsy;
    int lenx   = sz.grid_width  * sz.gridx + sz.grid_base;
    int leny   = sz.grid_height * sz.gridy + sz.grid_base;

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

    os << "<text x=\"" << startx + lenx/3 << "\" y=\"" << starty - _fontsz/2 << "\" ";
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

    os << "<text x=\"" << startx - _fontsz/2 << "\" y=\"" << starty + leny/3 << "\" ";
    os << " transform=\"rotate(-90," << startx - _fontsz/2 << "," << starty + leny/3 << ")\" ";
    os << " fill=\"grey\" font-size=\"" << _fontsz << "\" >";
    os << "Dimension " << dimy << std::endl;
    os << "</text>" << std::endl;
  }

  /**
   * Draws a list of units with their matching color
   */
  void draw_key(std::ostream & os,
                const sizes & sz,
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
      os << "<text x=\"" << startx << "\" y=\"" << starty << "\" ";
      os << " fill=\"grey\" font-size=\"" << _fontsz << "\"";
      os << " >";
      os << "Unit " << unit << std::endl;
      os << "</text>" << std::endl;
    }
  }

  /**
   * Draws the seperate tiles of the pattern
   */
  void draw_tiles(std::ostream & os,
                  const sizes & sz,
                  std::array<index_t, PatternT::ndim()> coords,
                  int dimx, int dimy) {
    for (int i = 0; i < _pattern.extent(dimx); i++) {
      for (int j = 0; j < _pattern.extent(dimy); j++) {

        os << "<rect x=\"" << (i * sz.gridx) << "\" y=\"" << (j * sz.gridy) << "\" ";
        os << "height=\"" << sz.tileszy << "\" width=\"" << sz.tileszx << "\" ";

        coords[dimx] = i;
        coords[dimy] = j;

        auto unit  = _pattern.unit_at(coords);
        auto loffs = _pattern.at(coords);

        os << tilestyle(unit);
        os << " tooltip=\"enable\" > ";
        os << " <title>Elem: (" << j << "," << i <<"),";
        os << " Unit " << unit;
        os << " Local offs. " << loffs;
        os << "</title>";

          //os << "<!-- i=" << i << " j=" << j << "--> ";
        os << "</rect>" << std::endl;
      }
    }
  }

  /**
   * Draws the blocks of the pattern
   */
  void draw_blocks(std::ostream & os,
                   const sizes & sz,
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

  /**
   * Draws the local blocks of the current unit (usually unit 0)
   */
  void draw_local_blocks(std::ostream & os,
                         const sizes & sz,
                         std::array<index_t, PatternT::ndim()> coords,
                         int dimx, int dimy) {

    std::array<index_t, PatternT::ndim()> block_coords = coords;
    std::array<index_t, PatternT::ndim()> block_begin_coords = coords;

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

        if( unit==0 ) {
          int i_grid = block.offset(dimx)*sz.gridx - 1;
          int j_grid = block.offset(dimy)*sz.gridy - 1;

          int width  = (block.extent(dimx)-1)*sz.gridx + sz.tileszx + 2;
          int height = (block.extent(dimy)-1)*sz.gridy + sz.tileszy + 2;

          os << "<rect x=\"" << i_grid << "\" y=\"" << j_grid << "\" ";
          os << "height=\"" << height << "\" width=\"" << width << "\" ";
          os << "style=\"fill:#999999;stroke-width:0\" >";
          os << "</rect>" << std::endl;
        }
      }
    }
  }

  /**
   * Draws the memory layout for the current unit (usually unit 0)
   */
  void draw_local_memlayout(std::ostream & os,
                            const sizes & sz,
                            int dimx, int dimy) {
    int startx, starty;
    int endx, endy;

    startx = starty = 0;
    for ( auto offset = 0; offset < _pattern.local_size(); offset++ ) {
      auto coords = _pattern.coords(_pattern.global(offset));

      endx = (coords[dimx] * sz.gridx) + sz.tileszx / 2;
      endy = (coords[dimy] * sz.gridy) + sz.tileszy / 2;

      if ( startx > 0 && starty > 0 ) {
        os << "<line x1=\"" << startx << "\" y1=\"" << starty << "\"";
        os << " x2=\"" << endx << "\" y2=\"" << endy << "\"";
        os << " style=\"stroke:#E0E0E0;stroke-width:1\"/>";
        os << " <!-- (" << offset << ") -->";
        os << std::endl;
      }

      os << "<circle cx=\"" << endx << "\" cy=\"" << endy << "\" r=\"1.5\" ";
      os << " style=\"stroke:#E0E0E0;stroke-width:1;fill:#E0E0E0\" />";
      os << std::endl;

      startx = endx;
      starty = endy;
    }

  }

private:
  #ifndef PATTERN_VISUALIZER_HSV
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
  #else
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
    hsv.s = 0.5 + 0.5 * unit_s_perc;
    hsv.v = 0.5 + 0.4 * unit_v_perc;

    auto rgb = dash::tools::color::hsv2rgb(hsv);
    int  r   = static_cast<int>(rgb.r * 255);
    int  g   = static_cast<int>(rgb.g * 255);
    int  b   = static_cast<int>(rgb.b * 255);

    return RGB(r,g,b);
  }
  #endif

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
