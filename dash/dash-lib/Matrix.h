#ifndef DASH_MATRIX_H_INCLUDED
#define DASH_MATRIX_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "dart.h"

#include "Team.h"
#include "Pattern.h"
#include "GlobIter.h"
#include "GlobRef.h"
#include "HView.h"

namespace dash {

/// Forward-declaration
template <typename T, size_t DIM> class Matrix;
/// Forward-declaration
template <typename T, size_t DIM, size_t CUR> class Matrix_Ref;
/// Forward-declaration
template <typename T, size_t DIM, size_t CUR> class Local_Ref;

// RefProxy stores information needed by subscripting and subdim selection.
// New RefProxy should be created once for multi-subscripting.
template <typename T, size_t DIM> class Matrix_RefProxy {
 private:
  int _dim;
  Matrix<T, DIM> * _mat;
  ::std::array<long long, DIM> _coord;
  ViewSpec<DIM> _viewspec;

 public:
  template<typename T_, size_t DIM1, size_t DIM2> friend class Matrix_Ref;
  template<typename T_, size_t DIM1, size_t DIM2> friend class Local_Ref;
  template<typename T_, size_t DIM1> friend class Matrix;

  Matrix_RefProxy<T, DIM>();
};

// Wrapper class for RefProxy. Local_Ref represents local part of a Matrix and provices local operations.
template <typename T, size_t DIM, size_t CUR = DIM> class Local_Ref {
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

  Local_Ref<T, DIM, CUR>() = default;

  Local_Ref<T, DIM, CUR>(Matrix<T, DIM> * mat);

  inline operator Local_Ref<T, DIM, CUR - 1> && ();
  // SHOULD avoid cast from Matrix_Ref to Local_Ref. Different operation semantics.
  inline operator Matrix_Ref<T, DIM, CUR> ();
  inline long long extent(size_t dim) const;
  inline size_type size() const;
  inline T & at_(size_type pos);

  template<typename ... Args>
  T& at(Args... args);

  template<typename... Args>
  T& operator()(Args... args);

  Local_Ref<T, DIM, CUR - 1> && operator[](size_t n);
  Local_Ref<T, DIM, CUR - 1> operator[](size_t n) const;

  template<size_t SUBDIM>
  Local_Ref<T, DIM, DIM - 1> sub(size_type n);

  inline Local_Ref<T, DIM, DIM - 1> col(size_type n);
  inline Local_Ref<T, DIM, DIM - 1> row(size_type n);

  template<size_t SUBDIM>
  Local_Ref<T, DIM, DIM> submat(size_type n, size_type range);

  inline Local_Ref<T, DIM, DIM> rows(size_type n, size_type range)
  inline Local_Ref<T, DIM, DIM> cols(size_type n, size_type range)
};

// Wrapper class for RefProxy. Matrix_Ref represents Matrix and Submatrix a Matrix and provices global operations.
template <typename T, size_t DIM, size_t CUR = DIM> class Matrix_Ref {
 public:
  template<typename T_, size_t DIM_> friend class Matrix;

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

  inline operator Matrix_Ref<T, DIM, CUR - 1> && ();

  Matrix_Ref<T, DIM, CUR>() = default;

  Pattern<DIM> & pattern();

  Team & team();

  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept
  inline constexpr bool empty() const noexcept
  inline void barrier() const
  inline void forall(std::function<void(long long)> func)
  inline Pattern<DIM> pattern() const;

  Matrix_Ref<T, DIM, CUR-1> && operator[](size_t n)
  Matrix_Ref<T, DIM, CUR-1> operator[](size_t n) const;

  template<size_t SUBDIM>
  Matrix_Ref<T, DIM, DIM-1> sub(size_type n);
  Matrix_Ref<T, DIM, DIM-1> col(size_type n);
  Matrix_Ref<T, DIM, DIM-1> row(size_type n);

  template<size_t SUBDIM>
  Matrix_Ref<T, DIM, DIM> submat(size_type n, size_type range);
  Matrix_Ref<T, DIM, DIM> rows(size_type n, size_type range);
  Matrix_Ref<T, DIM, DIM> cols(size_type n, size_type range);

  reference at_(size_type unit , size_type elem);

  template<typename ... Args>
  reference at(Args... args);

  template<typename... Args>
  reference operator()(Args... args);

  // For 1D. OBSOLETE
  inline bool islocal(size_type n);
  inline bool islocal(size_t dim, size_type n);

  inline template <int level> dash::HView<Matrix<T, DIM>, level, DIM> hview();

 private:
  Matrix_RefProxy<T, DIM> * _proxy;
};

// Partial Specialization for value deferencing.
template <typename T, size_t DIM>
class Local_Ref < T, DIM, 0 > {
 public:
  template<typename T_, size_t DIM_> friend class Matrix;
  Matrix_RefProxy<T, DIM> * _proxy;

