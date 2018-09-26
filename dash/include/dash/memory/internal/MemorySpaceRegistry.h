#ifndef DASH__MEMORY__INTERNAL__POINTER_REGISTRY__INCLUDED_H
#define DASH__MEMORY__INTERNAL__POINTER_REGISTRY__INCLUDED_H

#include <memory>
#include <mutex>
#include <vector>

#include <dash/Types.h>

namespace dash {

namespace internal {

class MemorySpaceRegistry {
  using segid_t  = int16_t;
  using teamid_t = int16_t;

  using key_t   = std::pair<teamid_t, segid_t>;

  using value_t = void*;

public:
  using element_t = std::pair<key_t, value_t>;

  ~MemorySpaceRegistry() = default;
  static MemorySpaceRegistry& GetInstance();

  bool    add(key_t key, value_t value);
  value_t lookup(key_t key) const noexcept;
  void    erase(key_t key);

private:
  static std::unique_ptr<MemorySpaceRegistry> m_instance;
  static std::vector<element_t>           m_segments;
  static std::once_flag                   m_onceFlag;
  MemorySpaceRegistry()                           = default;
  MemorySpaceRegistry(const MemorySpaceRegistry& src) = delete;
  MemorySpaceRegistry& operator=(const MemorySpaceRegistry& rhs) = delete;

  std::vector<element_t>::iterator do_lookup(key_t key) const noexcept;
};

}  // namespace internal
}  // namespace dash
#endif
