#ifndef DASH__SHARED_H_
#define DASH__SHARED_H_

#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Allocator.h>

namespace dash {

/**
 * Shared access to a value in global memory across a team.
 * TODO: No support for atomic operations (like increment) so far.
 *
 * \tparam  ElementType  The type of the shared value.
 */
template<typename ElementType>
class Shared
{
public:
  typedef ElementType                                value_type;
  typedef size_t                                      size_type;
  typedef size_t                                difference_type;

  typedef       GlobRef<value_type>                   reference;
  typedef const GlobRef<value_type>             const_reference;

  typedef       GlobPtr<value_type>                     pointer;
  typedef const GlobPtr<value_type>               const_pointer;

private:
  typedef dash::GlobMem<
            value_type,
            dash::allocator::LocalAllocator<value_type> >
    GlobMem_t;

public:
  /**
   * Constructor.
   */
  Shared(
    /// Team containing all units accessing the element in shared memory
    Team & team = dash::Team::All())
  : m_team(team)
  {
    if (m_team.myid() == 0) {
      m_globmem = new GlobMem_t(1);
      m_ptr     = m_globmem->begin();
    }
    dart_bcast(
      &m_ptr,
      sizeof(pointer),
	    0,
      m_team.dart_id());
  }

  /**
   * Destructor, frees shared memory.
   */
  ~Shared()
  {
    if (m_team.myid() == 0) {
      delete m_globmem;
    }
  }

  /**
   * Set the value of the shared element.
   */
  void set(ElementType val) noexcept
  {
    *m_ptr = val;
  }

  /**
   * Get the value of the shared element.
   */
  reference get() noexcept
  {
    return *m_ptr;
  }

private:
  GlobMem_t  *  m_globmem = nullptr;
  dash::Team &  m_team;
  pointer       m_ptr;

private:
  /// Prevent copy-construction.
  Shared(const Shared & other) = delete;

};

} // namespace dash

#endif // DASH__SHARED_H_