  Local_Ref<T, DIM, 0>() = default;

  inline T * at_(size_t pos);
  inline operator T();
  inline T operator=(const T value);
};

// Partial Specialization for value deferencing.
template <typename T, size_t DIM>
class Matrix_Ref < T, DIM, 0 > {
 public:
  template<typename T_, size_t DIM_> friend class Matrix;
  Matrix_RefProxy<T, DIM> * _proxy;
  
  inline GlobRef<T> at_(size_t unit, size_t elem) const;

  Matrix_Ref<T, DIM, 0>() = default;

  operator T();
  T operator=(const T value);
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

 public:
  template<typename T_, size_t DIM1, size_t DIM2> friend class Matrix_Ref;
  template<typename T_, size_t DIM1, size_t DIM2> friend class Local_Ref;

  Local_Ref<ELEMENT_TYPE, DIM, DIM> _local;

  static_assert(std::is_trivial<ELEMENT_TYPE>::value,
    "Element type must be trivial copyable");

 public:
  inline Local_Ref<ELEMENT_TYPE, DIM, DIM> local() const;

  // Proxy, Matrix_Ref and Local_Ref are created at initialization.
  Matrix(
    const dash::SizeSpec<DIM> & ss,
    const dash::DistSpec<DIM> &ds = dash::DistSpec<DIM>(),
    Team &t = dash::Team::All(), const TeamSpec<DIM> &ts = TeamSpec<DIM>());

  // delegating constructor
  inline Matrix(const dash::Pattern<DIM> &pat)
    : Matrix(pat.sizespec(),
             pat.distspec(),
             pat.team(),
             pat.teamspec()) {  }
  // delegating constructor
  inline Matrix(size_t nelem,
                Team &t = dash::Team::All())
    : Matrix(dash::Pattern<DIM>(nelem, t)) {  }

  inline ~Matrix();

  inline Pattern<DIM> & pattern();
  inline Team &team();
  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline const_pointer data() const noexcept;
  inline iterator begin() noexcept;
  inline iterator end() noexcept;
  inline ELEMENT_TYPE * lbegin() noexcept;
  inline ELEMENT_TYPE * lend() noexcept;
  inline void forall(std::function<void(long long)> func);

  template<size_t SUBDIM>
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> sub(size_type n);
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> col(size_type n);
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> row(size_type n);

  template<size_t SUBDIM>
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM> submat(size_type n, size_type range);
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM> rows(size_type n, size_type range);
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM> cols(size_type n, size_type range);
  inline Matrix_Ref<ELEMENT_TYPE, DIM, DIM - 1> operator[](size_type n);

  template<typename ... Args>
  inline reference at(Args... args);

  template<typename... Args>
  inline reference operator()(Args... args);
  inline Pattern<DIM> pattern() const;
  inline bool islocal(size_type n);
  inline bool islocal(size_t dim, size_type n);

  template <int level>
  inline dash::HView<Matrix<ELEMENT_TYPE, DIM>, level, DIM> hview();
  inline operator Matrix_Ref<ELEMENT_TYPE, DIM, DIM> ();

 private:
  dash::Team &_team;
  dart_unit_t _myid;
  dash::Pattern<DIM> _pattern;
  size_type _size;  // total size (#elements)
  size_type _lsize; // local size (#local elements)
  pointer *_ptr;
  dart_gptr_t _dart_gptr;
  Matrix_Ref<ELEMENT_TYPE, DIM, DIM> _ref;
};

}  // namespace dash

#include "internal/Matrix-inl.h"

#endif  // DASH_MATRIX_H_INCLUDED
