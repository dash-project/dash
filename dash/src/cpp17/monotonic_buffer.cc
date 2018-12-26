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
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
//     * Redistributions of source code must retain the above copyright
//     notice,
//       this list of conditions and the following disclaimers.
//
//     * Redistributions in binary form must reproduce the above copyright
//     notice,
//       this list of conditions and the following disclaimers in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the names of the LLVM Team, University of Illinois at
//       Urbana-Champaign, nor the names of its contributors may be used to
//       endorse or promote products derived from this Software without
//       specific prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS WITH THE SOFTWARE.

/*
 * Source: https://reviews.llvm.org/D47111
 **/

#if __cpp_lib_memory_resource < 201603

#include <cpp17/monotonic_buffer.h>

namespace std {
namespace pmr {

static size_t roundup(size_t count, size_t alignment)
{
  size_t mask = alignment - 1;
  return (count + mask) & ~mask;
}

void *monotonic_buffer_resource::__initial_header::__try_allocate_from_chunk(
    size_t bytes, size_t align)
{
  if (!__cur_) return nullptr;
  void * new_ptr      = static_cast<void *>(__cur_);
  size_t new_capacity = (__end_ - __cur_);
  void * aligned_ptr  = std::align(align, bytes, new_ptr, new_capacity);
  if (aligned_ptr != nullptr) __cur_ = static_cast<char *>(new_ptr) + bytes;
  return aligned_ptr;
}

void *monotonic_buffer_resource::__chunk_header::__try_allocate_from_chunk(
    size_t bytes, size_t align)
{
  void * new_ptr      = static_cast<void *>(__cur_);
  size_t new_capacity = (reinterpret_cast<char *>(this) - __cur_);
  void * aligned_ptr  = std::align(align, bytes, new_ptr, new_capacity);
  if (aligned_ptr != nullptr) __cur_ = static_cast<char *>(new_ptr) + bytes;
  return aligned_ptr;
}

void *monotonic_buffer_resource::do_allocate(size_t bytes, size_t align)
{
  const size_t header_size  = sizeof(__chunk_header);
  const size_t header_align = alignof(__chunk_header);

  auto previous_allocation_size = [&]() {
    if (__chunks_ != nullptr) return __chunks_->__allocation_size();

    size_t newsize = (__initial_.__start_ != nullptr)
                         ? (__initial_.__end_ - __initial_.__start_)
                         : __initial_.__size_;

    return roundup(newsize, header_align) + header_size;
  };

  if (void *result = __initial_.__try_allocate_from_chunk(bytes, align))
    return result;
  if (__chunks_ != nullptr) {
    if (void *result = __chunks_->__try_allocate_from_chunk(bytes, align))
      return result;
  }

  // Allocate a brand-new chunk.

  if (align < header_align) align = header_align;

  size_t aligned_capacity  = roundup(bytes, header_align) + header_size;
  size_t previous_capacity = previous_allocation_size();

  if (aligned_capacity <= previous_capacity) {
    size_t newsize   = 2 * (previous_capacity - header_size);
    aligned_capacity = roundup(newsize, header_align) + header_size;
  }

  char *          start = (char *)__res_->allocate(aligned_capacity, align);
  __chunk_header *header =
      (__chunk_header *)(start + aligned_capacity - header_size);
  header->__next_  = __chunks_;
  header->__start_ = start;
  header->__cur_   = start;
  header->__align_ = align;
  __chunks_        = header;

  return __chunks_->__try_allocate_from_chunk(bytes, align);
}
}  // namespace pmr
}  // namespace std

#endif
