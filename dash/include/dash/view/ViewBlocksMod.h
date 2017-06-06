#ifndef DASH__VIEW__VIEW_BLOCKS_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_BLOCKS_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Range.h>
#include <dash/Iterator.h>

#include <dash/util/UniversalMember.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewMod.h>

#include <dash/view/Local.h>
#include <dash/view/Global.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/Apply.h>
#include <dash/view/Chunked.h>
#include <dash/view/Sub.h>

#include <type_traits>


namespace dash {

#ifndef DOXYGEN

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------
//
template <
  class DomainType,
  dim_t NDim = dash::view_traits<
                 typename std::decay<DomainType>::type>::rank::value >
class ViewBlocksMod;


// ------------------------------------------------------------------------
// ViewBlockMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t NDim >
struct view_traits<ViewBlockMod<DomainType, NDim> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef ViewBlockMod<DomainType, NDim>                        image_type;
  typedef ViewBlockMod<DomainType, NDim>                        local_type;
  typedef ViewBlockMod<DomainType, NDim>                       global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef typename view_traits<domain_type>::size_type           size_type;
  typedef typename std::conditional<
                     NDim == 1,
                     dash::IndexSetSub<DomainType, 0>,
                     dash::IndexSetBlock<DomainType>
                   >::type                                  index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool,
    view_traits<domain_type>::is_local::value >              is_local;

  typedef std::integral_constant<dim_t, NDim>                rank;
};


