#ifndef DASH__HALO_HALOMEMORY_H
#define DASH__HALO_HALOMEMORY_H

#include <dash/halo/Types.h>
#include<dash/halo/Region.h>

#include <dash/Array.h>

namespace dash {

namespace halo {

using namespace internal;

/**
 * Mangages the memory for all halo regions provided by the given
 * \ref HaloBlock
 */
template <typename HaloBlockT>
class HaloMemory {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  using RegionCoords_t = RegionCoords<NumDimensions>;
  using Pattern_t      = typename HaloBlockT::Pattern_t;
  using ViewSpec_t     = typename HaloBlockT::ViewSpec_t;
  using extent_t       = typename ViewSpec_t::size_type;
  static constexpr auto MemoryArrange = Pattern_t::memory_order();

public:
  using Element_t = typename HaloBlockT::Element_t;
  using ElementCoords_t =
    std::array<typename Pattern_t::index_type, NumDimensions>;
  using HaloBuffer_t   = std::vector<Element_t>;
  using pattern_size_t = typename Pattern_t::size_type;

  using iterator       = typename HaloBuffer_t::iterator;
  using const_iterator = const iterator;

  using MemRange_t = std::pair<iterator, iterator>;

public:
  /**
   * Constructor
   */
  HaloMemory(const HaloBlockT& haloblock) : _haloblock(haloblock) {
    _halobuffer.resize(haloblock.halo_size());
    auto it = _halobuffer.begin();
    std::fill(_halo_offsets.begin(), _halo_offsets.end(), _halobuffer.end());
    for(const auto& region : haloblock.halo_regions()) {
      _halo_offsets[region.index()] = it;
      it += region.size();
    }
  }

  /**
   * Iterator to the first halo element for the given region index
   * \param index halo region index
   * \return Iterator to the first halo element. If no region exists the
   *         end iterator will be returned.
   */
  iterator first_element_at(region_index_t index) {
    return _halo_offsets[index];
  }

  /**
   * Returns the range of all halo elements for the given region index.
   * \param index halo region index
   * \return Pair of iterator. First points ot the beginning and second to the
   *         end.
   */
  MemRange_t range_at(region_index_t index) {
    auto it = _halo_offsets[index];
    if(it == _halobuffer.end())
      return std::make_pair(it, it);

    auto* region = _haloblock.halo_region(index);

    DASH_ASSERT_MSG(
      region != nullptr,
      "HaloMemory manages memory for a region that seemed to be empty.");

    return std::make_pair(it, it + region->size());
  }

  /**
   * Returns an iterator to the first halo element
   */
  iterator begin() { return _halobuffer.begin(); }

  /**
   * Returns a const iterator to the first halo element
   */
  const_iterator begin() const { return _halobuffer.begin(); }

  /**
   * Returns an iterator to the end of the halo elements
   */
  iterator end() { return _halobuffer.end(); }

  /**
   * Returns a const iterator to the end of the halo elements
   */
  const_iterator end() const { return _halobuffer.end(); }

  /**
   * Container storing all halo elements
   *
   * \return Reference to the container storing all halo elements
   */
  const HaloBuffer_t& buffer() const { return _halobuffer; }

  /**
   * Converts coordinates to halo memory coordinates for a given
   * region index and returns true if the coordinates are valid and
   * false if not.
   */
  bool to_halo_mem_coords_check(const region_index_t region_index,
                                ElementCoords_t&     coords) const {
    const auto& extents =
      _haloblock.halo_region(region_index)->view().extents();
    for(auto d = 0; d < NumDimensions; ++d) {
      if(coords[d] < 0)
        coords[d] += extents[d];
      else if(static_cast<extent_t>(coords[d]) >= _haloblock.view().extent(d))
        coords[d] -= _haloblock.view().extent(d);

      if(static_cast<extent_t>(coords[d]) >= extents[d] || coords[d] < 0)
        return false;
    }

    return true;
  }

  /**
   * Converts coordinates to halo memory coordinates for a given region index.
   */
  void to_halo_mem_coords(const region_index_t region_index,
                          ElementCoords_t&     coords) const {
    const auto& extents =
      _haloblock.halo_region(region_index)->view().extents();
    for(dim_t d = 0; d < NumDimensions; ++d) {
      if(coords[d] < 0) {
        coords[d] += extents[d];
        continue;
      }

      if(static_cast<extent_t>(coords[d]) >= _haloblock.view().extent(d))
        coords[d] -= _haloblock.view().extent(d);
    }
  }

