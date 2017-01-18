#ifndef DASH__VIEW__INDEX_SET_H__INCLUDED
#define DASH__VIEW__INDEX_SET_H__INCLUDED

#include <dash/view/ViewTraits.h>

#include <dash/view/Origin.h>
// #include <dash/view/Local.h>
// #include <dash/view/Global.h>

#include <dash/view/SetUnion.h>

#include <dash/pattern/PatternProperties.h>

#include <dash/Iterator.h>


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

template <class ViewTypeA, class ViewTypeB>
constexpr ViewTypeA
intersect(
  const ViewTypeA & va,
  const ViewTypeB & vb);

template <class IndexSetType, class ViewType>
class IndexSetBase;

template <class ViewType>
class IndexSetIdentity;

template <class ViewType>
class IndexSetLocal;

template <class ViewType>
class IndexSetGlobal;

template <class ViewType>
class IndexSetSub;



template <class ViewType>
constexpr auto
index(const ViewType & v)
-> typename std::enable_if<
     dash::view_traits<ViewType>::is_view::value,
//   decltype(v.index_set())
     const typename ViewType::index_set_type
   >::type {
  return v.index_set();
}

// template <class ViewType>
// auto
// index(ViewType & v)
// -> typename std::enable_if<
//      dash::view_traits<ViewType>::is_view::value,
//      decltype(v.index_set())
//    >::type {
//   return v.index_set();
// }



template <class ContainerType>
constexpr auto
index(const ContainerType & c)
-> typename std::enable_if <
     dash::view_traits<ContainerType>::is_origin::value,
     IndexSetIdentity<ContainerType>
   >::type {
  return IndexSetIdentity<ContainerType>(c);
}

// template <class ContainerType>
// auto
// index(ContainerType & c)
// -> typename std::enable_if <
//      dash::view_traits<ContainerType>::is_origin::value,
//      IndexSetIdentity<ContainerType>
//    >::type {
//   return IndexSetIdentity<ContainerType>(c);
// }


namespace detail {

template <
  class IndexSetType,
  int   BaseStride   = 1 >
class IndexSetIterator {
  typedef IndexSetIterator<IndexSetType, BaseStride>   self_t;
  typedef int                                          index_type;
private:
  const IndexSetType * const _index_set;
  index_type                 _pos;
  index_type                 _stride = BaseStride;
public:
  constexpr IndexSetIterator()                 = delete;
  constexpr IndexSetIterator(self_t &&)        = default;
  constexpr IndexSetIterator(const self_t &)   = default;
  ~IndexSetIterator()                          = default;
  self_t & operator=(self_t &&)                = default;
  self_t & operator=(const self_t &)           = default;

  constexpr IndexSetIterator(
    const IndexSetType & index_set,
    index_type           position)
  : _index_set(&index_set), _pos(position), _stride(BaseStride)
  { }

  constexpr IndexSetIterator(
    const IndexSetType & index_set,
    index_type           position,
    index_type           stride)
  : _index_set(&index_set), _pos(position), _stride(stride)
  { }

  constexpr index_type operator*() const {
#if 1
    return _pos < dash::size(*_index_set)
              ? (*_index_set)[_pos]
              : ((*_index_set)[dash::size(*_index_set)-1]
                  + (_pos - (dash::size(*_index_set) - 1))
                );
#else
    return (*_index_set)[_pos];
#endif
  }

  constexpr index_type operator->() const {
#if 1
    return _pos < dash::size(*_index_set)
              ? (*_index_set)[_pos]
              : ((*_index_set)[dash::size(*_index_set)-1]
                  + (_pos - (dash::size(*_index_set) - 1))
                );
#else
    return (*_index_set)[_pos];
#endif
  }

  self_t & operator++() {
    _pos += _stride;
    return *this;
  }

  self_t & operator--() {
    _pos -= _stride;
    return *this;
  }

  constexpr self_t operator++(int) const {
    return self_t(*_index_set, _pos + _stride, _stride);
  }

  constexpr self_t operator--(int) const {
    return self_t(*_index_set, _pos - _stride, _stride);
  }

  self_t & operator+=(int i) {
    _pos += i * _stride;
    return *this;
  }

