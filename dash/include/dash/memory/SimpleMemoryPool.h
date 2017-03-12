#ifndef DASH__MEMORY__SIMPLE_MEMORY_POOL_H_
#define DASH__MEMORY__SIMPLE_MEMORY_POOL_H_

#include <cstddef>
#include <memory>

#include <dash/Exception.h>

template <typename ValueType, typename LocalAlloc>
class SimpleMemoryPool {
 private:
  // PRIVATE TYPES
  typedef struct Block {
    struct Block* next;

    char _data[sizeof(ValueType)];

    // TODO rko: ensure proper Alignment;
  } Block;

  typedef struct Chunk {
    struct Chunk* next;

    // TODO rko: ensure proper Alignment;
  } Chunk;

 public:
  // PUBLIC TYPES
  // TODO rko: use size_type from allocator traits
  typedef std::size_t size_type;

  typedef typename std::allocator_traits<LocalAlloc> AllocatorTraits;

 public:
  // CONSTRUCTOR
  explicit SimpleMemoryPool(LocalAlloc& alloc) noexcept;
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
  //provide more space in the pool
  void refill();
  //allocate a Chunk for at least nbytes
  Block* allocateChunk(size_type nbytes);

 public:
  // Allocate Memory Blocks of size ValueType
  ValueType* allocate();
  // Deallocate a specific Memory Block
  void deallocate(void* address);
  // Reserve Space for at least n Memory Blocks of size ValueType
  void reserve(std::size_t nblocks);
  // returns the underlying memory allocator
  LocalAlloc allocator();
  // deallocate all memory blocks of all chunks
  void release();

 private:
  Chunk* _chunklist = nullptr;
  Block* _freelist = nullptr;
  LocalAlloc _alloc;
  int _blocks_per_chunk;
};

// CONSTRUCTOR
template <typename ValueType, typename Alloc>
inline SimpleMemoryPool<ValueType, Alloc>::SimpleMemoryPool(
    Alloc& alloc) noexcept
  : _chunklist(nullptr)
  , _freelist(nullptr)
  , _alloc(alloc)
  , _blocks_per_chunk(1)
{
}

// MOVE CONSTRUCTOR
template <typename ValueType, typename Alloc>
inline SimpleMemoryPool<ValueType, Alloc>::SimpleMemoryPool(
    SimpleMemoryPool&& other) noexcept
{
  // TODO rko
}
template <typename ValueType, typename Alloc>
ValueType* SimpleMemoryPool<ValueType, Alloc>::allocate()
{
  if (!_freelist) {
    refill();
  }

  ValueType* block = reinterpret_cast<ValueType*>(_freelist);
  _freelist = _freelist->next;
  return block;
}

template <typename ValueType, typename Alloc>
inline void SimpleMemoryPool<ValueType, Alloc>::reserve(std::size_t nblocks)
{
  // allocate a chunk with header and space for nblocks
  if (!_freelist) reserve(_blocks_per_chunk);
}

template <typename ValueType, typename Alloc>
inline void SimpleMemoryPool<ValueType, Alloc>::deallocate(void* address)
{
  DASH_ASSERT(address);
  reinterpret_cast<Block*>(address)->next = _freelist;
  _freelist = reinterpret_cast<Block*>(address);
}

template <typename ValueType, typename Alloc>
inline Alloc SimpleMemoryPool<ValueType, Alloc>::allocator()
{
  return this->_allocator;
}

template <typename ValueType, typename Alloc>
inline void SimpleMemoryPool<ValueType, Alloc>::release()
{
  while (_chunklist) {
    typename AllocatorTraits::value_type* lastChunk =
        reinterpret_cast<typename AllocatorTraits::value_type*>(_chunklist);
    _chunklist = _chunklist->next;
    AllocatorTraits::deallocate(allocator(), lastChunk, 1);
  }
  _freelist = 0;
}

template <typename ValueType, typename Alloc>
inline void SimpleMemoryPool<ValueType, Alloc>::refill()
{
  // allocate a chunk with header and space for nblocks
  reserve(_blocks_per_chunk);

  size_t const max_blocks_per_chunk = 32;

  if (_blocks_per_chunk < max_blocks_per_chunk) _blocks_per_chunk *= 2;
}

template <typename ValueType, typename Alloc>
typename SimpleMemoryPool<ValueType, Alloc>::Block*
SimpleMemoryPool<ValueType, Alloc>::allocateChunk(size_type nbytes)
{
  size_type numBytes = static_cast<size_type>(sizeof(Chunk)) + nbytes;
  // TODO rko: allocate always aligned

  Chunk* chunkPtr =
      reinterpret_cast<Chunk*>(AllocatorTraits::allocate(allocator(), nbytes));

  // TODO rko: check for aligned boundary

  chunkPtr->next = _chunklist;
  _chunklist = chunkPtr;

  return reinterpret_cast<Block*>(chunkPtr + 1);
}

#endif
