#ifndef DASH__PATTERN_ITERATOR_H_
#define DASH__PATTERN_ITERATOR_H_

namespace dash {

/**
 *
 * \ingroup{DashPatternConcept}
 *
 * Usage:
 *
 * \code
 *   Pattern<2> pattern(...)
 *
 *   // Iterate global indices:
 *   for(auto global_index : pattern) {
 *     // ...
 *   }
 *   // Same as
 *   for(auto gi_it  = pattern.begin(),
 *            gi_end = pattern.end();
 *       gi_it != gi_end;
 *       ++gi_it) {
 *     Pattern<2>::index_type global_index = *gi_it;
 *     // ...
 *   }
 *
 *   // Iterate local indices:
 *   for(auto local_index : pattern.local) {
 *     // ...
 *   }
 *   // Same as
 *   for(auto li_it  = pattern.lbegin(),
 *            li_end = pattern.lend();
 *       li_it != li_end;
 *       ++li_it) {
 *     Pattern<2>::index_type local_index = *li_it;
 *     // ...
 *   }
 * \endcode
 */
template<
  class PatternType,
  typename IndexType = dash::default_index_t>
class PatternIterator {
private:
  typedef PatternIterator<PatternType, IndexType> self_t;

private:
  const PatternType & _pattern;
  IndexType           _global_pos;

public:
  class PatternLocalIterator {
  private:
    typedef PatternIterator<PatternType, IndexType> pattern_it_t;
    typedef PatternLocalIterator self_t;

  private:
    const pattern_it_t & _pattern_it;

  public:
    PatternLocalIterator(const pattern_it_t & pattern_it)
    : _pattern_it(pattern_it) {
    }
  };

  /**
   * Constructor
   */
  PatternIterator(
    /// The pattern to iterate
    const PatternType & pattern)
  : _pattern(pattern),
    _global_pos(0) {
  }

  /**
   * Copy constructor
   */
  PatternIterator(
    /// The pattern to iterate
    const self_t & other)
  : _pattern(other._pattern),
    _global_pos(other._global_pos) {
  }

  /**
   * Dereference operator, returns unit and local offset at
   * current iterator position in global cartesian index space.
   */
  typename PatternType::local_index_t operator*() {
    return _pattern.local(_global_pos);
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++() {
    ++_global_pos;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int) {
    self_t ret(*this);
    ++_global_pos;
    return ret;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--() {
    --_global_pos;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int) {
    self_t ret(*this);
    --_global_pos;
    return ret;
  }

private:
  /**
   * Forbid default-construction
   */
  PatternIterator() = delete;
};

} // namespace dash

#endif // DASH__PATTERN_ITERATOR_H_