  self_t & operator-=(int i) {
    _pos -= i * _stride;
    return *this;
  }

  constexpr self_t operator+(int i) const {
    return self_t(*_index_set, _pos + (i * _stride), _stride);
  }

  constexpr self_t operator-(int i) const {
    return self_t(*_index_set, _pos - (i * _stride), _stride);
  }

  constexpr index_type pos() const {
    return _pos;
  }

  constexpr index_type operator+(const self_t & rhs) const {
    return _pos + rhs._pos;
  }

  constexpr index_type operator-(const self_t & rhs) const {
    return _pos - rhs._pos;
  }

  constexpr bool operator==(const self_t & rhs) const {
    return _pos == rhs._pos && _stride == rhs._stride;
  }

  constexpr bool operator!=(const self_t & rhs) const {
    return not (*this == rhs);
  }
};

} // namespace detail

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
  class IndexSetType,
  class ViewType >
constexpr auto
local(
  const IndexSetBase<IndexSetType, ViewType> & index_set)
//  -> decltype(index_set.local()) {
//  -> typename view_traits<IndexSetBase<ViewType>>::global_type & { 
    -> const IndexSetLocal<ViewType> & { 
  return index_set.local();
}

template <
  class IndexSetType,
  class ViewType >
constexpr auto
global(
  const IndexSetBase<IndexSetType, ViewType> & index_set)
//   ->  decltype(index_set.global()) {
//   -> typename view_traits<IndexSetSub<ViewType>>::global_type & { 
     -> const IndexSetGlobal<ViewType> & { 
  return index_set.global();
}

/**
 * \concept{DashRangeConcept}
 */
template <
  class IndexSetType,
  class ViewType >
class IndexSetBase
{
  typedef IndexSetBase<IndexSetType, ViewType> self_t;
public:
  typedef typename ViewType::index_type
    index_type;
  typedef typename dash::view_traits<ViewType>::origin_type
    origin_type;
  typedef typename dash::view_traits<ViewType>::domain_type
    view_domain_type;
  typedef typename ViewType::local_type
    view_local_type;
  typedef typename dash::view_traits<ViewType>::global_type
    view_global_type;
  typedef typename dash::view_traits<view_domain_type>::index_set_type
    index_set_domain_type;

  typedef typename origin_type::pattern_type
    pattern_type;
  typedef typename dash::view_traits<view_local_type>::index_set_type
    local_type;
  typedef typename dash::view_traits<view_global_type>::index_set_type
    global_type;

  typedef detail::IndexSetIterator<IndexSetType> iterator;

protected:
  const ViewType              &  _view;
  const pattern_type          &  _pattern;

  IndexSetType & derived() {
    return static_cast<IndexSetType &>(*this);
  }
  constexpr const IndexSetType & derived() const {
    return static_cast<const IndexSetType &>(*this);
  }
  
  constexpr explicit IndexSetBase(const ViewType & view)
  : _view(view)
  , _pattern(dash::origin(_view).pattern())
  { }

  constexpr IndexSetBase(ViewType && view)
  : _view(std::forward(view))
  , _pattern(dash::origin(std::forward(_view)).pattern())
  { }

  ~IndexSetBase()                        = default;
public:
  constexpr IndexSetBase()               = delete;
  constexpr IndexSetBase(self_t &&)      = default;
  constexpr IndexSetBase(const self_t &) = default;
  self_t & operator=(self_t &&)          = default;
  self_t & operator=(const self_t &)     = default;
  
  ViewType & view() {
    return _view;
  }
  constexpr const ViewType & view() const {
    return _view;
  }

  constexpr iterator begin() const {
    return iterator(derived(), 0);
  }

  constexpr iterator end() const {
    return iterator(derived(), derived().size());
  }

  constexpr index_type first() const {
    return *begin();
  }

  constexpr index_type last() const {
    return *(begin() + (derived().size() - 1));
  }

  constexpr const local_type & local() const {
    return dash::index(dash::local(_view));
  }

  constexpr const global_type & global() const {
    return dash::index(dash::global(_view));
  }

  constexpr const index_set_domain_type domain() const {
    // To allow subclasses to overwrite method view():
//  return dash::index(dash::domain(derived().view()));
    return dash::index(dash::domain(_view));
  }

