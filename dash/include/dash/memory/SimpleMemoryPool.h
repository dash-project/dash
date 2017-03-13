#ifndef DASH__MEMORY__SIMPLE_MEMORY_POOL_H_
#define DASH__MEMORY__SIMPLE_MEMORY_POOL_H_

#include <cstddef>
#include <memory>

#include <dash/Exception.h>

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
namespace dash {

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
  typedef typename AllocatorTraits::allocator_type AllocatorType;

 public:
  // CONSTRUCTOR
  explicit SimpleMemoryPool(const LocalAlloc& alloc) noexcept;
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
  Block* allocateChunk(size_type nbytes);

 public:
  // Allocate Memory Blocks of size ValueType
  ValueType* allocate();
  // Deallocate a specific Memory Block
  void deallocate(void* address);
  // Reserve Space for at least n Memory Blocks of size ValueType
  void reserve(std::size_t nblocks);
  // returns the underlying memory allocator
  AllocatorType& allocator();
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
    const Alloc& alloc) noexcept
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
    DASH_ASSERT(0 < nblocks);

    Block *begin = allocateChunk(
                            nblocks * static_cast<size_type>(sizeof(Block)));
    Block *end   = begin + nblocks - 1;

    for (Block *p = begin; p < end; ++p) {
        p->next = p + 1;
    }
    end->next = _freelist;
    _freelist  = begin;
}

template <typename ValueType, typename Alloc>
inline void SimpleMemoryPool<ValueType, Alloc>::deallocate(void* address)
{
  DASH_ASSERT(address);
  reinterpret_cast<Block*>(address)->next = _freelist;
  _freelist = reinterpret_cast<Block*>(address);
}

template <typename ValueType, typename Alloc>
inline typename SimpleMemoryPool<ValueType, Alloc>::AllocatorType&
SimpleMemoryPool<ValueType, Alloc>::allocator()
{
  return this->_alloc;
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

template <typename ValueType, typename Alloc>
SimpleMemoryPool<ValueType, Alloc>::~SimpleMemoryPool() noexcept {
  release();
}

}  // namespace dash

#endif
