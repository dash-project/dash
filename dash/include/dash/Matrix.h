#ifndef DASH_MATRIX_H_INCLUDED
#define DASH_MATRIX_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>

namespace dash {

/// Forward-declaration
template <typename T, size_t NumDimensions> class Matrix;
/// Forward-declaration
template <typename T, size_t NumDimensions, size_t CUR> class Matrix_Ref;
/// Forward-declaration
template <typename T, size_t NumDimensions, size_t CUR> class Local_Ref;

/**
 * Stores information needed by subscripting and subdim selection.
 * A new RefProxy instance is created once for every dimension in multi-subscripting.
 */
template <typename T, size_t NumDimensions> class Matrix_RefProxy {
 private:
  int _dim;
  Matrix<T, NumDimensions> * _mat;
  ::std::array<long long, NumDimensions> _coord;
  ViewSpec<NumDimensions> _viewspec;

 public:
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> friend class Matrix_Ref;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> friend class Local_Ref;
  template<typename T_, size_t NumDimensions1> friend class Matrix;

  Matrix_RefProxy<T, NumDimensions>();
};

/**
 * Local_Ref represents local part of a Matrix and provices local operations.
 * Wrapper class for RefProxy. 
 */
template <typename T, size_t NumDimensions, size_t CUR = NumDimensions> class Local_Ref {
 public:
  template<typename T_, size_t NumDimensions_> friend class Matrix;

  Matrix_RefProxy<T, NumDimensions> * _proxy;

  typedef T value_type;

  // NO allocator_type!
  typedef size_t size_type;
  typedef size_t difference_type;

  typedef GlobIter<value_type, Pattern<NumDimensions>> iterator;
  typedef const GlobIter<value_type, Pattern<NumDimensions>> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type> reference;
  typedef const GlobRef<value_type> const_reference;

  typedef GlobIter<value_type, Pattern<NumDimensions>> pointer;
  typedef const GlobIter<value_type, Pattern<NumDimensions>> const_pointer;

  Local_Ref<T, NumDimensions, CUR>() = default;

  Local_Ref<T, NumDimensions, CUR>(Matrix<T, NumDimensions> * mat);

  inline operator Local_Ref<T, NumDimensions, CUR - 1> && ();
  // SHOULD avoid cast from Matrix_Ref to Local_Ref. Different operation semantics.
  inline operator Matrix_Ref<T, NumDimensions, CUR> ();
  inline long long extent(size_t dim) const;
  inline size_type size() const;
  inline T & at_(size_type pos);

  template<typename ... Args>
  T& at(Args... args);

  template<typename... Args>
  T& operator()(Args... args);

  Local_Ref<T, NumDimensions, CUR - 1> && operator[](size_t n);
  Local_Ref<T, NumDimensions, CUR - 1> operator[](size_t n) const;

  template<size_t NumSubDimensions>
  Local_Ref<T, NumDimensions, NumDimensions - 1> sub(size_type n);

  inline Local_Ref<T, NumDimensions, NumDimensions - 1> col(size_type n);
  inline Local_Ref<T, NumDimensions, NumDimensions - 1> row(size_type n);

  template<size_t NumSubDimensions>
  Local_Ref<T, NumDimensions, NumDimensions> submat(size_type n, size_type range);

  inline Local_Ref<T, NumDimensions, NumDimensions> rows(size_type n, size_type range);
  inline Local_Ref<T, NumDimensions, NumDimensions> cols(size_type n, size_type range);
};

// Wrapper class for RefProxy. Matrix_Ref represents Matrix and Submatrix a Matrix and
// provices global operations.
template <typename T, size_t NumDimensions, size_t CUR = NumDimensions>
class Matrix_Ref {
 public:
  template<typename T_, size_t NumDimensions_>
    friend class Matrix;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2>
    friend class Matrix_Ref;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2>
    friend class Local_Ref;

  typedef T value_type;

  // NO allocator_type!
  typedef size_t size_type;
  typedef size_t difference_type;

  typedef GlobIter<value_type, Pattern<NumDimensions>> iterator;
  typedef const GlobIter<value_type, Pattern<NumDimensions>> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type> reference;
  typedef const GlobRef<value_type> const_reference;

  typedef GlobIter<value_type, Pattern<NumDimensions>> pointer;
  typedef const GlobIter<value_type, Pattern<NumDimensions>> const_pointer;

  inline operator Matrix_Ref<T, NumDimensions, CUR-1> && ();

  Matrix_Ref<T, NumDimensions, CUR>() = default;

  Pattern<NumDimensions> & pattern();

  Team & team();

  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline void forall(::std::function<void(long long)> func);
  inline Pattern<NumDimensions> pattern() const;

  Matrix_Ref<T, NumDimensions, CUR-1> && operator[](size_t n);
  Matrix_Ref<T, NumDimensions, CUR-1> operator[](size_t n) const;

  template<size_t NumSubDimensions>
  Matrix_Ref<T, NumDimensions, NumDimensions-1> sub(size_type n);
  Matrix_Ref<T, NumDimensions, NumDimensions-1> col(size_type n);
  Matrix_Ref<T, NumDimensions, NumDimensions-1> row(size_type n);

