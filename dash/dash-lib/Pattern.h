#ifndef PATTERN_H_INCLUDED
#define PATTERN_H_INCLUDED

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>

#include "Cartesian.h"
#include "Team.h"

using std::cout;
using std::endl;

namespace dash {

	struct DistEnum {
		enum disttype {
			BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
			CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
			BLOCKCYCLIC,
			TILE,
			NONE
		}; // general blocked distribution

		disttype type;
		long long blocksz;
	};

	struct DistEnum BLOCKED { DistEnum::BLOCKED, -1 };
	//obsolete
	struct DistEnum CYCLIC_ { DistEnum::CYCLIC, -1 };
	struct DistEnum CYCLIC { DistEnum::BLOCKCYCLIC, 1 };

	struct DistEnum NONE { DistEnum::NONE, -1 };

	struct DistEnum TILE(int bs) {
		return{ DistEnum::TILE, bs };
	}

	struct DistEnum BLOCKCYCLIC(int bs) {
		return{ DistEnum::BLOCKCYCLIC, bs };
	}

	// Base class for dim-related specification, ie DistSpec etc.
	template<typename T, size_t ndim_>
	class DimBase {
	private:

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

	// Wrapper class of CartCoord
	template<size_t ndim_, MemArrange arr = ROW_MAJOR>
	class DimRangeBase : public CartCoord < ndim_, long long, arr > {

	protected:

	public:

		template<size_t ndim__, MemArrange arr2> friend class Pattern;

		DimRangeBase() {
		}

		// Need to be called manually when elements are changed to update size
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

		// Variadic constructor
		template<typename ... values>
		DimRangeBase(values ... Values) :
			CartCoord<ndim_, long long>::CartCoord(Values...) {
		}
	};

	// DistSpec describes distribution patterns of all dimensions.
	template<size_t ndim_>
	class DistSpec : public DimBase < DistEnum, ndim_ > {
	public:

		// Default distribution: BLOCKED, NONE, ...
		DistSpec() {
			for (size_t i = 1; i < ndim_; i++)
				this->m_extent[i] = NONE;
			this->m_extent[0] = BLOCKED;
		}

		template<typename T_, typename ... values>
		DistSpec(T_ value, values ... Values) :
			DimBase<DistEnum, ndim_>::DimBase(value, Values...) {
		}
	};

	// AccessBase represents the local laylout according to the specified pattern.
	// 
	// TODO: can be optimized
	template<size_t ndim_, MemArrange arr = ROW_MAJOR>
	class AccessBase : public DimRangeBase < ndim_, arr > {
	public:
		AccessBase() {
		}

		template<typename T, typename ... values>
		AccessBase(T value, values ... Values) :
			DimRangeBase<ndim_>::DimRangeBase(value, Values...) {
		}
	};

	// TeamSpec specifies the arrangement of team units on all dimentions
	// Size of TeamSpec implies the size of the team.
	// TODO: unit reoccurrence not supported
	
	template<size_t ndim_>
	class TeamSpec : public DimRangeBase < ndim_ > {
	public:
		TeamSpec() {
			for (size_t i = 0; i < ndim_; i++)
				this->m_extent[i] = 1;
			this->m_extent[ndim_ - 1] = this->m_size = dash::Team::All().size();
			this->construct();
			this->m_ndim = 1;
		}

		TeamSpec(dash::Team& t)
		{
			for (size_t i = 0; i < ndim_; i++)
				this->m_extent[i] = 1;
			this->m_extent[ndim_ - 1] = this->m_size = t.size();
			this->construct();
			this->m_ndim = 1;
		}

		template<typename T, typename ... values>
		TeamSpec(T value, values ... Values) :
			DimRangeBase<ndim_>::DimRangeBase(value, Values...) {
		}

		int ndim() const {
			return this->m_ndim;
		}
	};

	typedef TeamSpec<1> DefaultTeamSpec;
	DefaultTeamSpec DEFAULT1DTEAM(long long nunit) {
		return{ nunit };
	}

	// SizeSpec specifies the data sizes on all dimentions
	template<size_t ndim_, MemArrange arr = ROW_MAJOR>
	class SizeSpec : public DimRangeBase < ndim_ > {
	public:

		template<size_t ndim__> friend class ViewSpec;

		SizeSpec() {
		}

