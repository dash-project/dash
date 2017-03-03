#ifndef DASH__VIEW__INDEX_SET_H__INCLUDED
#define DASH__VIEW__INDEX_SET_H__INCLUDED

#include <dash/view/ViewTraits.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/SetUnion.h>

#include <dash/view/Local.h>
#include <dash/view/Global.h>

#include <dash/pattern/PatternProperties.h>

#include <dash/Iterator.h>

#include <dash/util/FunctionalExpr.h>
#include <dash/util/ArrayExpr.h>
#include <dash/util/IndexSequence.h>

#include <dash/util/internal/IteratorBase.h>

#include <memory>


#ifndef DOXYGEN
namespace dash {

namespace detail {

enum index_scope {
  local_index,
  global_index
};

template <
  class       IndexType,
  index_scope IndexScope >
struct scoped_index {
  static const index_scope scope = IndexScope;
  IndexType                value;
};

} // namespace detail


template <class IndexType>
using local_index_t
  = dash::detail::scoped_index<IndexType, dash::detail::local_index>;

template <class IndexType>
using global_index_t
  = dash::detail::scoped_index<IndexType, dash::detail::global_index>;


// Forward-declarations

template <class IndexSetType, class DomainType, std::size_t NDim>
class IndexSetBase;

template <class DomainType>
class IndexSetIdentity;

template <class DomainType>
class IndexSetLocal;

template <class DomainType>
class IndexSetGlobal;

template <class DomainType, std::size_t SubDim>
class IndexSetSub;



template <
  class    DomainType,
  typename DomainValueType =
             typename std::remove_const<
               typename std::remove_reference<DomainType>::type
             >::type
>
constexpr auto
index(const DomainType & v)
  -> typename std::enable_if<
       !dash::view_traits<DomainValueType>::is_origin::value,
       decltype(v.index_set())
     >::type {
  return v.index_set();
}

template <class ContainerType>
constexpr auto
index(const ContainerType & c)
-> typename std::enable_if <
     dash::view_traits<ContainerType>::is_origin::value,
     IndexSetIdentity<ContainerType>
   >::type {
  return IndexSetIdentity<ContainerType>(c);
}


namespace detail {

template <
  class IndexSetType,
  int   BaseStride   = 1 >
class IndexSetIterator
  : public dash::internal::IndexIteratorBase<
      IndexSetIterator<IndexSetType, BaseStride>,
      int,            // value type
      int,            // difference type
      std::nullptr_t, // pointer
      int             // reference
> {
  typedef int                                          index_type;

  typedef IndexSetIterator<IndexSetType, BaseStride>   self_t;
  typedef dash::internal::IndexIteratorBase<
      IndexSetIterator<
        IndexSetType,
        BaseStride >,
      index_type, int, std::nullptr_t, index_type >    base_t;
 private:
  const IndexSetType * const _index_set;
  index_type                 _stride                 = BaseStride;
 public:
  constexpr IndexSetIterator()                       = delete;
  constexpr IndexSetIterator(self_t &&)              = default;
  constexpr IndexSetIterator(const self_t &)         = default;
  ~IndexSetIterator()                                = default;
  self_t & operator=(self_t &&)                      = default;
  self_t & operator=(const self_t &)                 = default;

  constexpr IndexSetIterator(
    const IndexSetType & index_set,
    index_type           position,
    index_type           stride = BaseStride)
  : base_t(position)
  , _index_set(&index_set)
  { }

  constexpr IndexSetIterator(
    const self_t &       other,
    index_type           position)
  : base_t(position)
  , _index_set(other._index_set)
  , _stride(other._stride)
  { }

  constexpr index_type dereference(index_type idx) const {
    return (idx * _stride) < dash::size(*_index_set)
              ? (*_index_set)[idx * _stride]
              : ((*_index_set)[dash::size(*_index_set)-1]
                  + ((idx * _stride) - (dash::size(*_index_set) - 1))
                );
  }
};

} // namespace detail

// -----------------------------------------------------------------------
// IndexSetBase
// -----------------------------------------------------------------------


/* NOTE: Local and global mappings of index sets should be implemented
 *       without IndexSet member functions like this:
 *
 *       dash::local(index_set) {
 *         return dash::index(
 *                  // map the index set's view to local type, not the
 *                  // index set itself:
 *                  dash::local(index_set.view())
 *                );
 *
 */


template <
  class       IndexSetType,
  class       DomainType,
  std::size_t NDim >
constexpr auto
local(
  const IndexSetBase<IndexSetType, DomainType, NDim> & index_set)
//  -> decltype(index_set.local()) {
//  -> typename view_traits<IndexSetBase<DomainType>>::global_type & { 
    -> const IndexSetLocal<DomainType> & { 
  return index_set.local();
}

template <
  class       IndexSetType,
  class       DomainType,
  std::size_t NDim >
constexpr auto
global(
  const IndexSetBase<IndexSetType, DomainType, NDim> & index_set)
//   ->  decltype(index_set.global()) {
//   -> typename view_traits<IndexSetSub<DomainType>>::global_type & { 
     -> const IndexSetGlobal<DomainType> & { 
  return index_set.global();
}


namespace detail {

template <class DomainT>
struct index_set_domain_bind_t {
  typedef typename
            std::conditional< dash::is_view<DomainT>::value,
                              DomainT,
                              const DomainT & >::type
    type;
};

} // namespace detail

/**
 * \concept{DashRangeConcept}
 */
template <
  class       IndexSetType,
  class       DomainType,
  std::size_t NDim = DomainType::ndim() >
class IndexSetBase
{
  typedef IndexSetBase<IndexSetType, DomainType, NDim> self_t;

