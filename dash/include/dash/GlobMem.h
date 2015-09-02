/* 
 * dash-lib/GlobMem.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__GLOBMEM_H_
#define DASH__GLOBMEM_H_

#include <dash/dart/if/dart.h>
#include <dash/GlobPtr.h>
#include <dash/Team.h>

namespace dash {

namespace internal {

enum class GlobMemKind {
  COLLECTIVE,
  LOCAL
};

constexpr GlobMemKind COLLECTIVE { GlobMemKind::COLLECTIVE };
constexpr GlobMemKind COLL       { GlobMemKind::COLLECTIVE };
constexpr GlobMemKind LOCAL      { GlobMemKind::LOCAL };

} // namespace internal

/**
 * Block until completion of local and global operations on a global address.
 */
template<typename T>
void fence(
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_fence(gptr.dartptr()),
    DART_OK);
}

/**
 * Block until completion of local operations on a global address.
 */
template<typename T>
void fence_local(
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_fence_local(gptr.dartptr()),
    DART_OK);
}

/**
 * Write a value to a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T>
void put_value_nonblock(
  /// [IN]  Value to set
  const T & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_put(gptr.dartptr(),
             (void *)(&newval),
             sizeof(T)),
    DART_OK);
}

/**
 * Read a value fom a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T>
void get_value_nonblock(
  /// [OUT] Local pointer that will contain the value of the 
  //        global address
  T * ptr,
  /// [IN]  Global pointer to read
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_get(ptr,
             gptr.dartptr(),
             sizeof(T)),
    DART_OK);
}

/**
 * Write a value to a global pointer
 *
 * \blocking
 */
template<typename T>
void put_value(
  /// [IN]  Value to set
  const T & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_put_blocking(gptr.dartptr(),
                      (void *)(&newval),
                      sizeof(T)),
    DART_OK);
}

/**
 * Read a value fom a global pointer.
 *
 * \blocking
 */
template<typename T>
void get_value(
  /// [OUT] Local pointer that will contain the value of the 
  //        global address
  T * ptr,
  /// [IN]  Global pointer to read
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_get_blocking(ptr,
                      gptr.dartptr(),
                      sizeof(T)),
    DART_OK);
}

template<typename ElementType>
class GlobMem {
private:
  dart_gptr_t             m_begptr;
  dart_team_t             m_teamid;
  size_t                  m_nunits;
  size_t                  m_nlelem;
  internal::GlobMemKind   m_kind;
  ElementType           * m_lbegin;
  ElementType           * m_lend;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   */
  GlobMem(
    /// Team containing all units operating on global memory
    Team & team,
    /// Number of local elements to allocate
    size_t nlelem
  ) {
    DASH_LOG_TRACE("GlobMem(nunits,nelem)", team.size(), nlelem);
    m_begptr     = DART_GPTR_NULL;
    m_teamid     = team.dart_id();
    m_nlelem     = nlelem;
    m_kind       = dash::internal::COLLECTIVE;
    size_t lsize = sizeof(ElementType) * m_nlelem;
    DASH_LOG_TRACE_VAR("GlobMem(nunits, nelem)", lsize);
    DASH_LOG_TRACE_VAR("GlobMem(nunits, nelem)", m_teamid);
    DASH_ASSERT_RETURNS(
      dart_team_size(m_teamid, &m_nunits),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_team_memalloc_aligned(
        m_teamid, 
        lsize,
        &m_begptr),
      DART_OK);
    m_lbegin     = lbegin(dash::myid());
    m_lend       = lend(dash::myid());
  }

