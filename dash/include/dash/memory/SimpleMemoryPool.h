#ifndef DASH__MEMORY__SIMPLE_MEMORY_POOL_H_
#define DASH__MEMORY__SIMPLE_MEMORY_POOL_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include <dash/Exception.h>

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

template <typename ValueType, typename PoolAlloc>
class SimpleMemoryPool {
 private:
  // PRIVATE TYPES

  union Block {
    Block* next;

    char _data[sizeof(ValueType)];
  };

  union Chunk {
    Chunk* next;
  };

  // Ensure properly aligned Blocks and Chunks
  //
  typedef
      typename std::aligned_union<sizeof(Block), Block*,
                                  char[sizeof(ValueType)]>::type Aligned_Block;
  typedef typename std::aligned_union<sizeof(Chunk), Aligned_Block>::type
      Aligned_Chunk;

 private:
  typedef std::allocator_traits<PoolAlloc> PoolAllocTraits;

 public:
  // PUBLIC TYPES

  // clang-format off

  //Ensure that we have maximum aligned chunks
  typedef typename PoolAllocTraits::
    template rebind_traits<std::max_align_t>               AllocatorTraits;

  typedef typename AllocatorTraits::allocator_type           allocator_type;
  typedef typename AllocatorTraits::size_type                    size_type;
  typedef typename AllocatorTraits::difference_type        difference_type;

  typedef typename PoolAllocTraits::value_type                  value_type;
  typedef typename PoolAllocTraits::pointer                        pointer;
  typedef typename PoolAllocTraits::const_pointer            const_pointer;
  typedef typename PoolAllocTraits::void_pointer              void_pointer;
  typedef typename PoolAllocTraits::const_void_pointer  const_void_pointer;

  // clang-format on

 public:
  // CONSTRUCTOR
  explicit SimpleMemoryPool(const PoolAlloc& alloc) noexcept;
  // MOVE CONSTRUCTOR
  SimpleMemoryPool(SimpleMemoryPool&& other) noexcept;

  // DELETED MOVE ASSIGNMENT
  SimpleMemoryPool& operator=(SimpleMemoryPool&&) = delete;
  // DELETED COPY ASSIGNMENT
  SimpleMemoryPool& operator=(SimpleMemoryPool const&) = delete;
  // DELETED COPY CONSTRUCTOR
  SimpleMemoryPool(SimpleMemoryPool const&) = delete;

  ~SimpleMemoryPool() noexcept;

 private:
  // provide more space in the pool
  void refill();
  // allocate a Chunk for at least nbytes
  Aligned_Block* allocateChunk(size_type nbytes);

 public:
  // Allocate Memory Blocks of size ValueType
  pointer allocate(size_type = 0 /* internally not used */);
  // Deallocate a specific Memory Block
  void deallocate(void_pointer address,
                  size_type = 0 /* internally not used */);
  // Reserve Space for at least n Memory Blocks of size ValueType
  void reserve(size_type nblocks);
  // returns the underlying memory allocator
  allocator_type& allocator() const;
  // deallocate all memory blocks of all chunks
  void release();

 private:
  Aligned_Chunk* _chunklist = nullptr;
  Aligned_Block* _freelist = nullptr;
  int _blocks_per_chunk;
};

// CONSTRUCTOR
template <typename ValueType, typename PoolAlloc>
inline SimpleMemoryPool<ValueType, PoolAlloc>::SimpleMemoryPool(
    const PoolAlloc& alloc) noexcept
  : _chunklist(nullptr)
  , _freelist(nullptr)
  , _blocks_per_chunk(1)
{
}

// MOVE CONSTRUCTOR
template <typename ValueType, typename PoolAlloc>
inline SimpleMemoryPool<ValueType, PoolAlloc>::SimpleMemoryPool(
    SimpleMemoryPool&& other) noexcept
{
  // TODO rko
}
template <typename ValueType, typename PoolAlloc>
typename SimpleMemoryPool<ValueType, PoolAlloc>::pointer
    SimpleMemoryPool<ValueType, PoolAlloc>::allocate(size_type)
{
  if (!_freelist) {
    refill();
  }

  ValueType* block = reinterpret_cast<ValueType*>(_freelist);
  _freelist = _freelist->next;
  return block;
}

template <typename ValueType, typename PoolAlloc>
inline void SimpleMemoryPool<ValueType, PoolAlloc>::reserve(size_type nblocks)
{
  // allocate a chunk with header and space for nblocks
  DASH_ASSERT(0 < nblocks);

  Aligned_Block* begin =
      allocateChunk(nblocks * static_cast<size_type>(sizeof(Aligned_Block)));
  Aligned_Block* end = begin + nblocks - 1;

  for (Aligned_Block* p = begin; p < end; ++p) {
    p->next = p + 1;
  }
  end->next = _freelist;
  _freelist = begin;
}

template <typename ValueType, typename PoolAlloc>
inline void SimpleMemoryPool<ValueType, PoolAlloc>::deallocate(
    void_pointer address, size_type)
{
  DASH_ASSERT(address);
  reinterpret_cast<Aligned_Block*>(address)->next = _freelist;
  _freelist = reinterpret_cast<Aligned_Block*>(address);
}

template <typename ValueType, typename PoolAlloc>
inline typename SimpleMemoryPool<ValueType, PoolAlloc>::allocator_type&
SimpleMemoryPool<ValueType, PoolAlloc>::allocator() const
{
  return *this;
}

template <typename ValueType, typename PoolAlloc>
inline void SimpleMemoryPool<ValueType, PoolAlloc>::release()
{
  while (_chunklist) {
    typename AllocatorTraits::value_type* lastChunk =
        reinterpret_cast<typename AllocatorTraits::value_type*>(_chunklist);
    _chunklist = _chunklist->next;
    AllocatorTraits::deallocate(allocator(), lastChunk, 1);
  }
  _freelist = 0;
}

template <typename ValueType, typename PoolAlloc>
inline void SimpleMemoryPool<ValueType, PoolAlloc>::refill()
{
  // allocate a chunk with header and space for nblocks
  reserve(_blocks_per_chunk);

  size_t const max_blocks_per_chunk = 32;

  if (_blocks_per_chunk < max_blocks_per_chunk) _blocks_per_chunk *= 2;
}

template <typename ValueType, typename PoolAlloc>
typename SimpleMemoryPool<ValueType, PoolAlloc>::Aligned_Block*
SimpleMemoryPool<ValueType, PoolAlloc>::allocateChunk(size_type nbytes)
{
  size_type numBytes = static_cast<size_type>(sizeof(Aligned_Chunk)) + nbytes;

  std::size_t const maxAlign = alignof(std::max_align_t);
  size_type max_aligned = (numBytes + maxAlign - 1) / maxAlign;

  Aligned_Chunk* chunkPtr = reinterpret_cast<Aligned_Chunk*>(
      AllocatorTraits::allocate(allocator(), max_aligned));

  DASH_ASSERT(
      0 == reinterpret_cast<std::uintptr_t>(chunkPtr) % sizeof(Aligned_Chunk));

  chunkPtr->next = _chunklist;
  _chunklist = chunkPtr;

  return reinterpret_cast<Aligned_Block*>(chunkPtr + 1);
}

template <typename ValueType, typename PoolAlloc>
SimpleMemoryPool<ValueType, PoolAlloc>::~SimpleMemoryPool() noexcept
{
  release();
}

}  // namespace dash

#endif
