#ifndef DASH__HALO_HALOCOORDINATEACCESS_H
#define DASH__HALO_HALOCOORDINATEACCESS_H

#include <dash/halo/Halo.h> 

namespace dash {

namespace halo {

using namespace internal;

// forward declaration
template<typename HaloBlockT>
class CoordinateAccess;

template<typename CoordinateAccessT>
class CoordinateInnerAccess;

template<typename CoordinateInnerAccessT, dim_t CurrentDimension>
class DataInnerAccess {
private:
  using Self_t = DataInnerAccess<CoordinateInnerAccessT, CurrentDimension>;

  static constexpr auto NumDimensions    = CoordinateInnerAccessT::ndim();

public:
  using Offsets_t = typename CoordinateInnerAccessT::Offsets_t;
  using Element_t = typename CoordinateInnerAccessT::Element_t;
  using index_t = typename CoordinateInnerAccessT::index_t;

public:

  DataInnerAccess(const Offsets_t* offsets, Element_t* mem)
  : _offsets(offsets), _mem(mem) {
  }

  template<dim_t _CurrentDimension = CurrentDimension + 1>
  std::enable_if_t<(_CurrentDimension != NumDimensions), DataInnerAccess<CoordinateInnerAccessT, CurrentDimension+1>>
  operator[](index_t pos) {
    
    return DataInnerAccess<CoordinateInnerAccessT, CurrentDimension+1>(_offsets, _mem + pos * (*_offsets)[CurrentDimension]);
  }

  template<dim_t _CurrentDimension = CurrentDimension + 1>
  std::enable_if_t<(_CurrentDimension == NumDimensions), Element_t>&
  operator[](index_t pos) {
    return _mem[pos];
  }

private:
  const Offsets_t* _offsets;
  Element_t*       _mem;
};

template<typename CoordinateAccessT>
class CoordinateInnerAccess {
private:
  using Self_t = CoordinateInnerAccess<CoordinateAccessT>;

  static constexpr auto NumDimensions    = CoordinateAccessT::ndim();
  static constexpr auto MemoryArrange    = CoordinateAccessT::memory_order();

public:
  using Element_t    = typename CoordinateAccessT::Element_t;
  using index_t      = typename CoordinateAccessT::index_t;
  using ViewSpec_t   = typename CoordinateAccessT::ViewSpec_t;
  using Offsets_t    = typename CoordinateAccessT::Offsets_t;
  using DataInnerAccess_t = DataInnerAccess<Self_t, 0>;
  using ViewRange_t     = typename CoordinateAccessT::ViewRange_t;
  using AllViewRanges_t = typename CoordinateAccessT::AllViewRanges_t;

public:

  CoordinateInnerAccess(const AllViewRanges_t& ranges, Element_t* mem, const Offsets_t* offsets)
  : _ranges(ranges)
  , _mem(mem)
  , _offsets(offsets)
  , _data_access(DataInnerAccess_t(_offsets, _mem)) {
  }

  static constexpr decltype(auto) ndim() { return NumDimensions; }

  static constexpr decltype(auto) memory_order() { return MemoryArrange; }

  ViewRange_t range_dim(dim_t dim) {
    return _ranges(dim);
  }

  AllViewRanges_t ranges() {
    return _ranges;
  }

  decltype(auto) operator[] (index_t pos) {
    return _data_access[pos];
  }

  decltype(auto) operator[] (index_t pos) const {
    return _data_access[pos];
  }

private:
  AllViewRanges_t   _ranges;
  Element_t*        _mem;
  const Offsets_t*  _offsets;
  DataInnerAccess_t _data_access;
};


template<typename CoordinateAccessT, dim_t CurrentDimension>
class DataAccess {
private:
  using Self_t = DataAccess<CoordinateAccessT, CurrentDimension>;

  static constexpr auto NumDimensions    = CoordinateAccessT::ndim();
  static constexpr auto MemoryArrange    = CoordinateAccessT::memory_order();

  using RegCoords_t = RegionCoords<NumDimensions>;
  static constexpr auto RegIndexCenter = RegCoords_t::center_index();

public:
  using Element_t = typename CoordinateAccessT::Element_t;
  using index_t = typename CoordinateAccessT::index_t;
  using Coords_t     = typename CoordinateAccessT::Coords_t;

public:

  DataAccess(const CoordinateAccessT* access, Element_t* mem, const Coords_t& coords, region_index_t reg_index, bool halo)
  : _access(access), _mem(mem), _coords(coords), _reg_index(reg_index), _halo(halo) {
  }