  /*
   * Retuns the offset for a given region index and coordinates within the
   * region.
   */
  pattern_size_t offset(const region_index_t   region_index,
                        const ElementCoords_t& coords) const {
    const auto& extents =
      _haloblock.halo_region(region_index)->view().extents();
    pattern_size_t off = 0;
    if(MemoryArrange == ROW_MAJOR) {
      off = coords[0];
      for(dim_t d = 1; d < NumDimensions; ++d)
        off = off * extents[d] + coords[d];
    } else {
      off = coords[NumDimensions - 1];
      for(dim_t d = NumDimensions - 1; d > 0;) {
        --d;
        off = off * extents[d] + coords[d];
      }
    }

    return off;
  }

private:
  const HaloBlockT&              _haloblock;
  HaloBuffer_t                   _halobuffer;
  std::array<iterator, RegionsMax> _halo_offsets{};
};  // class HaloMemory

template <typename HaloBlockT>
class HaloPackBuffer {
private:
  static constexpr auto NumDimensions = HaloBlockT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  using RegionCoords_t = RegionCoords<NumDimensions>;
  using Team_t         = dash::Team;
  using Pattern_t      = typename HaloBlockT::Pattern_t;
  using signed_pattern_size_t =
    typename std::make_signed<typename Pattern_t::size_type>::type;
  using ViewSpec_t     = typename Pattern_t::viewspec_type;


  
  static constexpr auto MemoryArrange = Pattern_t::memory_order();
  // value not related to array index
  static constexpr auto FastestDim    =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;
  static constexpr auto ContiguousDim    =
    MemoryArrange == ROW_MAJOR ? 1 : NumDimensions;

public:
  using Element_t = typename HaloBlockT::Element_t;
  using ElementCoords_t =
    std::array<typename Pattern_t::index_type, NumDimensions>;
  using HaloBuffer_t   = dash::Array<Element_t>;
  using HaloSignalBuffer_t   = dash::Array<bool>;
  using pattern_size_t = typename Pattern_t::size_type;

public:
  /**
   * Constructor
   */
  HaloPackBuffer(const HaloBlockT& halo_block, Element_t* local_memory, Team_t& team)
  : _halo_block(halo_block),
    _local_memory(local_memory),
    _num_halo_elems(num_halo_elems()),
    _halo_buffer(_num_halo_elems * team.size(), team),
    _signal_buffer(RegionsMax * team.size(), team) {

    init_block_data();
    for(auto& signal : _signal_buffer.local) {
      signal = 0;
    }

    _signal = 1;
  }

  void pack() {
    region_index_t handle_pos = 0;
    for(auto r = 0; r < RegionsMax; ++r) {
      const auto& update_data = _halo_update_data[r];
      if(!update_data.put_data.needs_signal) {
        continue;
      }

      if(update_data.pack_data.needs_packing) {
        auto buffer_offset = _halo_buffer.lbegin() + update_data.pack_data.buffer_offset;
        for(auto& offset : update_data.pack_data.block_offs) {
          auto block_begin = _local_memory + offset;
          std::copy(block_begin, block_begin + update_data.pack_data.block_len, buffer_offset);
          buffer_offset += update_data.pack_data.block_len;
        }
      }

      dash::internal::put_handle(update_data.put_data.signal_gptr, &_signal, 1, &_signal_handles[handle_pos]);
      ++handle_pos;
    }

    dart_waitall_local(_signal_handles.data(), _signal_handles.size());
  }

  void pack(region_index_t region_index) {
    const auto& update_data = _halo_update_data[region_index];
    if(!update_data.put_data.needs_signal) {
      return;
    }

    if(update_data.pack_data.needs_packing) {
      auto buffer_offset = _halo_buffer.lbegin() + update_data.pack_data.buffer_offset;
      for(auto& offset : update_data.pack_data.block_offs) {
        auto block_begin = _local_memory + offset;
        std::copy(block_begin, block_begin + update_data.pack_data.block_len, buffer_offset);
        buffer_offset += update_data.pack_data.block_len;
      }
    }

    dash::internal::put_blocking(update_data.put_data.signal_gptr, &_signal, 1);
  }

  dart_gptr_t buffer_region(region_index_t region_index) {
    return _halo_update_data[region_index].get_data.halo_gptr;
  }

  void update_ready(region_index_t region_index) {
    auto& get_data = _halo_update_data[region_index].get_data;
    if(!get_data.awaits_signal) {
      return;
    }


    bool signal = false;
    while(!signal) {
      dash::internal::get_blocking(get_data.signal_gptr, &signal, 1);
    }
    _signal_buffer.lbegin()[region_index] = 0;
  }

  void print_block_data() {
    std::cout << "BlockData:\n";
    for(auto r = 0; r < RegionsMax; ++r) {
      std::cout << "region [" << r << "] {";
      for(auto& offset : _halo_update_data[r].pack_data.block_offs) {
        std::cout << " (" << offset << "," << _halo_update_data[r].pack_data.block_len << ")";
      }
      std::cout << " }\n";
    }
    std::cout << std::endl;
  }

