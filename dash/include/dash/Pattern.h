#ifndef DASH__PATTERN_H_
#define DASH__PATTERN_H_

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>

#include <dash/Enums.h>
#include <dash/Cartesian.h>
#include <dash/Team.h>

namespace dash {

#ifdef DOXYGEN
  
/**
 * Defines how a list of global indices is mapped to single units within
 * a Team.
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class Pattern {
public:
  template<typename ... Args>
  Pattern(Args && ... args);

  Pattern(
    const SizeSpec<NumDimensions, arr> & sizespec,
    const DistributionSpec<NumDimensions> & dist =
      DistributionSpec<NumDimensions>(), 
    const TeamSpec<NumDimensions> & teamorg =
      TeamSpec<NumDimensions>::TeamSpec(),
    dash::Team & team =
      dash::Team::All());

  Pattern(
    const SizeSpec<NumDimensions, arr> & sizespec,
    const DistributionSpec<NumDimensions> & dist =
      DistributionSpec<NumDimensions>(),
    dash::Team & team =
      dash::Team::All());

  template<typename ... values>
  long long atunit(values ... Values) const;

  long long local_extent(size_t dim) const;

  long long local_size() const;

  long long unit_and_elem_to_index(
    long long unit,
    long long elem);

  long long max_elem_per_unit() const;

  long long index_to_unit(
    std::array<long long, NumDimensions> input) const;

  long long index_to_elem(
    std::array<long long, NumDimensions> input) const;

  long long glob_index_to_elem(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> & vs) const;

  long long glob_at_(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> &vs) const;

  long long index_to_elem(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> &vs) const;

  template<typename ... values>
  long long at(values ... Values) const;

};

#else // ifdef DOXYGEN

/**
 * Base class for dimension-related attribute types, such as
 * DistributionSpec and TeamSpec.
 * Contains an extent value for every dimension.
 */
template<typename T, size_t ndim_>
class DimBase {
protected:
  size_t m_ndim = ndim_;
  T m_extent[ndim_];

public:
  template<size_t ndim__, MemArrange arr> friend class Pattern;

  DimBase() {
  }

  template<typename ... values>
  DimBase(values ... Values) :
    m_extent{ T(Values)... } {
    static_assert(sizeof...(Values) == ndim_,
      "Invalid number of arguments");
  }
};

/**
 * Base class for dimensional range attribute types, containing
 * offset and extent for every dimension.
 * Specialization of CartCoord.
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class DimRangeBase : public CartCoord<NumDimensions, long long> {
public:
  template<size_t NumDimensions_, MemArrange Arrange_> friend class Pattern;
  DimRangeBase() { }

  // Variadic constructor
  template<typename ... values>
  DimRangeBase(values ... Values) :
    CartCoord<NumDimensions, long long>::CartCoord(Values...) {
  }

protected:
  // Need to be called manually when elements are changed to
  // update size
  void construct() {
    long long cap = 1;
    this->m_offset[this->m_ndim - 1] = 1;
    for (size_t i = this->m_ndim - 1; i >= 1; i--) {
      assert(this->m_extent[i]>0);
      cap *= this->m_extent[i];
      this->m_offset[i - 1] = cap;
    }
    this->m_size = cap * this->m_extent[0];
  }
};

// DistributionSpec describes distribution patterns of all dimensions.
template<size_t NumDimensions>
class DistributionSpec : public DimBase<DistEnum, NumDimensions> {
public:

  // Default distribution: BLOCKED, NONE, ...
  DistributionSpec() {
    for (size_t i = 1; i < NumDimensions; i++) {
      this->m_extent[i] = NONE;
    }
    this->m_extent[0] = BLOCKED;
  }

  template<typename T_, typename ... values>
  DistributionSpec(T_ value, values ... Values)
  : DimBase<DistEnum, NumDimensions>::DimBase(value, Values...) {
  }
};

/** 
 * Represents the local laylout according to the specified pattern.
 * 
 * TODO: Can be optimized
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class AccessBase : public DimRangeBase<NumDimensions, Arrange> {
public:
  AccessBase() {
  }

  template<typename T, typename ... values>
  AccessBase(T value, values ... Values)
  : DimRangeBase<NumDimensions>::DimRangeBase(value, Values...) {
  }
};

/** 
 * TeamSpec specifies the arrangement of team units in all dimensions.
 * Size of TeamSpec implies the size of the team.
 * 
 * TODO: unit reoccurrence not supported
 *
 * \tparam  NumDimensions  Number of dimensions
 */
template<size_t NumDimensions>
class TeamSpec : public DimRangeBase<NumDimensions> {
public:
  TeamSpec() {
    // Set extent in all dimensions to 1 (minimum)
    for (size_t i = 0; i < NumDimensions; i++) {
      this->m_extent[i] = 1;
    }
    // Set extent in highest dimension to size of team
    this->m_size = dash::Team::All().size();
    this->m_extent[NumDimensions - 1] = this->m_size;
    this->construct();
    this->m_ndim = 1;
  }

