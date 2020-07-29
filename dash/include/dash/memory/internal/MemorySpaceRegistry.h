#ifndef DASH__MEMORY__INTERNAL__POINTER_REGISTRY__INCLUDED_H
#define DASH__MEMORY__INTERNAL__POINTER_REGISTRY__INCLUDED_H

#include <memory>
#include <mutex>
#include <vector>

#include <dash/Types.h>
#include <dash/dart/if/dart_globmem.h>

namespace dash {

namespace internal {

class MemorySpaceRegistry {
  using segid_t  = int16_t;
  using teamid_t = int16_t;

  using key_t = std::pair<teamid_t, segid_t>;

  static constexpr int IDX_TEAMID = 0;
  static constexpr int IDX_SEGID  = 1;

  using value_t = void*;

public:
  using element_t = std::pair<key_t, value_t>;

  ~MemorySpaceRegistry() = default;
  static bool Init();
  static MemorySpaceRegistry& GetInstance();

  bool add(dart_gptr_t gptr, value_t mem_space);
  void erase(dart_gptr_t gptr);

  inline value_t lookup(dart_gptr_t pointer) const noexcept
  {
    auto it = do_lookup(std::make_pair(pointer.teamid, pointer.segid));

    if (it != std::end(m_segments)) {
      return it->second;
    }

    return nullptr;
  }

private:
  static std::unique_ptr<MemorySpaceRegistry> m_instance;
  static std::vector<element_t>               m_segments;
  static bool                                 m_init;

  MemorySpaceRegistry()                               = default;
  MemorySpaceRegistry(const MemorySpaceRegistry& src) = delete;
  MemorySpaceRegistry& operator=(const MemorySpaceRegistry& rhs) = delete;

  std::vector<element_t>::iterator do_lookup(key_t key) const noexcept;
};

}  // namespace internal
}  // namespace dash
#endif
