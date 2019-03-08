/*
 * Buddy allocator to be used with externally allocated blocks.
 *
 * The main use for this allocator is \c dart_memalloc where a
 * fixed-size pre-allocated shared window is used to facilitate
 * shared-memory optimizations.
 *
 * The code was taken from https://github.com/cloudwu/buddy and
 * the right to use it has been kindly granted by the author.
 *
 */

#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/assert.h>

/* For PRIu64, uint64_t in printf */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// 8-byte minimum allocations to reduce storage overhead
#define DART_MEM_ALIGN_BITS 3
#define DART_MEM_ALIGN_BYTES (1<<DART_MEM_ALIGN_BITS)

enum {
 NODE_UNUSED = 0,
 NODE_USED   = 1,
 NODE_SPLIT  = 2,
 NODE_FULL   = 3
};

struct dart_buddy {
  dart_mutex_t mutex;
  int level;
  uint8_t tree[1];
};

/* Help to do memory management work for local allocation/free */
char* dart_mempool_localalloc;
struct dart_buddy  *  dart_localpool;

static inline unsigned int
num_level(size_t size)
{
  unsigned int level = 1;
  size_t shifter = 0x02;

  /* Check that most significan bit is not 1 because that means:
   *   a) You are requesting for sure more memory than available
   *   b) It will make the level calculation code to dead-lock */
  if(size > ((size_t)1 << (sizeof(size_t)*8 - 1))) {
    return 0xFFFFFFFF;
  }

  while (shifter < size) {
    shifter <<= 1;
    level++;
  }
  return level;
}

static inline int
is_pow_of_2(uint32_t x) {
  return !(x & (x - 1));
}

struct dart_buddy *
dart_buddy_new(size_t size)
{
  DART_ASSERT(is_pow_of_2(size));
  unsigned int level  = num_level(size) - DART_MEM_ALIGN_BITS;
  /* Modern CPUs are able to use 48-bits virtual addresses,
   * this allows to address up to 256 TiB of memory */
  if(level > 48) {
    DART_LOG_ERROR("Level of buddy allocator invalid");
    return NULL;
  }
  unsigned int lsize  = (((unsigned int) 1) << level);
	struct dart_buddy * self =
    malloc(sizeof(struct dart_buddy) + sizeof(uint8_t) * (lsize * 2 - 2));
	self->level = level;
	memset(self->tree, NODE_UNUSED, lsize * 2 - 1);
	dart__base__mutex_init(&self->mutex);
	return self;
}

void
dart_buddy_delete(struct dart_buddy * self) {
  dart__base__mutex_destroy(&self->mutex);
	free(self);
}

static inline size_t
next_pow_of_2(size_t x) {
  if (is_pow_of_2(x))
    return x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  if (sizeof(size_t) > 4) {
    /* to avoid compiler warning on 32-bit targets */
    x |= x >> (8 * sizeof(size_t) / 2);
  }
  return x + 1;
}

static inline size_t
_index_offset(int index, int level, int max_level) {
  return (((index + 1) - (1 << level))
                << (max_level - level)) * DART_MEM_ALIGN_BYTES;
}

static void
_mark_parent(struct dart_buddy * self, int index) {
  for (;;) {
    int buddy = index - 1 + (index & 1) * 2;
    if (buddy > 0 && (self->tree[buddy] == NODE_USED ||
        self->tree[buddy] == NODE_FULL)) {
      index = (index + 1) / 2 - 1;
      self->tree[index] = NODE_FULL;
    }
    else {
      return;
    }
  }
}

ssize_t
dart_buddy_alloc(struct dart_buddy * self, size_t s) {
  // honor the alignment
  size_t size = (s >> DART_MEM_ALIGN_BITS);
  if ((size<<DART_MEM_ALIGN_BITS) < s) ++size;
  size = (int)next_pow_of_2(size);
  size_t length = 1 << self->level;

  if (size > length) {
    DART_LOG_ERROR("Allocation size larger than total allocator size (%zu > %zu)",
                   s, length<<DART_MEM_ALIGN_BITS);
    return -1;
  }

  int index = 0;
  int level = 0;

  dart__base__mutex_lock(&self->mutex);

  while (index >= 0) {
    if (size == length) {
      if (self->tree[index] == NODE_UNUSED) {
        self->tree[index] = NODE_USED;
        _mark_parent(self, index);
        dart__base__mutex_unlock(&self->mutex);
        return _index_offset(index, level, self->level);
      }
    }
    else {
      // size < length
      switch (self->tree[index]) {
      case NODE_USED:
      case NODE_FULL:
        break;
      case NODE_UNUSED:
        // split first
        self->tree[index] = NODE_SPLIT;
        self->tree[index * 2 + 1] = NODE_UNUSED;
        self->tree[index * 2 + 2] = NODE_UNUSED;
        // intentional fall-through (?)
      default:
        index = index * 2 + 1;
        length /= 2;
        level++;
        continue;
      }
    }
    if (index & 1) {
      ++index;
      continue;
    }
    for (;;) {
      level--;
      length *= 2;
      index = (index + 1) / 2 - 1;
      if (index < 0) {
        dart__base__mutex_unlock(&self->mutex);
        return -1;
      }
      if (index & 1) {
        ++index;
        break;
      }
    }
  }

  dart__base__mutex_unlock(&self->mutex);
  DART_LOG_ERROR(
    "Allocation larger than remaining available allocator memory (%zu)", s);
  return -1;
}