  TeamSpec(dash::Team & t) {
    for (size_t i = 0; i < NumDimensions; i++)
      this->m_extent[i] = 1;
    this->m_extent[NumDimensions - 1] = this->m_size = t.size();
    this->construct();
    this->m_ndim = 1;
  }

  template<typename ExtentType, typename ... values>
  TeamSpec(ExtentType value, values ... Values)
  : DimRangeBase<NumDimensions>::DimRangeBase(value, Values...) {
  }

  int ndim() const {
    return this->m_ndim;
  }
};

/**
 * Specifies the data sizes on all dimensions
 */
template<size_t NumDimensions, MemArrange Arrange = ROW_MAJOR>
class SizeSpec : public DimRangeBase<NumDimensions> {
public:
  template<size_t NumDimensions_> friend class ViewSpec;

  SizeSpec() {
  }

  SizeSpec(size_t nelem) {
    static_assert(
      NumDimensions == 1,
      "Not enough parameters for extent");
    this->m_extent[0] = nelem;
    this->construct();
    this->m_ndim = 1;
  }

  template<typename T_, typename ... values>
  SizeSpec(T_ value, values ... Values) :
    DimRangeBase<NumDimensions>::DimRangeBase(value, Values...) {
  }
};

class ViewPair {
  long long begin;
  long long range;
};

/**
 * Specifies view parameters for implementing submat, rows and cols
 */
template<size_t NumDimensions>
class ViewSpec : public DimBase<ViewPair, NumDimensions> {
public:
  void update_size() {
    nelem = 1;

    for (size_t i = NumDimensions - view_dim; i < NumDimensions; i++)
    {
      if(range[i]<=0)
      printf("i %d rangei %d\n", i, range[i]);
      assert(range[i]>0);
      nelem *= range[i];
    }
  }

public:
  long long begin[NumDimensions];
  long long range[NumDimensions];
  size_t    ndim;
  size_t    view_dim;
  long long nelem;

  ViewSpec()
  : ndim(NumDimensions),
    view_dim(NumDimensions),
    nelem(0) {
  }

  ViewSpec(SizeSpec<NumDimensions> sizespec)
  : ndim(NumDimensions),
    view_dim(NumDimensions),
    nelem(0) {
    // TODO
    memcpy(this->m_extent,
           sizespec.m_extent,
           sizeof(long long) * NumDimensions);
    memcpy(range,
           sizespec.m_extent,
           sizeof(long long) * NumDimensions);
    nelem = sizespec.size();
    for (size_t i = 0; i < NumDimensions; i++) {
      begin[i] = 0;
      range[i] = sizespec.m_extent[i];
    }
  }

  template<typename T_, typename ... values>
  ViewSpec(ViewPair value, values ... Values)
  : DimBase<ViewPair, NumDimensions>::DimBase(value, Values...),
    ndim(NumDimensions),
    view_dim(NumDimensions),
    nelem(0) {
  }

