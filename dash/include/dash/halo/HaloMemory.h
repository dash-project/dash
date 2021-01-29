#ifndef DASH__HALO_HALOMEMORY_H
#define DASH__HALO_HALOMEMORY_H

#include <dash/halo/Types.h>
#include <dash/halo/Region.h>
#include <dash/halo/Halo.h>
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

template<typename HaloBlockT>
class SignalEnv {

  struct SignalData {
    bool        signal_used{false};
    dart_gptr_t gptr{DART_GPTR_NULL};
  };

  static constexpr auto NumDimensions = HaloBlockT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  using signal_t           = bool;
  using HaloSignalBuffer_t = dash::Array<signal_t>;
  using SignalDataSet_t    = std::array<SignalData,RegionsMax>;
  using SignalHandles_t    = std::vector<dart_handle_t>;
  using Pattern_t          = typename HaloBlockT::Pattern_t;

public:
  using Team_t = dash::Team;

public:
  SignalEnv(const HaloBlockT& halo_block, Team_t& team)
  : _signal_buffer(RegionsMax * team.size(), team),
    _signal_ready_buffer(RegionsMax * team.size(), team) {

    for(region_index_t r = 0; r < RegionsMax; ++r) {
      _signal_buffer.local[r] = 0;
      _signal_ready_buffer.local[r] = 1;
    }

    init_signal_env(halo_block);
  }

  void put_signal_async(region_index_t region_index) {
    auto& put_sig = _put_signals[region_index];
    if(!put_sig.signal_used) {
      return;
    }

    dart_handle_t handle;
    dash::internal::put_handle(put_sig.gptr, &_signal, 1, &handle);
    _signal_handles.push_back(std::move(handle));
  }

  void put_signal_blocking(region_index_t region_index) {
    auto& put_sig = _put_signals[region_index];
    if(!put_sig.signal_used) {
      return;
    }

    dash::internal::put_blocking(put_sig.gptr, &_signal, 1);
  }

  void put_ready_signal_async(region_index_t region_index) {
    auto& put_sig = _put_ready_signals[region_index];
    if(!put_sig.signal_used) {
      return;
    }

    dart_handle_t handle;
    dash::internal::put_handle(put_sig.gptr, &_signal, 1, &handle);
    _signal_ready_handles.push_back(std::move(handle));
  }

  void put_ready_signal_blocking(region_index_t region_index) {
    auto& put_sig = _put_ready_signals[region_index];
    if(!put_sig.signal_used) {
      return;
    }

    dash::internal::put_blocking(put_sig.gptr, &_signal, 1);
  }

  void ready_to_update(region_index_t region_index) {
    auto& get_data = _get_ready_signals[region_index];
    if(!get_data.signal_used) {
      return;
    }

    signal_t signal = false;
    while(!signal) {
      dash::internal::get_blocking(get_data.gptr, &signal, 1);
    }
    _signal_ready_buffer.lbegin()[region_index] = 0;
  }

  void wait_put_signals() {
    dart_waitall_local(_signal_handles.data(), _signal_handles.size());
    _signal_handles.clear();
  }

  void wait_put_ready_signals() {
    dart_waitall_local(_signal_ready_handles.data(), _signal_ready_handles.size());
    _signal_ready_handles.clear();
  }

