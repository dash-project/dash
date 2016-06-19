#include <dash/Types.h>
#include <dash/Distribution.h>

const dash::Distribution dash::BLOCKED =
  dash::Distribution(dash::internal::DIST_BLOCKED, -1);

const dash::Distribution dash::CYCLIC =
  dash::Distribution(dash::internal::DIST_CYCLIC, 1);

const dash::Distribution dash::NONE =
  dash::Distribution(dash::internal::DIST_NONE, -1);

dash::Distribution dash::TILE(int blockSize) {
  return Distribution(dash::internal::DIST_TILE, blockSize);
}

dash::Distribution dash::BLOCKCYCLIC(int blockSize) {
  return Distribution(dash::internal::DIST_BLOCKCYCLIC, blockSize);
}

namespace dash {

std::ostream & operator<<(
  std::ostream & os,
  const dash::Distribution & distribution)
{
  os << "Distribution(";
  if (distribution.type == dash::internal::DIST_TILE) {
    os << "TILE(" << distribution.blocksz << ")";
  }
  else if (distribution.type == dash::internal::DIST_BLOCKCYCLIC) {
    os << "BLOCKCYCLIC(" << distribution.blocksz << ")";
  }
  else if (distribution.type == dash::internal::DIST_CYCLIC) {
    os << "CYCLIC";
  }
  else if (distribution.type == dash::internal::DIST_BLOCKED) {
    os << "BLOCKED";
  }
  else if (distribution.type == dash::internal::DIST_NONE) {
    os << "NONE";
  }
  os << ")";
  return os;
}

}