  template<dim_t _CurrentDimension = CurrentDimension + 1>
  std::enable_if_t<(_CurrentDimension != NumDimensions), DataAccess<CoordinateAccessT, CurrentDimension+1>>
  operator[](index_t pos) {
    _coords[CurrentDimension] = pos;
    _reg_index *= REGION_INDEX_BASE;
    if(_halo || pos < 0 || pos >= static_cast<index_t>(_access->_view_local->extent(CurrentDimension))) {
      if(pos < 0) {
        return DataAccess<CoordinateAccessT, CurrentDimension+1>(_access, _mem + pos * _access->_offsets[CurrentDimension], _coords, _reg_index, true);
      }
      
      if(pos >= static_cast<index_t>(_access->_view_local->extent(CurrentDimension))) {
        return DataAccess<CoordinateAccessT, CurrentDimension+1>(_access, _mem + pos * _access->_offsets[CurrentDimension], _coords, _reg_index + 2, true);
      }

      return DataAccess<CoordinateAccessT, CurrentDimension+1>(_access, _mem + pos * _access->_offsets[CurrentDimension], _coords, _reg_index + 1, true);
    }
    
    return DataAccess<CoordinateAccessT, CurrentDimension+1>(_access, _mem + pos * _access->_offsets[CurrentDimension], _coords, _reg_index + 1, false);
  }

  template<dim_t _CurrentDimension = CurrentDimension + 1>
  std::enable_if_t<(_CurrentDimension == NumDimensions), Element_t>&
  operator[](index_t pos) {
    if(_halo || pos < 0 || pos >= static_cast<index_t>(_access->_view_local->extent(CurrentDimension))) {
      _reg_index *= REGION_INDEX_BASE;
      
      if(pos >= 0) {
        ++_reg_index;
      } 
      
      if(pos >= static_cast<index_t>(_access->_view_local->extent(CurrentDimension))) {
        ++_reg_index;
      }

      _coords[CurrentDimension] = pos;
      auto halo_memory = _access->_halo_memory;
      halo_memory->to_halo_mem_coords(_reg_index, _coords);

      return *(halo_memory->first_element_at(_reg_index)
            + halo_memory->offset(_reg_index, _coords));
    }
    
    return _mem[pos];
  }

private:
  const CoordinateAccessT* _access;
  Element_t*               _mem;
  Coords_t                 _coords;
  region_index_t           _reg_index;
  bool                     _halo;
};

template<typename CoordinateAccessT>
class CoordinateHaloAccess {
private:
  using Self_t = CoordinateHaloAccess<CoordinateAccessT>;
  
  static constexpr auto NumDimensions    = CoordinateAccessT::ndim();
  static constexpr auto MemoryArrange    = CoordinateAccessT::memory_order();

public:
  using Element_t    = typename CoordinateAccessT::Element_t;
  using index_t      = typename CoordinateAccessT::index_t;
  using ViewSpec_t   = typename CoordinateAccessT::ViewSpec_t;
  using Offsets_t    = typename CoordinateAccessT::Offsets_t;
  using DataAccess_t = DataAccess<CoordinateAccessT, 0>;
  using ViewRange_t     = typename CoordinateAccessT::ViewRange_t;
  using AllViewRanges_t = typename CoordinateAccessT::AllViewRanges_t;
  using AllBndViewRanges = std::vector<AllViewRanges_t>;

  using HaloBlock_t  = typename CoordinateAccessT::HaloBlock_t;
  using HaloMemory_t = typename CoordinateAccessT::HaloMemory_t;
  using Coords_t     = typename CoordinateAccessT::Coords_t;

public:
  CoordinateHaloAccess(const CoordinateAccessT* _access)
  : _access(_access)
  , _data_access(_access->_data_access)
  , _ranges(set_ranges(_access->_halo_block)) {
  }

  static constexpr decltype(auto) ndim() { return NumDimensions; }

  static constexpr decltype(auto) memory_order() { return MemoryArrange; }

  AllBndViewRanges ranges() {
    return _ranges;
  }

  decltype(auto) operator[] (index_t pos) {
    return _data_access[pos];
  }

  decltype(auto) operator[] (index_t pos) const {
    return _data_access[pos];
  }

private:

  AllBndViewRanges set_ranges(const HaloBlock_t* halo_block) const {
    AllBndViewRanges all_ranges;
    const auto& bnd_views = halo_block->boundary_views();
    all_ranges.reserve(bnd_views.size());

    for(const auto& view : bnd_views) {
      AllViewRanges_t ranges;
      for(dim_t d = 0; d < NumDimensions; ++d) {
        ranges[d] = {static_cast<index_t>(view.offset(d)), 
                    static_cast<index_t>(view.offset(d) + view.extent(d))};
      }
      all_ranges.push_back(ranges);
    }

    return all_ranges;
  }


private:
  const CoordinateAccessT* _access;
  DataAccess_t             _data_access;
  AllBndViewRanges         _ranges;
};

template<typename HaloBlockT>
class CoordinateAccess {
private:
  using Self_t = CoordinateAccess<HaloBlockT>;
  using Pattern_t = typename HaloBlockT::Pattern_t;

  static constexpr auto NumDimensions    = Pattern_t::ndim();
  static constexpr auto MemoryArrange    = Pattern_t::memory_order();

  template <typename _CA>
  friend class CoordinateInnerAccess;

  template <typename _CA>
  friend class CoordinateHaloAccess;

