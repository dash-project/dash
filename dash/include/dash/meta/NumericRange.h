#ifndef DASH__META__NUMERICRANGE_H
#define DASH__META__NUMERICRANGE_H
// -*- C++ -*-
// Copyright (c) 2017, Just Software Solutions Ltd
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted provided that the
// following conditions are met:
//
// 1. Redistributions of source code must retain the above
// copyright notice, this list of conditions and the following
// disclaimer.
//
// 2. Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following
// disclaimer in the documentation and/or other materials
// provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of
// its contributors may be used to endorse or promote products
// derived from this software without specific prior written
// permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Slightly modified by the DASH contributors

#include <functional>
#include <iostream>
#include <stdexcept>

namespace dash {
namespace meta {

template <typename T>
struct IncrementValue {
  void operator()(T& x) const
  {
    ++x;
  }
};

template <typename T>
struct IncrementBy {
  T delta;

  IncrementBy(T delta_)
    : delta(std::move(delta_))
  {
  }

  void operator()(T& x) const
  {
    x += delta;
  }
};

template <typename T, typename Increment = IncrementValue<T>>
class numeric_range {
public:
  enum class direction { increasing, decreasing };

private:
  T         m_current;
  T         m_final;
  Increment m_inc;
  direction m_dir;

  bool at_end()
  {
    if (m_dir == direction::increasing) {
      return m_current >= m_final;
    }
    else {
      return m_current <= m_final;
    }
  }

public:
  class iterator {
    numeric_range* range;

    void check_done()
    {
      if (range->at_end()) {
        range = nullptr;
      }
    }

    class postinc_return {
      T value;

    public:
      postinc_return(T value_)
        : value(std::move(value_))
      {
      }
      T operator*()
      {
        return std::move(value);
      }
    };

  public:
    using value_type        = T;
    using reference         = T;
    using iterator_category = std::input_iterator_tag;
    using pointer           = T*;
    using difference_type   = void;

    iterator(numeric_range* range_)
      : range(range_)
    {
      if (range) check_done();
    }

    T operator*() const
    {
      return range->m_current;
    }

    T* operator->() const
    {
      return &range->m_current;
    }

    iterator& operator++()
    {
      if (!range)
        throw std::runtime_error("Increment a past-the-end iterator");
      range->m_inc(range->m_current);
      check_done();
      return *this;
    }

    postinc_return operator++(int)
    {
      postinc_return temp(**this);
      ++*this;
      return temp;
    }

    friend bool operator==(iterator const& lhs, iterator const& rhs)
    {
      return lhs.range == rhs.range;
    }
    friend bool operator!=(iterator const& lhs, iterator const& rhs)
    {
      return !(lhs == rhs);
    }
  };

  iterator begin()
  {
    return iterator(this);
  }

  iterator end()
  {
    return iterator(nullptr);
  }

  numeric_range(T initial_, T final_)
    : m_current(std::move(initial_))
    , m_final(std::move(final_))
    , m_dir(direction::increasing)
  {
  }
  numeric_range(T initial_, T final_, Increment inc_)
    : m_current(std::move(initial_))
    , m_final(std::move(final_))
    , m_inc(std::move(inc_))
    , m_dir(direction::increasing)
  {
  }
  numeric_range(T initial_, T final_, Increment inc_, direction dir_)
    : m_current(std::move(initial_))
    , m_final(std::move(final_))
    , m_inc(std::move(inc_))
    , m_dir(dir_)
  {
  }
};

template <typename T>
numeric_range<T> range(T from, T to)
{
  if (to < from) throw std::runtime_error("Cannot count down ");
  return numeric_range<T>(std::move(from), std::move(to));
}

template <typename T>
numeric_range<T> range(T to)
{
  return range(T(), std::move(to));
}

template <typename T>
numeric_range<T, IncrementBy<T>> range(T from, T to, T delta)
{
  if (!delta) throw std::runtime_error("Step must be non-zero");
  using direction = typename numeric_range<T, IncrementBy<T>>::direction;
  direction const m_dir =
      (delta > T()) ? direction::increasing : direction::decreasing;
  return numeric_range<T, IncrementBy<T>>(
      std::move(from),
      std::move(to),
      IncrementBy<T>(std::move(delta)),
      m_dir);
}
}  // namespace meta
}  // namespace dash
#endif
