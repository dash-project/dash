#ifndef DASH__VIEW__UNION_H__INCLUDED
#define DASH__VIEW__UNION_H__INCLUDED



namespace dash {


template <class ComponentViewType>
class CompositeView
{

public:

private:
  std::array<ComponentViewType &> _views;
};


template <class ComponentViewType>
constexpr CompositeView<ComponentViewType>
union(
  std::initializer_list<ComponentViewType &> views) {
  return CompositeView<ComponentViewType>(views);
}

} // namespace dash

#endif // DASH__VIEW__UNION_H__INCLUDED

} // namespace dash

#endif // DASH__VIEW__UNION_H__INCLUDED