  template <typename _CA, dim_t CurrentDim>
  friend class DataInnerAccess;

  template <typename _CA, dim_t CurrentDim>
  friend class DataAccess;

public:
  using Element_t    = typename HaloBlockT::Element_t;
  using HaloBlock_t  = HaloBlockT;
  using HaloMemory_t = HaloMemory<HaloBlock_t>;
  using index_t      = typename std::make_signed<typename Pattern_t::index_type>::type;
  using uindex_t     = typename std::make_unsigned<index_t>::type;
  using ViewSpec_t   = typename HaloBlockT::ViewSpec_t;
  using Offsets_t    = std::array<index_t, NumDimensions>;
  using DataAccess_t = DataAccess<Self_t, 0>;
  using Coords_t     = typename HaloMemory_t::ElementCoords_t;
  using ViewRange_t  = ViewRange<index_t>;
  using AllViewRanges_t = std::array<ViewRange_t, NumDimensions>;

  using CoordInnerAcc_t = CoordinateInnerAccess<Self_t>;
  using CoordHaloAcc_t = CoordinateHaloAccess<Self_t>;

public:
  CoordinateAccess(const HaloBlockT*   haloblock,
                   Element_t*          local_memory,
                   HaloMemory_t*       halomemory)
  : _halo_block(haloblock)
  , _local_memory(local_memory)
  , _halo_memory(halomemory)
  , _view_local(&(haloblock->view_local()))
  , _offsets(set_offsets())
  , _data_access(DataAccess_t(this, _local_memory, Coords_t(), 0, false))
  , _ranges(set_ranges(_halo_block->view_inner_with_boundaries()))
  , _ranges_local(set_ranges(_halo_block->view_local()))
  , _ranges_halo(set_ranges_halo(_halo_block->view_inner_with_boundaries()))
  , inner(set_ranges(_halo_block->view_inner()), _local_memory, &_offsets)
  , boundary(this) {
  }

  static constexpr decltype(auto) ndim() { return NumDimensions; }

  static constexpr decltype(auto) memory_order() { return MemoryArrange; }

  ViewRange_t range_dim(dim_t dim) {
    return _ranges(dim);
  }

  AllViewRanges_t ranges() {
    return _ranges;
  }

  ViewRange_t range_local_dim(dim_t dim) {
    return _ranges_local(dim);
  }

  AllViewRanges_t ranges_local() {
    return _ranges_local;
  }

  ViewRange_t range_halo_dim(dim_t dim) {
    return _ranges_halo(dim);
  }

  AllViewRanges_t ranges_halo() {
    return _ranges_halo;
  }

  decltype(auto) operator[] (index_t pos) {
    return _data_access[pos];
  }

  decltype(auto) operator[] (index_t pos) const {
    return _data_access[pos];
  }

private:
  Offsets_t set_offsets() {
    Offsets_t offsets;
    if(MemoryArrange == ROW_MAJOR) {
      offsets[NumDimensions - 1] = 1;
      for(dim_t d = NumDimensions - 1; d > 0;) {
        --d;
        offsets[d] = 1;
        for(dim_t d_tmp = d + 1; d_tmp < NumDimensions; ++d_tmp) {
          offsets[d] *= _view_local->extent(d_tmp);
        }
      }
    } else {
      offsets[0] = 1;
      for(dim_t d = 1; d < NumDimensions; ++d) {
        offsets[d] = 1;
        for(dim_t d_tmp = 0; d_tmp < d; ++d_tmp) {
          offsets[d] *= _view_local->extent(d_tmp);
        }
      }
    }

    return offsets;
  }

  AllViewRanges_t set_ranges(ViewSpec_t view) const {
    AllViewRanges_t ranges;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      ranges[d] = {static_cast<index_t>(view.offset(d)), 
                   static_cast<index_t>(view.offset(d) + view.extent(d))};
    }

    return ranges;
  }

  AllViewRanges_t set_ranges_halo(ViewSpec_t view) const {
    AllViewRanges_t ranges;
    for(dim_t d = 0; d < NumDimensions; ++d) {
      const auto& ext_max = _halo_block->halo_extension_max(d);
      ranges[d] = {static_cast<index_t>(view.offset(d)) - ext_max.first, 
                   static_cast<index_t>(view.offset(d) + view.extent(d)) + ext_max.second};
    }

    return ranges;
  }


private:
  const HaloBlock_t* _halo_block;
  Element_t*         _local_memory;
  HaloMemory_t*      _halo_memory;
  const ViewSpec_t*  _view_local;
  Offsets_t          _offsets;
  DataAccess_t       _data_access;
  AllViewRanges_t    _ranges;
  AllViewRanges_t    _ranges_local;
  AllViewRanges_t    _ranges_halo;

public:
  CoordInnerAcc_t inner;
  CoordHaloAcc_t  boundary;

};

}  // namespace halo

}  // namespace dash
#endif  // DASH__HALO_HALOCOORDINATEACCESS_H