#include <algorithm>

#include <dash/Exception.h>
#include <dash/internal/Logging.h>
#include <dash/memory/internal/MemorySpaceRegistry.h>

std::unique_ptr<dash::internal::MemorySpaceRegistry>
    dash::internal::MemorySpaceRegistry::m_instance;

std::vector<dash::internal::MemorySpaceRegistry::element_t>
    dash::internal::MemorySpaceRegistry::m_segments;

bool dash::internal::MemorySpaceRegistry::m_init = dash::internal::MemorySpaceRegistry::Init();

std::ostream &operator<<(std::ostream &os, const dart_gptr_t &dartptr);

namespace dash {
namespace internal {

bool MemorySpaceRegistry::Init()
{
  m_instance.reset(new MemorySpaceRegistry);

  return true;
}

MemorySpaceRegistry& MemorySpaceRegistry::GetInstance()
{
  return *m_instance.get();
}

bool MemorySpaceRegistry::add(dart_gptr_t p, value_t value)
{
  auto key = std::make_pair(p.teamid, p.segid);

  auto it = do_lookup(key);

  if (it != std::end(m_segments)) {
    DASH_LOG_TRACE(
        "MemorySpaceRegistry.add",
        "updating memory space segment to new value",
        p,
        value);
    it->second = value;
  }
  else {
    DASH_LOG_TRACE(
        "MemorySpaceRegistry.add", "adding memory space segment", p, value);

    m_segments.emplace_back(std::make_pair(key, value));
  }
  return true;
}

void MemorySpaceRegistry::erase(dart_gptr_t p)
{
  auto const key = std::make_pair(p.teamid, p.segid);

  auto it = do_lookup(key);

  if (it == std::end(m_segments)) {
    return;
  }

  DASH_LOG_TRACE(
      "MemorySpaceRegistry.erase",
      "removing memory space",
      std::get<IDX_TEAMID>(key),
      std::get<IDX_SEGID>(key));

  m_segments.erase(it);
}

std::vector<MemorySpaceRegistry::element_t>::iterator
MemorySpaceRegistry::do_lookup(key_t key) const noexcept
{
  return std::find_if(
      std::begin(m_segments),
      std::end(m_segments),
      [key](const element_t& item) {
        return item.first == key;
      });
}
}  // namespace internal
}  // namespace dash