  /**
   * Constructor, allocates the given number of elements in local memory.
   */
  GlobMem(
      /// [IN] Number of local elements to allocate
      size_t nlelem
  ) {
    DASH_LOG_TRACE("GlobMem(nelem)", nlelem);
    m_begptr     = DART_GPTR_NULL;
    m_teamid     = DART_TEAM_NULL;
    m_nlelem     = nlelem;
    m_nunits     = 1;
    m_kind       = dash::internal::LOCAL;
    size_t lsize = sizeof(ElementType) * m_nlelem;
    DASH_ASSERT_RETURNS(
      dart_memalloc(lsize, &m_begptr),
      DART_OK);
    m_lbegin     = lbegin(dash::myid());
    m_lend       = lend(dash::myid());
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobMem() {
    DASH_LOG_TRACE_VAR("GlobMem.~GlobMem()", m_begptr);
    if (!DART_GPTR_ISNULL(m_begptr)) {
      if (m_kind == dash::internal::COLLECTIVE) {
        DASH_LOG_TRACE_VAR("GlobMem.~GlobMem()", m_teamid);
        DASH_ASSERT_RETURNS(
          dart_team_memfree(m_teamid, m_begptr),
          DART_OK);
      } else {
        DASH_ASSERT_RETURNS(
          dart_memfree(m_begptr),
          DART_OK);
      } 
    }
    DASH_LOG_TRACE("GlobMem.~GlobMem >");
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const GlobPtr<ElementType> begin() const {
    return GlobPtr<ElementType>(m_begptr);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  GlobPtr<ElementType> begin() {
    return GlobPtr<ElementType>(m_begptr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  const ElementType * lbegin(dart_unit_t unit_id) const {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobMem.lbegin const()", unit_id);
    dart_gptr_t gptr = begin().dartptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobMem.lbegin const >", addr);
    return static_cast<const ElementType *>(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  ElementType * lbegin(dart_unit_t unit_id) {
    void *addr;
    DASH_LOG_TRACE_VAR("GlobMem.lbegin()", unit_id);
    dart_gptr_t gptr = begin().dartptr();
    DASH_LOG_TRACE_VAR("GlobMem.lbegin", 
                       GlobPtr<ElementType>((dart_gptr_t)gptr));
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    DASH_LOG_TRACE_VAR("GlobMem.lbegin >", addr);
    return static_cast<ElementType *>(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  const ElementType * lbegin() const {
    return m_lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  ElementType * lbegin() {
    return m_lbegin;
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  const ElementType * lend(dart_unit_t unit_id) const {
    void *addr;
    dart_gptr_t gptr = begin().dartptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, m_nlelem * sizeof(ElementType)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    return static_cast<const ElementType *>(addr);
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  ElementType * lend(dart_unit_t unit_id) {
    void *addr;
    dart_gptr_t gptr = begin().dartptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, m_nlelem * sizeof(ElementType)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    return static_cast<ElementType *>(addr);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  const ElementType * lend() const {
    return m_lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobMem instance.
   */
  ElementType * lend() {
    return m_lend;
  }

  /*
   * Write value to global memory at given offset.
   *
   * \see  dash::put_value
   */
  template<typename ValueType = ElementType>
  void put_value(
    const ValueType & newval,
    size_t global_index) {
    DASH_LOG_TRACE("GlobMem.put_value(newval, gidx = %d)", global_index);
    dart_gptr_t gptr = m_begptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(
        &gptr,
        global_index * sizeof(ValueType)),
      DART_OK);
    dash::put_value(newval, GlobPtr<ValueType>(gptr));
  }

  /*
   * Read value from global memory at given offset.
   *
   * \see  dash::get_value
   */
  template<typename ValueType = ElementType>
  void get_value(
    ValueType * ptr,
    size_t global_index) {
    DASH_LOG_TRACE("GlobMem.get_value(newval, gidx = %d)", global_index);
    dart_gptr_t gptr = m_begptr;
    dart_gptr_incaddr(&gptr, global_index * sizeof(ValueType));
    dash::get_value(ptr, GlobPtr<ValueType>(gptr));
  }

  /**
   * Resolve the global pointer from an element position in a unit's
   * local memory.
   *
   * TODO: Clarify if dart-calls can be avoided if address is local.
   */
  template <typename IndexType>
  GlobPtr<ElementType> index_to_gptr(
    /// The unit id
    dart_unit_t unit,
    /// The unit's local address offset
    IndexType local_index) const {
    DASH_LOG_DEBUG("GlobMem.index_to_gptr(unit,l_idx)", unit, local_index);
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = m_begptr;
    // Resolve global unit id
    dart_unit_t lunit, gunit; 
    DASH_LOG_DEBUG("GlobMem.index_to_gptr (=g_begp)  ",
                   GlobPtr<ElementType>(gptr));
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", gptr.unitid);
    // Resolve local unit id from global unit id in global pointer:
    dart_team_unit_g2l(m_teamid, gptr.unitid, &lunit);
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", lunit);
    lunit = (lunit + unit) % m_nunits;
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", lunit);
    dart_team_unit_l2g(m_teamid, lunit, &gunit);
    DASH_LOG_TRACE_VAR("GlobMem.index_to_gptr", gunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, gunit);
    // Apply local offset to global pointer:
    dart_gptr_incaddr(&gptr, local_index * sizeof(ElementType));
    DASH_LOG_DEBUG("GlobMem.index_to_gptr (+g_unit) >",
                   GlobPtr<ElementType>(gptr));
    return GlobPtr<ElementType>(gptr);
  }
};

template<typename T>
GlobPtr<T> memalloc(size_t nelem) {
  dart_gptr_t gptr;
  size_t lsize = sizeof(T) * nelem;
  
  dart_memalloc(lsize, &gptr);
  return GlobPtr<T>(gptr);
}

} // namespace dash

#endif // DASH__GLOBMEM_H_
