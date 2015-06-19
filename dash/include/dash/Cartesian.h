/* 
 * dash-lib/Cartesian.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__CARTESIAN_H_
#define DASH__CARTESIAN_H_

#include <array>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <type_traits>

#include <dash/Enums.h>
#include <dash/Dimensional.h>
#include <dash/Exception.h>

namespace dash {

/**
 * Defines a cartesian, totally-ordered index space by mapping linear
 * indices to cartesian coordinates depending on memory order.
 */
template<
  int NumDimensions,
  MemArrange Arrangement = ROW_MAJOR,
  typename IndexType     = long long >
class CartesianIndexSpace {
private:
  typedef typename std::make_unsigned<IndexType>::type SizeType;
  typedef 
    CartesianIndexSpace<NumDimensions, Arrangement, IndexType> 
    self_t;

protected:
  /// Number of elements in the cartesian space spanned by this instance.
  SizeType m_size;
  /// Number of dimensions of the cartesian space.
  SizeType m_ndim;
  /// Extents of the cartesian space by dimension.
  std::array<SizeType, NumDimensions> m_extent;
  /// Cumulative index offsets of the index space by dimension. Avoids
  /// recalculation of \c NumDimensions-1 offsets in every call of \at().
  std::array<SizeType, NumDimensions> m_offset;

public:
  /**
   * Default constructor, creates a cartesian index space of extent 0
   * in all dimensions.
   */
  CartesianIndexSpace()
  : m_size(0),
    m_ndim(NumDimensions) {
    for(auto i = 0; i < NumDimensions; i++) {
      m_extent[i] = 0;
      m_offset[i] = 0;
    }
  }

  /**
   * Constructor, creates a cartesian index space of given extents in
   * all dimensions.
   */
  CartesianIndexSpace(::std::array<SizeType, NumDimensions> extents)
  : m_size(0),
    m_ndim(NumDimensions) {
    resize(extents);
  }

  /**
   * Constructor, creates a cartesian index space of given extents in
   * all dimensions.
   */
  template<typename... Args>
  CartesianIndexSpace(SizeType arg, Args... args) 
  : m_size(0),
    m_ndim(NumDimensions) {
    resize(arg, args...);
  }