  template<size_t NumSubDimensions>
  Matrix_Ref<T, NumDimensions, NumDimensions> submat(size_type n, size_type range);
  Matrix_Ref<T, NumDimensions, NumDimensions> rows(size_type n, size_type range);
  Matrix_Ref<T, NumDimensions, NumDimensions> cols(size_type n, size_type range);

  inline reference at_(size_type unit, size_type elem);

  template<typename ... Args>
  reference at(Args... args);

  template<typename... Args>
  reference operator()(Args... args);

  // For 1D. OBSOLETE
  inline bool isLocal(size_type n);
  inline bool isLocal(size_t dim, size_type n);

  template <int level> dash::HView<Matrix<T, NumDimensions>, level> inline hview();

 private:
  Matrix_RefProxy<T, NumDimensions> * _proxy;
};

// Partial Specialization for value deferencing.
template <typename T, size_t NumDimensions>
class Local_Ref < T, NumDimensions, 0 > {
 public:
  template<typename T_, size_t NumDimensions_> friend class Matrix;

  Matrix_RefProxy<T, NumDimensions> * _proxy;

  Local_Ref<T, NumDimensions, 0>() = default;

  inline T * at_(size_t pos);
  inline operator T();
  inline T operator=(const T value);
};

// Partial Specialization for value deferencing.
template <typename T, size_t NumDimensions>
class Matrix_Ref < T, NumDimensions, 0 > {
 public:
  template<typename T_, size_t NumDimensions_> friend class Matrix;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> friend class Matrix_Ref;

  Matrix_RefProxy<T, NumDimensions> * _proxy;
  
  inline GlobRef<T> at_(size_t unit, size_t elem) const;

  Matrix_Ref<T, NumDimensions, 0>() = default;

  operator T();
  T operator=(const T value);
};

template <typename ElementType, size_t NumDimensions>
class Matrix {
 public:
  typedef ElementType value_type;

  typedef size_t size_type;
  typedef size_t difference_type;

  typedef GlobIter<value_type, Pattern<NumDimensions>> iterator;
  typedef const GlobIter<value_type, Pattern<NumDimensions>> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type> reference;
  typedef const GlobRef<value_type> const_reference;

  typedef GlobIter<value_type, Pattern<NumDimensions>> pointer;
  typedef const GlobIter<value_type, Pattern<NumDimensions>> const_pointer;

 public:
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> friend class Matrix_Ref;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> friend class Local_Ref;

  Local_Ref<ElementType, NumDimensions, NumDimensions> _local;

  static_assert(std::is_trivial<ElementType>::value,
    "Element type must be trivial copyable");

 public:
  inline Local_Ref<ElementType, NumDimensions, NumDimensions> local() const;

  // Proxy, Matrix_Ref and Local_Ref are created at initialization.
  Matrix(
    const dash::SizeSpec<NumDimensions> & ss,
    const dash::DistributionSpec<NumDimensions> & ds =
      dash::DistributionSpec<NumDimensions>(),
    Team &t = dash::Team::All(), const TeamSpec<NumDimensions> & ts =
      TeamSpec<NumDimensions>());

  // delegating constructor
  inline Matrix(const dash::Pattern<NumDimensions> &pat)
    : Matrix(pat.sizespec(),
             pat.distspec(),
             pat.team(),
             pat.teamspec()) {  }
  // delegating constructor
  inline Matrix(size_t nelem,
                Team &t = dash::Team::All())
    : Matrix(dash::Pattern<NumDimensions>(nelem, t)) {  }

  inline ~Matrix();

  inline Pattern<NumDimensions> & pattern();
  inline Team &team();
  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline const_pointer data() const noexcept;
  inline iterator begin() noexcept;
  inline iterator end() noexcept;
  inline ElementType * lbegin() noexcept;
  inline ElementType * lend() noexcept;
  inline void forall(std::function<void(long long)> func);

  template<size_t NumSubDimensions>
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions - 1> sub(size_type n);
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions - 1> col(size_type n);
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions - 1> row(size_type n);

  template<size_t NumSubDimensions>
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions> submat(size_type n, size_type range);
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions> rows(size_type n, size_type range);
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions> cols(size_type n, size_type range);
  inline Matrix_Ref<ElementType, NumDimensions, NumDimensions - 1> operator[](size_type n);

  template<typename ... Args>
  inline reference at(Args... args);

  template<typename... Args>
  inline reference operator()(Args... args);
  inline Pattern<NumDimensions> pattern() const;
  inline bool isLocal(size_type n);
  inline bool isLocal(size_t dim, size_type n);

  template <int level>
  inline dash::HView<Matrix<ElementType, NumDimensions>, level> hview();
  inline operator Matrix_Ref<ElementType, NumDimensions, NumDimensions> ();

 private:
  dash::Team & _team;
  /// DART id of the unit that owns this matrix instance
  dart_unit_t _myid;
  /// The matrix elements' distribution pattern
  dash::Pattern<NumDimensions> _pattern;
  /// Capacity (total number of elements) of the matrix
  size_type _size;
  /// Number of elements in the matrix local to this unit
  size_type _local_mem_size;
  pointer * _ptr;
  dart_gptr_t _dart_gptr;
  GlobMem<ElementType> _glob_mem;
  Matrix_Ref<ElementType, NumDimensions, NumDimensions> _ref;
};

}  // namespace dash

#include "internal/Matrix-inl.h"

#endif  // DASH_MATRIX_H_INCLUDED