  inline long long size() const {
    return nelem;
  }
};

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 */
template<size_t NumDimensions, MemArrange arr = ROW_MAJOR>
class Pattern {
private:
  DistributionSpec<NumDimensions> m_distspec;
  TeamSpec<NumDimensions>         m_teamspec;
  AccessBase<NumDimensions, arr>  m_accessbase;
  SizeSpec<NumDimensions, arr>    m_sizespec;

public:
  ViewSpec<NumDimensions>         m_viewspec;

private:
  long long      m_local_begin[NumDimensions];
  long long      m_lextent[NumDimensions];
  long long      m_local_capacity = 1;
  long long      m_nunits = dash::Team::All().size();
  long long      m_blocksz;
  int            argc_DistEnum = 0;
  int            argc_extents = 0;
  int            argc_ts = 0;
  dash::Team &   m_team = dash::Team::All();

public:
  template<typename ... Args>
  Pattern(Args && ... args) {
    static_assert(sizeof...(Args) >= NumDimensions,
      "Invalid number of constructor arguments.");

    check<0>(std::forward<Args>(args)...);
    m_nunits = m_team.size();

    int argc = sizeof...(Args);

    //Speficy default patterns for all dims
    //BLOCKED for the 1st, NONE for the rest
    if (argc_DistEnum == 0) {
      m_distspec.m_extent[0] = BLOCKED;
      argc_DistEnum = 1;
    }
    for (size_t i = argc_DistEnum; i < NumDimensions; i++) {
      m_distspec.m_extent[i] = NONE;
    }

    assert(argc_extents == NumDimensions);
    checkValidDistEnum();

    m_sizespec.construct();
    m_viewspec = ViewSpec<NumDimensions>(m_sizespec);
    checkTile();

    if (argc_ts == 0)
      m_teamspec = TeamSpec<NumDimensions>(m_team);

    constructAccessBase();
  }

  Pattern(
    const SizeSpec<NumDimensions, arr> & sizespec,
    const DistributionSpec<NumDimensions> & dist =
      DistributionSpec<NumDimensions>(), 
    const TeamSpec<NumDimensions> & teamorg =
      TeamSpec<NumDimensions>::TeamSpec(),
    dash::Team & team =
      dash::Team::All()) 
  : m_sizespec(sizespec),
    m_distspec(dist),
    m_teamspec(teamorg),
    m_team(team) {
    m_nunits   = m_team.size();
    m_viewspec = ViewSpec<NumDimensions>(m_sizespec);
    checkValidDistEnum();
    checkTile();
    constructAccessBase();
  }

  Pattern(
    const SizeSpec<NumDimensions, arr> & sizespec,
    const DistributionSpec<NumDimensions> & dist =
      DistributionSpec<NumDimensions>(),
    dash::Team & team =
      dash::Team::All())
  : m_sizespec(sizespec),
    m_distspec(dist),
    m_teamspec(m_team) {
    m_team     = team;
    m_nunits   = m_team.size();
    m_viewspec = ViewSpec<NumDimensions>(m_sizespec);
    checkValidDistEnum();
    checkTile();
    constructAccessBase();
  }

  template<typename ... values>
  long long atunit(values ... Values) const {
    assert(sizeof...(Values) == NumDimensions);
    std::array<long long, NumDimensions> inputindex = { Values... };
    return atunit_(inputindex, m_viewspec);
  }

  long long local_extent(size_t dim) const {
    assert(dim < NumDimensions && dim >= 0);
    return m_lextent[dim];
  }

  long long local_size() const {
    return m_local_capacity;
  }

