#ifndef DASH__MEMORY__SIMPLE_MEMORY_POOL_RESOURCE_H_
#define DASH__MEMORY__SIMPLE_MEMORY_POOL_RESOURCE_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include <dash/Exception.h>
#include <dash/Types.h>

#include <dash/allocator/AllocatorBase.h>
#include <dash/memory/MemorySpace.h>

// clang-format off

/**
 * This class is a simple memory pool which holds allocates elements of size
 * ValueType. Efficient allocation is achieved in terms of the memory regions.
 * Each region represents a chunk of memory blocks and each block
 * is a single element of size ValueType.
 *
 * \par Methods
 *
 * Return Type          | Method              | Parameters                  | Description                                                                                                |
 * -------------------- | ------------------  | --------------------------- | ---------------------------------------------------------------------------------------------------------- |
 * <tt>ValueType *</tt> | <tt>allocate</tt>   | nbsp;                       | Allocates an aligned block to store a single element of size ValueType                                     |
 * <tt>void</tt>        | <tt>deallocate</tt> | <tt>addr</tt>               | Deallocates the specified memory address and keeps memory internall in a freelist.                         |
 * <tt>void</tt>        | <tt>reserve</tt>    | <tt>nblocks</tt>            | Reserve a chunk of memory blocks to hold at least n elements of size ValueType.                            |
 * <tt>void</tt>        | <tt>release</tt>    | nbsp;                       | Release all memory chunks and deallocate everything at one.                                                |
 *
 */

// clang-format on
namespace dash {

namespace detail {

template <size_t Blocksize>
union Block {
  Block* next;

