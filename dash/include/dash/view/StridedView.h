#ifndef DASH__VIEW__STRIDED_VIEW_H__INCLUDED
#define DASH__VIEW__STRIDED_VIEW_H__INCLUDED

#include <dash/Types.h>

#include <dash/view/SetUnion.h>
#include <dash/view/MultiView.h>

#include <vector>


namespace dash {

template <dim_t NDim>
class StridedView;

template <>
class StridedView<0>;


template <dim_t NDim>
class StridedView
: public dash::CompositeView<
           dash::MultiView<NDim-1>
         >
{

};

}


#endif // DASH__VIEW__STRIDED_VIEW_H__INCLUDED