		SizeSpec(size_t nelem) {
			static_assert(ndim_ == 1, "Not enough parameters for extent");
			this->m_extent[0] = nelem;
			this->construct();
			this->m_ndim = 1;
		}

		template<typename T_, typename ... values>
		SizeSpec(T_ value, values ... Values) :
			DimRangeBase<ndim_>::DimRangeBase(value, Values...) {
		}
	};

	class ViewPair {
		long long begin;
		long long range;
	};

	// ViewSpec specifies view parameters for implementing submat, rows and cols
	template<size_t ndim_>
	class ViewSpec : public DimBase < ViewPair, ndim_ > {

	public:

		void update_size()
		{
			nelem = 1;

			for (size_t i = ndim_ - view_dim; i < ndim_; i++)
			{
				if(range[i]<=0)
				printf("i %d rangei %d\n", i, range[i]);
				assert(range[i]>0);
				nelem *= range[i];
			}
		}

	public:
		long long begin[ndim_];
		long long range[ndim_];
		size_t ndim = ndim_;
		size_t view_dim = ndim_;
		long long nelem = 0;

		ViewSpec() {
		}

		ViewSpec(SizeSpec<ndim_> sizespec) {
			//TODO
			memcpy(this->m_extent, sizespec.m_extent, sizeof(long long) * ndim_);
			memcpy(range, sizespec.m_extent, sizeof(long long) * ndim_);
			nelem = sizespec.size();
			for (size_t i = 0; i < ndim_; i++)
			{
				begin[i] = 0;
				range[i] = sizespec.m_extent[i];
			}
		}

		template<typename T_, typename ... values>
		ViewSpec(ViewPair value, values ... Values) :
			DimBase<ViewPair, ndim_>::DimBase(value, Values...) {
		}

		long long size()
		{
			return nelem;
		}
	};

	template<size_t ndim_, MemArrange arr = ROW_MAJOR>
	class Pattern {
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
		void check(const TeamSpec<ndim_> & ts) {
			m_teamspec = ts;
			argc_ts++;
		}

		template<int count>
		void check(dash::Team & t) {
			m_team = Team(t);
		}


		template<int count>
		void check(const SizeSpec<ndim_, arr> & ds) {
			m_sizespec = ds;
			argc_extents += ndim_;
		}

		template<int count>
		void check(const DistSpec<ndim_> & ds) {
			argc_DistEnum = ndim_;
			m_distspec = ds;
		}

		template<int count>
		void check(const DistEnum& ds) {
			int dim = count - ndim_;
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
			for (int i = 0; i < ndim_ - 1; i++) {
				if (m_distspec.m_extent[i].type == DistEnum::disttype::TILE)
					hastile = 1;
				if (m_distspec.m_extent[i].type != m_distspec.m_extent[i + 1].type)
					invalid = 1;
			}
			assert(!(hastile && invalid));
			if (hastile) {
				for (int i = 0; i < ndim_; i++) {
					assert(
						m_sizespec.m_extent[i] % (m_distspec.m_extent[i].blocksz)
						== 0);
				}
			}
		}

		void checkValidDistEnum() {
			int n_validdist = 0;
			for (int i = 0; i < ndim_; i++) {
				if (m_distspec.m_extent[i].type != DistEnum::disttype::NONE)
					n_validdist++;
			}
			assert(n_validdist == m_teamspec.ndim());
		}