  /**
   * Expects input coordinates and view coordinates and returns the
   * corresponding unit id of the point.
   */
  long long atunit_(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> vs) const {
    assert(input.size() == NumDimensions);
    long long rs = -1;
    long long index[NumDimensions];
    std::array<long long, NumDimensions> accessbase_coord;

    if (m_teamspec.ndim() == 1) {
      rs = 0;
      for (size_t i = 0; i < NumDimensions; i++) {
        index[i] = vs.begin[i] + input[i];
        //if (i >= vs.view_dim)
        //  index[i] += input[i + NumDimensions - vs.view_dim];
        long long cycle = m_teamspec.size() *
                          m_distspec.m_extent[i].blocksz;
        switch (m_distspec.m_extent[i].type) {
        case DistEnum::disttype::BLOCKED:
          rs = index[i]
            / getCeil(m_sizespec.m_extent[i] , m_teamspec.size());
          break;
        case DistEnum::disttype::CYCLIC:
          rs = modulo(index[i], m_teamspec.size());
          break;
        case DistEnum::disttype::BLOCKCYCLIC:
          rs = (index[i] % cycle) / m_distspec.m_extent[i].blocksz;
          break;
        case DistEnum::disttype::TILE:
          rs = (index[i] % cycle) / m_distspec.m_extent[i].blocksz;
          break;
        case DistEnum::disttype::NONE:
          break;
        }
      }
    }
    else {
      for (size_t i = 0; i < NumDimensions; i++) {
        index[i] = vs.begin[i] + input[i];

        long long cycle = m_teamspec.m_extent[i]
          * m_distspec.m_extent[i].blocksz;
        assert(index[i] >= 0);
        switch (m_distspec.m_extent[i].type) {
        case DistEnum::disttype::BLOCKED:
          accessbase_coord[i] = index[i] /
                                  getCeil(m_sizespec.m_extent[i],
                                          m_teamspec.m_extent[i]);
          break;
        case DistEnum::disttype::CYCLIC:
          accessbase_coord[i] = modulo(index[i], m_teamspec.m_extent[i]);
          break;
        case DistEnum::disttype::BLOCKCYCLIC:
          accessbase_coord[i] = (index[i] % cycle)
            / m_distspec.m_extent[i].blocksz;
          break;
        case DistEnum::disttype::TILE:
          accessbase_coord[i] = (index[i] % cycle)
            / m_distspec.m_extent[i].blocksz;
          break;
        case DistEnum::disttype::NONE:
          accessbase_coord[i] = -1;
          break;
        }
      }
      rs = m_teamspec.at(accessbase_coord);
    }
    return rs;
  }

  long long unit_and_elem_to_index(
    long long unit,
    long long elem) {
    long long blockoffs = elem / m_blocksz + 1;
    long long i = (blockoffs - 1) * m_blocksz * m_nunits +
      unit * m_blocksz + elem % m_blocksz;

    long long idx = modulo(i, m_sizespec.size());

    if (i < 0 || i >= m_sizespec.size()) // check if i in range
      return -1;
    else
      return idx;
  }

  long long max_elem_per_unit() const {
    long long res = 1;
    for (int i = 0; i < NumDimensions; i++) {
      long long dimunit;
      if (m_teamspec.ndim() == 1)
        dimunit = m_teamspec.size();
      else
        dimunit = m_teamspec.m_extent[i];

      long long cycle = dimunit * m_distspec.m_extent[i].blocksz;
      switch (m_distspec.m_extent[i].type) {
      case DistEnum::disttype::BLOCKED:
        res *= getCeil(m_sizespec.m_extent[i], dimunit);
        break;

      case DistEnum::disttype::CYCLIC:
        res *= getCeil(m_sizespec.m_extent[i], dimunit);
        break;

      case DistEnum::disttype::BLOCKCYCLIC:
        res *= m_distspec.m_extent[i].blocksz
          * getCeil(m_sizespec.m_extent[i], cycle);
        break;
      case DistEnum::disttype::NONE:
        res *= m_sizespec.m_extent[i];
        break;
      }
    }
    assert(res > 0);
    return res;
  }

  long long index_to_unit(std::array<long long, NumDimensions> input) const {
    return atunit_(input, m_viewspec);
  }

  long long index_to_elem(std::array<long long, NumDimensions> input) const {
    return at_(input, m_viewspec);
  }

  long long glob_index_to_elem(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> & vs) const {
    return glob_at_(input, vs);
  }

  long long glob_at_(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> &vs) const {
    assert(input.size() == NumDimensions);

    std::array<long long, NumDimensions> index;
    for (size_t i = 0; i < NumDimensions; i++) {
      index[i] = vs.begin[i] + input[i];
    }

    return  m_sizespec.at(index);
  }

  long long index_to_elem(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> &vs) const {
    return at_(input, vs);
  }

  template<typename ... values>
  long long at(values ... Values) const {
    static_assert(sizeof...(Values) == NumDimensions, "Wrong parameter number");
    std::array<long long, NumDimensions> inputindex = { Values... };
    return at_(inputindex, m_viewspec);
  }