  std::uint8_t _data[Blocksize];
};

union Chunk {
  Chunk* next;
};

struct Header {
  std::size_t size;
};

}  // namespace detail

template <class LocalMemSpace>
class SimpleMemoryPoolResource
  : public dash::MemorySpace<
        memory_domain_local,
        typename LocalMemSpace::memory_space_type_category> {
private:
  // PRIVATE TYPES

  static constexpr const size_t MAX_ALIGN = alignof(dash::max_align_t);

  static constexpr const size_t MAX_BLOCKS_PER_CHUNK = 32;

  static constexpr const size_t MAX_BLOCK_SIZE = 16;

  using memory_traits = dash::memory_space_traits<LocalMemSpace>;

  using Block = detail::Block<MAX_BLOCK_SIZE>;
  using Chunk = detail::Chunk;

  static_assert(
      memory_traits::is_local::value, "Upstream Memory Space must be local");

public:
  /// Require for memory traits
  using memory_space_type_category =
      typename memory_traits::memory_space_type_category;
  using memory_space_domain_category =
      typename memory_traits::memory_space_domain_category;

public:
  // CONSTRUCTOR
  explicit SimpleMemoryPoolResource(
      LocalMemSpace* resource = nullptr) noexcept;

  // COPY CONSTRUCTOR
  SimpleMemoryPoolResource(SimpleMemoryPoolResource const&) noexcept;

  // MOVE CONSTRUCTOR
  SimpleMemoryPoolResource(SimpleMemoryPoolResource&& other) noexcept;

  // DELETED MOVE ASSIGNMENT
  SimpleMemoryPoolResource& operator=(SimpleMemoryPoolResource&&) = delete;
  // DELETED COPY ASSIGNMENT
  SimpleMemoryPoolResource& operator=(SimpleMemoryPoolResource const&) =
      delete;

  ~SimpleMemoryPoolResource() noexcept;

  /// Returns the underlying memory resource
  inline LocalMemSpace* upstream_resource();

private:
  // provide more space in the pool
  void refill();
  // allocate a Chunk for at least nbytes
  Block* allocateChunk(std::size_t nbytes);

protected:
  void* do_allocate(size_t bytes, size_t alignment) override;
  void  do_deallocate(void* p, size_t bytes, size_t alignment) override;
  bool  do_is_equal(cpp17::pmr::memory_resource const& other) const
      noexcept override;

public:
  /// deallocate all memory blocks of all chunks
  void release();

  void reserve(std::size_t nblocks);

private:
  Chunk*         m_chunklist = nullptr;
  Block*         m_freelist  = nullptr;
  int            m_blocks_per_chunk;
  LocalMemSpace* m_resource;
};

// CONSTRUCTOR
template <class LocalMemSpace>
inline SimpleMemoryPoolResource<LocalMemSpace>::SimpleMemoryPoolResource(
    LocalMemSpace* r) noexcept
  : m_chunklist(nullptr)
  , m_freelist(nullptr)
  , m_blocks_per_chunk(1)
  , m_resource(
        r ? r
          : static_cast<LocalMemSpace*>(
                get_default_memory_space<
                    memory_domain_local,
                    typename memory_traits::memory_space_type_category>()))
{
}

// COPY CONSTRUCTOR
template <typename LocalMemSpace>
inline SimpleMemoryPoolResource<LocalMemSpace>::SimpleMemoryPoolResource(
    SimpleMemoryPoolResource const& other) noexcept
  : m_chunklist(nullptr)
  , m_freelist(nullptr)
  , m_blocks_per_chunk(other.m_blocks_per_chunk)
  , m_resource(other.m_resource)
{
}

// MOVE CONSTRUCTOR
template <typename LocalMemSpace>
inline SimpleMemoryPoolResource<LocalMemSpace>::SimpleMemoryPoolResource(
    SimpleMemoryPoolResource&& other) noexcept
  : m_chunklist(other.m_chunklist)
  , m_freelist(other.m_freelist)
  , m_blocks_per_chunk(other.m_blocks_per_chunk)
  , m_resource(other.m_resource)
{
  other.m_chunklist        = nullptr;
  other.m_freelist         = nullptr;
  other.m_blocks_per_chunk = 1;
}
template <typename LocalMemSpace>
void* SimpleMemoryPoolResource<LocalMemSpace>::do_allocate(
    size_t bytes, size_t alignment)
{
  (void)alignment;  // unused here, pools always allocate with alignment
                    // max_align_t

  if (bytes > MAX_BLOCK_SIZE) {
    std::size_t const header_sz = sizeof(detail::Header);

    if (std::size_t(-1) - header_sz < bytes) {
      throw std::bad_alloc();
    }

    detail::Header* p = reinterpret_cast<detail::Header*>(
        m_resource->allocate(bytes + header_sz, MAX_ALIGN));

    p->size = bytes + header_sz;

    return p + 1;
  }

  if (!m_freelist) {
    refill();
  }

  void* block = reinterpret_cast<void*>(m_freelist);
  m_freelist  = m_freelist->next;
  return block;
}

template <typename LocalMemSpace>
inline void SimpleMemoryPoolResource<LocalMemSpace>::do_deallocate(
    void* address, size_t bytes, size_t alignment)
{
  // Unused here
  (void)alignment;

  if (bytes > MAX_BLOCK_SIZE) {
    // m_resource->deallocate(address, bytes, MAX_ALIGN);
    std::size_t const header_sz = sizeof(detail::Header);

    auto header = static_cast<detail::Header*>(
        static_cast<void*>(reinterpret_cast<char*>(address) - header_sz));

    std::size_t const sz = header->size;

    m_resource->deallocate(header, sz, MAX_ALIGN);
  }

  DASH_ASSERT(address);
  reinterpret_cast<Block*>(address)->next = m_freelist;
  m_freelist                              = reinterpret_cast<Block*>(address);
}

template <typename LocalMemSpace>
inline bool SimpleMemoryPoolResource<LocalMemSpace>::do_is_equal(
    cpp17::pmr::memory_resource const& other) const noexcept
{
  const SimpleMemoryPoolResource* other_p =
      dynamic_cast<const SimpleMemoryPoolResource*>(&other);

  if (other_p)
    return this->m_resource == other_p->m_resource;
  else
    return false;
}

template <typename LocalMemSpace>
inline void SimpleMemoryPoolResource<LocalMemSpace>::reserve(
    std::size_t nblocks)
{
  // allocate a chunk with header and space for nblocks
  DASH_ASSERT(0 < nblocks);

  Block* begin = allocateChunk(nblocks * sizeof(Block));
  Block* end   = begin + nblocks - 1;

  for (Block* p = begin; p < end; ++p) {
    p->next = p + 1;
  }
  end->next  = m_freelist;
  m_freelist = begin;
}

template <typename LocalMemSpace>
inline LocalMemSpace*
SimpleMemoryPoolResource<LocalMemSpace>::upstream_resource()
{
  return m_resource;
}

template <typename LocalMemSpace>
inline void SimpleMemoryPoolResource<LocalMemSpace>::release()
{
  while (m_chunklist) {
    Chunk* lastChunk = m_chunklist;
    m_chunklist      = m_chunklist->next;
    m_resource->deallocate(lastChunk, MAX_ALIGN);
  }
  m_freelist = 0;
}

template <typename LocalMemSpace>
inline void SimpleMemoryPoolResource<LocalMemSpace>::refill()
{
  // allocate a chunk with header and space for nblocks
  reserve(m_blocks_per_chunk);

  // increase blocks per chunk by 2
  m_blocks_per_chunk *= 2;

  if (m_blocks_per_chunk > MAX_BLOCKS_PER_CHUNK)
    m_blocks_per_chunk = MAX_BLOCKS_PER_CHUNK;
}

template <typename LocalMemSpace>
typename SimpleMemoryPoolResource<LocalMemSpace>::Block*
SimpleMemoryPoolResource<LocalMemSpace>::allocateChunk(std::size_t nbytes)
{
  // nbytes == nblocks * sizeof(Block)
  std::size_t numBytes = sizeof(Chunk) + nbytes;

  Chunk* chunkPtr =
      reinterpret_cast<Chunk*>(m_resource->allocate(numBytes, MAX_ALIGN));

  // append allocated chunks to already existing chunks
  chunkPtr->next = m_chunklist;
  m_chunklist    = chunkPtr;

  // We have to increment by 1 since user memory starts at offset 1
  return reinterpret_cast<Block*>(chunkPtr + 1);
}

template <typename LocalMemSpace>
SimpleMemoryPoolResource<LocalMemSpace>::~SimpleMemoryPoolResource() noexcept
{
  release();
}

}  // namespace dash

#endif  // DASH__MEMORY__SIMPLE_MEMORY_POOL_RESOURCE_H_
