#ifndef DASH__DASH_LIB__ENUMS_H__INCLUDED__
#define DASH__DASH_LIB__ENUMS_H__INCLUDED__

namespace dash {

  enum MemArrange {
    Undefined = 0,
    ROW_MAJOR,
    COL_MAJOR
  };

  struct DistEnum {
    enum disttype {
      BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
      CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
      BLOCKCYCLIC,
      TILE,
      NONE
    }; // general blocked distribution

    disttype type;
    long long blocksz;
  };

  struct DistEnum BLOCKED { DistEnum::BLOCKED, -1 };

  // obsolete
  struct DistEnum CYCLIC_ { DistEnum::CYCLIC, -1 };
  struct DistEnum CYCLIC { DistEnum::BLOCKCYCLIC, 1 };

  struct DistEnum NONE { DistEnum::NONE, -1 };

  struct DistEnum TILE(int bs) {
    return{ DistEnum::TILE, bs };
  }

  struct DistEnum BLOCKCYCLIC(int bs) {
    return{ DistEnum::BLOCKCYCLIC, bs };
  }

} // namespace dash

#endif // DASH__DASH_LIB__ENUMS_H__INCLUDED__