  // Receive local coordicates and returns local offsets based on
  // AccessBase.
  long long local_at_(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> &local_vs) const {
    assert(input.size() == NumDimensions);
    long long rs = -1;
    std::array<long long, NumDimensions> index;
    std::array<long long, NumDimensions> cyclicfix;

    for (size_t i = 0; i < NumDimensions; i++) {
      index[i] = local_vs.begin[i] + input[i];
      cyclicfix[i] = 0;
    }
    rs = m_accessbase.at(index, cyclicfix);
  }

  // Receive global coordinates and returns local offsets.
  // TODO: cyclic can be eliminated when accessbase.m_extent[] has
  // m_lextent[] values.
  long long at_(
    std::array<long long, NumDimensions> input,
    ViewSpec<NumDimensions> vs) const {
    assert(input.size() == NumDimensions);
    long long rs = -1;
    long long index[NumDimensions];
    std::array<long long, NumDimensions> accessbase_coord;
    std::array<long long, NumDimensions> cyclicfix;
    long long DTeamfix = 0;
    long long blocksz[NumDimensions];

    for (size_t i = 0; i < NumDimensions; i++) {
      long long dimunit;
      if (NumDimensions > 1 && m_teamspec.ndim() == 1) {
        dimunit = m_teamspec.size();
      }
      else
        dimunit = m_teamspec.m_extent[i];

      index[i] = vs.begin[i] + input[i];

      long long cycle = dimunit * m_distspec.m_extent[i].blocksz;
      blocksz[i] = getCeil(m_sizespec.m_extent[i], dimunit);
      accessbase_coord[i] = (long long)(index[i] % blocksz[i]);
      cyclicfix[i] = 0;

      assert(index[i] >= 0);
      switch (m_distspec.m_extent[i].type) {
      case DistEnum::disttype::BLOCKED:
        accessbase_coord[i] = index[i] % blocksz[i];
        if (i > 0) {
          if (m_sizespec.m_extent[i] % dimunit != 0
            && getCeil(index[i] + 1, blocksz[i]) == dimunit)
            cyclicfix[i - 1] = -1;
        }
        break;
      case DistEnum::disttype::CYCLIC:
        accessbase_coord[i] = index[i] / dimunit;
        if (i > 0)
          cyclicfix[i - 1] =
          (index[i] % dimunit)
          < (m_sizespec.m_extent[i] % dimunit) ?
          1 : 0;
        break;
      case DistEnum::disttype::BLOCKCYCLIC:
        accessbase_coord[i] = (index[i] / cycle)
          * m_distspec.m_extent[i].blocksz
          + (index[i] % cycle) % m_distspec.m_extent[i].blocksz;
        if (i > 0) {
          if (m_sizespec.m_extent[i] < cycle)
            cyclicfix[i - 1] = 0;
          else if ((index[i] / m_distspec.m_extent[i].blocksz) % dimunit
            < getFloor(m_sizespec.m_extent[i] % cycle,
            m_distspec.m_extent[i].blocksz))
            cyclicfix[i - 1] = m_distspec.m_extent[i].blocksz;
          else if ((index[i] / m_distspec.m_extent[i].blocksz) % dimunit
            < getCeil(m_sizespec.m_extent[i] % cycle,
            m_distspec.m_extent[i].blocksz))
            cyclicfix[i - 1] = m_sizespec.m_extent[i]
            % m_distspec.m_extent[i].blocksz;
          else
            cyclicfix[i - 1] = 0;
        }
        break;
      case DistEnum::disttype::TILE:
        accessbase_coord[i] = (index[i] / cycle)
          * m_distspec.m_extent[i].blocksz
          + (index[i] % cycle) % m_distspec.m_extent[i].blocksz;
        if (i > 0) {
          if ((index[i] / m_distspec.m_extent[i].blocksz) % dimunit
            < getFloor(m_sizespec.m_extent[i] % cycle,
            m_distspec.m_extent[i].blocksz))
            cyclicfix[i - 1] = m_distspec.m_extent[i].blocksz;
          else if ((index[i] / m_distspec.m_extent[i].blocksz) % dimunit
            < getCeil(m_sizespec.m_extent[i] % cycle,
            m_distspec.m_extent[i].blocksz))
            cyclicfix[i - 1] = m_sizespec.m_extent[i] % cycle;
          else
            cyclicfix[i - 1] = 0;
        }
        break;
      case DistEnum::disttype::NONE:
        accessbase_coord[i] = index[i];
        break;
      }
    }

    if (m_distspec.m_extent[0].type == DistEnum::disttype::TILE) {
      rs = 0;
      long long incre[NumDimensions];
      incre[NumDimensions - 1] = m_accessbase.size();
      for (size_t i = 1; i < NumDimensions; i++) {
        long long dim = NumDimensions - i - 1;
        long long cycle = m_teamspec.m_extent[dim]
          * m_distspec.m_extent[dim].blocksz;
        long long ntile = m_sizespec.m_extent[i] / cycle
          + cyclicfix[dim] / m_accessbase.m_extent[i];
        incre[dim] = incre[dim + 1] * ntile;
      }
      for (size_t i = 0; i < NumDimensions; i++) {
        long long tile_index = (accessbase_coord[i])
          / m_accessbase.m_extent[i];
        long long tile_index_remain = (accessbase_coord[i])
          % m_accessbase.m_extent[i];
        rs += tile_index_remain * m_accessbase.m_offset[i]
          + tile_index * incre[i];
      }
      return rs;
    }
    else {
      rs = m_accessbase.at(accessbase_coord, cyclicfix);
    }

    return rs;
  }

