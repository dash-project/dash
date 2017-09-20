#ifndef DASH__MATRIX__MATRIX_REF_VIEW_INL_H_INCLUDED
#define DASH__MATRIX__MATRIX_REF_VIEW_INL_H_INCLUDED

#include <dash/matrix/MatrixRefView.h>


namespace dash {

template <typename T, dim_t NumDim, class PatternT>
MatrixRefView<T, NumDim, PatternT>
::MatrixRefView()
: _dim(0), _mat(NULL)
{
  DASH_LOG_TRACE("MatrixRefView()");
}

template <typename T, dim_t NumDim, class PatternT>
template <class T_>
MatrixRefView<T, NumDim, PatternT>
::MatrixRefView(
  const MatrixRefView<T_, NumDim, PatternT> & other)
: _dim(other._dim)
  // cast from Matrix<T, ...> * to Matrix<const T, ...> *
, _mat(reinterpret_cast< Matrix<T, NumDim, index_type, PatternT> * >(
         other._mat))
, _coord(other._coord)
, _viewspec(other._viewspec)
, _l_viewspec(other._l_viewspec)
{
  DASH_LOG_TRACE("MatrixRefView(other)");
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView(other)", _l_viewspec);
}

template <typename T, dim_t NumDim, class PatternT>
template <class T_>
MatrixRefView<T, NumDim, PatternT>
::MatrixRefView(
  Matrix<T_, NumDim, index_type, PatternT> * matrix)
: _dim(0)
, _mat(matrix)
, _viewspec(matrix->extents())
, _l_viewspec(matrix->pattern().local_extents())
{
  // TODO: Check if initializing local viewspec with default viewspec
  //       is okay.
  DASH_LOG_TRACE_VAR("MatrixRefView(matrix)", matrix);
  DASH_LOG_TRACE_VAR("MatrixRefView(matrix)", _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView(matrix)", _l_viewspec);
}

template <typename T, dim_t NumDim, class PatternT>
GlobRef<const T>
MatrixRefView<T, NumDim, PatternT>
::global_reference() const
{
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference()", _coord);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference",   _viewspec);
  const auto & pattern       = _mat->pattern();
  const auto & memory_layout = pattern.memory_layout();
  // MatrixRef coordinate and viewspec to global linear index:
  const auto & global_index  = memory_layout.at(_coord, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference", global_index);
  const auto & global_begin  = _mat->begin();
  // Global reference at global linear index:
  GlobRef<const T> ref(global_begin[global_index]);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference >", ref);
  return ref;
}

template <typename T, dim_t NumDim, class PatternT>
GlobRef<T>
MatrixRefView<T, NumDim, PatternT>
::global_reference()
{
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference()", _coord);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference",   _viewspec);
  const auto & pattern       = _mat->pattern();
  const auto & memory_layout = pattern.memory_layout();
  // MatrixRef coordinate and viewspec to global linear index:
  const auto & global_index  = memory_layout.at(_coord, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference", global_index);
  auto         global_begin  = _mat->begin();
  // Global reference at global linear index:
  GlobRef<T> ref(global_begin[global_index]);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference >", ref);
  return ref;
}

template <typename T, dim_t NumDim, class PatternT>
GlobRef<const T>
MatrixRefView<T, NumDim, PatternT>
::global_reference(
  const ::std::array<typename PatternT::index_type, NumDim> & c) const
{
  ::std::array<typename PatternT::index_type, NumDim> coords = _coord;
  for(auto i = _dim; i < NumDim; ++i) {
    coords[i] = c[i-_dim];
  }
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference()", coords);
  const auto & pattern       = _mat->pattern();
  const auto & memory_layout = pattern.memory_layout();
  // MatrixRef coordinate and viewspec to global linear index:
  const auto & global_index  = memory_layout.at(coords, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference", global_index);
  const auto & global_begin  = _mat->begin();
  // Global reference at global linear index:
  GlobRef<const T> ref(global_begin[global_index]);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference >", ref);
  return ref;
}

template <typename T, dim_t NumDim, class PatternT>
GlobRef<T>
MatrixRefView<T, NumDim, PatternT>
::global_reference(
  const ::std::array<typename PatternT::index_type, NumDim> & c)
{
  ::std::array<typename PatternT::index_type, NumDim> coords = _coord;
  for(auto i = _dim; i < NumDim; ++i) {
    coords[i] = c[i-_dim];
  }
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference()", coords);
  const auto & pattern       = _mat->pattern();
  const auto & memory_layout = pattern.memory_layout();
  // MatrixRef coordinate and viewspec to global linear index:
  const auto & global_index  = memory_layout.at(coords, _viewspec);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference", global_index);
  auto         global_begin  = _mat->begin();
  // Global reference at global linear index:
  GlobRef<T> ref(global_begin[global_index]);
  DASH_LOG_TRACE_VAR("MatrixRefView.global_reference >", ref);
  return ref;
}

} // namespace dash

#endif  // DASH__MATRIX__MATRIX_REF_VIEW_INL_H_INCLUDED