  void print_buffer_data() {
    std::cout << "bufferData: { ";
    for(auto& elem : _halo_buffer.local) {
      std::cout << elem << ",";
    }
    std::cout << " }" << std::endl;
  }

  void print_signal_data() {
    std::cout << "signalData: { ";
    for(auto& elem : _signal_buffer.local) {
      std::cout << elem << ",";
    }
    std::cout << " }" << std::endl;
  }

  void print_pack_data() {
    for(auto r = 0; r < RegionsMax; ++r) {
      auto& data = _halo_update_data[r];
      std::cout << "Halo Update Data (" << r << ")\n"
                << "  Get Data:\n"
                << "    awaits signal: " << data.get_data.awaits_signal << "\n"
                << "    signal gptr: " << " uid: " << data.get_data.signal_gptr.unitid << " off: " << data.get_data.signal_gptr.addr_or_offs.offset << "\n"
                << "    halo gptr: " << " uid: " << data.get_data.halo_gptr.unitid << " off: " << data.get_data.halo_gptr.addr_or_offs.offset << "\n"
                << "  Put Data:\n"
                << "    needs signal: " << data.put_data.needs_signal << "\n"
                << "    halo gptr: " << " uid: " << data.put_data.signal_gptr.unitid << " off: " << data.put_data.signal_gptr.addr_or_offs.offset << "\n"
                << "  Pack Data:\n"
                << "    needs packed: " << data.pack_data.needs_packing << "\n"
                << "    halo offset buffer: " << data.pack_data.buffer_offset << "\n"
                << "    block length: " << data.pack_data.block_len << "\n";
      std::cout << "    Block Offsets: { ";
      for(auto& offset : data.pack_data.block_offs) {
        std::cout << offset << " ";
      }
      std::cout << " }\n";
    }
    std::cout << std::endl;
  }

  void print_pack_data(region_index_t reg) {
    auto& data = _halo_update_data[reg];
    std::cout << "Halo Update Data (" << reg << ")\n"
              << "  Get Data:\n"
              << "    awaits signal: " << data.get_data.awaits_signal << "\n"
              << "    signal gptr: " << " uid: " << data.get_data.signal_gptr.unitid << " off: " << data.get_data.signal_gptr.addr_or_offs.offset << "\n"
              << "    halo gptr: " << " uid: " << data.get_data.halo_gptr.unitid << " off: " << data.get_data.halo_gptr.addr_or_offs.offset << "\n"
              << "  Put Data:\n"
              << "    needs signal: " << data.put_data.needs_signal << "\n"
              << "    halo gptr: " << " uid: " << data.put_data.signal_gptr.unitid << " off: " << data.put_data.signal_gptr.addr_or_offs.offset << "\n"
              << "  Pack Data:\n"
              << "    needs packed: " << data.pack_data.needs_packing << "\n"
              << "    halo offset buffer: " << data.put_data.buffer_offset << "\n"
              << "    block length: " << data.pack_data.block_len << "\n";
    std::cout << "    Block Offsets: { ";
    for(auto& offset : data.pack_data.block_offs) {
      std::cout << offset << " ";
    }
    std::cout << std::endl;
  }

  void print_gptr(dart_gptr_t gptr, region_index_t reg, const char* location) {
    std::cout << "[" << dash::myid() << "] loc: " << location << " reg: " << reg << " uid: " << gptr.unitid << " off: " << gptr.addr_or_offs.offset << std::endl;
  }
private:
  struct GetData {
    bool        awaits_signal{false};
    dart_gptr_t signal_gptr{DART_GPTR_NULL};
    dart_gptr_t halo_gptr{DART_GPTR_NULL};
  };

  struct PutData {
    bool        needs_signal{false};
    dart_gptr_t signal_gptr{DART_GPTR_NULL};
  };

  struct PackData{
    bool                        needs_packing{false};
    std::vector<pattern_size_t> block_offs{};
    pattern_size_t              block_len{0};
    signed_pattern_size_t       buffer_offset{-1};
  };

  struct HaloUpdateData {
    PackData pack_data;
    PutData  put_data;
    GetData  get_data;
  };

