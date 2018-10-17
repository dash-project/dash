#include <dash/Exception.h>
#include <dash/internal/Logging.h>
#include <dash/internal/Math.h>
#include <dash/memory/HBWSpace.h>
#include <dash/util/StaticConfig.h>
#include <stdexcept>

void* dash::HBWSpace::do_allocate(size_t bytes, size_t alignment)
{
  DASH_LOG_DEBUG("HBWSpace.do_allocate(bytes, alignment)", bytes, alignment);

  if (bytes == 0) {
    return nullptr;
  }

  void* ptr;

#ifdef DASH_ENABLE_MEMKIND
  if (alignment < alignof(void*)) {
    alignment = alignof(void*);
  }

  if (check_hbw_available()) {
    auto const is_power_of_two = (alignment & (alignment - 1)) == 0;

    if (!is_power_of_two) {
      alignment = dash::math::NextPowerOf2(alignment);
    }

    int ret = hbw_posix_memalign(&ptr, alignment, bytes);

    if (ret == ENOMEM) {
      DASH_LOG_ERROR(
          "HBWSpace.do_allocate(bytes, alignment) > Cannot allocate memory",
          bytes,
          alignment);
      throw std::bad_alloc();
    }
    else if (ret == EINVAL) {
      throw std::invalid_argument(
          "Invalid requirements for hbw_posix_memalign");
    }

    DASH_LOG_TRACE(
        "HBWSpace.do_allocate(bytes, alignment)",
        "Allocated memory segment on HBM (pointer, nbytes, alignment)",
        ptr,
        bytes,
        alignment);
  }
  else {
    DASH_LOG_WARN(
        "HBWSpace.do_allocate(bytes, alignment)",
        "hbw_malloc is not available. Falling back to default host space");

    ptr = std::pmr::get_default_resource()->allocate(bytes, alignment);
  }
#else
  DASH_LOG_WARN(
      "HBWSpace.do_allocate(bytes, alignment)",
      "libmemkind is not available. Falling back to default host space");

  ptr = std::pmr::get_default_resource()->allocate(bytes, alignment);
#endif

  DASH_LOG_DEBUG("HBWSpace.do_allocate(bytes, alignment) >");

  return ptr;
}
void dash::HBWSpace::do_deallocate(void* p, size_t bytes, size_t alignment)
{
#ifdef DASH_ENABLE_MEMKIND
  if (check_hbw_available()) {
    hbw_free(p);
  }
  else {
    std::pmr::get_default_resource()->deallocate(p, bytes, alignment);
  }
#else
  std::pmr::get_default_resource()->deallocate(p, bytes, alignment);
#endif
}
bool dash::HBWSpace::do_is_equal(
    std::pmr::memory_resource const& other) const noexcept
{
  const HBWSpace* other_p = dynamic_cast<const HBWSpace*>(&other);

  return nullptr != other_p;
}

bool dash::HBWSpace::check_hbw_available(void)
{
  static auto const hbw_available = dash::util::DashConfig.avail_memkind &&
  // clang-format off
#ifdef DASH_ENABLE_MEMKIND
    (0 == hbw_check_available());
#else
    (false);
#endif
  // clang-format on
  return hbw_available;
}
