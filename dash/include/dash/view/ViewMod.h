#ifndef DASH__VIEW__VIEW_MOD_H__INCLUDED
#define DASH__VIEW__VIEW_MOD_H__INCLUDED

#include <dash/Types.h>
#include <dash/Iterator.h>
#include <dash/Range.h>

#include <dash/util/UniversalMember.h>
#include <dash/util/ArrayExpr.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewIterator.h>
#include <dash/view/ViewOrigin.h>

#include <dash/view/Local.h>
#include <dash/view/Global.h>
#include <dash/view/Origin.h>
#include <dash/view/Domain.h>
#include <dash/view/Apply.h>


namespace dash {

/**
 * \defgroup  DashViewExpressionConcept  Multidimensional View Expressions
 *
 * \ingroup DashViewConcept
 * \{
 * \par Description
 *
 * Implementing view modifier chain as combination of command pattern
 * and chain of responsibility pattern.
 * For now, only compile-time projections/slices are supported such as:
 *
 * \code
 *   sub<0>(10,20).sub<1>(30,40)
 * \endcode
 *
 * but not run-time projections/slices like:
 *
 * \code
 *   sub(0, { 10,20 }).sub(1, { 30,40 })
 * \endcode
 *
 * \par Implementation Notes
 *
 * A view composition is a chained application of view modifier types
 * that depend on the type of their predecessor in the chain.
 *
 * Example:
 *
 * \code
 *  sub<0>(2).sub<1>(3,4)
 *  :         :
 *  |         |
 *  |         '--> ViewSubMod<0, ViewSubMod<-1, ViewOrigin> >
 *  |                            '------------.-----------'
 *  |                                         '--> parent
 *  '--> ViewSubMod<-1, ViewOrigin >
 *                      '----.---'
 *                           '--> parent
 * \endcode
 *
 * Consequently, specific ViewMod types are defined for every modifier
 * category.
 *
 * \}
 *
 *
 * \note
 *
 * As an alternative, all view modifications could be stored in command
 * objects of a single ViewMod type. Expressions then could not be
 * evalated at compile-time, however.
 *
 * However, View modifier types should subclass a common ViewMod base
 * class - or vice versa, following the policy pattern with the
 * operation specified as policy:
 *
 * \code
 *   template <dim_t DimDiff, class DomainType>
 *   class ViewMod : DomainType
 *   {
 *      // ...
 *   }
 * \endcode
 *
 * or:
 *
 * \code
 *   template <dim_t DimDiff, class ViewModOperation>
 *   class ViewMod : ViewModOperation
 *   {
 *      // ...
 *   }
 *
 *   class ViewModSubOperation;
 *   // defines
 *   // - sub<N>(...)
 *   // - view_mod_op() { return sub<N>(...); }
 *
 *   ViewMod<0, ViewModSubOperation> view_sub(initializer_list);
 *   // - either calls view_mod_op(initializer_list) in constructor
 *   // - or provides method sub<N>(...) directly
 * \endcode
 *
 * \todo Eventually, these probably are not public definitions.
 *       Move to namespace internal.
 *       Define dereference operator*() for view types, delegating to
 *       domain::operator* recursively.
 */


#ifndef DOXYGEN

// ------------------------------------------------------------------------
// Forward-declarations
// ------------------------------------------------------------------------

template <
  class ViewModType,
  class DomainType,
  dim_t NDim        = dash::view_traits<
                        typename std::decay<DomainType>::type>::rank::value >
class ViewModBase;

template <
  class DomainType,
  dim_t NDim        = dash::view_traits<
                        typename std::decay<DomainType>::type>::rank::value >
class ViewLocalMod;

template <
  class DomainType,
  dim_t SubDim      = 0,
  dim_t NDim        = dash::view_traits<
                        typename std::decay<DomainType>::type>::rank::value >
class ViewSubMod;

template <
  class DomainType,
  dim_t NDim        = dash::view_traits<
                        typename std::decay<DomainType>::type>::rank::value >
class ViewGlobalMod;

#endif // DOXYGEN

// ------------------------------------------------------------------------
// ViewModBase
// ------------------------------------------------------------------------

template <
  class ViewModType,
  class DomainType,
  dim_t NDim >
class ViewModBase
{
  typedef ViewModBase<ViewModType, DomainType, NDim> self_t;
 public:
  typedef DomainType                                          domain_type;

  // TODO: Clarify if origins should always be considered const
  //
  typedef typename std::conditional<(
                        view_traits<domain_type>::is_origin::value &&
                       !view_traits<domain_type>::is_view::value   &&
                       !std::is_copy_constructible<domain_type>::value
                     ),
                     const domain_type &,
                     domain_type
                   >::type
    domain_member_type;