  constexpr const pattern_type & pattern() const {
    return _pattern;
  }

  /*
   *  dash::index(r(10..100)).step(2)[8]  -> 26
   *  dash::index(r(10..100)).step(-5)[4] -> 80
   */
  constexpr iterator step(index_type stride) const {
    return (
      stride > 0
        ? iterator(derived(),                0, stride)
        : iterator(derived(), derived().size(), stride)
    );
  }
};

// -----------------------------------------------------------------------

template <class ViewType>
constexpr const IndexSetIdentity<ViewType> &
local(const IndexSetIdentity<ViewType> & index_set) {
  return index_set;
}

/**
 * \concept{DashRangeConcept}
 */
template <class ViewType>
class IndexSetIdentity
: public IndexSetBase<
           IndexSetIdentity<ViewType>,
           ViewType >
{
  typedef IndexSetIdentity<ViewType>            self_t;
  typedef IndexSetBase<self_t, ViewType>        base_t;
public:
  constexpr IndexSetIdentity()               = delete;
  constexpr IndexSetIdentity(self_t &&)      = default;
  constexpr IndexSetIdentity(const self_t &) = default;
  ~IndexSetIdentity()                        = default;
  self_t & operator=(self_t &&)              = default;
  self_t & operator=(const self_t &)         = default;
public:
  typedef typename ViewType::index_type     index_type;
public:
  constexpr explicit IndexSetIdentity(const ViewType & view)
  : base_t(view)
  { }

  constexpr index_type operator[](index_type image_index) const {
    return image_index;
  }

  constexpr index_type size() const {
    return this->view().size();
  }

  constexpr const self_t & pre() const {
    return *this;
  }
};

// -----------------------------------------------------------------------

template <class ViewType>
constexpr auto
local(const IndexSetSub<ViewType> & index_set) ->
// decltype(index_set.local()) {
  typename view_traits<IndexSetSub<ViewType>>::local_type & { 
  return index_set.local();
}

template <class ViewType>
constexpr auto
global(const IndexSetSub<ViewType> & index_set) ->
// decltype(index_set.global()) {
  typename view_traits<IndexSetSub<ViewType>>::global_type & { 
  return index_set.global();
}

/**
 * \concept{DashRangeConcept}
 */
template <class ViewType>
class IndexSetSub
: public IndexSetBase<
           IndexSetSub<ViewType>,
           ViewType >
{
  typedef IndexSetSub<ViewType>                                 self_t;
  typedef IndexSetBase<self_t, ViewType>                        base_t;
public:
  typedef typename ViewType::index_type                     index_type;
  typedef typename base_t::view_domain_type           view_domain_type;
  typedef typename base_t::local_type                       local_type;
  typedef typename base_t::global_type                     global_type;
  typedef typename base_t::iterator                           iterator;
//typedef typename base_t::index_set_domain_type index_set_domain_type;
  typedef IndexSetSub<ViewType>                          preimage_type;

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
public:
  constexpr IndexSetSub(
    const ViewType   & view,
    index_type         begin,
    index_type         end)
  : base_t(view)
  , _domain_begin_idx(begin)
  , _domain_end_idx(end)
  { }

  constexpr index_type operator[](index_type image_index) const {
//  TODO:
//  return this->domain()[_domain_begin_idx + image_index];
    return (_domain_begin_idx + image_index);
  }

  constexpr index_type size() const {
    return std::min<index_type>(
             (_domain_end_idx - _domain_begin_idx),
       // TODO:
       //    this->domain().size()
             (_domain_end_idx - _domain_begin_idx)
           );
  }

  constexpr preimage_type pre() const {
    return preimage_type(
             this->view(),
             -_domain_begin_idx,
             -_domain_begin_idx + this->view().size()
           );
  }
};

// -----------------------------------------------------------------------

template <class ViewType>
constexpr const IndexSetLocal<ViewType> &
local(const IndexSetLocal<ViewType> & index_set) {
  return index_set;
}

template <class ViewType>
constexpr auto
global(const IndexSetLocal<ViewType> & index_set) ->
// decltype(index_set.global()) {
  typename view_traits<IndexSetLocal<ViewType>>::global_type & { 
  return index_set.global();
}

/**
 * \concept{DashRangeConcept}
 */