  /**
   * Constructor, initializes a new instance from dimensional size
   * specification.
   */
  CartesianIndexSpace(
    const SizeSpec<NumDimensions,
    SizeType> & sizeSpec)
  : m_size(sizeSpec.size()),
    m_ndim(NumDimensions) {
    resize(sizeSpec.extents());
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const {
    if (this == &other) {
      return true;
    }
    for(auto i = 0; i < NumDimensions; i++) {
      if (m_extent[i] != other.m_extent[i]) return false;
      if (m_offset[i] != other.m_offset[i]) return false;
    }
    return (
      m_size == other.m_size &&
      m_ndim == other.m_ndim
    );
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & other) const {
    return !(*this == other);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename... Args>
  void resize(SizeType arg, Args... args) {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    std::array<SizeType, NumDimensions> extents =
      { arg, (SizeType)args... };
    resize(extents);
  }

  /**
   * Change the extent of the cartesian space in every dimension.
   */
  template<typename SizeType_>
  void resize(std::array<SizeType_, NumDimensions> extents) {
    // Check number of dimensions in extents:
    static_assert(
      extents.size() == NumDimensions,
      "Invalid number of arguments");
    // Set extents:
    // ::std::copy(extents.begin(), extents.end(), m_extent);
    // Update size:
    m_size = 1;
    for(auto i = 0; i < NumDimensions; i++ ) {
      m_extent[i] = static_cast<SizeType>(extents[i]);
      m_size     *= m_extent[i];
    }
    if (m_size <= 0) {
      std::cout << std::endl;
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Extents for CartesianIndexSpace::resize must be greater than 0");
    }
    // Update offsets:
    if (Arrangement == COL_MAJOR) {
      m_offset[NumDimensions-1] = 1;
      for(auto i = NumDimensions-2; i >= 0; --i) {
        m_offset[i] = m_offset[i+1] * m_extent[i+1];
      }
    } else if (Arrangement == ROW_MAJOR) {
      m_offset[0] = 1;
      for(auto i = 1; i < NumDimensions; ++i) {
        m_offset[i] = m_offset[i-1] * m_extent[i-1];
      }
    }
  }
  
  /**
   * The number of dimension in the cartesian space with extent greater
   * than 1.
   *
   * \see num_dimensions()
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType rank() const {
    return NumDimensions;
  }
  
  /**
   * The number of dimension in the cartesian space.
   *
   * \see rank()
   *
   * \return The number of dimensions in the coordinate
   */
  SizeType num_dimensions() const {
    return NumDimensions;
  }
  
  /**
   * The number of discrete elements within the space spanned by the
   * coordinate.
   *
   * \return The number of discrete elements in the coordinate's space
   */
  SizeType size() const {
    return m_size;
  }

  /**
   * Extents of the cartesian space, by dimension.
   */
  const std::array<SizeType, NumDimensions> & extents() const {
    return m_extent;
  }
  
  /**
   * The coordinate's extent in the given dimension.
   *
   * \param  dim  The dimension in the coordinate
   * \return      The extent in the given dimension
   */
  SizeType extent(SizeType dim) const {
    if (dim >= NumDimensions) {
      // Dimension out of bounds:
      DASH_THROW(
        dash::exception::OutOfRange,
        "Given dimension " << dim <<
        " for CartesianIndexSpace::extent(dim) is out of bounds" <<
        " (" << NumDimensions << ")");
    }
    return m_extent[dim];
  }
  
  /**
   * Convert the given coordinates to a linear index.
   *
   * \param  args  An argument list consisting of the coordinates, ordered
   *               by dimension (x, y, z, ...)
   */
  template<typename... Args>
  IndexType at(IndexType arg, Args... args) const {
    static_assert(
      sizeof...(Args) == NumDimensions-1,
      "Invalid number of arguments");
    ::std::array<IndexType, NumDimensions> pos =
      { arg, IndexType(args) ... };
    return at(pos);
  }
  
  /**
   * Convert the given coordinates to a linear index.
   *
   * \param  pos  An array containing the coordinates, ordered by
   *              dimension (x, y, z, ...)
   */
  template<typename OffsetType>
  IndexType at(std::array<OffsetType, NumDimensions> pos) const {
    static_assert(
      pos.size() == NumDimensions,
      "Invalid number of arguments");
    SizeType offs = 0;
    for (int i = 0; i < NumDimensions; i++) {
      if (pos[i] >= m_extent[i]) {
        // Coordinate out of bounds:
        DASH_THROW(
          dash::exception::OutOfRange,
          "Given coordinate " << pos[i] <<
          " for CartesianIndexSpace::at() is out of bounds");
      }
      offs += m_offset[i] * pos[i];
    }
    return offs;
  }
  
  /**
   * Convert given linear offset (index) to cartesian coordinates.
   * Inverse of \c at(...).
   */
  std::array<IndexType, NumDimensions> coords(IndexType index) const {
    if (index >= m_size) {
      // Index out of bounds:
      DASH_THROW(
        dash::exception::OutOfRange,
        "Given index " << index <<
        " for CartesianIndexSpace::coords() is out of bounds");
    }
    ::std::array<IndexType, NumDimensions> pos;
    if (Arrangement == COL_MAJOR) {
      for(int i = 0; i < NumDimensions; ++i) {
        pos[i] = index / m_offset[i];
        index  = index % m_offset[i];
      }
    } else if (Arrangement == ROW_MAJOR) {
      for(int i = NumDimensions-1; i >= 0; --i) {
        pos[i] = index / m_offset[i];
        index  = index % m_offset[i];
      }
    }
    return pos;
  }
  
  /**
   * Accessor for dimension 1 (x), enabled for dimensionality > 0.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 0), SizeType >::type
  x(SizeType offs) const {
    return coords(offs)[0];
  }
  
  /**
   * Accessor for dimension 2 (y), enabled for dimensionality > 1.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 1), SizeType >::type
  y(SizeType offs) const {
    return coords(offs)[1];
  }
  
  /**
   * Accessor for dimension 3 (z), enabled for dimensionality > 2.
   */
  template<int U = NumDimensions>
  typename std::enable_if< (U > 2), SizeType >::type 
  z(SizeType offs) const {
    return coords(offs)[2];
  }
};

/** 
 * TeamSpec specifies the arrangement of team units in a specified number
 * of dimensions.
 * Size of TeamSpec implies the number of units in the team.
 * 
 * Reoccurring units are currently not supported.
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<
  size_t MaxDimensions,
  typename IndexType = long long>
class TeamSpec :
  public CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType> {
private:
  typedef typename std::make_unsigned<IndexType>::type SizeType;
public:
  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) with all team units organized linearly in the first
   * dimension.
   */
  TeamSpec(
    Team & team = dash::Team::All()) {
    _rank = 1;
    this->m_extent[0] = team.size();
    for (size_t d = 1; d < MaxDimensions; ++d) {
      this->m_extent[d] = 1;
    }
    this->resize(this->m_extent);
  }

  /**
   * Constructor, creates an instance of TeamSpec with given extents
   * from a team (set of units) and a distribution spec.
   * The number of elements in the distribution different from NONE
   * must be equal to the rank of the extents.
   *
   * This constructor adjusts extents according to given distribution
   * spec if the passed team spec has been default constructed.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<2> ts(
   *     // default-constructed, extents: [Team::All().size(), 1]
   *     TeamSpec<2>(),
   *     // distributed in dimension 1 (y)
   *     DistSpec<2>(NONE, BLOCKED),
   *     Team::All().split(2));
   *   // Will be adjusted to:
   *   size_t units_x = ts.extent(0); // -> 1
   *   size_t units_y = ts.extent(1); // -> Team::All().size() / 2
   * \endcode
   */
  TeamSpec(
    const TeamSpec<MaxDimensions> & other,
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All()) 
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>(
      other.extents()) {
    if (this->size() != team.size()) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Size of team " << team.size() << " differs from " <<
        "size of teamspec " << this->size() << " in TeamSpec()");
    }
    // Test if other teamspec has been default-constructed:
    if (other.rank() == 1 && distribution.rank() > 1) {
      // Set extent of teamspec in the dimension the distribution is
      // different from NONE:
      for (size_t d = 0; d < MaxDimensions; ++d) {
        if (distribution[d].type == dash::internal::DIST_NONE) {
          this->m_extent[d] = 1;
        } else {
          // Use size of given team; possibly different from size
          // of default-constructed team spec:
          this->m_extent[d] = team.size();
        }
      }
    } 
    for (size_t d = 0; d < MaxDimensions; ++d) {
      if (distribution[d].type != dash::internal::DIST_NONE) {
        _rank++;
      }
    }
    this->resize(this->m_extent);
  }

  /**
   * Constructor, creates an instance of TeamSpec from a team (set of
   * units) and a distribution spec.
   * All but one element in the distribution spec must be \c NONE.
   */
  TeamSpec(
    const DistributionSpec<MaxDimensions> & distribution,
    Team & team = dash::Team::All()) {
    _rank = 1;
    bool distrib_dim_set = false;
    for (size_t d = 0; d < MaxDimensions; ++d) {
      if (distribution[d].type == dash::internal::DIST_NONE) {
        this->m_extent[d] = 1;
      } else {
        this->m_extent[d] = team.size();
        if (distrib_dim_set) {
          DASH_THROW(
            dash::exception::InvalidArgument,
            "TeamSpec(DistributionSpec, Team) only allows "
            "one distributed dimension");
        }
        distrib_dim_set = true;
      }
    }
    this->resize(this->m_extent);
  }

  /**
   * Constructor, initializes new instance of TeamSpec with
   * extents specified in argument list.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts(1,2,3); // extents 1x2x3
   * \endcode
   */
  template<typename ... Types>
  TeamSpec(size_t value, Types ... values)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(value, values...) {
  }

  /**
   * Copy constructor.
   */
  TeamSpec(
    /// Teamspec instance to copy
    const TeamSpec<MaxDimensions> & other)
  : CartesianIndexSpace<MaxDimensions, ROW_MAJOR, IndexType>::
      CartesianIndexSpace(other.extents()),
    _rank(other._rank) {
  }

  /**
   * The number of units (extent) available in the given dimension.
   *
   * \param    dimension  The dimension
   * \returns  The number of units in the given dimension
   */
  long long num_units(size_t dimension) const {
    return this->size();
  }

  /**
   * The actual number of dimensions with extent greater than 1 in
   * this team arragement, that is the dimension of the vector space
   * spanned by the team arrangement's extents.
   *
   * \b Example:
   *
   * \code
   *   TeamSpec<3> ts(1,2,3); // extents 1x2x3
   *   ts.rank(); // returns 2, as one dimension has extent 1
   * \endcode
   */
  size_t rank() const {
    return _rank;
  }

private:
  /// Actual number of dimensions of the team layout specification.
  size_t _rank;
};

} // namespace dash

#endif // DASH__CARTESIAN_H_
