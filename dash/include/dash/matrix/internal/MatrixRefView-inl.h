#ifndef DASH__MATRIX__MATRIX_REF_VIEW_INL_H_INCLUDED
#define DASH__MATRIX__MATRIX_REF_VIEW_INL_H_INCLUDED

#include <dash/matrix/MatrixRefView.h>


namespace dash {

template<typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>
::MatrixRefView()
: _dim(0)
{
  DASH_LOG_TRACE("MatrixRefView()");
}

template<typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>
::MatrixRefView(
  Matrix<T, NumDim, index_type, PatternT> * matrix)
: _dim(0),
  _mat(matrix),
  _viewspec(matrix->extents()),
  _l_viewspec()
{
  // TODO: Check if initializing local viewspec with default viewspec is okay.
  DASH_LOG_TRACE_VAR("MatrixRefView(matrix)", matrix);
}

template<typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>
::MatrixRefView(
  const MatrixRefView<T, NumDim, PatternT> & other)
: _dim(other._dim),
  _mat(other._mat),
  _coord(other._coord),
  _viewspec(other._viewspec),
  _l_viewspec(other._l_viewspec)
{
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _mat);
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _dim);
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _coord);
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _l_viewspec);
}


template<typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>&
MatrixRefView<T, NumDim, PatternT>
::operator=(
  const MatrixRefView<T, NumDim, PatternT> & other)
{
  if (this != &other) {
    _dim = other._dim;
    _mat = other._mat;
    _coord = other._coord;
    _viewspec = other._viewspec;
    _l_viewspec = other._l_viewspec;
  }
  return *this;
}

template<typename T, dim_t NumDim, class PatternT>
GlobRef<T>
MatrixRefView<T, NumDim, PatternT>
::global_reference() const
{
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference()", _coord);
  auto pattern       = _mat->pattern();
  auto memory_layout = pattern.memory_layout();
  // MatrixRef coordinate and viewspec to global linear index:
  auto global_index  = memory_layout.at(_coord, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference", global_index);
  auto global_begin  = _mat->begin();
  // Global reference at global linear index:
  GlobRef<T> ref(global_begin[global_index]);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference >", ref);
  return ref;
}

} // namespace dash

#endif  // DASH__MATRIX__MATRIX_REF_VIEW_INL_H_INCLUDED