template <class ViewType>
class IndexSetLocal
: public IndexSetBase<
           IndexSetLocal<ViewType>,
           ViewType >
{
  typedef IndexSetLocal<ViewType>                               self_t;
  typedef IndexSetBase<self_t, ViewType>                        base_t;
public:
  typedef typename ViewType::index_type                     index_type;
   
  typedef self_t                                            local_type;
  typedef IndexSetGlobal<ViewType>                         global_type;
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
  constexpr explicit IndexSetLocal(const ViewType & view)
  : base_t(view)
  , _size(calc_size())
  { }

  constexpr index_type
  operator[](index_type local_index) const {
    // NOTE:
    // Random access operator must allow access at [end] because
    // end iterator of an index range may be dereferenced.
    return // local_index >= size()
           //  ?
           //    ( this->pattern().global(
           //        (size() - 1) +
           //        // actually only required if local of sub
           //        this->pattern().at(
           //          std::max<index_type>(
           //            this->pattern().global(0),
           //            this->domain()[0]
           //        ))
           //      ) + (local_index - (size()-1))
           //    )
           //  :
              local_index +
              // only required if local of sub
              ( this->domain()[0] == 0
                ? 0
                : this->pattern().at(
                    std::max<index_type>(
                      this->pattern().global(0),
                      this->domain()[0]
                  ))
              );
  }

  constexpr index_type size() const {
    return _size;
  }

  constexpr index_type calc_size() const {
    typedef typename dash::pattern_partitioning_traits<pattern_type>::type
            pat_partitioning_traits;

    static_assert(
        pat_partitioning_traits::rectangular,
        "index sets for non-rectangular patterns are not supported yet");

    return (
      //pat_partitioning_traits::minimal ||
        this->pattern().blockspec().size()
          <= this->pattern().team().size()
      // blocked (not blockcyclic) distribution: single local
      // element space with contiguous global index range
      ? std::min<index_type>(
          this->pattern().local_size(),
          this->domain().size()
        )
      // blockcyclic distribution: local element space chunked
      // in global index range
      : this->pattern().local_size() + // <-- TODO: intersection of local
        this->domain().pre()[0]        //           blocks and domain
    );
  }

  constexpr const local_type & local() const {
    return *this;
  }

  constexpr global_type global() const {
    return global_type(this->view());
  }

  constexpr preimage_type pre() const {
    return preimage_type(this->view());
  }
};

// -----------------------------------------------------------------------

template <class ViewType>
constexpr auto
local(const IndexSetGlobal<ViewType> & index_set)
    -> decltype(index_set.local()) {
//  -> typename view_traits<IndexSetGlobal<ViewType>>::local_type & { 
  return index_set.local();
}

template <class ViewType>
constexpr const IndexSetGlobal<ViewType> &
global(const IndexSetGlobal<ViewType> & index_set) {
  return index_set;
}

/**
 * \concept{DashRangeConcept}
 */
template <class ViewType>
class IndexSetGlobal
: public IndexSetBase<
           IndexSetGlobal<ViewType>,
           ViewType >
{
  typedef IndexSetGlobal<ViewType>                              self_t;
  typedef IndexSetBase<self_t, ViewType>                        base_t;
public:
  typedef typename ViewType::index_type                     index_type;
   
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
  constexpr explicit IndexSetGlobal(const ViewType & view)
  : base_t(view)
  , _size(calc_size())
  { }

  constexpr index_type
  operator[](index_type global_index) const {
    // NOTE:
    // Random access operator must allow access at [end] because
    // end iterator of an index range may be dereferenced.
    return this->pattern().at(
             global_index
           );
  }

  constexpr index_type calc_size() const {
    return std::max<index_type>(
             this->pattern().size(),
             this->domain().size()
           );
  }

  constexpr index_type size() const {
    return _size;
  }

  constexpr const local_type & local() const {
    return dash::index(dash::local(this->view()));
  }

  constexpr const global_type & global() const {
    return *this;
  }

  constexpr const preimage_type & pre() const {
    return dash::index(dash::local(this->view()));
  }
};

} // namespace dash
#endif // DOXYGEN

#endif // DASH__VIEW__INDEX_SET_H__INCLUDED