		// AccessBase is an aggregation of local layout of a unit among the global unit.
		// It is initialized after pattern parameters are received based on the DistEnum and SizeSpec, and has to be construct() if extents are changed.
		// AccessBase is currently identical at all units, difference is further fixed during at() and atunit().
		// TODO: m_lextent[] is a revised apprach to calculate unit-dependent local layout. It is calcualted via myid£¬ and results in unit-dependent AccessBase. If AccessBase.m_extent[] values are replaced with m_lextent[] values, then on-the-fly cyclicfix[] can be eliminated.
		void constructAccessBase() {
			m_blocksz = 1;

			for (size_t i = 0; i < ndim_; i++) {
				long long dimunit;
				size_t myidx;

				if (ndim_ > 1 && m_teamspec.ndim() == 1)
				{
					dimunit = m_teamspec.size();
					myidx = m_teamspec.index_at_dim(m_team.myid(), ndim_ - 1);
				}
				else
				{
					dimunit = m_teamspec.m_extent[i];
					myidx = m_teamspec.index_at_dim(m_team.myid(), i);
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
								m_lextent[i] = m_sizespec.m_extent[i] % ( m_sizespec.m_extent[i] / dimunit + 1 );
							else
								m_lextent[i] = m_sizespec.m_extent[i] / dimunit + 1;							
					else		
						m_lextent[i] = m_sizespec.m_extent[i] / dimunit;

					break;
				case DistEnum::disttype::BLOCKCYCLIC:
					if (m_sizespec.m_extent[i]
						/ cycle == 0)
						m_accessbase.m_extent[i] = m_distspec.m_extent[i].blocksz;
					else
						m_accessbase.m_extent[i] = m_sizespec.m_extent[i]
						/ cycle
						* m_distspec.m_extent[i].blocksz;
					m_blocksz *= m_distspec.m_extent[i].blocksz;

					if (m_sizespec.m_extent[i] % cycle != 0)
						m_lextent[i] = (m_sizespec.m_extent[i] / cycle) * m_distspec.m_extent[i].blocksz + ( myidx - (m_sizespec.m_extent[i] % cycle) / m_distspec.m_extent[i].blocksz ) < 0 ? m_distspec.m_extent[i].blocksz : (m_sizespec.m_extent[i] % cycle) % m_distspec.m_extent[i].blocksz;
					else
						m_lextent[i] = m_sizespec.m_extent[i] / dimunit;

					break;
				case DistEnum::disttype::CYCLIC:
					m_accessbase.m_extent[i] = m_sizespec.m_extent[i] / dimunit;
					m_blocksz *= 1;

					if (m_sizespec.m_extent[i] % dimunit != 0 && myidx > (m_sizespec.m_extent[i] % dimunit) - 1)
						m_lextent[i] = m_sizespec.m_extent[i] / dimunit;
					else
						m_lextent[i] = m_sizespec.m_extent[i] / dimunit + 1;

					break;
				case DistEnum::disttype::TILE:
					m_accessbase.m_extent[i] = m_distspec.m_extent[i].blocksz;
					m_blocksz *= m_distspec.m_extent[i].blocksz;

					if (m_sizespec.m_extent[i] % cycle != 0)
						m_lextent[i] = (m_sizespec.m_extent[i] / cycle) * m_distspec.m_extent[i].blocksz + ( myidx - (m_sizespec.m_extent[i] % cycle) / m_distspec.m_extent[i].blocksz ) < 0 ? m_distspec.m_extent[i].blocksz : (m_sizespec.m_extent[i] % cycle) % m_distspec.m_extent[i].blocksz;
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

			for (int i = 0; i < ndim_; i++)
				m_lnelem *= m_lextent[i];
		}

		DistSpec<ndim_> 	m_distspec;
		TeamSpec<ndim_> 	m_teamspec;
		AccessBase<ndim_, arr> 	m_accessbase;
		SizeSpec<ndim_, arr> 	m_sizespec;
	public:
		ViewSpec<ndim_> 		m_viewspec;
	private:
		long long    		m_local_begin[ndim_];
		long long			m_lextent[ndim_];
		long long			m_lnelem = 1;
		long long 			m_nunits = dash::Team::All().size();
		long long			m_blocksz;
		int 				argc_DistEnum = 0;
		int 				argc_extents = 0;
		int                 argc_ts = 0;
		dash::Team&  		m_team = dash::Team::All();

	public:

	public:

		template<typename ... Args>
		Pattern(Args&&... args) {
			static_assert(sizeof...(Args) >= ndim_,
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
			for (size_t i = argc_DistEnum; i < ndim_; i++) {
				m_distspec.m_extent[i] = NONE;
			}

			assert(argc_extents == ndim_);
			checkValidDistEnum();

			m_sizespec.construct();
			m_viewspec = ViewSpec<ndim_>(m_sizespec);
			checkTile();

			if (argc_ts == 0)
				m_teamspec = TeamSpec<ndim_>(m_team);

			constructAccessBase();
		}

		//TODO: merge Pattern constructors
		Pattern(const SizeSpec<ndim_, arr> &sizespec, const DistSpec<ndim_> &dist =
			DistSpec<ndim_>(), const TeamSpec<ndim_> &teamorg = TeamSpec<ndim_>::TeamSpec(), dash::Team& team = dash::Team::All()) :
			m_sizespec(sizespec), m_distspec(dist), m_teamspec(teamorg), m_team(team) {

			m_nunits = m_team.size();
			m_viewspec = ViewSpec<ndim_>(m_sizespec);
		    m_distspec = dist;

			checkValidDistEnum();

			checkTile();
			constructAccessBase();
		}

		Pattern(const SizeSpec<ndim_, arr> &sizespec, const DistSpec<ndim_> &dist =
			DistSpec<ndim_>(), dash::Team& team = dash::Team::All()) :
			m_sizespec(sizespec), m_distspec(dist), m_teamspec(m_team) {

			m_team = team;
			m_nunits = m_team.size();
			m_viewspec = ViewSpec<ndim_>(m_sizespec);

			checkValidDistEnum();

			checkTile();
			constructAccessBase();
		}

		template<typename ... values>
		long long atunit(values ... Values) const {
			assert(sizeof...(Values) == ndim_);
			std::array<long long, ndim_> inputindex = { Values... };
			return atunit_(inputindex, m_viewspec);
		}

		long long local_extent(size_t dim) const
		{
			assert(dim < ndim_ && dim >= 0);
			return m_lextent[dim];
		}

		long long lsize() const
		{
			return m_lnelem;
		}

		// Get input coordinates and view coordinates and return the belonging unit id of the point.
		long long atunit_(std::array<long long, ndim_> input, ViewSpec<ndim_> vs) const {
			assert(input.size() == ndim_);
			long long rs = -1;
			long long index[ndim_];
			std::array<long long, ndim_> accessbase_coord;

			if (m_teamspec.ndim() == 1) {
				rs = 0;
				for (size_t i = 0; i < ndim_; i++) {
					index[i] = vs.begin[i] + input[i];
					//if (i >= vs.view_dim)
					//	index[i] += input[i + ndim_ - vs.view_dim];

					long long cycle = m_teamspec.size() * m_distspec.m_extent[i].blocksz;
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
						//rs = 0;
						break;
					}
				}
//				rs = rs % m_teamspec.size();
			}
			else {
				for (size_t i = 0; i < ndim_; i++) {
					index[i] = vs.begin[i] + input[i];
					//if (ndim_ - i <= vs.view_dim)
					//	index[i] += input[i + ndim_ - vs.view_dim];

					long long cycle = m_teamspec.m_extent[i]
						* m_distspec.m_extent[i].blocksz;
					assert(index[i] >= 0);
					switch (m_distspec.m_extent[i].type) {
					case DistEnum::disttype::BLOCKED:
					
						accessbase_coord[i] = index[i] / getCeil(m_sizespec.m_extent[i] , m_teamspec.m_extent[i]);
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

		long long unit_and_elem_to_index(long long unit,
			long long elem)
		{
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

			for (int i = 0; i < ndim_; i++) {
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

		// Obsolete for N-D. Should pass N-D coordinates

/*		long long index_to_unit(long long idx) const {
			std::array<long long, ndim_> input = m_sizespec.coords(idx);
			return index_to_unit(input);
		}

		long long index_to_elem(long long idx) const {
			std::array<long long, ndim_> input = m_sizespec.coords(idx);
			return index_to_elem(input);
		}*/

		long long index_to_unit(std::array<long long, ndim_> input) const {
			return atunit_(input, m_viewspec);
		}

		long long index_to_elem(std::array<long long, ndim_> input) const {
			return at_(input, m_viewspec);
		}

		long long glob_index_to_elem(std::array<long long, ndim_> input, ViewSpec<ndim_> &vs) const {
			return glob_at_(input, vs);
		}

		long long glob_at_(std::array<long long, ndim_> input, ViewSpec<ndim_> &vs) const {
			assert(input.size() == ndim_);

			std::array<long long, ndim_> index;
			for (size_t i = 0; i < ndim_; i++) {
				index[i] = vs.begin[i] + input[i];
				//if (ndim_ - i <= vs.view_dim)
				//		index[i] += input[i + ndim_ - vs.view_dim];
			}

			return	m_sizespec.at(index);
		}		

		long long index_to_elem(std::array<long long, ndim_> input, ViewSpec<ndim_> &vs) const {
			return at_(input, vs);
		}

		template<typename ... values>
		long long at(values ... Values) const {
			static_assert(sizeof...(Values) == ndim_, "Wrong parameter number");
			std::array<long long, ndim_> inputindex = { Values... };
			return at_(inputindex, m_viewspec);
		}

		// Receive local coordicates and returns local offsets based on AccessBase.
		long long local_at_(std::array<long long, ndim_> input, ViewSpec<ndim_> &local_vs) const {

			assert(input.size() == ndim_);
			long long rs = -1;
			std::array<long long, ndim_> index;
			std::array<long long, ndim_> cyclicfix;

			for (size_t i = 0; i < ndim_; i++) {

				index[i] = local_vs.begin[i] + input[i];
				//if (ndim_ - i <= local_vs.view_dim)
				//		index[i] += input[i + ndim_ - local_vs.view_dim];
				cyclicfix[i] = 0;
			}
			rs = m_accessbase.at(index, cyclicfix);
		}

		// Receive global coordicates and returns local offsets.
		// TODO: cyclic can be eliminated when accessbase.m_extent[] has m_lextent[] values.
		long long at_(std::array<long long, ndim_> input, ViewSpec<ndim_> vs) const {
			assert(input.size() == ndim_);
			long long rs = -1;
			long long index[ndim_];
			std::array<long long, ndim_> accessbase_coord;
			std::array<long long, ndim_> cyclicfix;
			long long DTeamfix = 0;
			long long blocksz[ndim_];

			for (size_t i = 0; i < ndim_; i++) {
				long long dimunit;
				if (ndim_ > 1 && m_teamspec.ndim() == 1) {
					dimunit = m_teamspec.size();
				}
				else
					dimunit = m_teamspec.m_extent[i];

				index[i] = vs.begin[i] + input[i];
				//if (ndim_ - i <= vs.view_dim)
				//	index[i] += input[i + ndim_ - vs.view_dim];

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
				long long incre[ndim_];
				incre[ndim_ - 1] = m_accessbase.size();
				for (size_t i = 1; i < ndim_; i++) {
					long long dim = ndim_ - i - 1;
					long long cycle = m_teamspec.m_extent[dim]
						* m_distspec.m_extent[dim].blocksz;
					long long ntile = m_sizespec.m_extent[i] / cycle
						+ cyclicfix[dim] / m_accessbase.m_extent[i];
					incre[dim] = incre[dim + 1] * ntile;
				}
				for (size_t i = 0; i < ndim_; i++) {
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

		Pattern& operator=(const Pattern& other) {
			if (this != &other) {
				m_distspec = other.m_distspec;
				m_teamspec = other.m_teamspec;
				m_accessbase = other.m_accessbase;
				m_sizespec = other.m_sizespec;
				m_viewspec = other.m_viewspec;
				m_nunits = other.m_nunits;
				m_blocksz = other.m_blocksz;
				argc_DistEnum = other.argc_DistEnum;
				argc_extents = other.argc_extents;

			}
			return *this;
		}

		long long nelem() const {
			return m_sizespec.size();
		}

		dash::Team& team() const {
			return m_team;
		}

		DistSpec<ndim_> distspec() const {
			return m_distspec;
		}

		SizeSpec<ndim_, arr> sizespec() const {
			return m_sizespec;
		}

		TeamSpec<ndim_> teamspec() const {
			return m_teamspec;
		}

		void forall(std::function<void(long long)> func)
		{
			for (long long i = 0; i < m_sizespec.size(); i++) {
				long long idx = unit_and_elem_to_index(m_team.myid(), i);
				if (idx < 0) {
					break;
				}
				func(idx);
			}
		}

		// Returns whether the given dim offset involves any local part
		bool is_local(size_t idx, size_t myid, size_t dim, ViewSpec<ndim_> &vs)
		{
			long long dimunit;
			size_t dim_offs;
			bool ret = false;
			long long cycle = m_teamspec.size() * m_distspec.m_extent[dim].blocksz;


			if (ndim_ > 1 && m_teamspec.ndim() == 1)
			{
				dimunit = m_teamspec.size();
				dim_offs = m_teamspec.index_at_dim(myid, ndim_ - 1);
			}
			else
			{
				dimunit = m_teamspec.m_extent[dim];
				dim_offs = m_teamspec.index_at_dim(myid, dim);
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
	};
}

#endif /* PATTERN_H_INCLUDED */