static void
_combine(struct dart_buddy * self, int index) {
	for (;;) {
		int buddy = index - 1 + (index & 1) * 2;
		if (buddy < 0 || self->tree[buddy] != NODE_UNUSED) {
			self->tree[index] = NODE_UNUSED;
			while (((index = (index + 1) / 2 - 1) >= 0) &&
             self->tree[index] == NODE_FULL){
				self->tree[index] = NODE_SPLIT;
			}
			return;
		}
		index = (index + 1) / 2 - 1;
	}
}

int dart_buddy_free(struct dart_buddy * self, uint64_t offset)
{
	int      length = 1 << self->level;
	uint64_t left   = 0;
	int      index  = 0;

	offset >>= DART_MEM_ALIGN_BITS;

	if (offset >= (uint64_t)length) {
		assert(offset < (uint64_t)length);
		return -1;
	}

  dart__base__mutex_lock(&self->mutex);
  for (;;) {
    switch (self->tree[index]) {
    case NODE_USED:
      if (offset != left){
        assert (offset == left);
        dart__base__mutex_unlock(&self->mutex);
        return -1;
      }
      _combine(self, index);
      dart__base__mutex_unlock(&self->mutex);
      return 0;
    case NODE_UNUSED:
      DART_LOG_ERROR("Invalid offset %lX in dart_buddy_free(alloc:%p)!",
                    offset, self);
      dart_abort(DART_EXIT_ABORT);
      dart__base__mutex_unlock(&self->mutex);
      return -1;
    default:
      length /= 2;
      if (offset < left + length) {
        index = index * 2 + 1;
      }
      else {
        left += length;
        index = index * 2 + 2;
      }
      break;
    }
  }

  dart__base__mutex_unlock(&self->mutex);

  // TODO: is this ever reached?
  DART_LOG_ERROR("Failed to free buddy allocation!");
  dart_abort(DART_EXIT_ABORT);
  return -1;
}

int buddy_size(struct dart_buddy * self, uint64_t offset)
{
	uint64_t left   = 0;
	int      length = 1 << self->level;
	int      index  = 0;

  assert(offset < (uint64_t)length);

	for (;;) {
		switch (self->tree[index]) {
		case NODE_USED:
			assert(offset == left);
			return length;
		case NODE_UNUSED:
			assert(0);
			return length;
		default:
			length /= 2;
			if (offset < left + length) {
				index = index * 2 + 1;
			}
			else {
				left += length;
				index = index * 2 + 2;
			}
			break;
		}
	}

  // TODO: is this ever reached?
  DART_LOG_ERROR("Failed to free buddy allocation!");
  dart_abort(DART_EXIT_ABORT);

  return -1;
}

static void
_dump(struct dart_buddy * self, int index, int level) {
	switch (self->tree[index]) {
	case NODE_UNUSED:
		printf("(%"PRIu64":%d)",
           _index_offset(index, level, self->level),
           1 << (self->level - level));
		break;
	case NODE_USED:
		printf("[%"PRIu64":%d]",
           _index_offset(index, level, self->level),
           1 << (self->level - level));
		break;
	case NODE_FULL:
		printf("{");
		_dump(self, index * 2 + 1, level + 1);
		_dump(self, index * 2 + 2, level + 1);
		printf("}");
		break;
	default:
		printf("(");
		_dump(self, index * 2 + 1, level + 1);
		_dump(self, index * 2 + 2, level + 1);
		printf(")");
		break;
	}
}

void buddy_dump(struct dart_buddy * self) {
	_dump(self, 0, 0);
	printf("\n");
}
