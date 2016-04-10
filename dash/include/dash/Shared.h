#ifndef DASH__SHARED_H_
#define DASH__SHARED_H_

#include <dash/dart/if/dart_types.h>

#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Allocator.h>

#include <memory>


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
private:
  typedef Shared<ElementType>
    self_t;

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
   * Constructor, allocates shared value at single unit in specified team.
   */
  Shared(
    /// Unit id of the shared value's owner.
    dart_unit_t   owner = 0,
    /// Team containing all units accessing the element in shared memory
    Team        & team  = dash::Team::All())
  : _team(&team),
    _owner(owner)
  {
    DASH_LOG_DEBUG_VAR("Shared.Shared(team,owner)()", owner);
    // Shared value is only allocated at unit 0:
    if (_team->myid() == _owner) {
      DASH_LOG_DEBUG("Shared.Shared(team,owner)",
                     "allocating shared value in local memory");
      _globmem = std::make_shared<GlobMem_t>(1);
      _ptr     = _globmem->begin();
    }
    // Broadcast global pointer of shared value at unit 0 to all units:
    dart_bcast(
      &_ptr,
      sizeof(pointer),
	    _owner,
      _team->dart_id());
    DASH_LOG_DEBUG_VAR("Shared.Shared(team,owner) >", _ptr);
  }

  /**
   * Copy-constructor.
   */
  Shared(const self_t & other)
  : _team(other._team),
    _owner(other._owner),
    _globmem(other._globmem),
    _ptr(other._ptr)
  {
    DASH_LOG_DEBUG("Shared.Shared(other) >");
  }

#if 0
  /**
   * Move-constructor. Transfers ownership from other instance.
   */
  Shared(const self_t && other)
  {
  }
#endif

  /**
   * Destructor, frees shared memory.
   */
  ~Shared()
  {
    DASH_LOG_DEBUG("Shared.~Shared()");
    if (_team->myid() != _owner) {
      _globmem = nullptr;
    }
    DASH_LOG_DEBUG("Shared.~Shared >");
  }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other)
  {
    DASH_LOG_DEBUG("Shared.=(other)()");
    _team    = other._team;
    _owner   = other._owner;
    _globmem = other._globmem;
    _ptr     = other._ptr;
    DASH_LOG_DEBUG("Shared.=(other) >");
    return *this;
  }

#if 0
  /**
   * Move-assignment operator.
   */
  self_t & operator=(const self_t && other)
  {
  }
#endif

  /**
   * Set the value of the shared element.
   */
  void set(ElementType val) noexcept
  {
    DASH_LOG_DEBUG_VAR("Shared.set()", val);
    *_ptr = val;
    DASH_LOG_DEBUG("Shared.set >");
  }

  /**
   * Get the value of the shared element.
   */
  reference get() noexcept
  {
    DASH_LOG_DEBUG("Shared.get()");
    reference ref = *_ptr;
    DASH_LOG_DEBUG_VAR("Shared.get >", static_cast<ElementType>(ref));
    return ref;
  }

private:
  dash::Team                   * _team    = nullptr;
  dart_unit_t                    _owner   = DART_UNDEFINED_UNIT_ID;
  std::shared_ptr<GlobMem_t>     _globmem = nullptr;
  pointer                        _ptr;

};

} // namespace dash

#endif // DASH__SHARED_H_
