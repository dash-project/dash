#ifndef DASH__VIEW__PATTERN_INDEX_SET_H__INCLUDED
#define DASH__VIEW__PATTERN_INDEX_SET_H__INCLUDED

#include <dash/Iterator.h>
#include <dash/Range.h>

#include <dash/view/ViewTraits.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/SetUnion.h>

#include <dash/view/Local.h>
#include <dash/view/Global.h>

#include <dash/pattern/PatternProperties.h>

#include <dash/util/FunctionalExpr.h>
#include <dash/util/ArrayExpr.h>
#include <dash/util/IndexSequence.h>

#include <dash/iterator/internal/IteratorBase.h>

#include <memory>


#ifndef DOXYGEN
namespace dash {


// Forward-declarations

template <class IndexSetT, class DomainT, std::size_t NDim>
class IndexSetBase;

template <class IndexSetT, class DomainT, class PatternT, std::size_t NDim>
class PatternIndexSetBase;

template <class DomainT>
class IndexSetIdentity;

template <class DomainT>
class IndexSetLocal;

template <class DomainT>
class IndexSetGlobal;

template <class DomainT, std::size_t SubDim>
class IndexSetSub;

template <class DomainT>
class IndexSetBlocks;

template <class DomainT>
class IndexSetBlock;


#if 0
template <
  class       IndexSetType,
  class       DomainType,
  std::size_t NDim = DomainType::rank::value >
class IndexSetBase
{
  typedef IndexSetBase<IndexSetType, DomainType, NDim> self_t;

  typedef typename std::decay<DomainType>::type DomainValueT;

 public:
  typedef typename dash::view_traits<DomainValueT>::origin_type
    view_origin_type;
  typedef DomainValueT
    view_domain_type;
  typedef typename dash::view_traits<DomainValueT>::local_type
    view_local_type;
  typedef typename dash::view_traits<DomainValueT>::global_type
    view_global_type;

  typedef typename view_traits<DomainValueT>::index_set_type
    domain_type;
  typedef typename dash::view_traits<view_local_type>::index_set_type
    local_type;
  typedef typename dash::view_traits<view_global_type>::index_set_type
    global_type;

  typedef detail::IndexSetIterator<IndexSetType>
    iterator;
  typedef detail::IndexSetIterator<IndexSetType>
    const_iterator;
  typedef typename DomainType::size_type
    size_type;
  typedef typename DomainType::index_type
    index_type;
  typedef index_type
    value_type;

  typedef typename detail::index_set_domain_bind_t<view_domain_type>::type
    domain_member_type;

  typedef std::integral_constant<std::size_t, NDim>
    rank;

  static constexpr std::size_t ndim() { return NDim; }

 protected:
  domain_member_type     _domain;

  constexpr const IndexSetType & derived() const {
    return static_cast<const IndexSetType &>(*this);
  }
  
  constexpr explicit IndexSetBase(const DomainType & domain)
  : _domain(domain)
  { }

  constexpr explicit IndexSetBase(DomainType && domain)
  : _domain(std::move(domain))
  { }

  typedef struct {
    index_type begin;
    index_type end;
  } index_range_t;

  static constexpr index_range_t index_range_intersect(
    const index_range_t & a,
    const index_range_t & b) noexcept {
    return index_range_t {
             std::max<index_type>(a.begin, b.begin),
             std::min<index_type>(a.end,   b.end)
           };
  }
  static constexpr index_type index_range_size(
    const index_range_t & irng) noexcept {
    return irng.end - irng.begin;
  }
  
  ~IndexSetBase()                        = default;
 public:
  constexpr IndexSetBase()               = delete;
  constexpr IndexSetBase(self_t &&)      = default;
  constexpr IndexSetBase(const self_t &) = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;
  
  constexpr const DomainType & view_domain() const & {
    return _domain;
  }

  constexpr DomainType view_domain() const && {
    return _domain;
  }

  constexpr auto domain() const
//  -> decltype(dash::index(
//                std::declval<const view_domain_type &>()
//              )) {
    -> typename view_traits<view_domain_type>::index_set_type {
    return dash::index(_domain);
  }

  constexpr const local_type local() const {
    return dash::index(dash::local(_domain));
  }
 
  constexpr const global_type global() const {
    return dash::index(dash::global(_domain));
  }

  constexpr bool is_local() const noexcept {
    return dash::view_traits<DomainValueT>::is_local::value;
  }

  constexpr bool is_strided() const noexcept {
    return false;
  }

  constexpr bool is_sub() const noexcept {
    return (
      derived().size() < this->extents().size()
    );
  }

  constexpr bool is_shifted() const noexcept {
    return false;
  }

  // ---- extents ---------------------------------------------------------

