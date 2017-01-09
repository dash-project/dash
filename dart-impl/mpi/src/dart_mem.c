//https://github.com/cloudwu/buddy

#include <dash/dart/mpi/dart_mem.h>

/* For PRIu64, uint64_t in printf */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define NODE_UNUSED 0
#define NODE_USED 1
#define NODE_SPLIT 2
#define NODE_FULL 3

struct dart_buddy *
	dart_buddy_new(int level)
{
	int size = 1 << level;
	struct dart_buddy * self =
    malloc(sizeof(struct dart_buddy) + sizeof(uint8_t) * (size * 2 - 2));
	self->level = level;
	memset(self->tree, NODE_UNUSED, size * 2 - 1);
	return self;
}

void
dart_buddy_delete(struct dart_buddy * self) {
	free(self);
}

static inline int
is_pow_of_2(uint32_t x) {
	return !(x & (x - 1));
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
  if (sizeof(size_t) > 4) {
    /* to avoid compiler warning on 32-bit targets */
	  x |= x >> (8 * sizeof(size_t) / 2);
  }
	return x + 1;
}

static inline uint64_t
_index_offset(int index, int level, int max_level) {
	return ((index + 1) - (1 << level)) << (max_level - level);
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

uint64_t
dart_buddy_alloc(struct dart_buddy * self, size_t s) {
	int size;
	if (s == 0) {
		size = 1;
	}
	else {
		size = (int)next_pow_of_2(s);
	}
	int length = 1 << self->level;

	if (size > length)
		return -1;

	int index = 0;
	int level = 0;

	while (index >= 0) {
		if (size == length) {
			if (self->tree[index] == NODE_UNUSED) {
				self->tree[index] = NODE_USED;
				_mark_parent(self, index);
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
			if (index < 0)
				return -1;
			if (index & 1) {
				++index;
				break;
			}
		}
	}

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

	if (offset >= (uint64_t)length) {
		assert(offset < (uint64_t)length);
		return -1;
	}

	for (;;) {
		switch (self->tree[index]) {
		case NODE_USED:
			if (offset != left){
				assert (offset == left);
				return -1;
			}
			_combine(self, index);
			return 0;
		case NODE_UNUSED:
			assert (0);
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
