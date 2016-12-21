#ifndef DASH__EXPERIMENTAL__HALOMATRIXITERATOR_H__INCLUDED
#define DASH__EXPERIMENTAL__HALOMATRIXITERATOR_H__INCLUDED

#include <dash/Types.h>

#include <dash/experimental/Halo.h>

#include <vector>


namespace dash {
namespace experimental {

enum class StencilViewScope: std::uint8_t{
  INNER,
  BOUNDARY,
  ALL
};

template <
  typename         ElementT,
  typename         PatternT,
  StencilViewScope Scope >
class HaloMatrixIterator
: public std::iterator<
    std::random_access_iterator_tag,
    ElementT,
    typename PatternT::index_type,
    ElementT*,
    ElementT > // TODO: Clarify: Why no reference type?
{
public:
  using index_type  = typename PatternT::index_type;
  using size_type   = typename PatternT::size_type;

  using reference       = ElementT;
  using const_reference = const reference;
  using pointer         = ElementT*;
  using const_pointer   = const pointer;

private:
  static constexpr dim_t NumDimensions = PatternT::ndim();
  static constexpr MemArrange MemoryArrange = PatternT::memory_order();

  using self_t         = HaloMatrixIterator<ElementT, PatternT, Scope>;
  using GlobMem_t      = GlobMem<ElementT, dash::allocator::CollectiveAllocator<ElementT>>;
  using HaloBlock_t    = HaloBlock<ElementT, PatternT>;
  using HaloSpec_t     = HaloSpec<NumDimensions>;
  using HaloMemory_t   = HaloMemory<HaloBlock_t>;
  using viewspec_t     = typename PatternT::viewspec_type;
  using offset_t       = typename HaloSpec_t::offset_t;
  using local_layout_t = CartesianIndexSpace<NumDimensions, MemoryArrange, index_type>;

public:

  HaloMatrixIterator(const HaloBlock_t & haloblock, HaloMemory_t & halomemory,
      index_type idx)
  : _haloblock(haloblock), _halomemory(halomemory), _local_memory(_haloblock.globmem().lbegin()),
    _pattern(_haloblock.pattern()), _halospec(_haloblock.halospec()), 
    _local_layout(_pattern.local_memory_layout()), _idx(idx)

  {
    if(Scope == StencilViewScope::INNER)
      setViewLocal(_haloblock.view_inner());

    if(Scope == StencilViewScope::ALL)
      setViewLocal(_haloblock.view_save());

    if(Scope == StencilViewScope::BOUNDARY)
      setViewLocal(_haloblock.view());

    if(Scope == StencilViewScope::BOUNDARY)
      _size = _haloblock.boundary_size();
    else
      _size = _view_local.size();

    setCoords();
  }

  /**
   * Copy constructor.
   */
  HaloMatrixIterator(const self_t & other) = default;

  /**
   * Assignment operator.
   *
   * \see DashGlobalIteratorConcept
   */
  self_t & operator=(const self_t & other) = default;

  /**
   * The number of dimensions of the iterator's underlying pattern.
   *
   * \see DashGlobalIteratorConcept
   */
  static constexpr dim_t ndim()
  {
    return NumDimensions;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const
  {
    return _local_memory[_local_layout.at(_coords)];
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   *
   * \see DashGlobalIteratorConcept
   */
  reference operator[](index_type idx) const
  {
    auto coords = setCoords(idx);
    return _local_memory[_local_layout.at(coords)];
  }

  index_type rpos() const
  {
    return _idx;
  }

  index_type lpos() const
  {
    return _local_layout.at(_coords);
  }

  ElementT halo_value(dim_t dim, offset_t offset)
  {
    //TODO: is the given offset in halospec range?


    if(Scope == StencilViewScope::INNER)
    {
        auto halo_coords = _coords;
        halo_coords[dim] += offset;

        return _local_memory[_local_layout.at(halo_coords)];
    }
    else
    {
      auto diff = _coords[dim] + offset;
      auto halo_coords = _coords;
      if(diff >= 0 && diff < _haloblock.view().extent(dim))
      {
        halo_coords[dim] += offset;

        return _local_memory[_local_layout.at(halo_coords)];
      }
      else
      {
        HaloRegion halo_region = HaloRegion::MINUS;

        if(diff < 0)
        {
          halo_coords[dim] = std::abs(_halospec.halo_offset(dim).minus) + offset;
        }
        else
        {
          halo_coords[dim] = offset - 1;
          halo_region = HaloRegion::PLUS;
        }

        const auto & extents = _haloblock.halo_region(dim, halo_region).region_view().extents();
        size_type off = 0;

        if(MemoryArrange == ROW_MAJOR)
        {
          off = halo_coords[0];
          for(auto d = 1; d < NumDimensions; ++d)
            off = off * extents[d] + halo_coords[d];
        }
        else
        {
          off = halo_coords[NumDimensions - 1];
          for(auto d = NumDimensions - 2; d >= 0; --d)
            off = off * extents[d] + halo_coords[d];
        }
        //std::cout << "off=" << off << ", dim=" << dim << std::endl;
        return *(_halomemory.haloPos(dim, halo_region) + off);
      }
    }
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++()
  {
    ++_idx;
    setCoords();

    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t result = *this;
    ++_idx;
    setCoords();

    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    --_idx;
    setCoords();

    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t result = *this;
    --_idx;
    setCoords();

    return result;
  }

  self_t & operator+=(index_type n)
  {
    _idx += n;
    setCoords();

    return *this;
  }

  self_t & operator-=(index_type n)
  {
    _idx -= n;
    setCoords();

    return *this;
  }

  self_t operator+(index_type n) const
  {
    self_t res{*this};
    res += n;

    return res;
  }

  self_t operator-(index_type n) const
  {
    self_t res{*this};
    res -= n;

    return res;
  }

  /*index_type operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }*/

  bool operator<(const self_t & other) const
  {
    return compare(other, std::less<index_type>());
  }

  bool operator<=(const self_t & other) const
  {
    return compare(other, std::less_equal<index_type>());
  }

  bool operator>(const self_t & other) const
  {
    return compare(other, std::greater<index_type>());
  }

  bool operator>=(const self_t & other) const
  {
    return compare(other, std::greater_equal<index_type>());
  }

  bool operator==(const self_t & other) const
  {
    return compare(other, std::equal_to<index_type>());
  }

  bool operator!=(const self_t & other) const
  {
    return compare(other, std::not_equal_to<index_type>());
  }

  const PatternT & pattern() const
  {
    return _pattern;
  }

private:
  /**
   * Compare position of this global iterator to the position of another
   * global iterator with respect to viewspec projection.
   */
  template<typename GlobIndexCmpFunc>
  bool compare( const self_t & other, const GlobIndexCmpFunc & gidx_cmp) const
  {
#if __REMARK__
    // Usually this is a best practice check, but it's an infrequent case
    // so we rather avoid this comparison:
    if (this == &other) {
      return true;
    }
#endif
    if (&_view_local == &(other._view_local) ||
        _view_local == other._view_local)
    {
      return gidx_cmp(_idx, other._idx);
    }
  // TODO not the best solution
    return false;
  }
  void setViewLocal(const viewspec_t & view_tmp)
  {
    if(Scope == StencilViewScope::BOUNDARY)
    {
      const auto & bnd_elems = _haloblock.boundary_elements();
      _bnd_elements.reserve(bnd_elems.size());
      const auto & view_offs = view_tmp.offsets();
      for(const auto & region : bnd_elems)
      {
        auto off = region.offsets();
        for(int d = 0; d < NumDimensions; ++d)
          off[d] -= view_offs[d];

        _bnd_elements.push_back(viewspec_t(off, region.extents()));
      }

      _view_local = viewspec_t(view_tmp.extents());
    }
    else
    {
      const auto & view_offsets = _haloblock.view().offsets();
      auto off = view_tmp.offsets();
      for(int d = 0; d < NumDimensions; ++d)
        off[d] -= view_offsets[d];

      _view_local = viewspec_t(off, view_tmp.extents());
    }

  }

  inline void setCoords()
  {
      _coords = setCoords(_idx);
  }

  std::array<index_type, NumDimensions> setCoords(index_type idx) const
  {
    if (Scope == StencilViewScope::BOUNDARY) {
      auto local_idx = idx;
      for (const auto & region : _bnd_elements)
      {
        if (local_idx < region.size()) {
          return _local_layout.coords(local_idx, region);
        }
        local_idx -= region.size();
      }
      // TODO return value for idx >= size
      DASH_ASSERT("idx >= size not implemented yet");
      return std::array<index_type, NumDimensions> { };
    } else {
      return _local_layout.coords(_idx, _view_local);
    }
  }

private:
  const HaloBlock_t &                _haloblock;
  HaloMemory_t &                     _halomemory;
  const ElementT *                   _local_memory;
  const PatternT &                   _pattern;
  const HaloSpec_t &                 _halospec;
  viewspec_t                         _view_local;
  std::vector<viewspec_t>            _bnd_elements;

  const local_layout_t &             _local_layout;
  index_type                         _idx{0};
  index_type                         _size{0};
  dart_unit_t                        _myid;

  std::array<index_type, NumDimensions> _coords;

}; // class HaloMatrixIterator

} // namespace experimental
} // namespace dash

#endif  // DASH__EXPERIMENTAL__HALOMATRIXITERATOR_H__INCLUDED

