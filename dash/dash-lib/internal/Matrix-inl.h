#ifndef DASH_INTERNAL_MATRIX_INL_H_INCLUDED
#define DASH_INTERNAL_MATRIX_INL_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "dart.h"

#include "Team.h"
#include "Pattern.h"
#include "GlobIter.h"
#include "GlobRef.h"
#include "HView.h"

namespace dash {

template<typename T, size_t DIM>
Matrix_RefProxy<T, DIM>::Matrix_RefProxy()
: _dim(0) {
}

template<typename T, size_t DIM, size_t CUR>
Local_Ref<T, DIM, CUR - 1> && Local_Ref<T, DIM, CUR>::operator() {
  Local_Ref<T, DIM, CUR - 1>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
Matrix_Ref<T, DIM, CUR> Local_Ref<T, DIM, CUR>::operator () {
  // Should avoid cast from Matrix_Ref to Local_Ref.
  // Different operation semantics.
  Matrix_Ref<T, DIM, CUR>  ref;
  ref._proxy = _proxy;
  return std::move(ref);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR>::Local_Ref<T, DIM, CUR>(Matrix<T, DIM> * mat) {
  _proxy = new Matrix_RefProxy < T, DIM >;
  *_proxy = *(mat->_ref._proxy);

  for (int i = 0; i < DIM; i++) {
    _proxy->_viewspec.begin[i] = 0;
    _proxy->_viewspec.range[i] = mat->_pattern.local_extent(i);
  }
  _proxy->_viewspec.update_size();
}

template<typename T, size_t DIM, size_t CUR>
long long Local_Ref<T, DIM, CUR>::extend(size_t dim) const {
  assert(dim < DIM && dim >= 0);
  return _proxy->_mat->_pattern.local_extent(dim);
}

template<typename T, size_t DIM, size_t CUR>
size_t Local_Ref<T, DIM, CUR>::size() const {
  return _proxy->_viewspec.size();
}

template<typename T, size_t DIM, size_t CUR>
T & Local_Ref<T, DIM, CUR>::at_(size_type pos) {
  if (!(pos < _proxy->_viewspec.size())) {
    throw std::out_of_range("Out of range");
  }
  return _proxy->_mat->lbegin()[pos];
}

template<typename T, size_t DIM, size_t CUR>
template<typename ... Args>
T & Local_Ref<T, DIM, CUR>::at(Args... args) {
  assert(sizeof...(Args) == DIM - _proxy->_dim);
  std::array<long long, DIM> coord = { args... };
  for(int i=_proxy->_dim;i<DIM;i++) {
    _proxy->_coord[i] = coord[i-_proxy->_dim];
  }
  return at_(
      _proxy->_mat->_pattern
      .local_at_(
        _proxy->_coord,
        _proxy->_viewspec));
}

template<typename T, size_t DIM, size_t CUR>
template<typename ... Args>
T & Local_Ref<T, DIM, CUR>::operator()(Args... args) {
  return at(args...);
}

template<typename T, size_t DIM, size_t CUR>
LocalRef<T, DIM, CUR-1> && Local_Ref<T, DIM, CUR>::operator[](size_t n) {
  Local_Ref<T, DIM, CUR - 1>  ref;
  ref._proxy = _proxy;
  _proxy->_coord[_proxy->_dim] = n;
  _proxy->_dim++;
  _proxy->_viewspec.update_size();
  return std::move(ref);
}

// Wrapper class for RefProxy. Local_Ref represents local part of a Matrix and provices local operations.
template <typename T, size_t DIM, size_t CUR = DIM> class Local_Ref {
 public:
    Local_Ref<T, DIM, CUR - 1> operator[](size_t n) const & {
      Local_Ref<T, DIM, CUR - 1> ref;
      ref._proxy = new Matrix_RefProxy < T, DIM > ;
      ref._proxy->_coord = _proxy->_coord;
      ref._proxy->_coord[_proxy->_dim] = n;
      ref._proxy->_dim = _proxy->_dim + 1;
      ref._proxy->_mat = _proxy->_mat;
      ref._proxy->_viewspec = _proxy->_viewspec;
      
      ref._proxy->_viewspec.view_dim--;
      ref._proxy->_viewspec.update_size();

      return ref;
    }

    template<size_t SUBDIM>
    Local_Ref<T, DIM, DIM - 1> sub(size_type n)
    {
      static_assert(DIM - 1 > 0, "Too low dim");
      static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");

      size_t target_dim = SUBDIM + _proxy->_dim;

      Local_Ref<T, DIM, DIM - 1> ref;
      Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
      std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

      ref._proxy = proxy;

      ref._proxy->_coord[target_dim] = 0;

      ref._proxy->_viewspec = _proxy->_viewspec;
      ref._proxy->_viewspec.begin[target_dim] = n;
      ref._proxy->_viewspec.range[target_dim] = 1;
      ref._proxy->_viewspec.view_dim--;
      ref._proxy->_viewspec.update_size();

      ref._proxy->_mat = _proxy->_mat;
      ref._proxy->_dim = _proxy->_dim + 1;

      return ref;
    }

    Local_Ref<T, DIM, DIM - 1> col(size_type n)
    {
      return sub<1>(n);
    }

    Local_Ref<T, DIM, DIM - 1> row(size_type n)
    {
      return sub<0>(n);
    }

    template<size_t SUBDIM>
    Local_Ref<T, DIM, DIM> submat(size_type n, size_type range)
    {
      static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");
      Local_Ref<T, DIM, DIM> ref;
      Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
      std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

      ref._proxy = proxy;
      ref._proxy->_viewspec = _proxy->_viewspec;
      ref._proxy->_viewspec.begin[SUBDIM] = n;
      ref._proxy->_viewspec.range[SUBDIM] = range;
      ref._proxy->_mat = _proxy->_mat;

      ref._proxy->_viewspec.update_size();

      return ref;
    }

    Local_Ref<T, DIM, DIM> rows(size_type n, size_type range)
    {
      return submat<0>(n, range);
    }

    Local_Ref<T, DIM, DIM> cols(size_type n, size_type range)
    {
      return submat<1>(n, range);
    }
  };


  // Wrapper class for RefProxy. Matrix_Ref represents Matrix and Submatrix a Matrix and provices global operations.
  template <typename T, size_t DIM, size_t CUR = DIM> class Matrix_Ref {

  private:
  public:
    template<typename T_, size_t DIM_> friend class Matrix;
    Matrix_RefProxy<T, DIM> * _proxy;

    typedef T value_type;

    // NO allocator_type!
    typedef size_t size_type;
    typedef size_t difference_type;

    typedef GlobIter<value_type, DIM> iterator;
    typedef const GlobIter<value_type, DIM> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef GlobRef<value_type> reference;
    typedef const GlobRef<value_type> const_reference;

    typedef GlobIter<value_type, DIM> pointer;
    typedef const GlobIter<value_type, DIM> const_pointer;

    operator Matrix_Ref<T, DIM, CUR - 1> && ()
    {
      Matrix_Ref<T, DIM, CUR - 1> ref =  Matrix_Ref < T, DIM, CUR - 1 >() ;
      ref._proxy = _proxy;
      return std::move(ref);
    }

    Matrix_Ref<T, DIM, CUR>() = default;

    Pattern<DIM> &pattern() {
      return _proxy->_mat->_pattern;
    }

    Team &team() {
      return _proxy->_mat->_team;
    }

    constexpr size_type size() const noexcept{
      return _proxy->_size;
    }

    size_type extent(size_type dim) const noexcept{
      return _proxy->_viewspec.range[dim];
    }

    constexpr bool empty() const noexcept{
      return size() == 0;
    }

    void barrier() const {
      _proxy->_mat->_team.barrier();
    }

    void forall(std::function<void(long long)> func) {
      _proxy->_mat->_pattern.forall(func, _proxy->_viewspec);
    }

    Matrix_Ref<T, DIM, CUR - 1> && operator[](size_t n) && {

      Matrix_Ref<T, DIM, CUR - 1> ref;

      _proxy->_coord[_proxy->_dim] = n;
      _proxy->_dim++;
      _proxy->_viewspec.update_size();

      ref._proxy = _proxy;

      return std::move(ref);
    };

    Matrix_Ref<T, DIM, CUR - 1> operator[](size_t n) const & {

      Matrix_Ref<T, DIM, CUR - 1> ref;
      ref._proxy = new Matrix_RefProxy < T, DIM > ;
      ref._proxy->_coord = _proxy->_coord;
      ref._proxy->_coord[_proxy->_dim] = n;
      ref._proxy->_dim = _proxy->_dim + 1;
      ref._proxy->_mat = _proxy->_mat;
      ref._proxy->_viewspec = _proxy->_viewspec;
      ref._proxy->_viewspec.view_dim--;
      ref._proxy->_viewspec.update_size();

      return ref;
    }

    template<size_t SUBDIM>
    Matrix_Ref<T, DIM, DIM - 1> sub(size_type n)
    {
      static_assert(DIM - 1 > 0, "Too low dim");
      static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");

      size_t target_dim = SUBDIM + _proxy->_dim;

      Matrix_Ref<T, DIM, DIM - 1> ref;
      printf("new6\n");
      Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
      std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

      ref._proxy = proxy;

      ref._proxy->_coord[target_dim] = 0;

      ref._proxy->_viewspec = _proxy->_viewspec;
      ref._proxy->_viewspec.begin[target_dim] = n;
      ref._proxy->_viewspec.range[target_dim] = 1;
      ref._proxy->_viewspec.view_dim--;
      ref._proxy->_viewspec.update_size();

      ref._proxy->_mat = _proxy->_mat;
      ref._proxy->_dim = _proxy->_dim + 1;

      return ref;
    }

    Matrix_Ref<T, DIM, DIM - 1> col(size_type n)
    {
      return sub<1>(n);
    }

    Matrix_Ref<T, DIM, DIM - 1> row(size_type n)
    {
      return sub<0>(n);
    }

    template<size_t SUBDIM>
    Matrix_Ref<T, DIM, DIM> submat(size_type n, size_type range)
    {
      static_assert(SUBDIM < DIM && SUBDIM >= 0, "Wrong SUBDIM");
      Matrix_Ref<T, DIM, DIM> ref;
      Matrix_RefProxy<T, DIM> * proxy = new Matrix_RefProxy < T, DIM > ;
      std::fill(proxy->_coord.begin(), proxy->_coord.end(), 0);

      ref._proxy = proxy;
      ref._proxy->_viewspec = _proxy->_viewspec;
      ref._proxy->_viewspec.begin[SUBDIM] = n;
      ref._proxy->_viewspec.range[SUBDIM] = range;
      ref._proxy->_mat = _proxy->_mat;
      ref._proxy->_viewspec.update_size();

      return ref;
    }

    Matrix_Ref<T, DIM, DIM> rows(size_type n, size_type range)
    {
      return submat<0>(n, range);
    }

    Matrix_Ref<T, DIM, DIM> cols(size_type n, size_type range)
    {
      return submat<1>(n, range);
    }

    reference at_(size_type unit , size_type elem) {
      if (!(elem < _proxy->_viewspec.size()))
        throw std::out_of_range("Out of range");

      return _proxy->_mat->begin().get(unit, elem);
    }

    template<typename ... Args>
    reference at(Args... args) {

      assert(sizeof...(Args) == DIM - _proxy->_dim);
      std::array<long long, DIM> coord = { args... };

      for(int i=_proxy->_dim;i<DIM;i++)
        _proxy->_coord[i] = coord[i-_proxy->_dim];
           
      size_t unit = _proxy->_mat->_pattern.atunit_(_proxy->_coord, _proxy->_viewspec);
      size_t elem = _proxy->_mat->_pattern.at_(_proxy->_coord, _proxy->_viewspec);
      return at_(unit , elem);

    }

    template<typename... Args>
    reference operator()(Args... args)
    {
      return at(args...);
    }

    Pattern<DIM> pattern() const {
      return _proxy->_mat->_pattern;
    }

    //For 1D. OBSOLETE
    bool islocal(size_type n) {
      return (_proxy->_mat->_pattern.index_to_unit(n, _proxy->_viewspec) ==
              _proxy->_mat->_myid);
    }

    // For ND
    bool islocal(size_t dim, size_type n) {
      return _proxy->_mat->_pattern.is_local(n, _proxy->_mat->_myid, dim, _proxy->_viewspec);
    }

    template <int level> dash::HView<Matrix<T, DIM>, level, DIM> hview() {
      return dash::HView<Matrix<T, DIM>, level, DIM>(*this);
    }
  };

  // Partial Specialization for value deferencing.
  template <typename T, size_t DIM>
  class Local_Ref < T, DIM, 0 >
  {

  private:

  public:
    template<typename T_, size_t DIM_> friend class Matrix;
    Matrix_RefProxy<T, DIM> * _proxy;

    Local_Ref<T, DIM, 0>() = default;

    T* at_(size_t pos) {
      if (!(pos < _proxy->_mat->size()))
        throw std::out_of_range("Out of range");
      return &(_proxy->_mat->lbegin()[pos]);
    }

    operator T()
    {          
      T ret = *at_(_proxy->_mat->_pattern.local_at_(_proxy->_coord, _proxy->_viewspec));
      delete _proxy;
      return ret;
    }

    T operator=(const T value)
    {
      T* ref = at_(_proxy->_mat->_pattern.local_at_(_proxy->_coord, _proxy->_viewspec));
      *ref = value;
      delete _proxy;
      return value;
    }
  };

  // Partial Specialization for value deferencing.
  template <typename T, size_t DIM>
  class Matrix_Ref < T, DIM, 0 >
  {

  private:

  public:
    template<typename T_, size_t DIM_> friend class Matrix;
    Matrix_RefProxy<T, DIM> * _proxy;

    
    GlobRef<T> at_(size_t unit, size_t elem) const {
      return _proxy->_mat->begin().get(unit, elem);

    }

    Matrix_Ref<T, DIM, 0>() = default;

    operator T()
    {
      GlobRef<T> ref = at_(
          _proxy->_mat->_pattern.atunit_(_proxy->_coord, _proxy->_viewspec),
          _proxy->_mat->_pattern.at_(_proxy->_coord, _proxy->_viewspec)
      );
      delete _proxy;  
      return ref;
    }

    T operator=(const T value)
    {
      GlobRef<T> ref = at_(_proxy->_mat->_pattern.atunit_(_proxy->_coord, _proxy->_viewspec), _proxy->_mat->_pattern.at_(_proxy->_coord, _proxy->_viewspec));
      ref = value;
      delete _proxy;
      return value;
    }
  };


  template <typename ELEMENT_TYPE, size_t DIM> class Matrix {

  public:
    typedef ELEMENT_TYPE value_type;

    // NO allocator_type!
    typedef size_t size_type;
    typedef size_t difference_type;

    typedef GlobIter<value_type, DIM> iterator;
    typedef const GlobIter<value_type, DIM> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef GlobRef<value_type> reference;
    typedef const GlobRef<value_type> const_reference;

    typedef GlobIter<value_type, DIM> pointer;
    typedef const GlobIter<value_type, DIM> const_pointer;

  private:
    dash::Team &_team;
    dart_unit_t _myid;
    dash::Pattern<DIM> _pattern;
    size_type _size;  // total size (#elements)
    size_type _lsize; // local size (#local elements)
    pointer *_ptr;
    dart_gptr_t _dart_gptr;
    Matrix_Ref<ELEMENT_TYPE, DIM, DIM> _ref;

  public:

    template<typename T_, size_t DIM1, size_t DIM2> friend class Matrix_Ref;
    template<typename T_, size_t DIM1, size_t DIM2> friend class Local_Ref;

    Local_Ref<ELEMENT_TYPE, DIM, DIM> _local;

    static_assert(std::is_trivial<ELEMENT_TYPE>::value,
      "Element type must be trivial copyable");

  public:

    Local_Ref<ELEMENT_TYPE, DIM, DIM> local() const
    {
      return _local;
    }

    // Proxy, Matrix_Ref and Local_Ref are created at initialization.
    Matrix(const dash::SizeSpec<DIM> &ss,
      const dash::DistSpec<DIM> &ds = dash::DistSpec<DIM>(),
      Team &t = dash::Team::All(), const TeamSpec<DIM> &ts = TeamSpec<DIM>())
      : _team(t), _pattern(ss, ds, ts, t) {
      // Matrix is a friend of class Team
      dart_tea_t teamid = _team._dartid;

      size_t lelem = _pattern.max_ele_per_unit();
      size_t lsize = lelem * sizeof(value_type);

      _dart_gptr = DART_GPTR_NULL;
      dart_ret_t ret = dart_tea_memalloc_aligned(teamid, lsize, &_dart_gptr);

      _ptr = new GlobIter<value_type, DIM>(_pattern, _dart_gptr, 0);

      _size = _pattern.nelem();
      _lsize = lelem;

      _myid = _team.myid();

      _ref._proxy = new Matrix_RefProxy < value_type, DIM > ;
      _ref._proxy->_dim = 0;
      _ref._proxy->_mat = this;
      _ref._proxy->_viewspec = _pattern._viewspec;
      _local = Local_Ref<ELEMENT_TYPE, DIM, DIM>(this);
    }

    // delegating constructor
    Matrix(const dash::Pattern<DIM> &pat)
      : Matrix(pat.sizespec(), pat.distspec(), pat.team(), pat.teamspec()) {}

    // delegating constructor
    Matrix(size_t nelem, Team &t = dash::Team::All())
      : Matrix(dash::Pattern<DIM>(nelem, t)) {}

    ~Matrix() {
      dart_tea_t teamid = _team._dartid;
      dart_tea_memfree(teamid, _dart_gptr);
    }

    Pattern<DIM> &pattern() {
      return _pattern;
    }

    Team &team() {
      return _team;
    }

    constexpr size_type size() const noexcept{
      return _size;
    }

    size_type extent(size_type dim) const noexcept{
      return _ref._proxy->_viewspec.range[dim];
    }

    constexpr bool empty() const noexcept{
      return size() == 0;
    }

    void barrier() const {
      _team.barrier();
    }

    const_pointer data() const noexcept{
      return *_ptr;
    }

    iterator begin() noexcept{ return iterator(data()); }

    iterator end() noexcept{ return iterator(data() + _size); }

    ELEMENT_TYPE *lbegin() noexcept{
      void *addr;
      dart_gptr_t gptr = _dart_gptr;

      dart_gptr_setunit(&gptr, _myid);
      dart_gptr_getaddr(gptr, &addr);
      return (ELEMENT_TYPE *)(addr);
    }

    ELEMENT_TYPE *lend() noexcept{
      void *addr;
      dart_gptr_t gptr = _dart_gptr;

      dart_gptr_setunit(&gptr, _myid);
      dart_gptr_incaddr(&gptr, _lsize * sizeof(ELEMENT_TYPE));
      dart_gptr_getaddr(gptr, &addr);
      return (ELEMENT_TYPE *)(addr);
    }

    void forall(std::function<void(long long)> func) {
      _pattern.forall(func);
    }

#if 0
    iterator lbegin() noexcept
    {
      // xxx needs fix
      return iterator(data() + _team.myid()*_lsize);
    }

    iterator lend() noexcept
    {
      // xxx needs fix
      size_type end = (_team.myid() + 1)*_lsize;
      if (_size < end) end = _size;

      return iterator(data() + end);
    }
#endif

    template<size_t SUBDIM>
    Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> sub(size_type n)
    {
      return _ref.sub<SUBDIM>(n);
    }

    Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> col(size_type n)
    {
      return _ref.sub<1>(n);
    }

    Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> row(size_type n)
    {
      return _ref.sub<0>(n);
    }

    template<size_t SUBDIM>
    Matrix_Ref<ELEMENT_TYPE, DIM, DIM> submat(size_type n, size_type range)
    {
      return _ref.submat<SUBDIM>(n, range);
    }

    Matrix_Ref<ELEMENT_TYPE, DIM, DIM> rows(size_type n, size_type range)
    {
      return _ref.submat<0>(n, range);
    }

    Matrix_Ref<ELEMENT_TYPE, DIM, DIM> cols(size_type n, size_type range)
    {
      return _ref.submat<1>(n, range);
    }

    Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> operator[](size_type n)
    {
      return _ref.operator[](n);
    }


    template<typename ... Args>
    reference at(Args... args) {
      return _ref.at(args...);
    }

    template<typename... Args>
    reference operator()(Args... args)
    {
      return _ref.at(args...);
    }

    Pattern<DIM> pattern() const {
      return _pattern;
    }

    //For 1D
    bool islocal(size_type n) {
      return _ref.islocal(n);
    }

    // For ND
    bool islocal(size_t dim, size_type n) {
      return _ref.islocal(dim, n);
    }

    template <int level> dash::HView<Matrix<ELEMENT_TYPE, DIM>, level, DIM> hview() {
      return _ref.hview<level>();
    }

    operator Matrix_Ref<ELEMENT_TYPE, DIM, DIM> ()
    {
      return _ref;
    }
  };

}  // namespace dash
#endif  // DASH_INTERNAL_MATRIX_INL_H_INCLUDED
