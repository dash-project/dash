#ifndef DASH__PATTERN__SCOPED_PATTERN_H__INCLUDED
#define DASH__PATTERN__SCOPED_PATTERN_H__INCLUDED

namespace dash {

template <
  class GlobalScopePattern,
  class BlockScopePattern >
class ScopedPattern : public GlobalScopePattern {
public:
  static constexpr char const * PatternName = "ScopedPattern";

public:
  /// Default properties in partitioning category to GlobalScopePattern
  /// properties:
  typedef typename GlobalScopePattern::partitioning_properties
    partitioning_properties;
  /// Default properties in mapping category to GlobalScopePattern
  /// properties:
  typedef typename GlobalScopePattern::mapping_properties
    mapping_properties;
  /// Default properties in layout category to BlockScopePattern
  /// properties:
  typedef typename GlobalScopePattern::layout_properties
    layout_properties;

  typedef typename GlobalScopePattern::index_type     index_type;

private:
  const GlobalScopePattern & derived() const {
    *static_cast<const GlobalScopePattern *>(this);
  }
  GlobalScopePattern & derived() {
    *static_cast<GlobalScopePattern *>(this);
  }

public:

  ScopedPattern()
  { }

};

} // namespace dash

#endif // DASH__PATTERN__SCOPED_PATTERN_H__INCLUDED
