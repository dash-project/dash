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
template <typename T, size_t NumDimensions, size_t CUR> class MatrixRef;
/// Forward-declaration
template <typename T, size_t NumDimensions, size_t CUR> class LocalRef;

/**
 * Stores information needed by subscripting and subdim selection.
 * A new RefProxy instance is created once for every dimension in
 * multi-subscripting.
 */
template <typename T, size_t NumDimensions>
class MatrixRefProxy {
 private:
  int                                      _dim;
  Matrix<T, NumDimensions>               * _mat;
  ::std::array<long long, NumDimensions>   _coord     = {  };
  ViewSpec<NumDimensions>                  _viewspec;

 public:
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> 
    friend class MatrixRef;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> 
    friend class LocalRef;
  template<typename T_, size_t NumDimensions1>
    friend class Matrix;

  MatrixRefProxy<T, NumDimensions>();
  MatrixRefProxy<T, NumDimensions>(
    Matrix<T, NumDimensions> * matrix);
  MatrixRefProxy<T, NumDimensions>(
    const MatrixRefProxy<T, NumDimensions> & other);

  GlobRef<T> global_reference() const;
};

/**
 * Local part of a Matrix, provides local operations.
 * Wrapper class for RefProxy. 
 */
template <
  typename T,
  size_t NumDimensions,
  size_t CUR = NumDimensions> 
class LocalRef {
 public:
  template<typename T_, size_t NumDimensions_> friend class Matrix;

 public:
  MatrixRefProxy<T, NumDimensions> * _proxy;

 public:
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

 public:
  LocalRef<T, NumDimensions, CUR>() = default;

  LocalRef<T, NumDimensions, CUR>(Matrix<T, NumDimensions> * mat);

  inline operator LocalRef<T, NumDimensions, CUR - 1> && ();
  // SHOULD avoid cast from MatrixRef to LocalRef.
  // Different operation semantics.
  inline operator MatrixRef<T, NumDimensions, CUR> ();
  inline long long extent(size_t dim) const;
  inline size_type size() const;
  inline T & at_(size_type pos);

  template<typename ... Args>
  T & at(Args... args);

  template<typename... Args>
  T & operator()(Args... args);

  LocalRef<T, NumDimensions, CUR - 1> && operator[](size_t n);
  LocalRef<T, NumDimensions, CUR - 1> operator[](size_t n) const;

  template<size_t NumSubDimensions>
  LocalRef<T, NumDimensions, NumDimensions - 1> sub(size_type n);

  inline LocalRef<T, NumDimensions, NumDimensions - 1> col(size_type n);
  inline LocalRef<T, NumDimensions, NumDimensions - 1> row(size_type n);

  template<size_t NumSubDimensions>
  LocalRef<T, NumDimensions, NumDimensions> submat(
    size_type n,
    size_type range);

  inline LocalRef<T, NumDimensions, NumDimensions> rows(
    size_type n,
    size_type range);

  inline LocalRef<T, NumDimensions, NumDimensions> cols(
    size_type n,
    size_type range);
};

// Partial Specialization for value deferencing.
template <typename T, size_t NumDimensions>
class LocalRef<T, NumDimensions, 0> {
 public:
  template<typename T_, size_t NumDimensions_> friend class Matrix;

  MatrixRefProxy<T, NumDimensions> * _proxy;

  LocalRef<T, NumDimensions, 0>() = default;

  inline T * at_(size_t pos);
  inline operator T();
  inline T operator=(const T value);
};

/**
 * Wrapper class for RefProxy, represents Matrix and submatrix of a
 * Matrix and provides global operations.
 */
template <
  typename ElementType,
  size_t NumDimensions,
  size_t CUR = NumDimensions>
class MatrixRef {
 private:
  typedef MatrixRef<ElementType, NumDimensions, CUR>
    self_t;
  typedef MatrixRefProxy<ElementType, NumDimensions>
    MatrixRefProxy_t;
  typedef LocalRef<ElementType, NumDimensions, NumDimensions>
    LocalRef_t;
  typedef GlobIter<ElementType, Pattern<NumDimensions> >
    GlobIter_t;