  typedef typename std::decay<DomainType>::type DomainValueT;

 public:
  typedef typename dash::view_traits<DomainValueT>::origin_type
    view_origin_type;
  typedef DomainValueT
    view_domain_type;
  typedef typename DomainValueT::local_type
    view_local_type;
  typedef typename dash::view_traits<DomainValueT>::global_type
    view_global_type;

  typedef typename view_traits<DomainValueT>::index_set_type
    domain_type;
  typedef typename view_traits<view_origin_type>::pattern_type
    pattern_type;
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

  typedef typename detail::index_set_domain_bind_t<DomainType>::type
    domain_member_type;

  typedef std::integral_constant<std::size_t, NDim>
    rank;

  static constexpr std::size_t ndim() { return NDim; }

 protected:
  domain_member_type      _domain;
  const pattern_type   * _pattern;

  constexpr const IndexSetType & derived() const {
    return static_cast<const IndexSetType &>(*this);
  }
  
  constexpr explicit IndexSetBase(const DomainType & domain)
  : _domain(domain)
  , _pattern(&dash::origin(domain).pattern())
  { }

  constexpr explicit IndexSetBase(DomainType && domain)
  : _domain(std::forward<DomainType>(domain))
  , _pattern(&dash::origin(view_domain()).pattern())
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
    -> decltype(dash::index(this->view_domain())) {
    return dash::index(this->view_domain());
  }

  constexpr const pattern_type & pattern() const {
    return *_pattern;
  }

  constexpr const local_type local() const {
    return dash::index(dash::local(_domain));
  }
 
  constexpr const global_type global() const {
    return dash::index(dash::global(_domain));
  }

  // ---- extents ---------------------------------------------------------

