#ifndef DASH__MATRIX__MATRIX_REF_VIEW_H_INCLUDED
#define DASH__MATRIX__MATRIX_REF_VIEW_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Container.h>

#include <dash/iterator/GlobIter.h>


namespace dash {

/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  typename IndexT,
  class PatternT >
class Matrix;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR,
  class PatternT >
class MatrixRef;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR,
  class PatternT >
class LocalMatrixRef;

/**
 * Stores information needed by subscripting and subdim selection.
 * A new \c MatrixRefView instance is created once for every dimension in
 * multi-subscripting.
 *
 * \ingroup DashMatrixConcept
 */
template <
  typename T,
  dim_t NumDimensions,
  class PatternT =
    TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t> >
class MatrixRefView
{
 public:
  typedef typename PatternT::index_type             index_type;

 private:
  /// The view's next unspecified dimension, initialized with 0.
  dim_t                                             _dim       = 0;
  /// The matrix referenced by the view.
  Matrix<T, NumDimensions, index_type, PatternT>  * _mat;
  /// Coordinates of a single referenced element if view references fully
  /// specified coordinates.
  ::std::array<index_type, NumDimensions>           _coord     = {{  }};
  /// View offset and extents in global index range.
  ViewSpec<NumDimensions, index_type>               _viewspec;
  /// View offset and extents in local index range.
  ViewSpec<NumDimensions, index_type>               _l_viewspec;

 public:
  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_ >
  friend class MatrixRef;
  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_ >
  friend class LocalMatrixRef;
  template<
    typename T_,
    dim_t NumDimensions1,
    typename IndexT_,
    class PatternT_ >
  friend class Matrix;

  MatrixRefView<T, NumDimensions, PatternT>();
  MatrixRefView<T, NumDimensions, PatternT>(
    Matrix<T, NumDimensions, index_type, PatternT> * matrix);

  GlobRef<T> global_reference() const;

  GlobRef<T> global_reference(
    const ::std::array<typename PatternT::index_type, NumDimensions> & coords
  ) const;
};

} // namespace dash

#include <dash/matrix/internal/MatrixRefView-inl.h>

#endif  // DASH__MATRIX__MATRIX_REF_VIEW_H_INCLUDED