  void wait_signal(region_index_t region_index) {
    auto& get_data = _get_signals[region_index];
    if(!get_data.signal_used) {
      return;
    }

    signal_t signal = false;
    while(!signal) {
      dash::internal::get_blocking(get_data.gptr, &signal, 1);
    }
    _signal_buffer.lbegin()[region_index] = 0;
  }

private:
  void init_signal_env(HaloBlockT halo_block) {
    const auto& env_info_md = halo_block.halo_env_info();

    long count_put_signals = 0;
    long count_put_ready_signals = 0;
    auto my_team_id = halo_block.pattern().team().myid();
    auto signal_gptr = _signal_buffer.begin().dart_gptr();
    auto signal_ready_gptr = _signal_ready_buffer.begin().dart_gptr();

    for(auto r = 0; r < RegionsMax; ++r) {
      auto signal_offset = r * sizeof(bool);

      const auto& env_md = env_info_md.info(r);

      if(env_md.neighbor_id_to >= 0) {
        auto& put_signal = _put_signals[r];
        put_signal.signal_used = true;
        put_signal.gptr = signal_gptr;
        put_signal.gptr.unitid = env_md.neighbor_id_to;
        put_signal.gptr.addr_or_offs.offset = signal_offset;

        auto& get_ready_signal = _get_ready_signals[r];
        get_ready_signal.signal_used = true;
        get_ready_signal.gptr = signal_ready_gptr;
        get_ready_signal.gptr.unitid = my_team_id;
        get_ready_signal.gptr.addr_or_offs.offset = signal_offset;

        ++count_put_signals;
      }

      auto region = halo_block.halo_region(r);
      if(region != nullptr && region->size() > 0 && env_md.neighbor_id_from >= 0) {
        auto& get_signal = _get_signals[r];
        // sets local signal gptr -> necessary for dart_get
        get_signal.signal_used = true;
        get_signal.gptr = signal_gptr;
        get_signal.gptr.unitid = my_team_id;
        get_signal.gptr.addr_or_offs.offset = signal_offset;

        auto& put_ready_signal = _put_ready_signals[r];
        put_ready_signal.signal_used = true;
        put_ready_signal.gptr = signal_ready_gptr;
        put_ready_signal.gptr.unitid = env_md.neighbor_id_from;
        put_ready_signal.gptr.addr_or_offs.offset = signal_offset;

        ++count_put_ready_signals;
      }
    }
    _signal_handles.reserve(count_put_signals);
    _signal_ready_handles.reserve(count_put_ready_signals);
  }

private:
  HaloSignalBuffer_t _signal_buffer;
  HaloSignalBuffer_t _signal_ready_buffer;
  signal_t           _signal = true;
  SignalDataSet_t    _get_signals{};
  SignalDataSet_t    _put_signals{};
  SignalDataSet_t    _get_ready_signals{};
  SignalDataSet_t    _put_ready_signals{};
  SignalHandles_t    _signal_handles;
  SignalHandles_t    _signal_ready_handles;
};

template<typename ElementT, typename LengthSizeT>
struct PackMetaData {
  bool                   needs_packing{false};
  std::vector<ElementT*> block_pos{};
  LengthSizeT            block_len{0};
  ElementT*              buffer_pos{nullptr};
  std::function<void()>  pack_func = [](){};
};

template<typename ElementT, typename LengthSizeT>
std::ostream& operator<<(std::ostream& os, const PackMetaData<ElementT, LengthSizeT>& pack) {
  os << "packing:" << std::boolalpha << pack.needs_packing
      << ", block_len " << pack.block_len
      << ", buffer_pos" << pack.buffer_pos;

  return os;
}

template<typename HaloBlockT>
class PackEnv {
  static constexpr auto NumDimensions = HaloBlockT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  using Pattern_t      = typename HaloBlockT::Pattern_t;
  using pattern_size_t  = std::make_signed_t<typename Pattern_t::size_type>;
  using upattern_size_t = std::make_unsigned_t<pattern_size_t>;
  using Element_t = typename HaloBlockT::Element_t;

  static constexpr auto MemoryArrange = Pattern_t::memory_order();
  // value not related to array index
  static constexpr auto FastestDim    =
    MemoryArrange == ROW_MAJOR ? NumDimensions - 1 : 0;
  static constexpr auto ContiguousDim    =
    MemoryArrange == ROW_MAJOR ? 1 : NumDimensions;

  using HaloBuffer_t  = dash::Array<Element_t>;
  using HaloPosAll_t  = std::array<dart_gptr_t, RegionsMax>;
  using PackMData_t   = PackMetaData<Element_t, upattern_size_t>;
  using PackMDataAll_t = std::array<PackMData_t, RegionsMax>;
  using PackOffs_t = std::array<pattern_size_t, RegionsMax>;

public:
  using Team_t = dash::Team;

public:
  PackEnv(const HaloBlockT& halo_block, Element_t* local_memory, Team_t& team)
  : _local_memory(local_memory),
    _pack_buffer() {
    auto pack_info = info_pack_buffer(halo_block);
    _pack_buffer.allocate(pack_info.first * team.size(), team);
    init_block_data(halo_block, pack_info.second);
  }

  void pack(region_index_t region) {
    _pack_md_all[region].pack_func();
  }

  dart_gptr_t halo_gptr(region_index_t region_index) {
    return _get_halos[region_index];
  }