  constexpr std::array<size_type, NDim>
  extents() const {
    return pattern().extents();
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

// -----------------------------------------------------------------------
// IndexSetIdentity
// -----------------------------------------------------------------------

template <class DomainType>
constexpr IndexSetIdentity<DomainType> &
local(const IndexSetIdentity<DomainType> & index_set) {
  return index_set;
}

/**
 * \concept{DashRangeConcept}
 */
template <class DomainType>
class IndexSetIdentity
: public IndexSetBase<
           IndexSetIdentity<DomainType>,
           DomainType >
{
  typedef IndexSetIdentity<DomainType>          self_t;
  typedef IndexSetBase<self_t, DomainType>      base_t;
 public:
  typedef typename base_t::iterator           iterator;
 public:
  constexpr IndexSetIdentity()               = delete;
  constexpr IndexSetIdentity(self_t &&)      = default;
  constexpr IndexSetIdentity(const self_t &) = default;
  ~IndexSetIdentity()                        = default;
  self_t & operator=(self_t &&)              = default;
  self_t & operator=(const self_t &)         = default;
 public:
  typedef typename DomainType::index_type     index_type;
 public:
  constexpr explicit IndexSetIdentity(const DomainType & view)
  : base_t(view)
  { }
  constexpr explicit IndexSetIdentity(DomainType && view)
  : base_t(std::forward<DomainType>(view))
  { }

  constexpr index_type rel(index_type image_index) const {
    return image_index;
  }

  constexpr index_type size() const {
    return this->view_domain().size();
  }

  constexpr iterator begin() const {
    return iterator(*this, 0);
  }

  constexpr iterator end() const {
    return iterator(*this, size());
  }

  constexpr index_type operator[](index_type image_index) const {
    return image_index;
  }

  template <dim_t NDim>
  constexpr index_type operator[](
    const std::array<index_type, NDim> & coords) const {
    return -1;
  }

  constexpr const self_t & pre() const {
    return *this;
  }
};

// -----------------------------------------------------------------------
// IndexSetBlocks
// -----------------------------------------------------------------------

/*
 * TODO:
 *   Assuming 1-dimensional views for blocks, some patterns provide
 *   n-dimensional arrangement of blocks, however.
 */

template <class DomainType>
class IndexSetBlocks
: public IndexSetBase<
           IndexSetBlocks<DomainType>,
           DomainType,
           1 >
{
  typedef IndexSetBlocks<DomainType>                            self_t;
  typedef IndexSetBase<self_t, DomainType, 1>                   base_t;
 public:
  typedef typename DomainType::index_type                   index_type;

  typedef self_t                                            local_type;
  typedef IndexSetGlobal<DomainType>                       global_type;
  typedef global_type                                    preimage_type;

  typedef typename base_t::iterator                           iterator;
  typedef typename base_t::pattern_type                   pattern_type;

  typedef dash::local_index_t<index_type>             local_index_type;
  typedef dash::global_index_t<index_type>           global_index_type;

 private:
  index_type _size;

  constexpr static dim_t NDim = 1;
  constexpr static bool  view_domain_is_local
    = dash::view_traits<DomainType>::is_local::value;
 public:
  constexpr IndexSetBlocks()               = delete;
  constexpr IndexSetBlocks(self_t &&)      = default;
  constexpr IndexSetBlocks(const self_t &) = default;
  ~IndexSetBlocks()                        = default;
  self_t & operator=(self_t &&)            = default;
  self_t & operator=(const self_t &)       = default;

 public:
  /**
   * Constructor, creates index set for given view.
   */
  constexpr explicit IndexSetBlocks(const DomainType & view)
  : base_t(view)
  , _size(calc_size())
  { }

  /**
   * Constructor, creates index set for given view.
   */
  constexpr explicit IndexSetBlocks(DomainType && view)
  : base_t(std::forward<DomainType>(view))
  , _size(calc_size())
  { }

  constexpr iterator begin() const {
    return iterator(*this, 0);
  }

  constexpr iterator end() const {
    return iterator(*this, size());
  }

  constexpr index_type rel(index_type block_index) const {
    return block_index +
           // index of block at first index in domain
           ( view_domain_is_local
               // global coords to local block index:
             ? this->pattern().local_block_at(
                 // global offset to global coords:
                 this->pattern().coords(
                   // local offset to global offset:
                   this->pattern().global(
                     this->domain()[0]
                   )
                 )
               ).index
               // global coords to local block index:
             : this->pattern().block_at(
                 // global offset to global coords:
                 this->pattern().coords(this->domain()[0] ))
           );
  }

  constexpr index_type operator[](index_type block_index) const noexcept {
    return rel(block_index);
  }

  template <dim_t NDim>
  constexpr index_type operator[](
    const std::array<index_type, NDim> & block_coords) const noexcept {
    return -1;
  }

  constexpr index_type size() const {
    return _size; // calc_size();
  }

 private:
  constexpr index_type calc_size() const {
    return (
      view_domain_is_local
      ? ( // index of block at last index in domain
          this->pattern().local_block_at(
            this->pattern().coords(
              // local offset to global offset:
              this->pattern().global(
                this->domain().last()
              )
            )
          ).index -
          // index of block at first index in domain
          this->pattern().local_block_at(
            this->pattern().coords(
              // local offset to global offset:
              this->pattern().global(
                this->domain().first()
              )
            )
          ).index + 1 )
      : ( // index of block at last index in domain
          this->pattern().block_at(
            this->pattern().coords(this->domain().last())
          ) -
          // index of block at first index in domain
          this->pattern().block_at(
            this->pattern().coords(this->domain().first())
          ) + 1 )
    );
  }
}; // class IndexSetBlocks

// -----------------------------------------------------------------------
// IndexSetBlock
// -----------------------------------------------------------------------
#if 0
// Currently using IndexSetSub instead
//
template <class DomainType>
class IndexSetBlock
: public IndexSetBase<
           IndexSetBlock<DomainType>,
           DomainType,
           1 >
{
  typedef IndexSetBlock<DomainType>                               self_t;
  typedef IndexSetBase<self_t, DomainType>                        base_t;
 public:
  typedef typename DomainType::index_type                     index_type;
   
  typedef self_t                                            local_type;
  typedef IndexSetGlobal<DomainType>                         global_type;
  typedef global_type                                    preimage_type;

  typedef typename base_t::iterator                           iterator;
  typedef typename base_t::pattern_type                   pattern_type;
  
  typedef dash::local_index_t<index_type>             local_index_type;
  typedef dash::global_index_t<index_type>           global_index_type;
  
 private:
  index_type _block_idx;
  index_type _size;

  constexpr static dim_t NDim = 1;
  constexpr static bool  view_domain_is_local
    = dash::view_traits<DomainType>::is_local::value;
 public:
  constexpr IndexSetBlock()                = delete;
  constexpr IndexSetBlock(self_t &&)       = default;
  constexpr IndexSetBlock(const self_t &)  = default;
  ~IndexSetBlock()                         = default;
  self_t & operator=(self_t &&)            = default;
  self_t & operator=(const self_t &)       = default;

 public:
  constexpr explicit IndexSetBlock(
    const DomainType & view,
    index_type       block_idx)
  : base_t(view)
  , _block_idx(block_idx)
  , _size(calc_size())
  { }

  constexpr iterator begin() const {
    return iterator(*this, 0);
  }

  constexpr iterator end() const {
    return iterator(*this, size());
  }

  constexpr index_type rel(index_type block_phase) const {
    return block_phase +
           ( view_domain_is_local
             ? ( // index of block at last index in domain
                 this->pattern().local_block_at(
                   this->pattern().coords(
                     // local offset to global offset:
                     this->pattern().global(
                       *(this->domain().begin())
                     )
                   )
                 ).index )
             : ( // index of block at first index in domain
                 this->pattern().block_at(
                   {{ *(this->domain().begin()) }}
                 ) )
           );
  }

  constexpr index_type operator[](index_type block_phase) const noexcept {
    return rel(block_phase);
  }

  template <dim_t NDim>
  constexpr index_type operator[](
    const std::array<index_type, NDim> & block_phase_coords) const noexcept {
    return -1;
  }

  constexpr index_type size() const {
    return _size; // calc_size();
  }

 private:
  constexpr index_type calc_size() const {
    return ( view_domain_is_local
             ? ( // index of block at last index in domain
                 this->pattern().local_block_at(
                   {{ *( this->domain().begin()
                         + (this->domain().size() - 1) ) }}
                 ).index -
                 // index of block at first index in domain
                 this->pattern().local_block_at(
                   {{ *( this->domain().begin() ) }}
                 ).index + 1 )
             : ( // index of block at last index in domain
                 this->pattern().block_at(
                   {{ *( this->domain().begin()
                         + (this->domain().size() - 1) ) }}
                 ) -
                 // index of block at first index in domain
                 this->pattern().block_at(
                   {{ *(this->domain().begin()) }}
                 ) + 1 )
           );
  }
}; // class IndexSetBlock
#endif

// -----------------------------------------------------------------------
// IndexSetSub
// -----------------------------------------------------------------------

template <
  class       DomainType,
  std::size_t SubDim >
constexpr auto
local(const IndexSetSub<DomainType, SubDim> & index_set) ->
// decltype(index_set.local()) {
  typename view_traits<IndexSetSub<DomainType, SubDim>>::local_type & {
  return index_set.local();
}

template <
  class       DomainType,
  std::size_t SubDim >
constexpr auto
global(const IndexSetSub<DomainType, SubDim> & index_set) ->
// decltype(index_set.global()) {
  typename view_traits<IndexSetSub<DomainType, SubDim>>::global_type & {
  return index_set.global();
}

/**
 * \concept{DashRangeConcept}
 */
template <
  class       DomainType,
  std::size_t SubDim = 0 >
class IndexSetSub
: public IndexSetBase<
           IndexSetSub<DomainType, SubDim>,
           DomainType >
{
  typedef IndexSetSub<DomainType, SubDim>                       self_t;
  typedef IndexSetBase<self_t, DomainType>                      base_t;
 public:
  typedef typename base_t::index_type                       index_type;
  typedef typename base_t::size_type                         size_type;
  typedef typename base_t::view_origin_type           view_origin_type;
  typedef typename base_t::view_domain_type           view_domain_type;
  typedef typename base_t::pattern_type                   pattern_type;
  typedef typename base_t::local_type                       local_type;
  typedef typename base_t::global_type                     global_type;
  typedef typename base_t::iterator                           iterator;
//typedef IndexSetSub<DomainType, SubDim>                preimage_type;
  typedef IndexSetSub<view_origin_type, SubDim>          preimage_type;

 public:
  constexpr IndexSetSub()               = delete;
  constexpr IndexSetSub(self_t &&)      = default;
  constexpr IndexSetSub(const self_t &) = default;
  ~IndexSetSub()                        = default;
  self_t & operator=(self_t &&)         = default;
  self_t & operator=(const self_t &)    = default;
 private:
  index_type _domain_begin_idx;
  index_type _domain_end_idx;

  static constexpr std::size_t NDim = base_t::ndim();
 public:
  /**
   * Constructor, creates index set for given view.
   */
  constexpr IndexSetSub(
    const DomainType   & view,
    index_type         begin_idx,
    index_type         end_idx)
  : base_t(view)
  , _domain_begin_idx(begin_idx)
  , _domain_end_idx(end_idx)
  { }

  /**
   * Constructor, creates index set for given view.
   */
  constexpr IndexSetSub(
    DomainType        && view,
    index_type         begin_idx,
    index_type         end_idx)
  : base_t(std::forward<DomainType>(view))
  , _domain_begin_idx(begin_idx)
  , _domain_end_idx(end_idx)
  { }

  // ---- extents ---------------------------------------------------------

  template <std::size_t ExtDim>
  constexpr size_type extent() const {
    return ( ExtDim == SubDim
             ? _domain_end_idx - _domain_begin_idx
             : this->domain().template extent<ExtDim>()
           );
  }

  constexpr size_type extent(std::size_t shape_dim) const {
    return ( shape_dim == SubDim
             ? _domain_end_idx - _domain_begin_idx
             : this->domain().extent(shape_dim)
           );
  }

  constexpr std::array<size_type, NDim> extents() const {
    return dash::ce::replace_nth<SubDim>(
             extent<SubDim>(),
             this->domain().extents());
  }

  // ---- offsets ---------------------------------------------------------

  template <std::size_t ExtDim>
  constexpr index_type offset() const {
    return ( ExtDim == SubDim
             ? _domain_begin_idx
             : this->domain().template offset<ExtDim>()
           );
  }

  constexpr index_type offset(std::size_t shape_dim) const {
    return ( shape_dim == SubDim
             ? _domain_begin_idx
             : this->domain().offset(shape_dim)
           );
  }

  constexpr std::array<index_type, NDim> offsets() const {
    return dash::ce::replace_nth<SubDim>(
             offset<SubDim>(),
             this->domain().offsets());
  }
  
  // ---- size ------------------------------------------------------------

  constexpr size_type size(std::size_t sub_dim = 0) const {
    return extent(sub_dim) *
             (sub_dim + 1 < NDim && NDim > 0
               ? size(sub_dim + 1)
               : 1);
  }

  // ---- access ----------------------------------------------------------

  /**
   * Domain index at specified linear offset.
   */
  constexpr index_type rel(index_type image_index) const {
    return ( 
             ( NDim == 1
               ? _domain_begin_idx + image_index
               : ( SubDim == 0
                   // Rows sub section:
                   ? ( // full rows in domain:
                       (offset(0) * this->domain().extent(1))
                     + image_index )
                   // Columns sub section:
                   : ( // first index:
                       offset(1)
                       // row in view region:
                     + ( (image_index / extent(1))
                         * this->domain().extent(1))
                     + image_index % extent(1) )
                 )
             )
           );
  }

  /**
   * Domain index at specified Cartesian coordinates.
   */
  constexpr index_type rel(
    const std::array<index_type, NDim> & coords) const {
    return -1;
  }

  constexpr iterator begin() const {
    return iterator(*this, 0);
  }

  constexpr iterator end() const {
    return iterator(*this, size());
  }

  constexpr preimage_type pre() const {
    return preimage_type(
             dash::origin(this->view_domain()),
             -(this->operator[](0)),
             -(this->operator[](0))
               + dash::origin(this->view_domain()).size()
           );
  }
}; // class IndexSetSub

// -----------------------------------------------------------------------
// IndexSetLocal
// -----------------------------------------------------------------------

template <class DomainType>
constexpr const IndexSetLocal<DomainType> &
local(const IndexSetLocal<DomainType> & index_set) {
  return index_set;
}

template <class DomainType>
constexpr auto
global(const IndexSetLocal<DomainType> & index_set) ->
// decltype(index_set.global()) {
  typename view_traits<IndexSetLocal<DomainType>>::global_type & { 
  return index_set.global();
}

/**
 * \concept{DashRangeConcept}
 */
template <class DomainType>
class IndexSetLocal
: public IndexSetBase<
           IndexSetLocal<DomainType>,
           DomainType >
{
  typedef IndexSetLocal<DomainType>                             self_t;
  typedef IndexSetBase<self_t, DomainType>                      base_t;
 public:
  typedef typename DomainType::index_type                   index_type;
  typedef typename DomainType::size_type                     size_type;

  typedef self_t                                            local_type;
  typedef IndexSetGlobal<DomainType>                       global_type;
  typedef global_type                                    preimage_type;

  typedef typename base_t::iterator                           iterator;
  typedef typename base_t::pattern_type                   pattern_type;

  typedef dash::local_index_t<index_type>             local_index_type;
  typedef dash::global_index_t<index_type>           global_index_type;

 private:
  index_type _size;
 public:
  constexpr IndexSetLocal()               = delete;
  constexpr IndexSetLocal(self_t &&)      = default;
  constexpr IndexSetLocal(const self_t &) = default;
  ~IndexSetLocal()                        = default;
  self_t & operator=(self_t &&)           = default;
  self_t & operator=(const self_t &)      = default;

 public:
  /**
   * Constructor, creates index set for given view.
   */
  constexpr explicit IndexSetLocal(const DomainType & view)
  : base_t(view)
  , _size(calc_size())
  { }

  /**
   * Constructor, creates index set for given view.
   */
  constexpr explicit IndexSetLocal(DomainType && view)
  : base_t(std::forward<DomainType>(view))
  , _size(calc_size())
  { }

  constexpr const local_type & local() const noexcept {
    return *this;
  }

  constexpr global_type global() const noexcept {
    return global_type(this->view_domain());
  }

  constexpr preimage_type pre() const noexcept {
    return preimage_type(this->view_domain());
  }

  // ---- extents ---------------------------------------------------------

  // TODO:
  //
  // Index set types should specify extent<D> and apply mapping of domain
  // (as in calc_size) with extents() implemented in IndexSetBase as
  // sequence { extent<d>... }.
  //
  constexpr auto extents() const noexcept
    -> decltype(
         std::declval<
           typename std::add_lvalue_reference<const pattern_type>::type
         >().local_extents()) {
    return this->pattern().local_extents();
  }

  template <std::size_t ShapeDim>
  constexpr index_type extent() const noexcept {
    return this->pattern().local_extents()[ShapeDim];
  }

  constexpr index_type extent(std::size_t shape_dim) const noexcept {
    return this->pattern().local_extents()[shape_dim];
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size(std::size_t sub_dim) const noexcept {
    return _size;
  }

  constexpr size_type size() const noexcept {
    return size(0);
  }

  // TODO:
  // 
  // Should be accumulate of extents().
  //
  constexpr index_type calc_size() const noexcept {
    typedef typename dash::pattern_partitioning_traits<pattern_type>::type
            pat_partitioning_traits;

    static_assert(
        pat_partitioning_traits::rectangular,
        "index sets for non-rectangular patterns are not supported yet");

    /*
      dash::ce::index_sequence<Dims...>
      return dash::ce::accumulate<index_type, NDim>(
               {{ (extent<Dims>())... }},     // values
               0, NDim,                       // index range
               0,                             // accumulate init
               std::multiplies<index_type>()  // reduce op
             );
    */
    return (
      this->pattern().blockspec().size() <= this->pattern().team().size()
       // blocked (not blockcyclic) distribution: single local
       // element space with contiguous global index range
        ? this->index_range_size(
            this->index_range_intersect(
              // local range in global index space:
              { this->pattern().global(0),
                this->pattern().global(
                  this->pattern().local_size() - 1) },
              // domain range in global index space;
              { this->domain().first(),
                this->domain().last() }
            )) + 1 
       // blockcyclic distribution: local element space chunked
       // in global index range
        : this->index_range_size(
            this->index_range_g2l(
              this->pattern(),
              // intersection of local range and domain range:
              this->index_range_intersect(
                // local range in global index space:
                { this->pattern().global(0),
                  this->pattern().global(
                    this->pattern().local_size() - 1) },
                // domain range in global index space;
                { this->domain().first(),
                  this->domain().last() }
            ))) + 1
    );
  }

  // ---- access ----------------------------------------------------------

  constexpr iterator begin() const noexcept {
    return iterator(*this, 0);
  }

  constexpr iterator end() const noexcept {
    return iterator(*this, size());
  }

  constexpr index_type rel(index_type local_index) const noexcept {
    // NOTE:
    // Random access operator must allow access at [end] because
    // end iterator of an index range may be dereferenced.
    return local_index +
              // only required if local of sub
              ( this->domain()[0] == 0
                ? 0
                : this->pattern().local(
                    std::max<index_type>(
                      this->pattern().global(0),
                      this->domain()[0]
                  )).index
              );
  }

  constexpr index_type operator[](index_type local_index) const noexcept {
    return rel(local_index);
  }

  template <dim_t NDim>
  constexpr index_type operator[](
    const std::array<index_type, NDim> & local_coords) const noexcept {
    return -1;
  }
}; // class IndexSetLocal

// -----------------------------------------------------------------------
// IndexSetGlobal
// -----------------------------------------------------------------------

template <class DomainType>
constexpr auto
local(const IndexSetGlobal<DomainType> & index_set)
    -> decltype(index_set.local())
{
//  -> typename view_traits<IndexSetGlobal<DomainType>>::local_type & {
  return index_set.local();
}

template <class DomainType>
constexpr const IndexSetGlobal<DomainType> &
global(const IndexSetGlobal<DomainType> & index_set)
{
  return index_set;
}

/**
 * \concept{DashRangeConcept}
 */
template <class DomainType>
class IndexSetGlobal
: public IndexSetBase<
           IndexSetGlobal<DomainType>,
           DomainType >
{
  typedef IndexSetGlobal<DomainType>                            self_t;
  typedef IndexSetBase<self_t, DomainType>                      base_t;
 public:
  typedef typename DomainType::index_type                   index_type;

  typedef IndexSetLocal<self_t>                             local_type;
  typedef self_t                                           global_type;
  typedef local_type                                     preimage_type;

  typedef typename base_t::iterator                           iterator;

  typedef dash::local_index_t<index_type>             local_index_type;
  typedef dash::global_index_t<index_type>           global_index_type;

 public:
  constexpr IndexSetGlobal()               = delete;
  constexpr IndexSetGlobal(self_t &&)      = default;
  constexpr IndexSetGlobal(const self_t &) = default;
  ~IndexSetGlobal()                        = default;
  self_t & operator=(self_t &&)            = default;
  self_t & operator=(const self_t &)       = default;
 private:
  index_type _size;
 public:
  /**
   * Constructor, creates index set for given view.
   */
  constexpr explicit IndexSetGlobal(const DomainType & view)
  : base_t(view)
  , _size(calc_size())
  { }

  /**
   * Constructor, creates index set for given view.
   */
  constexpr explicit IndexSetGlobal(DomainType && view)
  : base_t(std::forward<DomainType>(view))
  , _size(calc_size())
  { }

  constexpr index_type rel(index_type global_index) const {
    // NOTE:
    // Random access operator must allow access at [end] because
    // end iterator of an index range may be dereferenced.
    return this->pattern().local(
             global_index
           ).index;
  }

  constexpr index_type calc_size() const {
    return std::max<index_type>(
             this->pattern().size(),
             this->domain().size()
           );
  }

  constexpr index_type size() const {
    return _size; // calc_size();
  }

  constexpr const local_type & local() const {
    return dash::index(dash::local(this->domain()));
  }

  constexpr const global_type & global() const {
    return *this;
  }

  constexpr const preimage_type & pre() const {
    return dash::index(dash::local(this->domain()));
  }
}; // class IndexSetGlobal

} // namespace dash
#endif // DOXYGEN

#endif // DASH__VIEW__INDEX_SET_H__INCLUDED
