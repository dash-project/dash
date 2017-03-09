#ifndef DASH__VIEW__UNION_H__INCLUDED
#define DASH__VIEW__UNION_H__INCLUDED

#include <dash/Types.h>
#include <dash/Dimensional.h>
#include <dash/Cartesian.h>

#include <vector>


namespace dash {


template <class ComponentViewType>
class CompositeView
{

public:
  CompositeView(std::initializer_list<ComponentViewType> views)
  : _views(views)
  { }

  CompositeView(const std::vector<ComponentViewType> & views)
  : _views(views)
  { }

private:
  std::vector<ComponentViewType> _views;
};


template <class ComponentViewType>
constexpr CompositeView<ComponentViewType>
set_union(
  const std::vector<ComponentViewType> & views) {
  return CompositeView<ComponentViewType>(views);
}

template <class ComponentViewType>
constexpr CompositeView<ComponentViewType>
set_union(
  std::initializer_list<ComponentViewType> views) {
  return CompositeView<ComponentViewType>(views);
}


} // namespace dash

#endif // DASH__VIEW__UNION_H__INCLUDED
