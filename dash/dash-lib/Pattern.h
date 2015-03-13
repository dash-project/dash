#ifndef PATTERN_H_INCLUDED
#define PATTERN_H_INCLUDED

#include <assert.h>
#include <functional>
#include <cstring>
#include <iostream>
#include <array>

#include "Cart.h"
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

	template<typename T, size_t ndim_>
	class DimBase {
	private:

	protected:
		size_t m_ndim = ndim_;
		T m_extent[ndim_];

	public:

		template<size_t ndim__> friend class Pattern;

		DimBase() {
		}

		template<typename ... values>
		DimBase(values ... Values) :
			m_extent{ T(Values)... } {
			static_assert(sizeof...(Values) == ndim_,
				"Invalid number of arguments");
		}
	};

	template<size_t ndim_>
	class DimRangeBase : public CartCoord < ndim_, long long > {

	protected:

	public:

		template<size_t ndim__> friend class Pattern;

		DimRangeBase() {
		}

		void construct() {
			long long cap = 1;
			this->m_offset[this->m_ndim - 1] = 1;
			for (size_t i = this->m_ndim - 1; i >= 1; i--) {
				cap *= this->m_extent[i];
				this->m_offset[i - 1] = cap;
			}
			this->m_size = cap * this->m_extent[0];
		}

		template<typename ... values>
		DimRangeBase(values ... Values) :
			CartCoord<ndim_, long long>::CartCoord(Values...) {
		}
	};

	template<size_t ndim_>
	class DistSpec : public DimBase < DistEnum, ndim_ > {
	public:

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

	template<size_t ndim_>
	class AccessUnit : public DimRangeBase < ndim_ > {
	public:
		AccessUnit() {
		}

		template<typename T, typename ... values>
		AccessUnit(T value, values ... Values) :
			DimRangeBase<ndim_>::DimRangeBase(value, Values...) {
		}
	};

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

	template<size_t ndim_>
	class SizeSpec : public DimRangeBase < ndim_ > {
	public:

		template<size_t ndim__> friend class ViewOrg;

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

	template<size_t ndim_>
	class ViewOrg : public DimBase < ViewPair, ndim_ > {
	public:
		long long begin[ndim_];
		long long range[ndim_];
		size_t ndim = ndim_;
		long long nelem = 0;

		ViewOrg() {
		}

		ViewOrg(SizeSpec<ndim_> m_extentorg) {
			memcpy(this->m_extent, m_extentorg.m_extent, sizeof(long long) * ndim_);
			memcpy(range, m_extentorg.m_extent, sizeof(long long) * ndim_);
			nelem = m_extentorg.size();
			for (size_t i = 0; i < ndim_; i++)
				begin[i] = 0;
		}

		template<typename T_, typename ... values>
		ViewOrg(ViewPair value, values ... Values) :
			DimBase<ViewPair, ndim_>::DimBase(value, Values...) {
		}
	};

	template<size_t ndim_>
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
			//static_assert(count < ndim_, "Too many parameters for extent");
			m_sizespec.m_extent[count] = extent;
			argc_extents++;
			cout << "I got " << extent << " for extent in dimension " << count
				<< endl;
		}
		;

		// the next (up to DIM) parameters may be used to 
		// specify the distribution pattern
		// TODO: How many are required? 0/1/DIM ?
		template<int count>
		void check(const TeamSpec<ndim_> & ts) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			//constexpr int dim = count - ndim_;
			//m_dist.m_extent[dim] = ds;
			//cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
			m_teamorg = ts;
			argc_ts++;
		}

		template<int count>
		void check(dash::Team & t) {
			m_team = Team(t);
		}


		template<int count>
		void check(const SizeSpec<ndim_> & ds) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			//constexpr int dim = count - ndim_;
			//m_dist.m_extent[dim] = ds;
			//cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
			m_sizespec = ds;
			argc_extents += ndim_;
		}

		template<int count>
		void check(const DistSpec<ndim_> & ds) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			//constexpr int dim = count - ndim_;
			//m_dist.m_extent[dim] = ds;
			//cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
			m_dist = ds;
		}

		// the next (up to DIM) parameters may be used to 
		// specify the distribution pattern
		// TODO: How many are required? 0/1/DIM ?
		template<int count>
		void check(const DistEnum& ds) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			int dim = count - ndim_;
			m_dist.m_extent[dim] = ds;
			argc_DistEnum++;
			//	cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
		}

		//	template<int count>
		//	void check(Team& team) {
		//		static_assert(count >= DIM, "Not enough parameters for extent");
		//
		//		cout << "I got the following team: " << team << endl;
		//	}

		// peel off one argument and call 
		// the appropriate check() function

		template<int count, typename T, typename ... Args>
		void check(T t, Args&&... args) {
			check<count>(t);
			check<count + 1>(std::forward<Args>(args)...);
		}

		// check tile pattern constraints
		void checkTile() const {
			int hastile = 0;
			int invalid = 0;
			for (int i = 0; i < ndim_ - 1; i++) {
				if (m_dist.m_extent[i].type == DistEnum::disttype::TILE)
					hastile = 1;
				if (m_dist.m_extent[i].type != m_dist.m_extent[i + 1].type)
					invalid = 1;
			}
			assert(!(hastile && invalid));
			if (hastile) {
				for (int i = 0; i < ndim_; i++) {
					assert(
						m_sizespec.m_extent[i] % (m_dist.m_extent[i].blocksz)
						== 0);
				}
			}
		}

		void checkValidDistEnum() {
			int n_validdist = 0;
			for (int i = 0; i < ndim_; i++) {
				if (m_dist.m_extent[i].type != DistEnum::disttype::NONE)
					n_validdist++;
			}
			assert(n_validdist == m_teamorg.ndim());
		}

		void constructAccessBase() {
			m_blocksz = 1;

			for (size_t i = 0; i < ndim_; i++) {
				long long dimunit;

				if (ndim_ > 1 && m_teamorg.ndim() == 1)
					dimunit = m_teamorg.size();
				else
					dimunit = m_teamorg.m_extent[i];

				switch (m_dist.m_extent[i].type) {
				case DistEnum::disttype::BLOCKED:
					m_accessunit.m_extent[i] =
						m_sizespec.m_extent[i] % dimunit == 0 ?
						m_sizespec.m_extent[i] / dimunit :
						m_sizespec.m_extent[i] / dimunit + 1;
					m_blocksz *= m_accessunit.m_extent[i];
					break;
				case DistEnum::disttype::BLOCKCYCLIC:
					if (m_sizespec.m_extent[i]
						/ (dimunit * m_dist.m_extent[i].blocksz) == 0)
						m_accessunit.m_extent[i] = m_dist.m_extent[i].blocksz;
					else
						m_accessunit.m_extent[i] = m_sizespec.m_extent[i]
						/ (dimunit * m_dist.m_extent[i].blocksz)
						* m_dist.m_extent[i].blocksz;
					m_blocksz *= m_dist.m_extent[i].blocksz;
					break;
				case DistEnum::disttype::CYCLIC:
					m_accessunit.m_extent[i] = m_sizespec.m_extent[i] / dimunit;
					m_blocksz *= 1;
					break;
				case DistEnum::disttype::TILE:
					m_accessunit.m_extent[i] = m_dist.m_extent[i].blocksz;
					m_blocksz *= m_dist.m_extent[i].blocksz;
					break;
				case DistEnum::disttype::NONE:
					m_accessunit.m_extent[i] = m_sizespec.m_extent[i];
					m_blocksz *= m_sizespec.m_extent[i];
					break;
				default:
					break;
				}
			}
			m_accessunit.construct();
		}

		DistSpec<ndim_> 	m_dist;
		TeamSpec<ndim_> 	m_teamorg;
		AccessUnit<ndim_> 	m_accessunit;
		SizeSpec<ndim_> 	m_sizespec;
		ViewOrg<ndim_> 		m_vieworg;
		long long 			m_nunits = dash::Team::All().size();
		long long 			view_dim;
		long long			m_blocksz;
		int 				argc_DistEnum = 0;
		int 				argc_extents = 0;
		int                             argc_ts = 0;
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
				m_dist.m_extent[0] = BLOCKED;
				argc_DistEnum = 1;
			}
			for (size_t i = argc_DistEnum; i < ndim_; i++) {
				m_dist.m_extent[i] = NONE;
			}

			assert(argc_extents == ndim_);
			checkValidDistEnum();

			m_sizespec.construct();
			m_vieworg = ViewOrg<ndim_>(m_sizespec);
			view_dim = ndim_;

			checkTile();

			if (argc_ts == 0)
				m_teamorg = TeamSpec<ndim_>(m_team);

			constructAccessBase();
		}

		//TODO
		Pattern(const SizeSpec<ndim_> &sizespec, const DistSpec<ndim_> &dist =
			DistSpec<ndim_>(), const TeamSpec<ndim_> &teamorg = TeamSpec<ndim_>::TeamSpec(), dash::Team& team = dash::Team::All()) :
			m_sizespec(sizespec), m_dist(dist), m_teamorg(teamorg), m_team(team) {
			// unnecessary check
			// assert(dist.ndim == teamorg.ndim && dist.ndim == m_extentorg.ndim);

			m_nunits = m_team.size();
			m_vieworg = ViewOrg<ndim_>(m_sizespec);
			view_dim = ndim_;

			checkValidDistEnum();

			checkTile();
			constructAccessBase();
		}

		Pattern(const SizeSpec<ndim_> &sizespec, const DistSpec<ndim_> &dist =
			DistSpec<ndim_>(), dash::Team& team = dash::Team::All()) :
			m_sizespec(sizespec), m_dist(dist), m_teamorg(m_team) {
			// unnecessary check
			// assert(dist.ndim == teamorg.ndim && dist.ndim == m_extentorg.ndim);

			m_team = team;
			m_nunits = m_team.size();
			m_vieworg = ViewOrg<ndim_>(m_sizespec);
			view_dim = ndim_;

			checkValidDistEnum();

			checkTile();
			constructAccessBase();
		}

		template<typename ... values>
		long long atunit(values ... Values) const {
			assert(sizeof...(Values) == view_dim);
			std::array<long long, ndim_> inputindex = { Values... };
			return atunit_(inputindex);
		}

		long long atunit_(std::array<long long, ndim_> input) const {
			assert(input.size() == ndim_);
			long long rs = -1;
			long long index[ndim_];
			std::array<long long, ndim_> teamorgindex;

			if (m_teamorg.ndim() == 1) {
				rs = 0;
				for (size_t i = 0; i < ndim_; i++) {
					index[i] = m_vieworg.begin[i];
					if (ndim_ - i <= view_dim)
						index[i] += input[i - (ndim_ - view_dim)];

					long long cycle = m_teamorg.size() * m_dist.m_extent[i].blocksz;
					switch (m_dist.m_extent[i].type) {
					case DistEnum::disttype::BLOCKED:
						rs += index[i]
							/ (m_sizespec.m_extent[i] % m_teamorg.size() == 0 ?
							m_sizespec.m_extent[i] / m_teamorg.size() :
							m_sizespec.m_extent[i] / m_teamorg.size()
							+ 1);
						break;
					case DistEnum::disttype::CYCLIC:
						rs += modulo(index[i], m_teamorg.size());
						break;
					case DistEnum::disttype::BLOCKCYCLIC:
						rs += (index[i] % cycle) / m_dist.m_extent[i].blocksz;
						break;
					case DistEnum::disttype::TILE:
						rs += (index[i] % cycle) / m_dist.m_extent[i].blocksz;
						break;
					case DistEnum::disttype::NONE:
						rs += 0;
						break;
					}
				}
				rs = rs % m_teamorg.size();
			}
			else {
				for (size_t i = 0; i < ndim_; i++) {
					index[i] = m_vieworg.begin[i];
					if (ndim_ - i <= view_dim)
						index[i] += input[i - (ndim_ - view_dim)];

					long long cycle = m_teamorg.m_extent[i]
						* m_dist.m_extent[i].blocksz;
					assert(index[i] >= 0);
					switch (m_dist.m_extent[i].type) {
					case DistEnum::disttype::BLOCKED:
						teamorgindex[i] = index[i]
							/ (m_sizespec.m_extent[i] % m_teamorg.m_extent[i]
							== 0 ?
							m_sizespec.m_extent[i]
							/ m_teamorg.m_extent[i] :
							m_sizespec.m_extent[i]
							/ m_teamorg.m_extent[i] + 1);
						break;
					case DistEnum::disttype::CYCLIC:
						teamorgindex[i] = modulo(index[i], m_teamorg.m_extent[i]);
						break;
					case DistEnum::disttype::BLOCKCYCLIC:
						teamorgindex[i] = (index[i] % cycle)
							/ m_dist.m_extent[i].blocksz;
						break;
					case DistEnum::disttype::TILE:
						teamorgindex[i] = (index[i] % cycle)
							/ m_dist.m_extent[i].blocksz;
						break;
					case DistEnum::disttype::NONE:
						teamorgindex[i] = -1;
						break;
					}
				}
				rs = m_teamorg.at(teamorgindex);
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


				if (m_teamorg.ndim() == 1)
					dimunit = m_teamorg.size();
				else
					dimunit = m_teamorg.m_extent[i];

				long long cycle = dimunit * m_dist.m_extent[i].blocksz;

				switch (m_dist.m_extent[i].type) {
				case DistEnum::disttype::BLOCKED:
					res *= getCeil(m_sizespec.m_extent[i], dimunit);
					break;

				case DistEnum::disttype::CYCLIC:
					res *= getCeil(m_sizespec.m_extent[i], dimunit);
					break;

				case DistEnum::disttype::BLOCKCYCLIC:
					res *= m_dist.m_extent[i].blocksz
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

		long long index_to_unit(long long idx) const {
			// i -> [0, nelem)
			std::array<long long, ndim_> input = m_teamorg.coords(idx);

			return index_to_unit(input);
		}

		long long index_to_elem(long long idx) const {
			// i -> [0, nelem)
			std::array<long long, ndim_> input = m_sizespec.coords(idx);

			return index_to_elem(input);
		}

		long long index_to_unit(std::array<long long, ndim_> input) const {
			// i -> [0, nelem)

			return atunit_(input);
		}

		long long index_to_elem(std::array<long long, ndim_> input) const {
			// i -> [0, nelem)

			return at_(input);
		}

		template<typename ... values>
		long long at(values ... Values) const {
			assert(sizeof...(Values) == view_dim);
			std::array<long long, ndim_> inputindex = { Values... };
			return at_(inputindex);
		}

		long long at_(std::array<long long, ndim_> input) const {
			assert(input.size() == view_dim);
			long long rs = -1;
			long long index[ndim_];
			std::array<long long, ndim_> teamorgindex;
			std::array<long long, ndim_> cyclicfix;
			long long DTeamfix = 0;
			long long blocksz[ndim_];

			for (size_t i = 0; i < ndim_; i++) {
				long long dimunit;
				if (ndim_ > 1 && m_teamorg.ndim() == 1) {
					dimunit = m_teamorg.size();
				}
				else
					dimunit = m_teamorg.m_extent[i];

				index[i] = m_vieworg.begin[i];
				if (ndim_ - i <= view_dim)
					index[i] += input[i - (ndim_ - view_dim)];

				long long cycle = dimunit * m_dist.m_extent[i].blocksz;
				blocksz[i] = getCeil(m_sizespec.m_extent[i], dimunit);
				teamorgindex[i] = (long long)(index[i] % blocksz[i]);
				cyclicfix[i] = 0;

				assert(index[i] >= 0);
				switch (m_dist.m_extent[i].type) {
				case DistEnum::disttype::BLOCKED:
					teamorgindex[i] = index[i] % blocksz[i];
					if (i > 0) {
						if (m_sizespec.m_extent[i] % dimunit != 0
							&& getCeil(index[i] + 1, blocksz[i]) == dimunit)
							cyclicfix[i - 1] = -1;
					}
					break;
				case DistEnum::disttype::CYCLIC:
					teamorgindex[i] = index[i] / dimunit;
					if (i > 0)
						cyclicfix[i - 1] =
						(index[i] % dimunit)
						< (m_sizespec.m_extent[i] % dimunit) ?
						1 : 0;
					break;
				case DistEnum::disttype::BLOCKCYCLIC:
					teamorgindex[i] = (index[i] / cycle)
						* m_dist.m_extent[i].blocksz
						+ (index[i] % cycle) % m_dist.m_extent[i].blocksz;
					if (i > 0) {
						if (m_sizespec.m_extent[i] < cycle)
							cyclicfix[i - 1] = 0;
						else if ((index[i] / m_dist.m_extent[i].blocksz) % dimunit
							< getFloor(m_sizespec.m_extent[i] % cycle,
							m_dist.m_extent[i].blocksz))
							cyclicfix[i - 1] = m_dist.m_extent[i].blocksz;
						else if ((index[i] / m_dist.m_extent[i].blocksz) % dimunit
							< getCeil(m_sizespec.m_extent[i] % cycle,
							m_dist.m_extent[i].blocksz))
							cyclicfix[i - 1] = m_sizespec.m_extent[i]
							% m_dist.m_extent[i].blocksz;
						else
							cyclicfix[i - 1] = 0;
					}
					break;
				case DistEnum::disttype::TILE:
					teamorgindex[i] = (index[i] / cycle)
						* m_dist.m_extent[i].blocksz
						+ (index[i] % cycle) % m_dist.m_extent[i].blocksz;
					if (i > 0) {
						if ((index[i] / m_dist.m_extent[i].blocksz) % dimunit
							< getFloor(m_sizespec.m_extent[i] % cycle,
							m_dist.m_extent[i].blocksz))
							cyclicfix[i - 1] = m_dist.m_extent[i].blocksz;
						else if ((index[i] / m_dist.m_extent[i].blocksz) % dimunit
							< getCeil(m_sizespec.m_extent[i] % cycle,
							m_dist.m_extent[i].blocksz))
							cyclicfix[i - 1] = m_sizespec.m_extent[i] % cycle;
						else
							cyclicfix[i - 1] = 0;
					}
					break;
				case DistEnum::disttype::NONE:
					teamorgindex[i] = index[i];
					break;
				}
			}

			if (m_dist.m_extent[0].type == DistEnum::disttype::TILE) {
				rs = 0;
				long long incre[ndim_];
				incre[ndim_ - 1] = m_accessunit.size();
				for (size_t i = 1; i < ndim_; i++) {
					long long dim = ndim_ - i - 1;
					long long cycle = m_teamorg.m_extent[dim]
						* m_dist.m_extent[dim].blocksz;
					long long ntile = m_sizespec.m_extent[i] / cycle
						+ cyclicfix[dim] / m_accessunit.m_extent[i];
					incre[dim] = incre[dim + 1] * ntile;
				}
				for (size_t i = 0; i < ndim_; i++) {
					long long tile_index = (teamorgindex[i])
						/ m_accessunit.m_extent[i];
					long long tile_index_remain = (teamorgindex[i])
						% m_accessunit.m_extent[i];
					rs += tile_index_remain * m_accessunit.m_offset[i]
						+ tile_index * incre[i];
				}
				return rs;
			}
			else {
				rs = m_accessunit.at(teamorgindex, cyclicfix);
			}

			return rs;
		}

		long long nunits() const {
			return m_nunits;
		}

		Pattern& operator=(const Pattern& other) {
			if (this != &other) {
				m_dist = other.m_dist;
				m_teamorg = other.m_teamorg;
				m_accessunit = other.m_accessunit;
				m_sizespec = other.m_sizespec;
				m_vieworg = other.m_vieworg;
				m_nunits = other.m_nunits;
				view_dim = other.view_dim;
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
			return m_dist;
		}

		SizeSpec<ndim_> sizespec() const {
			return m_sizespec;
		}

		TeamSpec<ndim_> teamspec() const {
			return m_teamorg;
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

		bool is_local(size_t idx, size_t myid, size_t dim)
		{
			long long dimunit;
			size_t myidx;
			bool ret = false;
			long long cycle = m_teamorg.size() * m_dist.m_extent[dim].blocksz;

                        
			if (ndim_ > 1 && m_teamorg.ndim() == 1)
			{
				dimunit = m_teamorg.size();
				myidx = m_teamorg.index_at_dim(myid, ndim_ - 1);
			}
			else
			{
				dimunit = m_teamorg.m_extent[dim];
				myidx = m_teamorg.index_at_dim(myid, dim);
			}

//                        printf("idx %d myid %d dim %d myidx %d dimunit %d cycle %d\n", idx, myid, dim, myidx, dimunit, cycle);			

			switch (m_dist.m_extent[dim].type) {
			case DistEnum::disttype::BLOCKED:


				if ( (idx >= getCeil(m_sizespec.m_extent[dim], dimunit)*(myidx)) &&
					(idx < getCeil(m_sizespec.m_extent[dim], dimunit)*(myidx + 1) ) )
					ret = true;

				break;
			case DistEnum::disttype::BLOCKCYCLIC:

				ret = ((idx % cycle) >= m_dist.m_extent[dim].blocksz * (myidx) ) &&
					((idx % cycle) < m_dist.m_extent[dim].blocksz * (myidx + 1) );

				break;
			case DistEnum::disttype::CYCLIC:

				ret = idx % dimunit == myidx;

				break;
			case DistEnum::disttype::TILE:

				ret = ((idx % cycle) >= m_dist.m_extent[dim].blocksz * (myidx)) &&
					((idx % cycle) < m_dist.m_extent[dim].blocksz * (myidx + 1) );

				break;
			case DistEnum::disttype::NONE:
				ret = true;
				break;
			default:
				break;
			}
			
			return ret;
		}

		Pattern row(const long long index) const {
			Pattern<ndim_> rs = this;
			rs.view_dim = view_dim - 1;
			assert(ndim_ - rs.view_dim - 1 >= 0);
			rs.m_vieworg.begin[ndim_ - rs.view_dim - 1] = index;

			return rs;
		}


	};
}

#endif /* PATTERN_H_INCLUDED */