  long long nunits() const {
    return m_nunits;
  }

  Pattern & operator=(const Pattern& other) {
    if (this != &other) {
      m_distspec    = other.m_distspec;
      m_teamspec    = other.m_teamspec;
      m_accessbase  = other.m_accessbase;
      m_sizespec    = other.m_sizespec;
      m_viewspec    = other.m_viewspec;
      m_nunits      = other.m_nunits;
      m_blocksz     = other.m_blocksz;
      argc_DistEnum = other.argc_DistEnum;
      argc_extents  = other.argc_extents;

    }
    return *this;
  }

  /**
   * The number of elements arranged in this pattern.
   */
  long long capacity() const {
    return m_sizespec.size();
  }

  /**
   * The Team containing the units this Pattern's index range is mapped to.
   */
  dash::Team & team() const {
    return m_team;
  }

  DistributionSpec<NumDimensions> distspec() const {
    return m_distspec;
  }

  SizeSpec<NumDimensions, arr> sizespec() const {
    return m_sizespec;
  }

  TeamSpec<NumDimensions> teamspec() const {
    return m_teamspec;
  }

  long long blocksize(size_t dim) const {
    return m_accessbase.m_extent[dim];
  }

  void forall(std::function<void(long long)> func) {
    for (long long i = 0; i < m_sizespec.size(); i++) {
      long long idx = unit_and_elem_to_index(m_team.myid(), i);
      if (idx < 0) {
        break;
      }
      func(idx);
    }
  }

  /**
   * Whether the given dim offset involves any local part
   */
  bool is_local(
    size_t idx,
    size_t myid,
    size_t dim,
    ViewSpec<NumDimensions> & vs) {
    long long dimunit;
    size_t dim_offs;
    bool ret = false;
    long long cycle = m_teamspec.size() * m_distspec.m_extent[dim].blocksz;

    if (NumDimensions > 1 && m_teamspec.ndim() == 1) {
      dimunit = m_teamspec.size();
      dim_offs = m_teamspec.coords(myid)[NumDimensions-1];
    } else {
      dimunit = m_teamspec.m_extent[dim];
      dim_offs = m_teamspec.coords(myid)[dim];
    }

    switch (m_distspec.m_extent[dim].type) {
    case DistEnum::disttype::BLOCKED:
      if ((idx >= getCeil(m_sizespec.m_extent[dim], dimunit)*(dim_offs)) &&
        (idx < getCeil(m_sizespec.m_extent[dim], dimunit)*(dim_offs + 1)))
        ret = true;
      break;
    case DistEnum::disttype::BLOCKCYCLIC:
      ret = ((idx % cycle) >= m_distspec.m_extent[dim].blocksz * (dim_offs)) &&
        ((idx % cycle) < m_distspec.m_extent[dim].blocksz * (dim_offs + 1));
      break;
    case DistEnum::disttype::CYCLIC:
      ret = idx % dimunit == dim_offs;
      break;
    case DistEnum::disttype::TILE:
      ret = ((idx % cycle) >= m_distspec.m_extent[dim].blocksz * (dim_offs)) &&
        ((idx % cycle) < m_distspec.m_extent[dim].blocksz * (dim_offs + 1));
      break;
    case DistEnum::disttype::NONE:
      ret = true;
      break;
    default:
      break;
    }
    return ret;
  }

private:
  inline long long modulo(const long long i, const long long k) const {
    long long res = i % k;
    if (res < 0)
      res += k;
    return res;
  }