  typedef typename std::conditional<
            view_traits<domain_type>::is_local::value,
            typename view_traits<
              typename view_traits<domain_type>::origin_type
            >::local_type,
            typename view_traits<domain_type>::origin_type
          >::type
    origin_type;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<origin_type>::type
              >() ))
    origin_iterator;

  typedef decltype(
            dash::begin(
              std::declval<
                typename std::add_lvalue_reference<const origin_type>::type
              >() ))
    const_origin_iterator;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<origin_type>::type
               >() ))
    reference;

  typedef
    decltype(*dash::begin(
               std::declval<
                 typename std::add_lvalue_reference<const origin_type>::type
               >() ))
    const_reference;

  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;
  typedef typename origin_type::value_type                      value_type;

  typedef std::integral_constant<dim_t, NDim>                         rank;

  static constexpr dim_t ndim() { return NDim; }

 protected:
  domain_member_type _domain;

  // TODO: Index set should be member of ViewModBase as all models of the
  //       view concept depend on an index set type.
  //       As constructors of index set classes depend on the concrete view
  //       type, index sets should be instantiated in derived view subclass
  //       and passed to ViewModBase base constructor.

  ViewModType & derived() {
    return static_cast<ViewModType &>(*this);
  }
  constexpr const ViewModType & derived() const {
    return static_cast<const ViewModType &>(*this);
  }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewModBase(domain_type && domain)
  : _domain(std::move(domain))
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewModBase(const domain_type & domain)
  : _domain(domain)
  { }

  constexpr ViewModBase()               = delete;
  ~ViewModBase()                        = default;

 public:
  constexpr ViewModBase(const self_t &) = default;
  constexpr ViewModBase(self_t &&)      = default;
  self_t & operator=(const self_t &)    = default;
  self_t & operator=(self_t &&)         = default;

  constexpr domain_type domain() const && {
    return _domain;
  }

  constexpr const domain_type & domain() const & {
    return _domain;
  }

  constexpr bool operator==(const ViewModType & rhs) const {
    return &derived() == &rhs;
  }
  
  constexpr bool operator!=(const ViewModType & rhs) const {
    return !(derived() == rhs);
  }

  constexpr bool is_local() const {
    return view_traits<ViewModType>::is_local::value;
  }

  // ---- extents ---------------------------------------------------------

  constexpr const std::array<size_type, NDim> extents() const {
    return domain().extents();
  }

  template <dim_t ShapeDim>
  constexpr size_type extent() const {
    return domain().template extent<ShapeDim>();
  }

  constexpr size_type extent(dim_t shape_dim) const {
    return domain().extent(shape_dim);
  }

  // ---- offsets ---------------------------------------------------------

  constexpr const std::array<index_type, NDim> offsets() const {
    return domain().offsets();
  }

  template <dim_t ShapeDim>
  constexpr index_type offset() const {
    return domain().template offset<ShapeDim>();
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return domain().offset(shape_dim);
  }

  // ---- size ------------------------------------------------------------

  constexpr index_type size() const {
    return dash::index(derived()).size();
  }
};


// ------------------------------------------------------------------------
// ViewLocalMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t NDim >
struct view_traits<ViewLocalMod<DomainType, NDim> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef typename view_traits<domain_type>::local_type         image_type;
  typedef ViewLocalMod<DomainType, NDim>                        local_type;
  typedef domain_type                                          global_type;

  typedef typename view_traits<domain_type>::index_type         index_type;
  typedef typename view_traits<domain_type>::size_type           size_type;
  typedef dash::IndexSetLocal<DomainType>                   index_set_type;

  typedef std::integral_constant<bool, false> is_projection;
  typedef std::integral_constant<bool, true > is_view;
  typedef std::integral_constant<bool, false> is_origin;
  typedef std::integral_constant<bool, true > is_local;

  typedef typename view_traits<DomainType>::is_contiguous is_contiguous;

  typedef std::integral_constant<dim_t, DomainType::rank::value> rank;
};

template <
  class DomainType,
  dim_t NDim >
