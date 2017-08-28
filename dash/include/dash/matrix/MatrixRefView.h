#ifndef DASH__MATRIX__MATRIX_REF_VIEW_H_INCLUDED
#define DASH__MATRIX__MATRIX_REF_VIEW_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Meta.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>

#include <dash/iterator/GlobIter.h>

#include <iostream>
#include <sstream>


namespace dash {

/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  typename IndexT,
  class PatternT,
  typename MSpaceC>
class Matrix;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR,
  class PatternT,
  typename MSpaceC >
class MatrixRef;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR,
  class PatternT,
  typename MSpaceC>
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
    TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t>,
  typename MSpaceC = dash::memory_space_host_tag>
class MatrixRefView
{
  using matrix_t =
      Matrix<T, NumDimensions, typename PatternT::index_type, PatternT, MSpaceC>;

public:
  typedef typename PatternT::index_type             index_type;

 private:
  /// The view's next unspecified dimension, initialized with 0.
  dim_t                                             _dim       = 0;
  /// The matrix referenced by the view.
  matrix_t  * _mat;
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
    class PatternT_,
    typename MSpaceC_>
  friend std::ostream & operator<<(
    std::ostream & os,
    const MatrixRefView<T_, NumDimensions1, PatternT_, MSpaceC_> & mrefview);

  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_,
    typename MSpaceC_>
  friend class MatrixRef;
  template<
    typename T_,
    dim_t NumDimensions1,
    class PatternT_,
    typename MSpaceC_ >
  friend class MatrixRefView;
  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_,
    typename MSpaceC_ >
  friend class LocalMatrixRef;
  template<
    typename T_,
    dim_t NumDimensions1,
    typename IndexT_,
    class PatternT_,
    typename MSpaceC_ >
  friend class Matrix;

  MatrixRefView<T, NumDimensions, PatternT, MSpaceC>();

  template <class T_>
  MatrixRefView<T, NumDimensions, PatternT, MSpaceC>(
    const MatrixRefView<T_, NumDimensions, PatternT, MSpaceC> & other);

  template <class T_>
  MatrixRefView<T, NumDimensions, PatternT, MSpaceC>(
    Matrix<T_, NumDimensions, index_type, PatternT, MSpaceC> * matrix);

  GlobRef<T>       global_reference();
  GlobRef<const T> global_reference() const;

  GlobRef<T>       global_reference(
    const ::std::array<typename PatternT::index_type, NumDimensions> & coords
  );
  GlobRef<const T> global_reference(
    const ::std::array<typename PatternT::index_type, NumDimensions> & coords
  ) const;
};

template<
  typename T_,
  dim_t NumDimensions1,
  class PatternT_,
  typename MSpaceC_ >
std::ostream & operator<<(
  std::ostream & os,
  const MatrixRefView<T_, NumDimensions1, PatternT_, MSpaceC_> & mrefview) {
  std::ostringstream ss;
  ss << dash::typestr(mrefview)
     << "("
     << "dim:"    << mrefview._dim      << ", "
     << "coords:" << mrefview._coord    << ", "
     << "view:"   << mrefview._viewspec
     << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#include <dash/matrix/internal/MatrixRefView-inl.h>

#endif  // DASH__MATRIX__MATRIX_REF_VIEW_H_INCLUDED