// ------------------------------------------------------------------------
// ViewBlockMod<N>
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t NDim >
class ViewBlockMod
: public ViewModBase <
           ViewBlockMod<DomainType, NDim>,
           DomainType,
           NDim >
{
 private:
  typedef ViewBlockMod<DomainType, NDim>                            self_t;
  typedef ViewModBase<
            ViewBlockMod<DomainType, NDim>, DomainType, NDim>       base_t;
 public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;
  typedef typename base_t::origin_type                         origin_type;
 public:
  typedef dash::IndexSetBlock<DomainType>                   index_set_type;
  typedef ViewLocalMod<self_t>                                  local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, false>                     is_local;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<
                  origin_type>::type
              >() ))
    origin_iterator;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<
                  const origin_type>::type
              >() ))
    const_origin_iterator;

  typedef ViewIterator<origin_iterator, index_set_type>
    iterator;
  typedef ViewIterator<const_origin_iterator, index_set_type>
    const_iterator;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<
                   origin_type>::type
               >() ))
    reference;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<
                   const origin_type>::type
               >() ))
    const_reference;

 private:
  index_set_type _index_set;

 public:
  constexpr ViewBlockMod()               = delete;
  constexpr ViewBlockMod(self_t &&)      = default;
  constexpr ViewBlockMod(const self_t &) = default;
  ~ViewBlockMod()                        = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;

  constexpr ViewBlockMod(
    const domain_type & domain,
    index_type          block_idx)
  : base_t(domain)
  , _index_set(domain,
               block_idx)
  { }

  /**
   * Constructor, creates a view on a block in the specified domain.
   */
  constexpr ViewBlockMod(
    domain_type     && domain,
    index_type         block_idx)
  : base_t(std::move(domain))
  , _index_set(this->domain(),
               block_idx)
  { }

  // ---- extents ---------------------------------------------------------

  constexpr std::array<size_type, NDim> extents() const {
    return _index_set.extents();
  }

  template <dim_t ExtDim>
  constexpr size_type extent() const {
    return _index_set.template extent<ExtDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return _index_set.extent(shape_dim);
  }

  // ---- offsets ---------------------------------------------------------

  template <dim_t ExtDim>
  constexpr index_type offset() const {
    return _index_set.template offset<ExtDim>();
  }

  constexpr std::array<index_type, NDim> offsets() const {
    return _index_set.offsets();
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return _index_set.offset(shape_dim);
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size() const {
    return index_set().size();
  }

  constexpr const_iterator begin() const {
    return const_iterator(dash::origin(*this).begin(),
                          _index_set, 0);
  }

  iterator begin() {
    return iterator(const_cast<origin_type &>(
                      dash::origin(*this)
                    ).begin(),
                    _index_set, 0);
  }

  constexpr const_iterator end() const {
    return const_iterator(dash::origin(*this).begin(),
                          _index_set, _index_set.size());
  }

  iterator end() {
    return iterator(const_cast<origin_type &>(
                      dash::origin(*this)
                    ).begin(),
                    _index_set, _index_set.size());
  }

  constexpr const_reference operator[](int offset) const {
    return *(const_iterator(dash::origin(*this).begin(),
                            _index_set, offset));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr local_type local() const {
    return local_type(*this);
  }
}; // class ViewBlockMod<N>


// ------------------------------------------------------------------------
// ViewBlockMod<1>
// ------------------------------------------------------------------------

template <
  class DomainType >
class ViewBlockMod<DomainType, 1>
// One-dimensional blocks are actually just an adapter for
// block_idx -> sub(begin_idx, end_idx), could be defined by sublassing
//
//   public ViewSubMod<DomainType, 0>
//
: public ViewModBase <
           ViewBlockMod<DomainType, 1>,
           DomainType,
           1 >
{
 private:
  typedef ViewBlockMod<DomainType, 1>                               self_t;
  typedef ViewModBase< ViewBlockMod<DomainType, 1>, DomainType, 1 > base_t;
 public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename base_t::origin_type                         origin_type;
 public:
  typedef dash::IndexSetSub< DomainType, 0 >                index_set_type;
  typedef ViewLocalMod<self_t>                                  local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, false>                     is_local;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<
                  origin_type>::type
              >() ))
    origin_iterator;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<
                  const origin_type>::type
              >() ))
    const_origin_iterator;

  typedef ViewIterator<origin_iterator, index_set_type>
    iterator;
  typedef ViewIterator<const_origin_iterator, index_set_type>
    const_iterator;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<
                   origin_type>::type
               >() ))
    reference;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<
                   const origin_type>::type
               >() ))
    const_reference;

 private:
  index_set_type _index_set;

 public:
  constexpr ViewBlockMod()               = delete;
  constexpr ViewBlockMod(self_t &&)      = default;
  constexpr ViewBlockMod(const self_t &) = default;
  ~ViewBlockMod()                        = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;

  constexpr ViewBlockMod(
    const domain_type & domain,
    index_type          block_idx)
  : base_t(domain)
  , _index_set(domain,
               block_first_gidx(domain, block_idx),
               block_final_gidx(domain, block_idx))
  { }

  /**
   * Constructor, creates a view on a block in the specified domain.
   */
  constexpr ViewBlockMod(
    domain_type     && domain,
    index_type         block_idx)
  : base_t(std::move(domain))
  , _index_set(this->domain(),
               block_first_gidx(this->domain(), block_idx),
               block_final_gidx(this->domain(), block_idx))
  { }

  constexpr const_iterator begin() const {
    return const_iterator(dash::origin(*this).begin(),
                          _index_set, 0);
  }

  iterator begin() {
    return iterator(const_cast<origin_type &>(
                      dash::origin(*this)
                    ).begin(),
                    _index_set, 0);
  }

  constexpr const_iterator end() const {
    return const_iterator(dash::origin(*this).begin(),
                          _index_set, _index_set.size());
  }

  iterator end() {
    return iterator(const_cast<origin_type &>(
                      dash::origin(*this)
                    ).begin(),
                    _index_set, _index_set.size());
  }

  constexpr const_reference operator[](int offset) const {
    return *(const_iterator(dash::origin(*this).begin(),
                            _index_set, offset));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr local_type local() const {
    return local_type(*this);
  }

 private:
  /// Index of first element in block view
  ///
  constexpr index_type block_first_gidx(
      const DomainType & vdomain,
      index_type         block_idx) const {
    // If domain is local, block_idx refers to local block range
    // so use pattern().local_block(block_idx)
    //
    // TODO: Currently values passed as `block_idx` are global block
    //       indices, even if domain is local
    return std::max(
             ( // block viewspec (extents, offsets)
               ( false &&
                 dash::view_traits<DomainType>::is_local::value
                 ? dash::index(vdomain)
                     .pattern().local_block(block_idx).offsets()[0]
                 : dash::index(vdomain)
                     .pattern().block(block_idx).offsets()[0]
               )
             ),
             dash::index(vdomain).first()
           )
           - dash::index(vdomain).first();
  }

  /// Index past last element in block view:
  ///
  constexpr index_type block_final_gidx(
      const DomainType & vdomain,
      index_type         block_idx) const {
    // If domain is local, block_idx refers to local block range
    // so use pattern().local_block(block_idx)
    //
    // TODO: Currently values passed as `block_idx` are global block 
    //       indices, even if domain is local
    return std::min<index_type>(
             dash::index(vdomain).last() + 1,
             ( // block viewspec (extents, offsets)
               ( false &&
                 dash::view_traits<DomainType>::is_local::value
                 ? dash::index(vdomain)
                     .pattern().local_block(block_idx).offsets()[0]
                 : dash::index(vdomain)
                     .pattern().block(block_idx).offsets()[0]
               )
             + ( false &&
                 dash::view_traits<DomainType>::is_local::value
                 ? dash::index(vdomain)
                     .pattern().local_block(block_idx).extents()[0]
                 : dash::index(vdomain)
                     .pattern().block(block_idx).extents()[0]
               )
             )
           )
           - dash::index(vdomain).first();
  }
}; // class ViewBlockMod<1>

