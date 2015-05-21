#ifndef ENUMS_H_INCLUDED
#define ENUMS_H_INCLUDED

namespace dash {

enum MemArrange {
  Undefined = 0,
  ROW_MAJOR,
  COL_MAJOR
};

typedef struct DistEnum {
  enum disttype {
    BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
    CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
    BLOCKCYCLIC,
    TILE,
    NONE
  }; // general blocked distribution

  disttype type;
  long long blocksz;
} DistEnum;

static DistEnum BLOCKED { DistEnum::BLOCKED, -1 };
static DistEnum CYCLIC { DistEnum::BLOCKCYCLIC, 1 };
static DistEnum NONE { DistEnum::NONE, -1 };

DistEnum TILE(int blockSize);

DistEnum BLOCKCYCLIC(int blockSize);

} // namespace dash

#endif // DASH__DASH_LIB__ENUMS_H__INCLUDED__
