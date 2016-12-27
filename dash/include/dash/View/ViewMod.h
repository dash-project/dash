#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/View/ViewTraits.h>

namespace dash {

template <
  dim_t DimDiff,
  class OriginType,
  class IndexType = typename OriginType::IndexType >
class ViewSubMod
{
  typedef ViewSubMod<DimDiff, OriginType, IndexType> self_t;

public:
  constexpr static dim_t dimdiff = DimDiff;

public:
  ViewSubMod() = delete;

  ViewSubMod(
    OriginType & origin,
    IndexType    begin,
    IndexType    end)
  : _origin(origin), _begin(begin), _end(end)
  { }

private:
  OriginType & _origin;
  IndexType    _begin;
  IndexType    _end;

}; // class ViewSubMod


template <
  // Difference of view and origin dimensionality.
  // For example, when selecting a single row in the origin domain,
  // the view projection eliminates one dimension of the origin domain
  // difference of dimensionality is \f$(vdim - odim) = -1\f$.
  dim_t DimDiff,
  class IndexType
>
class ViewMod
{
  typedef ViewMod<DimDiff, IndexType> self_t;

public:

  constexpr static dim_t dimdiff = DimDiff;

  template <dim_t SubDim>
  inline self_t & sub(IndexType begin, IndexType end) {
    // record this sub operation:
    _begin = begin;
    _end   = end;
    return *this;
  }

  template <dim_t SubDim>
  inline self_t & sub(IndexType offset) {
    // record this sub operation:
    _begin = offset;
    _end   = offset;
    return *this;
  }

private:

  IndexType _begin;
  IndexType _end;

}; // class ViewMod

} // namespace dash

#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