// ------------------------------------------------------------------------
// ViewBlocksMod
// ------------------------------------------------------------------------

template <
  class ViewType,
  class ViewValueT
          = typename std::decay<ViewType>::type >
constexpr ViewBlocksMod<ViewValueT>
blocks(ViewType && domain) {
  return ViewBlocksMod<ViewValueT>(std::forward<ViewType>(domain));
}

template <
  class DomainType,
  dim_t NDim >
struct view_traits<ViewBlocksMod<DomainType, NDim> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef ViewBlocksMod<DomainType, NDim>                       image_type;
  typedef typename view_traits<domain_type>::local_type         local_type;
  typedef ViewBlocksMod<DomainType, NDim>                      global_type;

  typedef typename view_traits<domain_type>::index_type         index_type;
  typedef typename view_traits<domain_type>::size_type           size_type;
  typedef dash::IndexSetBlocks<DomainType>
                                                            index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef std::integral_constant<bool,
    view_traits<domain_type>::is_local::value >              is_local;

  typedef std::integral_constant<dim_t, DomainType::rank::value> rank;
};

template <
  class DomainType,
  dim_t NDim >
class ViewBlocksMod
: public ViewModBase< ViewBlocksMod<DomainType, NDim>, DomainType, NDim > {
 private:
  typedef ViewBlocksMod<DomainType, NDim>                           self_t;
//typedef ViewBlocksMod<const DomainType, NDim>               const_self_t;
  typedef ViewBlocksMod<DomainType, NDim>                     const_self_t;
  typedef ViewModBase<ViewBlocksMod<DomainType, NDim>, DomainType, NDim>
                                                                    base_t;
 public:
  typedef DomainType                                           domain_type;
  typedef typename base_t::origin_type                         origin_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;
 private:
  typedef ViewBlockMod<DomainType, NDim>                        block_type;
  typedef typename view_traits<domain_type>::local_type  domain_local_type;
 public:
  typedef dash::IndexSetBlocks<DomainType>                  index_set_type;
  typedef self_t                                               global_type;
  typedef ViewBlocksMod<domain_local_type, NDim>                local_type;

  typedef std::integral_constant<bool, false>                     is_local;

 private:
  index_set_type  _index_set;

 public:
  template <class ViewBlocksModType>
  class block_iterator
  : public internal::IndexIteratorBase<
             block_iterator<ViewBlocksModType>,
             block_type,
             index_type,
             std::nullptr_t,
             block_type >
  { 
   private:
    typedef internal::IndexIteratorBase<
              block_iterator<ViewBlocksModType>,
              block_type,     // value type
              index_type,     // difference type
              std::nullptr_t, // pointer type
              block_type >    // reference type
      iterator_base_t;
    typedef typename view_traits<ViewBlocksModType>::domain_type
      blocks_view_domain_type;
   private:
    const blocks_view_domain_type & _blocks_view_domain;
   public:
    constexpr block_iterator()                         = delete;
    constexpr block_iterator(block_iterator &&)        = default;
    constexpr block_iterator(const block_iterator &)   = default;
    ~block_iterator()                                  = default;
    block_iterator & operator=(block_iterator &&)      = default;
    block_iterator & operator=(const block_iterator &) = default;

    constexpr block_iterator(
      const block_iterator    & other,
      index_type                position)
    : iterator_base_t(position)
    , _blocks_view_domain(other._blocks_view_domain)
    { }

    constexpr block_iterator(
      const ViewBlocksModType & blocks_view,
      index_type                position)
    : iterator_base_t(position)
    , _blocks_view_domain(dash::domain(blocks_view))
    { }

    constexpr block_iterator(
      ViewBlocksModType && blocks_view,
      index_type           position)
    : iterator_base_t(position)
    , _blocks_view_domain(
        dash::domain(std::move(blocks_view)))
    { }

    constexpr block_type dereference(index_type idx) const {
      // Dereferencing block iterator returns block at block index
      // with iterator position.
      // Note that block index is relative to the domain and is
      // translated to global block index in IndexSetBlocks.
      return block_type(_blocks_view_domain, idx);
    }
  };

 public:
  typedef block_iterator<self_t>                    iterator;
  typedef block_iterator<const_self_t>        const_iterator;

  using reference       = typename       iterator::reference;
  using const_reference = typename const_iterator::reference;

 public:
  ~ViewBlocksMod()                        = default;
  constexpr ViewBlocksMod()               = delete;
  constexpr ViewBlocksMod(const self_t &) = default;
  constexpr ViewBlocksMod(self_t &&)      = default;
  self_t & operator=(self_t &&)           = default;
  self_t & operator=(const self_t &)      = default;

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewBlocksMod(
    const domain_type & domain)
  : base_t(domain)
  , _index_set(domain)
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewBlocksMod(
    domain_type && domain)
  : base_t(std::move(domain))
  , _index_set(this->domain())
  { }

  // ---- extents ---------------------------------------------------------

  constexpr std::array<size_type, NDim> extents() const {
    return _index_set.extents();
  }

  template <dim_t ExtDim>
  constexpr size_type extent() const {
    return _index_set.template extent<ExtDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return _index_set.extent(shape_dim);
  }

  // ---- offsets ---------------------------------------------------------

  template <dim_t ExtDim>
  constexpr index_type offset() const {
    return _index_set.template offset<ExtDim>();
  }

  constexpr std::array<index_type, NDim> offsets() const {
    return _index_set.offsets();
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return _index_set.offset(shape_dim);
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size() const {
    return index_set().size();
  }

  // ---- access ----------------------------------------------------------

  constexpr const_iterator begin() const {
    return const_iterator(*this,
                          _index_set.first());
  }
  iterator begin() {
    return iterator(const_cast<self_t &>(*this),
                    _index_set.first());
  }

  constexpr const_iterator end() const {
    return const_iterator(*this,
                          _index_set.last() + 1);
  }
  iterator end() {
    return iterator(const_cast<self_t &>(*this),
                    _index_set.last() + 1);
  }

  constexpr block_type operator[](int offset) const {
    return *iterator(*this, _index_set[offset]);
  }
  block_type operator[](int offset) {
    return *iterator(*this, _index_set[offset]);
  }

  constexpr local_type local() const {
    return local_type(dash::local(this->domain()));
  }

  constexpr const global_type global() const {
    return dash::global(dash::domain(*this));
  }

  global_type global() {
    return dash::global(dash::domain(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }
};

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__VIEW_BLOCKS_MOD_H__INCLUDED
