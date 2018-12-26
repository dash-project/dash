#ifndef DASH__ALGORITHM__SORT__LOCAL_DATA_H
#include <iterator>
#include <memory>
#include <vector>

#include <dash/memory/HostSpace.h>

namespace dash {
namespace impl {
template <class T>
class LocalData {
  using element_t = T;

  using iter_pair       = std::pair<element_t*, element_t*>;
  using const_iter_pair = std::pair<element_t const*, element_t const*>;

private:
  element_t*                   m_input{};
  element_t*                   m_output{};
  size_t                       m_size{};
  std::unique_ptr<element_t[]> m_buffer{};

public:
  LocalData(T* first, T* last, T* out)
    : m_input(first)
    , m_output(out)
    , m_size(std::distance(first, last))
  {
    if (m_input == m_output) {
      // using operator new does not apply any default value initialization.
      // So let's use that and encapsulate it in a pointer

      // in-place
      m_buffer =
          std::move(std::unique_ptr<element_t[]>{new element_t[m_size]});

      // We dot not have to copy but can move instead
      //std::copy(first, last, m_buffer.get());
    }
    else {
      //std::copy(first, last, m_output);
    }
  }

  // prevent copies
  LocalData(const LocalData& other) = delete;
  LocalData& operator=(const LocalData& other) = delete;

  constexpr element_t const* input() const noexcept
  {
    return m_input;
  }

  element_t* input() noexcept
  {
    return m_input;
  }

  element_t const* buffer() const noexcept
  {
    return m_buffer.get();
  }

  element_t* buffer() noexcept
  {
    return m_buffer.get();
  }

  constexpr element_t const* output() const noexcept
  {
    return m_output;
  }

  element_t* output() noexcept
  {
    return m_output;
  }

  std::size_t size() const noexcept
  {
    return m_size;
  }
};
}  // namespace impl
}  // namespace dash
#endif
