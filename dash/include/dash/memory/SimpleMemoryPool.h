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

  union alignas(ValueType) Block {
    Block* next;

    char _data[sizeof(ValueType)];
  };

  union alignas(Block) Chunk {
    Chunk* next;
  };

  // Ensure properly aligned Blocks and Chunks
  //
  /*
  typedef
      typename std::aligned_union<sizeof(Block), Block*,
                                  char[sizeof(ValueType)]>::type Block;
  typedef typename std::aligned_union<sizeof(Chunk), Block>::type
      Aligned_Chunk;
      */

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

  typedef  ValueType                  value_type;

  template <class T>
  struct rebind {
    typedef SimpleMemoryPool<T, typename std::allocator_traits<
                                 PoolAlloc>::template rebind_alloc<T>>
        other;
  };

  // clang-format on

 public:
  // CONSTRUCTOR
  explicit SimpleMemoryPool(const PoolAlloc& alloc = PoolAlloc()) noexcept;

  // COPY CONSTRUCTOR
  SimpleMemoryPool(SimpleMemoryPool const&) noexcept;

  // MOVE CONSTRUCTOR
  SimpleMemoryPool(SimpleMemoryPool&& other) noexcept;

  // DELETED MOVE ASSIGNMENT
  SimpleMemoryPool& operator=(SimpleMemoryPool&&) = delete;
  // DELETED COPY ASSIGNMENT
  SimpleMemoryPool& operator=(SimpleMemoryPool const&) = delete;

  ~SimpleMemoryPool() noexcept;

 private:
  // provide more space in the pool
  void refill();
  // allocate a Chunk for at least nbytes
  Block* allocateChunk(size_type nbytes);

 public:
  // Allocate Memory Blocks of size ValueType
  value_type* allocate(size_type = 0 /* internally not used */);
  // Deallocate a specific Memory Block
  void deallocate(void* address, size_type = 0 /* internally not used */);
  // Reserve Space for at least n Memory Blocks of size ValueType
  void reserve(size_type nblocks);
  // returns the underlying memory allocator
  allocator_type& allocator();
  // deallocate all memory blocks of all chunks
  void release();

 private:
  Chunk* _chunklist = nullptr;
  Block* _freelist = nullptr;
  int _blocks_per_chunk;
  allocator_type _alloc;
};

// CONSTRUCTOR
template <typename ValueType, typename PoolAlloc>
inline SimpleMemoryPool<ValueType, PoolAlloc>::SimpleMemoryPool(
    const PoolAlloc& alloc) noexcept
  : _chunklist(nullptr)
  , _freelist(nullptr)
  , _blocks_per_chunk(1)
  , _alloc(alloc)
{
}

// COPY CONSTRUCTOR
template <typename ValueType, typename PoolAlloc>
inline SimpleMemoryPool<ValueType, PoolAlloc>::SimpleMemoryPool(
    SimpleMemoryPool const& other) noexcept
  : _chunklist(nullptr)
  , _freelist(nullptr)
  , _blocks_per_chunk(other._blocks_per_chunk)
  , _alloc(other._alloc)
{
}

// MOVE CONSTRUCTOR
template <typename ValueType, typename PoolAlloc>
inline SimpleMemoryPool<ValueType, PoolAlloc>::SimpleMemoryPool(
    SimpleMemoryPool&& other) noexcept
  : _chunklist(other._chunklist)
  , _freelist(other._freelist)
  , _blocks_per_chunk(other._blocks_per_chunk)
  , _alloc(other._alloc)
{
  other._chunklist = nullptr;
  other._freelist = nullptr;
  other._blocks_per_chunk = 1;
}
template <typename ValueType, typename PoolAlloc>
typename SimpleMemoryPool<ValueType, PoolAlloc>::value_type*
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

  Block* begin = allocateChunk(nblocks * static_cast<size_type>(sizeof(Block)));
  Block* end = begin + nblocks - 1;

  for (Block* p = begin; p < end; ++p) {
    p->next = p + 1;
  }
  end->next = _freelist;
  _freelist = begin;
}

template <typename ValueType, typename PoolAlloc>
inline void SimpleMemoryPool<ValueType, PoolAlloc>::deallocate(void* address,
                                                               size_type)
{
  DASH_ASSERT(address);
  reinterpret_cast<Block*>(address)->next = _freelist;
  _freelist = reinterpret_cast<Block*>(address);
}

template <typename ValueType, typename PoolAlloc>
inline typename SimpleMemoryPool<ValueType, PoolAlloc>::allocator_type&
SimpleMemoryPool<ValueType, PoolAlloc>::allocator()
{
  return _alloc;
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
typename SimpleMemoryPool<ValueType, PoolAlloc>::Block*
SimpleMemoryPool<ValueType, PoolAlloc>::allocateChunk(size_type nbytes)
{
  size_type numBytes = static_cast<size_type>(sizeof(Chunk)) + nbytes;

  std::size_t const maxAlign = alignof(std::max_align_t);
  size_type max_aligned = (numBytes + maxAlign - 1) / maxAlign;

  Chunk* chunkPtr = reinterpret_cast<Chunk*>(
      AllocatorTraits::allocate(allocator(), max_aligned));

  /*
  DASH_ASSERT(
      0 == reinterpret_cast<std::uintptr_t>(chunkPtr) % sizeof(Chunk));
      */

  chunkPtr->next = _chunklist;
  _chunklist = chunkPtr;

  return reinterpret_cast<Block*>(chunkPtr + 1);
}

template <typename ValueType, typename PoolAlloc>
SimpleMemoryPool<ValueType, PoolAlloc>::~SimpleMemoryPool() noexcept
{
  release();
}

}  // namespace dash

#endif
