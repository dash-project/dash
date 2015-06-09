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
#include <dash/Dimensional.h>

namespace dash {

/**
 * Defines how a list of global indices is mapped to single units
 * within a Team.
 */
template<
  size_t NumDimensions,
  MemArrange Arrangement = ROW_MAJOR>
class Pattern {
private:
  DistributionSpec<NumDimensions>        m_distspec;
  TeamSpec<NumDimensions, Arrangement>   m_teamspec;
  AccessBase<NumDimensions, Arrangement> m_accessbase;
  SizeSpec<NumDimensions, Arrangement>   m_sizespec;

public:
  ViewSpec<NumDimensions>                m_viewspec;

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
    static_assert(
      sizeof...(Args) >= NumDimensions,
      "Invalid number of constructor arguments.");

    check<0>(std::forward<Args>(args)...);
    m_nunits = m_team.size();

    int argc = sizeof...(Args);

    // Speficy default patterns for all dims
    // BLOCKED for the 1st, NONE for the rest
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
    const SizeSpec<NumDimensions, Arrangement> & sizespec,
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
    const SizeSpec<NumDimensions, Arrangement> & sizespec,
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
            / divCeil(m_sizespec.m_extent[i] , m_teamspec.size());
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
                                  divCeil(m_sizespec.m_extent[i],
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
        res *= divCeil(m_sizespec.m_extent[i], dimunit);
        break;

      case DistEnum::disttype::CYCLIC:
        res *= divCeil(m_sizespec.m_extent[i], dimunit);
        break;

      case DistEnum::disttype::BLOCKCYCLIC:
        res *= m_distspec.m_extent[i].blocksz
          * divCeil(m_sizespec.m_extent[i], cycle);
        break;
      case DistEnum::disttype::NONE:
        res *= m_sizespec.m_extent[i];
        break;
      }
    }
    assert(res > 0);
    return res;
  }

  /**
   * Convert given coordinate in pattern to its assigned unit id.
   * TODO: Will be renamed to \c coord_to_unit.
   */
  long long index_to_unit(std::array<long long, NumDimensions> input) const {
    return atunit_(input, m_viewspec);
  }

  /**
   * Convert given coordinate in pattern to its linear local index.
   * TODO: Will be renamed to \c coord_to_local_index.
   */
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
    ViewSpec<NumDimensions> & vs) const {
    if (input.size() != NumDimensions) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Invalid number of arguments for Pattern::glob_at_: "
        << "Epected " << NumDimensions << ", got " << input.size());
    }
    std::array<long long, NumDimensions> index;
    for (size_t i = 0; i < NumDimensions; i++) {
      index[i] = vs.begin[i] + input[i];
    }
    return m_sizespec.at(index);
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
    if (input.size() != NumDimensions) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Invalid number of arguments for Pattern::local_at_: "
        << "Epected " << NumDimensions << ", got " << input.size());
    }
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
    if (input.size() != NumDimensions) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Invalid number of arguments for Pattern::at_: "
        << "Epected " << NumDimensions << ", got " << input.size());
    }
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
      blocksz[i] = divCeil(m_sizespec.m_extent[i], dimunit);
      accessbase_coord[i] = (long long)(index[i] % blocksz[i]);
      cyclicfix[i] = 0;

      assert(index[i] >= 0);
      switch (m_distspec.m_extent[i].type) {
      case DistEnum::disttype::BLOCKED:
        accessbase_coord[i] = index[i] % blocksz[i];
        if (i > 0) {
          if (m_sizespec.m_extent[i] % dimunit != 0
            && divCeil(index[i] + 1, blocksz[i]) == dimunit)
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
            < divFloor(m_sizespec.m_extent[i] % cycle,
            m_distspec.m_extent[i].blocksz))
            cyclicfix[i - 1] = m_distspec.m_extent[i].blocksz;
          else if ((index[i] / m_distspec.m_extent[i].blocksz) % dimunit
            < divCeil(m_sizespec.m_extent[i] % cycle,
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
            < divFloor(m_sizespec.m_extent[i] % cycle,
            m_distspec.m_extent[i].blocksz))
            cyclicfix[i - 1] = m_distspec.m_extent[i].blocksz;
          else if ((index[i] / m_distspec.m_extent[i].blocksz) % dimunit
            < divCeil(m_sizespec.m_extent[i] % cycle,
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
   * The maximum number of elements arranged in this pattern.
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

  SizeSpec<NumDimensions, Arrangement> sizespec() const {
    return m_sizespec;
  }

  TeamSpec<NumDimensions> teamspec() const {
    return m_teamspec;
  }

  /**
   * Number of elements in the block in given dimension.
   *
   * \param  dim  The dimension in the pattern
   * \return  The blocksize in the given dimension
   */
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
      if ((idx >= divCeil(m_sizespec.m_extent[dim], dimunit)*(dim_offs)) &&
        (idx < divCeil(m_sizespec.m_extent[dim], dimunit)*(dim_offs + 1)))
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
  long long modulo(const long long i, const long long k) const {
    long long res = i % k;
    if (res < 0)
      res += k;
    return res;
  }

  long long divCeil(const long long i, const long long k) const {
    if (i % k == 0)
      return i / k;
    else
      return i / k + 1;
  }

  long long divFloor(const long long i, const long long k) const {
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
  void check(const SizeSpec<NumDimensions, Arrangement> & ds) {
    m_sizespec = ds;
    argc_extents += NumDimensions;
  }

  template<int count>
  void check(const DistributionSpec<NumDimensions> & ds) {
    argc_DistEnum = NumDimensions;
    m_distspec = ds;
  }

  template<int count>
  void check(const DistEnum & ds) {
    int dim = count - NumDimensions;
    m_distspec.m_extent[dim] = ds;
    argc_DistEnum++;
  }

  // Isolates first argument and calls the appropriate check() function.
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
  // local layout. It is calcualted via myid and results in unit-
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

#endif // DASH__PATTERN_H_