 public:
  typedef ElementType value_type;

  typedef size_t size_type;
  typedef size_t difference_type;

  typedef GlobIter_t                                          iterator;
  typedef const GlobIter_t                              const_iterator;
  typedef std::reverse_iterator<iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                    const_reference;

  typedef GlobIter_t                                           pointer;
  typedef const GlobIter_t                               const_pointer;

 public:
  template<typename T_, size_t NumDimensions_>
    friend class Matrix;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2>
    friend class MatrixRef;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2>
    friend class LocalRef;
  inline operator MatrixRef<ElementType, NumDimensions, CUR-1> && ();

 public:
  MatrixRef<ElementType, NumDimensions, CUR>()
  : _proxy(nullptr) { // = default;
    DASH_LOG_TRACE_VAR("MatrixRef<T, D, C>()", NumDimensions);
  }
  MatrixRef<ElementType, NumDimensions, CUR>(
    const MatrixRef<ElementType, NumDimensions, CUR+1> & previous,
    long long coord);

  Pattern<NumDimensions> & pattern();

  Team & team();

  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline void forall(::std::function<void(long long)> func);
  inline Pattern<NumDimensions> pattern() const;

//MatrixRef<ElementType, NumDimensions, CUR-1> && operator[](size_t n);
  MatrixRef<ElementType, NumDimensions, CUR-1> operator[](size_t n) const;

  template<size_t NumSubDimensions>
  MatrixRef<ElementType, NumDimensions, NumDimensions-1>
  sub(size_type n);
  
  MatrixRef<ElementType, NumDimensions, NumDimensions-1>
  col(size_type n);
  
  MatrixRef<ElementType, NumDimensions, NumDimensions-1>
  row(size_type n);

  template<size_t NumSubDimensions>
  MatrixRef<ElementType, NumDimensions, NumDimensions> submat(
    size_type n,
    size_type range);

  MatrixRef<ElementType, NumDimensions, NumDimensions> rows(
    size_type n,
    size_type range);
  
  MatrixRef<ElementType, NumDimensions, NumDimensions> cols(
    size_type n,
    size_type range);

  template<typename ... Args>
  reference at(Args... args);

  template<typename... Args>
  reference operator()(Args... args);

  inline bool is_local(size_type n);
  inline bool is_local(size_t dim, size_type n);

  template <int level>
  dash::HView<Matrix<ElementType, NumDimensions>, level> inline hview();

 private:
  MatrixRefProxy<ElementType, NumDimensions> * _proxy;
};

// Partial Specialization for value deferencing.
template <typename ElementType, size_t NumDimensions>
class MatrixRef<ElementType, NumDimensions, 0> {
 public:
  template<typename T_, size_t NumDimensions_> friend class Matrix;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2> 
    friend class MatrixRef;

  MatrixRefProxy<ElementType, NumDimensions> * _proxy;
  
  inline const GlobRef<ElementType> at_(size_t unit, size_t elem) const;
  inline GlobRef<ElementType> at_(size_t unit, size_t elem);

  MatrixRef<ElementType, NumDimensions, 0>()
  : _proxy(nullptr)
  {
    DASH_LOG_TRACE_VAR("MatrixRef<T, D, 0>()", NumDimensions);
  }

  MatrixRef<ElementType, NumDimensions, 0>(
    const MatrixRef<ElementType, NumDimensions, 1> & previous,
    long long coord);

  operator ElementType();
  ElementType operator=(const ElementType value);
};

template<
  typename ElementType,
  size_t NumDimensions>
class Matrix {
  static_assert(std::is_trivial<ElementType>::value,
    "Element type must be trivial copyable");

 private:
  typedef Matrix<ElementType, NumDimensions>
    self_t;
  typedef MatrixRef<ElementType, NumDimensions, NumDimensions>
    MatrixRef_t;
  typedef MatrixRefProxy<ElementType, NumDimensions>
    MatrixRefProxy_t;
  typedef LocalRef<ElementType, NumDimensions, NumDimensions>
    LocalRef_t;
  typedef Pattern<NumDimensions>
    Pattern_t;
  typedef GlobIter<ElementType, Pattern<NumDimensions> >
    GlobIter_t;