  const dart_gptr_t& halo_gptr(region_index_t region_index) const {
    return _get_halos[region_index];
  }

private:
  auto info_pack_buffer(const HaloBlockT& halo_block) {
    const auto& halo_spec = halo_block.halo_spec();
    team_unit_t rank_0(0);
    auto max_local_extents = halo_block.pattern().local_extents(rank_0);
    PackOffs_t packed_offs;

    pattern_size_t num_pack_elems = 0;
    pattern_size_t current_offset = 0;
    for(auto r = 0; r < RegionsMax; ++r) {
      const auto& region_spec = halo_spec.spec(r);
      if(region_spec.extent() == 0 ||
         (region_spec.level() == 1 && region_spec.relevant_dim() == ContiguousDim)) {
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
      num_pack_elems += reg_size;
      packed_offs[r] = current_offset;
      current_offset += reg_size;
    }

    return std::make_pair(num_pack_elems, packed_offs);
  }


  void init_block_data(const HaloBlockT& halo_block, const PackOffs_t& packed_offs) {
    using ViewSpec_t     = typename Pattern_t::viewspec_type;

    const auto& env_info_md = halo_block.halo_env_info();
    for(auto r = 0; r < RegionsMax; ++r) {
      const auto& env_md = env_info_md.info(r);

      auto region = halo_block.halo_region(r);
      auto& halo_gptr = _get_halos[r];
      if(region != nullptr && region->size() > 0) {
        // Halo elements can be updated with one request
        if(region->spec().relevant_dim() == ContiguousDim && region->spec().level() == 1) {
          halo_gptr = region->begin().dart_gptr();
        } else {
          halo_gptr = _pack_buffer.begin().dart_gptr();
          halo_gptr.unitid = region->begin().dart_gptr().unitid;
          halo_gptr.addr_or_offs.offset = packed_offs[r] * sizeof(Element_t);
        }
      } else {
        halo_gptr = DART_GPTR_NULL;
      }

      if(env_md.neighbor_id_to < 0) {
        continue;
      }

      // Setting all packing data
      // no packing needed -> all elements are contiguous
      auto& pack_md = _pack_md_all[r];
      const auto& reg_spec = halo_block.halo_spec().spec(r);
      if(reg_spec.relevant_dim() == ContiguousDim && reg_spec.level() == 1) {
        continue;
      }

      pack_md.needs_packing = true;
      pack_md.buffer_pos = _pack_buffer.lbegin() + packed_offs[r];

      const auto& view_glob = halo_block.view();
      auto reg_offsets = view_glob.offsets();

      const auto& region_extents = env_md.halo_reg_data.view.extents();
      for(dim_t d = 0; d < NumDimensions; ++d) {
        if(reg_spec[d] == 1) {
          continue;
        }

        if(reg_spec[d] == 0) {
          reg_offsets[d] += view_glob.extent(d) - region_extents[d];
        } else {
          reg_offsets[d] = view_glob.offset(d);
        }
      }
      ViewSpec_t view_pack(reg_offsets, region_extents);
      pattern_size_t num_elems_block = region_extents[FastestDim];
      pattern_size_t num_blocks      = view_pack.size() / num_elems_block;

      pack_md.block_len = num_elems_block;
      pack_md.block_pos.resize(num_blocks);

      auto it_region = region->begin();
      decltype(it_region) it_pack_data(&(it_region.globmem()), it_region.pattern(), view_pack);
      for(auto& pos : pack_md.block_pos) {
        pos = _local_memory + it_pack_data.lpos().index;
        it_pack_data += num_elems_block;
      }
      auto pack = &pack_md;
      pack_md.pack_func = [pack](){
        auto buffer_offset = pack->buffer_pos;
        for(auto& pos : pack->block_pos) {
          std::copy(pos, pos + pack->block_len, buffer_offset);
          buffer_offset += pack->block_len;
        }
      };
    }
  }

private:
  Element_t*     _local_memory;
  HaloBuffer_t   _pack_buffer;
  HaloPosAll_t   _get_halos;
  PackMDataAll_t _pack_md_all;
};

template <typename HaloBlockT, SignalReady SigReady>
class HaloUpdateEnv {
  struct UpdateData {
    std::function<void(dart_handle_t&)> get_halos;
    dart_handle_t                       handle{};
  };

  static constexpr auto NumDimensions = HaloBlockT::ndim();
  static constexpr auto RegionsMax = NumRegionsMax<NumDimensions>;

  using TeamSpec_t   = TeamSpec<NumDimensions>;
  using HaloSpec_t   = typename HaloBlockT::HaloSpec_t;
  using ViewSpec_t   = typename HaloBlockT::ViewSpec_t;
  using Pattern_t    = typename HaloBlockT::Pattern_t;
  using EnvInfoMD_t = EnvironmentInfo<Pattern_t>;
  using SignalEnv_t = SignalEnv<HaloBlockT>;
  using PackEnv_t   = PackEnv<HaloBlockT>;



public:
  using HaloMemory_t = HaloMemory<HaloBlockT>;
  using Element_t    = typename HaloBlockT::Element_t;
  using Team_t       = dash::Team;

public:
  HaloUpdateEnv(const HaloBlockT& halo_block, Element_t* local_memory, Team_t& team, const TeamSpec_t& tspec)
  : _halo_block(halo_block),
    //_env_info_md(tspec, halo_block),
    _halo_memory(halo_block),
    _signal_env(halo_block, team),
    _pack_env(_halo_block, local_memory, team) {
    init_update_data();
  }