class ViewLocalMod
: public ViewModBase<
           ViewLocalMod<DomainType, NDim>,
           DomainType,
           NDim > {
 public:
  typedef DomainType                                           domain_type;
  typedef typename view_traits<DomainType>::origin_type        origin_type;
  typedef typename domain_type::local_type                      image_type;
  typedef typename view_traits<DomainType>::index_type          index_type;
  typedef typename view_traits<DomainType>::size_type            size_type;
 private:
  typedef ViewLocalMod<DomainType, NDim>                            self_t;
  typedef ViewModBase<
            ViewLocalMod<DomainType, NDim>, DomainType, NDim >      base_t;
 public:
  typedef dash::IndexSetLocal<DomainType>                   index_set_type;
  typedef self_t                                                local_type;
  typedef typename domain_type::global_type                    global_type;

  typedef std::integral_constant<bool, true>                      is_local;

  typedef
    decltype(
      dash::begin(
        dash::local(dash::origin(
          std::declval<
            typename std::add_lvalue_reference<
              domain_type>::type >()
      ))))
    origin_iterator;

  typedef
    decltype(
      dash::begin(
        dash::local(dash::origin(
          std::declval<
            typename std::add_lvalue_reference<
              const domain_type>::type >()
      ))))
    const_origin_iterator;

  typedef ViewIterator<
            origin_iterator, index_set_type >
    iterator;
  typedef ViewIterator<
            const_origin_iterator, index_set_type >
    const_iterator;

  typedef
    decltype(
      *(dash::begin(
          dash::local(dash::origin(
            std::declval<
              typename std::add_lvalue_reference<
                domain_type>::type >()
          )))))
    reference;

  typedef
    decltype(
      *(dash::begin(
          dash::local(dash::origin(
            std::declval<
              typename std::add_lvalue_reference<
                const domain_type>::type >()
          )))))
    const_reference;

 private:
  index_set_type  _index_set;
 public:
  constexpr ViewLocalMod()               = delete;
  constexpr ViewLocalMod(self_t &&)      = default;
  constexpr ViewLocalMod(const self_t &) = default;
  ~ViewLocalMod()                        = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewLocalMod(
    domain_type && domain)
  : base_t(std::move(domain))
  , _index_set(this->domain())
  { }

  /**
   * Constructor, creates a view on a given domain.
   */
  constexpr explicit ViewLocalMod(
    const DomainType & domain)
  : base_t(domain)
  , _index_set(this->domain())
  { }

  constexpr bool operator==(const self_t & rhs) const {
    return (this == &rhs ||
             ( base_t::operator==(rhs) &&
               _index_set == rhs._index_set ) );
  }

  constexpr bool operator!=(const self_t & rhs) const {
    return not (*this == rhs);
  }

  // ---- extents ---------------------------------------------------------

  constexpr const std::array<size_type, NDim> extents() const {
    return _index_set.extents();
  }

  template <dim_t ShapeDim>
  constexpr size_type extent() const {
//  return _index_set.template extent<ShapeDim>();
    return _index_set.extents()[ShapeDim];
  }

  constexpr size_type extent(dim_t shape_dim) const {
//  return _index_set.extent(shape_dim);
    return _index_set.extents()[shape_dim];
  }

  // ---- offsets ---------------------------------------------------------

  constexpr const std::array<index_type, NDim> offsets() const {
    return _index_set.offsets();
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size(dim_t sub_dim = 0) const {
    return index_set().size(sub_dim);
  }

  // ---- access ----------------------------------------------------------

  constexpr const_iterator begin() const {
    return const_iterator(
             dash::begin(
               dash::local(
                 dash::origin(*this) )),
             _index_set, 0);
  }

  iterator begin() {
    return iterator(
             dash::begin(
               dash::local(
                 dash::origin(
                   const_cast<self_t &>(*this) ))),
             _index_set, 0);
  }

  constexpr const_iterator end() const {
    return const_iterator(
             dash::begin(
               dash::local(
                 dash::origin(*this) )),
             _index_set, _index_set.size());
  }

  iterator end() {
    return iterator(
             dash::begin(
               dash::local(
                 dash::origin(
                   const_cast<self_t &>(*this) ))),
             _index_set, _index_set.size());
  }

  constexpr const_reference operator[](int offset) const {
    return *const_iterator(
             dash::begin(
               dash::local(
                 dash::origin(*this) )),
             _index_set, offset);
  }

  reference operator[](int offset) {
    return *iterator(
             dash::begin(
               dash::local(
                 dash::origin(
                   const_cast<self_t &>(*this) ))),
             _index_set, offset);
  }

  constexpr const local_type & local() const {
    return *this;
  }

  local_type & local() {
    return *this;
  }

  constexpr const global_type & global() const {
    return dash::global(dash::domain(*this));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }
}; // ViewLocalMod


// ------------------------------------------------------------------------
// ViewSubMod
// ------------------------------------------------------------------------

template <
  class DomainType,
  dim_t SubDim,
  dim_t NDim >
struct view_traits<ViewSubMod<DomainType, SubDim, NDim> > {
  typedef DomainType                                           domain_type;
  typedef typename view_traits<domain_type>::origin_type       origin_type;
  typedef ViewSubMod<DomainType, SubDim, NDim>                  image_type;
  typedef ViewLocalMod<
           ViewSubMod<DomainType, SubDim, NDim>, NDim>          local_type;
  typedef ViewSubMod<DomainType, SubDim, NDim>                 global_type;

  typedef typename DomainType::index_type                       index_type;
  typedef typename DomainType::size_type                         size_type;
  typedef dash::IndexSetSub<DomainType, SubDim>             index_set_type;

  typedef std::integral_constant<bool, false>                is_projection;
  typedef std::integral_constant<bool, true>                 is_view;
  typedef std::integral_constant<bool, false>                is_origin;
  typedef typename view_traits<domain_type>::is_local        is_local;

  typedef std::integral_constant<bool, NDim == 1>            is_contiguous;

  typedef std::integral_constant<dim_t, DomainType::rank::value> rank;
};


template <
  class DomainType,
  dim_t SubDim,
  dim_t NDim >
class ViewSubMod
: public ViewModBase<
           ViewSubMod<DomainType, SubDim, NDim>,
           DomainType,
           NDim >
{
 public:
  typedef DomainType                                           domain_type;
 private:
  typedef ViewSubMod<domain_type, SubDim, NDim>                     self_t;
  typedef ViewModBase<
            ViewSubMod<domain_type, SubDim, NDim>,
            domain_type, NDim >                                     base_t;
 public:
  typedef typename base_t::origin_type                         origin_type;

  typedef typename view_traits<domain_type>::index_type         index_type;
  typedef typename view_traits<domain_type>::size_type           size_type;
 public:
  typedef ViewLocalMod<self_t, NDim>                            local_type;
  typedef self_t                                               global_type;

  typedef std::integral_constant<bool, false>                     is_local;

  typedef dash::IndexSetSub<domain_type, SubDim>            index_set_type;

  typedef ViewIterator<
            typename base_t::origin_iterator, index_set_type >
    iterator;
  typedef ViewIterator<
            typename base_t::const_origin_iterator, index_set_type >
    const_iterator;

  using reference       = typename base_t::reference;
  using const_reference = typename base_t::const_reference;

 private:
  index_set_type _index_set;

 public:
  constexpr ViewSubMod()               = delete;
  constexpr ViewSubMod(self_t &&)      = default;
  constexpr ViewSubMod(const self_t &) = default;
  ~ViewSubMod()                        = default;
  self_t & operator=(self_t &&)        = default;
  self_t & operator=(const self_t &)   = default;

  constexpr ViewSubMod(
    domain_type && domain,
    index_type     begin,
    index_type     end)
  : base_t(std::move(domain))
  , _index_set(this->domain(), begin, end)
  { }

  constexpr ViewSubMod(
    const domain_type  & domain,
    index_type           begin,
    index_type           end)
  : base_t(domain)
  , _index_set(domain, begin, end)
  { }

  // ---- extents ---------------------------------------------------------

  constexpr decltype(_index_set.extents()) extents() const {
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

  constexpr decltype(_index_set.offsets()) offsets() const {
    return _index_set.offsets();
  }

  constexpr index_type offset(dim_t shape_dim) const {
    return _index_set.offset(shape_dim);
  }

  // ---- size ------------------------------------------------------------

  constexpr size_type size(dim_t sub_dim = 0) const {
    return _index_set.size(sub_dim);
  }

  // ---- access ----------------------------------------------------------

  constexpr const_iterator begin() const {
    return const_iterator(
             dash::origin(*this).begin(),
             _index_set, 0);
  }

  iterator begin() {
    return iterator(
             dash::origin(
               const_cast<self_t &>(*this)
             ).begin(),
             _index_set, 0);
  }

  constexpr const_iterator end() const {
    return const_iterator(
             dash::origin(*this).begin(),
             _index_set, _index_set.size());
  }

  iterator end() {
    return iterator(
             dash::origin(
               const_cast<self_t &>(*this)
             ).begin(),
             _index_set, _index_set.size());
  }

  constexpr const_reference operator[](int offset) const {
    return *(const_iterator(dash::origin(*this).begin(),
                            _index_set, offset));
  }

  reference operator[](int offset) {
    return *(iterator(dash::origin(
                        const_cast<self_t &>(*this)
                      ).begin(),
                      _index_set, offset));
  }

  constexpr const index_set_type & index_set() const {
    return _index_set;
  }

  constexpr local_type local() const {
    return local_type(*this);
  }
};


} // namespace dash

#include <dash/view/ViewMod1D.h>

#endif // DASH__VIEW__VIEW_MOD_H__INCLUDED