 public:
  typedef ElementType                                         value_type;
  typedef size_t                                               size_type;
  typedef size_t                                         difference_type;

  typedef GlobIter_t                                            iterator;
  typedef const GlobIter_t                                const_iterator;
  typedef std::reverse_iterator<iterator>               reverse_iterator;
  typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

  typedef GlobRef<value_type>                                  reference;
  typedef const GlobRef<value_type>                      const_reference;

  typedef GlobIter_t                                             pointer;
  typedef const GlobIter_t                                 const_pointer;

 public:
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2>
    friend class MatrixRef;
  template<typename T_, size_t NumDimensions1, size_t NumDimensions2>
    friend class LocalRef;

 public:
  LocalRef<ElementType, NumDimensions, NumDimensions> _local;

 public:
  inline LocalRef_t local() const;

  // Proxy, MatrixRef and LocalRef are created at initialization.
  Matrix(
    const dash::SizeSpec<NumDimensions> & ss,
    const dash::DistributionSpec<NumDimensions> & ds =
      dash::DistributionSpec<NumDimensions>(),
    Team & t =
      dash::Team::All(),
    const TeamSpec<NumDimensions> & ts =
      TeamSpec<NumDimensions>());

  // delegating constructor
  inline Matrix(const dash::Pattern<NumDimensions> & pat)
    : Matrix(pat.sizespec(),
             pat.distspec(),
             pat.team(),
             pat.teamspec()) { }
  // delegating constructor
  inline Matrix(size_t nelem,
                Team & t = dash::Team::All())
    : Matrix(dash::Pattern<NumDimensions>(nelem, t)) { }

  inline ~Matrix();

  inline Team & team();
  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline const_pointer data() const noexcept;
  inline iterator begin() noexcept;
  inline const_iterator begin() const noexcept;
  inline iterator end() noexcept;
  inline const_iterator end() const noexcept;
  inline ElementType * lbegin() noexcept;
  inline ElementType * lend() noexcept;
  inline void forall(std::function<void(long long)> func);

  template<size_t NumSubDimensions>
  inline MatrixRef<ElementType, NumDimensions, NumDimensions - 1> 
  sub(size_type n);
  
  inline MatrixRef<ElementType, NumDimensions, NumDimensions - 1> 
  col(size_type n);
  
  inline MatrixRef<ElementType, NumDimensions, NumDimensions - 1> 
  row(size_type n);

  template<size_t NumSubDimensions>
  inline MatrixRef<ElementType, NumDimensions, NumDimensions> 
  submat(size_type n, size_type range);

  inline MatrixRef<ElementType, NumDimensions, NumDimensions> 
  rows(size_type n, size_type range);
  
  inline MatrixRef<ElementType, NumDimensions, NumDimensions>
  cols(size_type n, size_type range);
  
  inline MatrixRef<ElementType, NumDimensions, NumDimensions - 1>   
  operator[](size_type n);

  template<typename ... Args>
  inline reference at(Args... args);

  template<typename... Args>
  inline reference operator()(Args... args);
  inline const Pattern<NumDimensions> & pattern() const;
  inline bool is_local(size_type n);
  inline bool is_local(size_t dim, size_type n);

  template <int level>
  inline dash::HView<Matrix<ElementType, NumDimensions>, level> hview();
  inline operator MatrixRef<ElementType, NumDimensions, NumDimensions> ();

 private:
  dash::Team           & _team;
  /// DART id of the unit that owns this matrix instance
  dart_unit_t _myid;
  /// The matrix elements' distribution pattern
  Pattern_t              _pattern;
  /// Capacity (total number of elements) of the matrix
  size_type              _size;
  /// Number of elements in the matrix local to this unit
  size_type              _local_mem_size;
  pointer              * _ptr;
  dart_gptr_t            _dart_gptr;
  GlobMem<ElementType>   _glob_mem;
  MatrixRef_t            _ref;
};

}  // namespace dash

#include "internal/Matrix-inl.h"

#endif  // DASH_MATRIX_H_INCLUDED
