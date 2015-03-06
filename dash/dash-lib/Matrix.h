
#ifndef DASH_ARRAY_H_INCLUDED
#define DASH_ARRAY_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include "Team.h"
//#include "Pattern.h"
#include "Pattern.h"
#include "GlobPtr.h"
#include "GlobRef.h"
#include "HView.h"

#include "dart.h"

namespace dash {

template <typename T, size_t DIM> class Matrix;

template <typename T, size_t DIM> class LocalProxyMatrix {
private:
  Matrix<T, DIM> *m_ptr;

public:
  LocalProxyArray(Matrix<T, DIM> *ptr) { m_ptr = ptr; }

  T *begin() noexcept { return m_ptr->lbegin(); }

  T *end() noexcept { return m_ptr->lend(); }
};

template <typename ELEMENT_TYPE> class Array {}
}
