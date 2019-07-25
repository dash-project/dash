#ifndef DASH__TASKS_DEPENDENCYCONTAINER_H__
#define DASH__TASKS_DEPENDENCYCONTAINER_H__

#include <type_traits>
#include <dash/Exception.h>

#include <dash/dart/if/dart_tasking.h>

#define DASH_TASKS_DEPENDENCYCONTAINER_SMALL_SIZE 8

namespace dash {
namespace tasks {
namespace internal {

  /**
   * A container for DART data dependencies that does not destroy elements upon
   * clear().
   */
  struct DependencyContainer {

    using self_t     = DependencyContainer;
    using value_type = dart_task_dep_t;
    using size_type  = size_t;

    static constexpr const size_type SmallContainerSize
                                    = DASH_TASKS_DEPENDENCYCONTAINER_SMALL_SIZE;

    struct DependencyContainerIterator {

      friend struct DependencyContainer;

      using self_t = DependencyContainerIterator;
      using value_type = typename DependencyContainer::value_type;
      using size_type  = typename DependencyContainer::size_type;
      using difference_type = std::make_signed<size_type>::type;
      using reference_type  = value_type&;

      DependencyContainerIterator(
        DependencyContainer &container,
        size_t pos)
      : m_pos(pos), m_container(&container)
      { }

      /* Copy c'tor */
      DependencyContainerIterator(const DependencyContainerIterator& container) = default;

      /* Move c'tor */
      DependencyContainerIterator(DependencyContainerIterator&& container) = default;

      /* Copy assignment operator */
      self_t&
      operator=(const self_t& value) = default;

      /* Move assignment operator */
      self_t&
      operator=(self_t&& value)      = default;

      self_t operator++(){
        ++m_pos;
        return *this;
      }

      self_t operator++(int){
        auto res = *this;
        ++m_pos;
        return res;
      }


      reference_type operator*() {
        return (*m_container)[m_pos];
      }

      reference_type operator=(const value_type& value) {
        return (*m_container)[m_pos] = value;
      }

      bool operator<(const self_t& other) const {
        return this->m_pos < other.m_pos;
      }

      bool operator>(const self_t& other) const {
        return this->m_pos > other.m_pos;
      }

      bool operator<=(const self_t& other) const {
        return this->m_pos <= other.m_pos;
      }

      bool operator>=(const self_t& other) const {
        return this->m_pos >= other.m_pos;
      }

      reference_type operator[](ssize_t pos) {
        return (*m_container)[m_pos + pos];
      }

      self_t operator+(ssize_t disp) const {
        self_t res = *this;
        res.m_pos += disp;
        return res;
      }

      self_t& operator+=(ssize_t disp) {
        m_pos += disp;
        return *this;
      }

      difference_type operator-(const self_t& other) {
        return m_pos - other.m_pos;
      }

      size_type pos() const {
        return m_pos;
      }

    private:
      size_type            m_pos = 0;
      DependencyContainer *m_container;
    };

    using iterator   = DependencyContainerIterator;

    explicit
    DependencyContainer(size_type capacity = SmallContainerSize)
    : m_capacity(capacity)
    {
      if (capacity > SmallContainerSize) {
        m_data = new value_type[capacity];
      }
    }

    DependencyContainer(const DependencyContainer& other) = delete;
    DependencyContainer(DependencyContainer&& other)
    : m_size(other.m_size), m_capacity(other.m_capacity), m_data(other.m_data)
    {
      other.m_size = 0;
      other.m_capacity = 0;
      other.m_data = nullptr;

      if (m_data == nullptr) {
        for (size_t i = 0; i < m_size; ++i) {
          m_data_s[i] = std::move(other.m_data_s[i]);
        }
      }
    }

    self_t& operator=(const self_t& other)
    {
      m_size     = other.m_size;
      m_capacity = other.m_capacity;
      if (other.m_data != nullptr) {
        std::copy(other.m_data, other.m_data + m_size, m_data);
      } else {
        std::copy(other.m_data_s, other.m_data_s + m_size, m_data);
      }
    }

    self_t& operator=(self_t&& other) {
      std::swap(other.m_data, m_data);
      std::swap(other.m_size, m_size);
      std::swap(other.m_capacity, m_capacity);

      if (m_data == nullptr) {
        for (size_t i = 0; i < m_size; ++i) {
          m_data_s[i] = std::move(other.m_data_s[i]);
        }
      }
    }

    ~DependencyContainer(){
      delete[] m_data;
      m_data = nullptr;
    }

    iterator begin() {
      return iterator(*this, 0);
    }

    iterator end() {
      return iterator(*this, m_size);
    }

    value_type* data() {
      if (m_data == nullptr) return m_data_s;
      return m_data;
    }


    value_type& operator[](size_type pos) {
      DASH_ASSERT_MSG(pos < m_size, "Out-of-bounds detected!");
      if (m_data != nullptr) return m_data[pos];
      return m_data_s[pos];
    }

    iterator
    insert(iterator it, value_type value) {
      auto end_it = end();
      if (it > end_it)
        DASH_THROW(dash::exception::OutOfRange, "Out of bounds detected!");

      auto data_ptr = (m_data != nullptr) ? m_data : m_data_s;
      if (it.m_pos >= m_capacity) {
        // double the size
        grow(m_capacity*2);
        data_ptr = m_data;
      }

      data_ptr[it.m_pos] = value;

      if (m_size <= it.m_pos) {
        m_size = it.m_pos + 1;
      }
      return it;
    }

    // Simply reset the size, no destruction of elements necessary
    void clear() {
      m_size = 0;
    }

    size_type size() {
      return m_size;
    }

    size_type capacity() {
      return m_capacity;
    }

    bool is_empty() {
      return m_size == 0;
    }

  private:

    void grow(size_type new_capacity) {
        // double the size
        value_type *new_data = new value_type[new_capacity];
        if (m_data != nullptr) {
          std::memcpy(new_data, m_data, sizeof(value_type) * m_size);
          delete[] m_data;
        } else {
          std::memcpy(new_data, m_data_s, sizeof(value_type) * m_size);
        }
        m_data = new_data;
        m_capacity = new_capacity;
    }

  private:
    size_type   m_size      = 0;
    size_type   m_capacity  = 0;
    value_type *m_data      = nullptr;
    value_type  m_data_s[SmallContainerSize];
  };

} // namespace internal
} // namespace tasks
} // namespace dash

#endif // DASH__TASKS_DEPENDENCYCONTAINER_H__
