// ==============================================================================
// LLVM Release License
// ==============================================================================
// University of Illinois/NCSA
// Open Source License
//
// Copyright (c) 2003-2018 University of Illinois at Urbana-Champaign.
// All rights reserved.
//
// Developed by:
//
//     LLVM Team
//
//     University of Illinois at Urbana-Champaign
//
//     http://llvm.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimers.
//
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without specific
//       prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.

/*
 * Source: https://reviews.llvm.org/D47111
 **/

#ifndef EXPERIMENTAL_MEMORY_RESOURCE
#define EXPERIMENTAL_MEMORY_RESOURCE

#if __cpp_lib_memory_resource < 201603

#include <cpp17/polymorphic_allocator.h>

namespace std {
namespace pmr {

class monotonic_buffer_resource : public memory_resource {
  static constexpr const size_t __default_buffer_capacity  = 1024;
  static constexpr const size_t __default_buffer_alignment = 16;

  struct __chunk_header {
    __chunk_header *__next_;
    char *          __start_;
    char *          __cur_;
    size_t          __align_;
    size_t          __allocation_size()
    {
      return (reinterpret_cast<char *>(this) - __start_) + sizeof(*this);
    }
    void *__try_allocate_from_chunk(size_t, size_t);
  };

  struct __initial_header {
    char *__start_;
    char *__cur_;
    union {
      char * __end_;
      size_t __size_;
    };
    void *__try_allocate_from_chunk(size_t, size_t);
  };

public:
  monotonic_buffer_resource()
    : monotonic_buffer_resource(
          nullptr, __default_buffer_capacity, get_default_resource())
  {
  }

  explicit monotonic_buffer_resource(size_t __initial_size)
    : monotonic_buffer_resource(
          nullptr, __initial_size, get_default_resource())
  {
  }

  monotonic_buffer_resource(void *__buffer, size_t __buffer_size)
    : monotonic_buffer_resource(
          __buffer, __buffer_size, get_default_resource())
  {
  }

  explicit monotonic_buffer_resource(memory_resource *__upstream)
    : monotonic_buffer_resource(
          nullptr, __default_buffer_capacity, __upstream)
  {
  }

  monotonic_buffer_resource(
      size_t __initial_size, memory_resource *__upstream)
    : monotonic_buffer_resource(nullptr, __initial_size, __upstream)
  {
  }

  monotonic_buffer_resource(
      void *__buffer, size_t __buffer_size, memory_resource *__upstream)
    : __res_(__upstream)
  {
    __initial_.__start_ = static_cast<char *>(__buffer);
    if (__buffer != nullptr) {
      __initial_.__cur_ = static_cast<char *>(__buffer);
      __initial_.__end_ = static_cast<char *>(__buffer) + __buffer_size;
    }
    else {
      __initial_.__cur_  = nullptr;
      __initial_.__size_ = __buffer_size;
    }
    __chunks_ = nullptr;
  }

  monotonic_buffer_resource(const monotonic_buffer_resource &) = delete;

  ~monotonic_buffer_resource() override
  {
    release();
  }

  monotonic_buffer_resource &operator=(const monotonic_buffer_resource &) =
      delete;

  void release()
  {
    __initial_.__cur_ = __initial_.__start_;
    while (__chunks_ != nullptr) {
      __chunk_header *__next = __chunks_->__next_;
      __res_->deallocate(
          __chunks_->__start_,
          __chunks_->__allocation_size(),
          __chunks_->__align_);
      __chunks_ = __next;
    }
  }

  memory_resource *upstream_resource() const
  {
    return __res_;
  }

protected:
  void *do_allocate(
      size_t __bytes, size_t __alignment) override;  // key function

  void do_deallocate(void *, size_t, size_t) override
  {
  }

  bool do_is_equal(const memory_resource &__other) const noexcept override
  {
    return this == std::addressof(__other);
  }

private:
  __initial_header __initial_;
  __chunk_header * __chunks_;
  memory_resource *__res_;
};
}  // namespace pmr
}  // namespace std
#endif
#endif