  constexpr std::array<size_type, NDim>
  extents() const {
    return derived().extents();
  }

  template <std::size_t ShapeDim>
  constexpr size_type extent() const {
    return derived().extents()[ShapeDim];
  }

  constexpr size_type extent(std::size_t shape_dim) const {
    return derived().extents()[shape_dim];
  }

  // ---- offsets ---------------------------------------------------------

  constexpr std::array<index_type, NDim>
  offsets() const {
    return std::array<index_type, NDim> { };
  }

  template <std::size_t ShapeDim>
  constexpr index_type offset() const {
    return derived().offsets()[ShapeDim];
  }

  constexpr index_type offset(std::size_t shape_dim) const {
    return derived().offsets()[shape_dim];
  }

  // ---- access ----------------------------------------------------------

  constexpr index_type rel(index_type image_index) const {
    return image_index;
  }

  constexpr index_type rel(
    const std::array<index_type, NDim> & coords) const {
    return -1;
  }

  constexpr index_type operator[](index_type image_index) const {
    return domain()[derived().rel(image_index)];
  }

  constexpr index_type operator[](
    const std::array<index_type, NDim> & coords) const {
    return domain()[derived().rel(coords)];
  }

  constexpr const_iterator begin() const {
    return iterator(derived(), 0);
  }

  constexpr const_iterator end() const {
    return iterator(derived(), derived().size());
  }

  constexpr index_type first() const {
    return derived()[0];
  }

  constexpr index_type last() const {
    return derived()[ derived().size() - 1 ];
  }

  /*
   *  dash::index(r(10..100)).step(2)[8]  -> 26
   *  dash::index(r(10..100)).step(-5)[4] -> 80
   */
  constexpr const_iterator step(index_type stride) const {
    return (
      stride > 0
        ? iterator(derived(),                0, stride)
        : iterator(derived(), derived().size(), stride)
    );
  }
};
#endif

/**
 * \concept{DashIndexSetConcept}
 */
template <
  class       IndexSetType,
  class       DomainType,
  class       PatternType,
  std::size_t NDim = PatternType::ndim() >
class PatternIndexSetBase
: public IndexSetBase<
           PatternIndexSetBase<
             IndexSetType, DomainType, PatternType, NDim >,
           DomainType, NDim >
{
  typedef PatternIndexSetBase<
            IndexSetType, DomainType, PatternType, NDim>
    self_t;
  typedef IndexSetBase<
            IndexSetType, DomainType, NDim>
    base_t;

  typedef typename std::decay<DomainType>::type DomainValueT;

  using typename base_t::index_range_t;
  using typename base_t::size_type;
  using typename base_t::index_type;

 public:
  typedef PatternType pattern_type;

 private:
  const pattern_type * _pattern;

 protected:
  template <class PatternT_>
  static constexpr index_range_t index_range_g2l(
    const PatternT_     & pat,
    const index_range_t & grng) noexcept {
    return index_range_t {
             pat.local_coords({{ grng.begin }})[0],
             pat.local_coords({{ grng.end }})[0]
           };
  }

  template <class PatternT_>
  static constexpr index_range_t index_range_l2g(
    const PatternT_     & pat,
    const index_range_t & lrng) noexcept {
    return index_range_t {
             pat.global(lrng.begin),
             pat.global(lrng.end)
           };
  }

 public:
  constexpr explicit PatternIndexSetBase(const DomainType & domain)
  : base_t(domain)
  , _pattern(&dash::origin(this->domain()).pattern())
  { }

  constexpr explicit PatternIndexSetBase(DomainType && domain)
  : base_t(std::move(domain))
  , _pattern(&dash::origin(this->domain()).pattern())
  { }

  constexpr const pattern_type & pattern() const {
    return *_pattern;
  }

  constexpr bool is_strided() const noexcept {
    return (
      ( this->pattern().blockspec().size() >
          this->pattern().team().size() )
      ||
      ( this->pattern().ndim() > 1 &&
        this->domain().extent(1) < ( this->domain().is_local()
                                     ? this->pattern().local_extents()[1]
                                     : this->pattern().extents()[1] ))
    );
  }

  constexpr bool is_shifted() const noexcept {
    typedef typename dash::pattern_mapping_traits<pattern_type>::type
            pat_mapping_traits;
    return pat_mapping_traits::shifted ||
           pat_mapping_traits::diagonal;
  }

  // ---- extents ---------------------------------------------------------

  constexpr std::array<size_type, NDim>
  extents() const {
    return this->pattern().extents();
  }
};
#endif

} // namespace dash

#endif // DOXYGEN

#endif // DASH__VIEW__PATTERN_INDEX_SET_H__INCLUDED