  inline long long getCeil(const long long i, const long long k) const {
    if (i % k == 0)
      return i / k;
    else
      return i / k + 1;
  }

  inline long long getFloor(const long long i, const long long k) const {
    return i / k;
  }

  template<int count>
  void check(int extent) {
    check<count>((long long)(extent));
  }

  template<int count>
  void check(size_t extent) {
    check<count>((long long)(extent));
  }

  // the first DIM parameters must be used to
  // specify the extent
  template<int count>
  void check(long long extent) {
    m_sizespec.m_extent[count] = extent;
    argc_extents++;

  }

  // the next (up to DIM) parameters may be used to 
  // specify the distribution pattern
  // TODO: How many are required? 0/1/DIM ?
  template<int count>
  void check(const TeamSpec<NumDimensions> & ts) {
    m_teamspec = ts;
    argc_ts++;
  }

  template<int count>
  void check(dash::Team & t) {
    m_team = Team(t);
  }

  template<int count>
  void check(const SizeSpec<NumDimensions, arr> & ds) {
    m_sizespec = ds;
    argc_extents += NumDimensions;
  }

  template<int count>
  void check(const DistributionSpec<NumDimensions> & ds) {
    argc_DistEnum = NumDimensions;
    m_distspec = ds;
  }

  template<int count>
  void check(const DistEnum& ds) {
    int dim = count - NumDimensions;
    m_distspec.m_extent[dim] = ds;
    argc_DistEnum++;
  }

  // peel off one argument and call 
  // the appropriate check() function

  template<int count, typename T, typename ... Args>
  void check(T t, Args&&... args) {
    check<count>(t);
    check<count + 1>(std::forward<Args>(args)...);
  }

  // check pattern constraints for tile
  void checkTile() const {
    int hastile = 0;
    int invalid = 0;
    for (int i = 0; i < NumDimensions - 1; i++) {
      if (m_distspec.m_extent[i].type == DistEnum::disttype::TILE)
        hastile = 1;
      if (m_distspec.m_extent[i].type != m_distspec.m_extent[i + 1].type)
        invalid = 1;
    }
    assert(!(hastile && invalid));
    if (hastile) {
      for (int i = 0; i < NumDimensions; i++) {
        assert(
          m_sizespec.m_extent[i] % (m_distspec.m_extent[i].blocksz)
          == 0);
      }
    }
  }

  void checkValidDistEnum() {
    int n_validdist = 0;
    for (int i = 0; i < NumDimensions; i++) {
      if (m_distspec.m_extent[i].type != DistEnum::disttype::NONE)
        n_validdist++;
    }
    assert(n_validdist == m_teamspec.ndim());
  }

