#include <algorithm>

#include <dash/Exception.h>
#include <dash/internal/Logging.h>
#include <dash/memory/internal/MemorySpaceRegistry.h>

std::unique_ptr<dash::internal::MemorySpaceRegistry>
    dash::internal::MemorySpaceRegistry::m_instance;

std::vector<dash::internal::MemorySpaceRegistry::element_t>
    dash::internal::MemorySpaceRegistry::m_segments;

std::once_flag dash::internal::MemorySpaceRegistry::m_onceFlag;

namespace dash {
namespace internal {
MemorySpaceRegistry& MemorySpaceRegistry::GetInstance()
{
  std::call_once(m_onceFlag, [] { m_instance.reset(new MemorySpaceRegistry); });
  return *m_instance.get();
}

void* MemorySpaceRegistry::lookup(key_t key) const noexcept
{
  auto it = do_lookup(key);

  if (it != std::end(m_segments)) {
    return it->second;
  }

  return nullptr;
}

bool MemorySpaceRegistry::add(key_t key, value_t value)
{
  if (lookup(key)) {
    return false;
  }
  DASH_LOG_TRACE(
      "MemorySpaceRegistry.add",
      "adding memory space segment",
      key.first,
      key.second,
      value);
  m_segments.emplace_back(std::make_pair(key, value));
  return true;
}

void MemorySpaceRegistry::erase(key_t key)
{
  auto it = do_lookup(key);

  if (it != std::end(m_segments)) {
    DASH_LOG_TRACE(
        "MemorySpaceRegistry.erase",
        "erasing memory space segment",
        key.first,
        key.second,
        it->second);
    m_segments.erase(it);
  }
}

std::vector<dash::internal::MemorySpaceRegistry::element_t>::iterator
MemorySpaceRegistry::do_lookup(key_t key) const noexcept
{
  return std::find_if(
      std::begin(m_segments),
      std::end(m_segments),
      [key](const std::pair<key_t, value_t>& item) {
        return item.first == key;
      });
}
}  // namespace internal
}  // namespace dash