  pattern_size_t num_halo_elems() {
    const auto& halo_spec = _halo_block.halo_spec();
    team_unit_t rank_0(0);
    auto max_local_extents = _halo_block.pattern().local_extents(rank_0);

    pattern_size_t num_halo_elems = 0;
    for(auto r = 0; r < RegionsMax; ++r) {
      const auto& region_spec = halo_spec.spec(r);
      auto& pack_data = _halo_update_data[r].pack_data;
      if(region_spec.extent() == 0 ||
         (region_spec.level() == 1 && region_spec.relevant_dim() == ContiguousDim)) {
        pack_data.buffer_offset = -1;
        continue;
      }

      pattern_size_t reg_size = 1;
      for(auto d = 0; d < NumDimensions; ++d) {
        if(region_spec[d] != 1) {
          reg_size *= region_spec.extent();
        } else {
          reg_size *= max_local_extents[d];
        }
      }
      pack_data.buffer_offset = num_halo_elems;
      num_halo_elems += reg_size;
    }

    return num_halo_elems;
  }

  void init_block_data() {
    for(auto r = 0; r < RegionsMax; ++r) {
      auto region = _halo_block.halo_region(r);
      if(region == nullptr || region->size() == 0) {
        continue;
      }

      auto remote_region_index = RegionsMax - 1 - r;
      auto& update_data_loc = _halo_update_data[r];
      auto& update_data_rem = _halo_update_data[remote_region_index];

      update_data_rem.put_data.needs_signal = true;
      update_data_loc.get_data.awaits_signal = true;
      _signal_handles.push_back(nullptr);

      auto signal_gptr = _signal_buffer.begin().dart_gptr();
      // sets local signal gptr -> necessary for dart_get
      update_data_loc.get_data.signal_gptr = signal_gptr;
      update_data_loc.get_data.signal_gptr.unitid = _halo_block.pattern().team().myid();
      update_data_loc.get_data.signal_gptr.addr_or_offs.offset = r * sizeof(bool);

      // sets remote neighbor signal gptr -> necessary for dart_put
      auto neighbor_id = region->begin().dart_gptr().unitid;
      update_data_rem.put_data.signal_gptr = signal_gptr;
      update_data_rem.put_data.signal_gptr.unitid = neighbor_id;
      update_data_rem.put_data.signal_gptr.addr_or_offs.offset = remote_region_index * sizeof(bool);

      // Halo elements can be updated with one request
      if(region->spec().relevant_dim() == ContiguousDim && region->spec().level() == 1) {
        update_data_loc.get_data.halo_gptr = region->begin().dart_gptr();
        continue;
      }

      update_data_loc.get_data.halo_gptr = _halo_buffer.begin().dart_gptr();
      update_data_loc.get_data.halo_gptr.unitid = neighbor_id;
      update_data_loc.get_data.halo_gptr.addr_or_offs.offset = update_data_loc.pack_data.buffer_offset * sizeof(Element_t);

      // Setting all packing data
      // no packing needed -> all elements are contiguous
      const auto& reg_spec = _halo_block.halo_spec().spec(remote_region_index);
      if(reg_spec.extent() == 0 ||
         (reg_spec.relevant_dim() == ContiguousDim && reg_spec.level() == 1)) {
        update_data_rem.pack_data.buffer_offset = -1;
        continue;
      }

      auto& pack_data = update_data_rem.pack_data;
      pack_data.needs_packing = true;


      const auto& view_glob = _halo_block.view();
      auto reg_extents = view_glob.extents();
      auto reg_offsets = view_glob.offsets();

      for(dim_t d = 0; d < NumDimensions; ++d) {
        if(reg_spec[d] == 1) {
          continue;
        }

        reg_extents[d] = reg_spec.extent();
        if(reg_spec[d] == 0) {
          reg_offsets[d] += view_glob.extent(d) - reg_extents[d];
        } else {
          reg_offsets[d] = view_glob.offset(d);
        }
      }
      ViewSpec_t view_pack(reg_offsets, reg_extents);

      pattern_size_t num_elems_block = reg_extents[FastestDim];
      pattern_size_t num_blocks      = view_pack.size() / num_elems_block;

      pack_data.block_len = num_elems_block;
      pack_data.block_offs.resize(num_blocks);

      auto it_region = region->begin();
      decltype(it_region) it_pack_data(&(it_region.globmem()), it_region.pattern(), view_pack);
      for(auto& offset : pack_data.block_offs) {
        offset  = it_pack_data.lpos().index;
        it_pack_data += num_elems_block;
      }
    }
  }

private:
  const HaloBlockT&              _halo_block;
  std::vector<dart_handle_t>     _signal_handles;
  bool                           _signal;
  Element_t*                     _local_memory;
  std::array<HaloUpdateData,RegionsMax> _halo_update_data;
  pattern_size_t                 _num_halo_elems;
  HaloBuffer_t                   _halo_buffer;
  HaloSignalBuffer_t             _signal_buffer;

};  // class HaloPackBuffer

}  // namespace halo

}  // namespace dash

#endif // DASH__HALO_HALOMEMORY_H