  // AccessBase is an aggregation of local layout of a unit among the
  // global unit.
  // It is initialized after pattern parameters are received based on 
  // the DistEnum and SizeSpec, and has to be construct() if extents 
  // are changed.
  // AccessBase is currently identical at all units, difference is
  // further fixed during at() and atunit().
  // TODO: m_lextent[] is a revised apprach to calculate 
  // unit-dependent
  // local layout. It is calcualted via myid£¬ and results in unit-
  // dependent AccessBase. If AccessBase.m_extent[] values are 
  // replaced
  // with m_lextent[] values, then on-the-fly cyclicfix[] can be
  // eliminated.
  void constructAccessBase() {
    m_blocksz = 1;

    for (size_t i = 0; i < NumDimensions; i++) {
      long long dimunit;
      size_t myidx;

      if (NumDimensions > 1 && m_teamspec.ndim() == 1) {
        dimunit = m_teamspec.size();
        myidx = m_teamspec.coords(m_team.myid())[NumDimensions-1];
      }
      else {
        dimunit = m_teamspec.m_extent[i];
        myidx = m_teamspec.coords(m_team.myid())[i];
      }

      long long cycle = dimunit * m_distspec.m_extent[i].blocksz;

      switch (m_distspec.m_extent[i].type) {
      case DistEnum::disttype::BLOCKED:
        m_accessbase.m_extent[i] =
          m_sizespec.m_extent[i] % dimunit == 0 ?
          m_sizespec.m_extent[i] / dimunit :
          m_sizespec.m_extent[i] / dimunit + 1;
        m_blocksz *= m_accessbase.m_extent[i];

        if (m_sizespec.m_extent[i] % dimunit != 0)
            if( myidx == dimunit - 1)
              m_lextent[i] = m_sizespec.m_extent[i] % (
                  m_sizespec.m_extent[i] / dimunit + 1);
            else
              m_lextent[i] = m_sizespec.m_extent[i] / dimunit + 1;
        else    
          m_lextent[i] = m_sizespec.m_extent[i] / dimunit;

        break;
      case DistEnum::disttype::BLOCKCYCLIC:
        if (m_sizespec.m_extent[i] / cycle == 0)
          m_accessbase.m_extent[i] = m_distspec.m_extent[i].blocksz;
        else
          m_accessbase.m_extent[i] = m_sizespec.m_extent[i]
          / cycle
          * m_distspec.m_extent[i].blocksz;
        m_blocksz *= m_distspec.m_extent[i].blocksz;

        if (m_sizespec.m_extent[i] % cycle != 0)
          m_lextent[i] =
            (m_sizespec.m_extent[i] / cycle) *
            m_distspec.m_extent[i].blocksz +
            (myidx - (m_sizespec.m_extent[i] % cycle) /
                      m_distspec.m_extent[i].blocksz ) < 0
              ? m_distspec.m_extent[i].blocksz
              : (m_sizespec.m_extent[i] % cycle) %
                m_distspec.m_extent[i].blocksz;
        else
          m_lextent[i] = m_sizespec.m_extent[i] / dimunit;

        break;
      case DistEnum::disttype::CYCLIC:
        m_accessbase.m_extent[i] = m_sizespec.m_extent[i] / dimunit;
        m_blocksz *= 1;

        if (m_sizespec.m_extent[i] % dimunit != 0 &&
            myidx > (m_sizespec.m_extent[i] % dimunit) - 1)
          m_lextent[i] = m_sizespec.m_extent[i] / dimunit;
        else
          m_lextent[i] = m_sizespec.m_extent[i] / dimunit + 1;

        break;
      case DistEnum::disttype::TILE:
        m_accessbase.m_extent[i] = m_distspec.m_extent[i].blocksz;
        m_blocksz *= m_distspec.m_extent[i].blocksz;

        if (m_sizespec.m_extent[i] % cycle != 0)
          m_lextent[i] = 
            (m_sizespec.m_extent[i] / cycle) *
            m_distspec.m_extent[i].blocksz + 
            (myidx - (m_sizespec.m_extent[i] % cycle) /
                      m_distspec.m_extent[i].blocksz) < 0
              ? m_distspec.m_extent[i].blocksz
              : (m_sizespec.m_extent[i] % cycle) %
                m_distspec.m_extent[i].blocksz;
        else
          m_lextent[i] = m_sizespec.m_extent[i] / dimunit;

        break;
      case DistEnum::disttype::NONE:
        m_accessbase.m_extent[i] = m_sizespec.m_extent[i];
        m_blocksz *= m_sizespec.m_extent[i];

        m_lextent[i] = m_sizespec.m_extent[i];

        break;
      default:
        break;
      }
    }
    m_accessbase.construct();

    for (int i = 0; i < NumDimensions; i++)
      m_local_capacity *= m_lextent[i];
  }
};

} // namespace dash

#endif // ifdef DOXYGEN

#endif // DASH__PATTERN_H_