  /**
   * Initiates a blocking halo region update for all halo elements.
   */
  void update() {
    prepare_update();
    for(auto& data : _region_data) {
      update_halo_intern(data.first, data.second);
    }
    wait();
  }

  /**
   * Initiates a blocking halo region update for all halo elements within the
   * the given region.
   *
   * TODO: find a solution for prepare update
   */
  void update_at(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end()) {
      update_halo_intern(it_find->first, it_find->second);
      dart_wait_local(&it_find->second.handle);
      if(SigReady == SignalReady::ON) {
        _signal_env.put_ready_signal_blocking(it_find->first);
      }
    }
  }

  /**
   * Initiates an asychronous halo region update for all halo elements.
   */
  void update_async() {
    prepare_update();
    for(auto& data : _region_data) {
      update_halo_intern(data.first, data.second);
    }
  }

  /**
   * Initiates an asychronous halo region update for all halo elements within
   * the given region.
   *
   * TODO: find a solution for prepare update
   */
  void update_async_at(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find != _region_data.end()) {
      update_halo_intern(it_find->first, it_find->second);
    }
  }

  /**
   * Waits until all halo updates are finished. Only useful for asynchronous
   * halo updates.
   */
  void wait() {
    for(auto& region : _region_data) {
      dart_wait_local(&region.second.handle);
      if(SigReady == SignalReady::ON) {
        _signal_env.put_ready_signal_async(region.first);
      }
    }
    if(SigReady == SignalReady::ON) {
      _signal_env.wait_put_ready_signals();
    }
  }

  /**
   * Waits until the halo updates for the given halo region is finished.
   * Only useful for asynchronous halo updates.
   */
  void wait(region_index_t index) {
    auto it_find = _region_data.find(index);
    if(it_find == _region_data.end()) {
      return;
    }

    dart_wait_local(&it_find->second.handle);
    if(SigReady == SignalReady::ON) {
      _signal_env.put_ready_signal_blocking(it_find->first);
    }
  }

  // prepares the halo elements for update -> packing and sending signals to all relevant neighbors
  void prepare_update() {
    for(region_index_t r = 0; r < RegionsMax; ++r) {
      if(SigReady == SignalReady::ON) {
        _signal_env.ready_to_update(r);
      }
      _pack_env.pack(r);
      _signal_env.put_signal_async(r);
    }
    _signal_env.wait_put_signals();
  }

  /**
   * Returns the halo memory management object \ref HaloMemory
   */
  HaloMemory_t& halo_memory() { return _halo_memory; }

  /**
   * Returns the halo memory management object \ref HaloMemory
   */
  const HaloMemory_t& halo_memory() const { return _halo_memory; }

  /**
   * Returns the halo environment information object \ref EnvironmentInfo
   */
  EnvInfoMD_t halo_env_info() { return _halo_block.halo_env_info(); }

  /**
   * Returns the halo environment information object \ref EnvironmentInfo
   */
  const EnvInfoMD_t& halo_env_info() const { return _halo_block.halo_env_info() ; }

private:
  void init_update_data() {
    for(const auto& region : _halo_block.halo_regions()) {
      size_t region_size  = region.size();
      if(region_size == 0) {
        continue;
      }

      auto gptr = _pack_env.halo_gptr(region.index());
      if(region.is_custom_region()) {
        _region_data.insert(std::make_pair(
            region.index(), UpdateData{ [](dart_handle_t& handle) {},
                                  DART_HANDLE_NULL }));
      } else {
        auto* pos = &*(_halo_memory.first_element_at(region.index()));
        const auto& gptr = _pack_env.halo_gptr(region.index());
        _region_data.insert(std::make_pair(
            region.index(), UpdateData{ [pos, gptr, region_size](dart_handle_t& handle) {
                                  dash::internal::get_handle(gptr, pos, region_size, &handle);
                                  },
                                  DART_HANDLE_NULL }));
      }
    }
  }

  void update_halo_intern(region_index_t region_index, UpdateData& data) {
    _signal_env.wait_signal(region_index);
    data.get_halos(data.handle);
  }

private:
  const HaloBlockT& _halo_block;
  HaloMemory_t _halo_memory;
  SignalEnv_t _signal_env;
  PackEnv_t   _pack_env;
  std::map<region_index_t, UpdateData> _region_data;
}; // HaloUpdateEnv

}  // namespace halo

}  // namespace dash

#endif // DASH__HALO_HALOMEMORY_H
