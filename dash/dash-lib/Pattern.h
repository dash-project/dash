#ifndef PATTERN_H_INCLUDED
#define PATTERN_H_INCLUDED

#include <assert.h>
#include <functional>
#include <cstring>
#include "Team.h"

namespace dash
{
	// For test. Will be replaced by DART vars
	const long long myid_ = 0;
	const long long DEFAULT_TEAM_UNIT = 4;

	struct DistEnum
	{
		enum disttype {
			BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
			CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
			BLOCKCYCLIC,
			TILE,
			NONE
		}; // general blocked distribution

		disttype   type;
		long long  blocksz;
	};

	struct DistEnum BLOCKED{ DistEnum::BLOCKED, -1 };
	//obsolete
	struct DistEnum CYCLIC_{ DistEnum::CYCLIC, -1 };
	struct DistEnum CYCLIC{ DistEnum::BLOCKCYCLIC, 1 };

	struct DistEnum NONE{ DistEnum::NONE, -1 };

	struct DistEnum TILE(int bs)
	{
		return{ DistEnum::TILE, bs };
	}

	struct DistEnum BLOCKCYCLIC(int bs)
	{
		return{ DistEnum::BLOCKCYCLIC, bs };
	}

	template<typename T, size_t ndim_>
	class DimBase
	{
	private:
		template<typename T_>
		void readRests(T_ value)
		{
			data[m_ndim - 1] = value;
		}

		template<typename T_, typename... values>
		void readRests(T_ value, values... Values)
		{
			size_t index = m_ndim - 1 - sizeof...(values);
			assert(index >= 0);
			data[index] = value;
			readRests(Values...);
		}

	protected:
		size_t m_ndim = ndim_;
		T data[ndim_];

	public:

		template<size_t ndim__> friend class Pattern;

		DimBase()
		{
		}

		template<typename T_, typename... values>
		DimBase(T_ value, values... Values)
		{
			assert(m_ndim - 1 == sizeof...(Values));
			readRests(value, Values...);
		}
	};

	template<size_t ndim_>
	class DimRangeBase :public DimBase < long long, ndim_ >
	{

	protected:
		long long m_nelem = -1;
		long long offset[ndim_];

	public:
		
		template<size_t ndim__> friend class Pattern;


		long long nelem() const
		{
			return m_nelem;
		}

		DimRangeBase()
		{
		}

		void construct()
		{
			long long cap = 1;
			offset[this->m_ndim - 1] = 1;
			for (size_t i = this->m_ndim - 1; i >= 1; i--)
			{
				cap *= this->data[i];
				offset[i - 1] = cap;
			}
			m_nelem = cap*this->data[0];
		}

		template<typename T_, typename... values>
		DimRangeBase(T_ value, values... Values):DimBase<long long, ndim_>::DimBase(value, Values...)
		{
			construct();
		}

		long long at(int value) const
		{
			return value - 1;
		}

		template<typename T, typename... values>
		long long at(T value, values... Values) const
		{
			size_t index = this->m_ndim - 1 - sizeof...(values);
			long long rs = value*offset[index] + getUnitIDbyIndex(Values...);
			assert(rs <= this->nunits - 1);
			return rs;
		}

		long long at(long long index[]) const
		{
			long long rs = 0;
			for (size_t i = 0; i < this->m_ndim; i++)
				if (index[i] != -1)//omit NONE distribution
					rs += index[i] * offset[i];
			assert(rs <= m_nelem - 1);
			return rs;
		}

		long long at(long long index[], long long cyclicfix[]) const
		{
			long long rs = 0;
			for (size_t i = 0; i < this->m_ndim; i++)
				if (index[i] != -1)//omit NONE distribution
					rs += index[i] * (offset[i] + cyclicfix[i]);
			//assert(rs <= nelem - 1);
			return rs;
		}

		std::array<long long, ndim_> coords(long long pos) const
		{
			std::array<long long, ndim_> indexes;
			long long remaind = pos;
			for (size_t i = 0; i < this->m_ndim; i++)
			{
				indexes[i] = remaind / this->capacity[i];
				remaind = remaind % this->capacity[i];
				cout << indexes[i] << " ";
			}
			cout << endl;
			return indexes;
		}
	};

	template<size_t ndim_>
	class DistSPec :public DimBase < DistEnum, ndim_ >
	{
	public:

		DistSPec()
		{
			for (size_t i = 1; i < ndim_; i++)
				this->data[i] = NONE;
			this->data[1] = BLOCKED;
		}

		template<typename T_, typename... values>
		DistSPec(T_ value, values... Values) :DimBase < DistEnum, ndim_ >::DimBase(value, Values...)
		{
		}
	};

	template<size_t ndim_>
	class AccessUnit :public DimRangeBase < ndim_ >
	{
	public:
		AccessUnit()
		{
		}

		template<typename T, typename... values>
		AccessUnit(T value, values... Values) :DimRangeBase < ndim_ >::DimRangeBase(value, Values...)
		{
		}
	};

	template<size_t ndim_>
	class TeamSpec :public DimRangeBase < ndim_ >
	{
	public:
		TeamSpec()
		{
			for (size_t i = 0; i < ndim_; i++)
				this->data[i] = 1;
			this->data[ndim_ - 1] = this->m_nelem = DEFAULT_TEAM_UNIT;
			this->construct();
			this->m_ndim = 1;
		}

		template<typename T, typename... values>
		TeamSpec(T value, values... Values) :DimRangeBase < ndim_ >::DimRangeBase(value, Values...)
		{
		}

		int ndim() const
		{
			return this->m_ndim;
		}
	};

	typedef TeamSpec<1> DefaultTeamSpec;
	DefaultTeamSpec DEFAULT1DTEAM(long long nunit)
	{
		return{ nunit };
	}

	template<size_t ndim_>
	class DataSpec :public DimRangeBase < ndim_ >
	{
	public:

		template<size_t ndim__> friend class ViewOrg;

		DataSpec()
		{
		}

		template<typename T_, typename... values>
		DataSpec(T_ value, values... Values) :DimRangeBase < ndim_ >::DimRangeBase(value, Values...)
		{
		}

	};

	class ViewPair{
		long long begin;
		long long range;
	};

	template<size_t ndim_>
	class ViewOrg :public DimBase < ViewPair, ndim_ >
	{
	public:
		long long begin[ndim_];
		long long range[ndim_];
		size_t ndim = ndim_;
		long long nelem = 0;

		ViewOrg()
		{
		}

		ViewOrg(DataSpec<ndim_> dataorg)
		{
			memcpy(this->data, dataorg.data, sizeof(long long)*ndim_);
			memcpy(range, dataorg.data, sizeof(long long)*ndim_);
			nelem = dataorg.nelem();
			for (size_t i = 0; i < ndim_; i++)
				begin[i] = 0;
		}

		template<typename T_, typename... values>
		ViewOrg(ViewPair value, values... Values) :DimBase < ViewPair, ndim_ >::DimBase(value, Values...)
		{
		}
	};

	template<size_t ndim_>
	class Pattern
	{
	private:

		inline long long modulo(const long long i, const long long k) const {
			long long res = i % k;
			if (res < 0) res += k;
			return res;
		}

		inline long long getCeil(const long long i, const long long k) const {
			if (i%k == 0)
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

		// the first DIM parameters must be used to
		// specify the extent
		template<int count>
		void check(long long extent) {
			//static_assert(count < ndim_, "Too many parameters for extent");
			m_dataorg.data[count] = extent;
			argc_extents++;
			cout << "I got " << extent << " for extent in dimension " << count << endl;
		};

		// the next (up to DIM) parameters may be used to 
		// specify the distribution pattern
		// TODO: How many are required? 0/1/DIM ?
		template<int count>
		void check(const TeamSpec<ndim_> & ts) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			//constexpr int dim = count - ndim_;
			//m_dist.data[dim] = ds;
			//cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
			m_teamorg = ts;
		}

		template<int count>
		void check(const DataSpec<ndim_> & ds) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			//constexpr int dim = count - ndim_;
			//m_dist.data[dim] = ds;
			//cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
			m_dataorg = ds;
			argc_extents += ndim_;
		}

		template<int count>
		void check(const DistSPec<ndim_> & ds){
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			//constexpr int dim = count - ndim_;
			//m_dist.data[dim] = ds;
			//cout << "I got " << ds << " for dist. pattern in dimension " << dim << endl;
			m_dist = ds;
		}

		// the next (up to DIM) parameters may be used to 
		// specify the distribution pattern
		// TODO: How many are required? 0/1/DIM ?
		template<int count>
		void check(DistEnum& ds) {
			//static_assert(count >= ndim_, "Not enough parameters for extent");
			//static_assert(count < 2 * ndim_, "Too many parameters for pattern");

			int dim = count - ndim_;
			m_dist.data[dim] = ds;
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

		template<int count,
			typename T, typename... Args>
			void check(T t, Args&&... args)
		{
			check<count>(t);
			check<count + 1>(std::forward<Args>(args)...);
		}

		// check tile pattern constraints
		void checkTile() const
		{
			int hastile = 0;
			int invalid = 0;
			for (int i = 0; i < ndim_ - 1; i++)
			{
				if (m_dist.data[i].type == DistEnum::disttype::TILE)
					hastile = 1;
				if (m_dist.data[i].type != m_dist.data[i + 1].type)
					invalid = 1;
			}
			assert(!(hastile && invalid));
			if (hastile)
			{
				for (int i = 0; i < ndim_; i++)
				{
					assert(m_dataorg.data[i] % (m_dist.data[i].blocksz) == 0);
				}
			}
		}

		void checkValidDistEnum()
		{
			int n_validdist = 0;
			for (int i = 0; i < ndim_; i++)
			{
				if (m_dist.data[i].type != DistEnum::disttype::NONE)
					n_validdist++;
			}
			assert(n_validdist == m_teamorg.ndim());
		}

		void constructAccessBase()
		{
			for (size_t i = 0; i < ndim_; i++)
			{
				long long dimunit;

				if (ndim_>1 && m_teamorg.ndim() == 1)
					dimunit = m_teamorg.nelem();
				else
					dimunit = m_teamorg.data[i];

				switch (m_dist.data[i].type)
				{
				case DistEnum::disttype::BLOCKED:
					m_accessunit.data[i] = m_dataorg.data[i] % dimunit == 0 ? m_dataorg.data[i] / dimunit : m_dataorg.data[i] / dimunit + 1;
					break;
				case DistEnum::disttype::BLOCKCYCLIC:
					if (m_dataorg.data[i] / (dimunit * m_dist.data[i].blocksz) == 0)
						m_accessunit.data[i] = m_dist.data[i].blocksz;
					else
						m_accessunit.data[i] = m_dataorg.data[i] / (dimunit * m_dist.data[i].blocksz) * m_dist.data[i].blocksz;
					break;
				case DistEnum::disttype::CYCLIC:
					m_accessunit.data[i] = m_dataorg.data[i] / dimunit;
					break;
				case DistEnum::disttype::TILE:
					m_accessunit.data[i] = m_dist.data[i].blocksz;
					break;
				case DistEnum::disttype::NONE:
					m_accessunit.data[i] = m_dataorg.data[i];
					break;
				default:
					break;
				}
			}
			m_accessunit.construct();
		}

		DistSPec<ndim_>		m_dist;
		TeamSpec<ndim_>		m_teamorg;
		AccessUnit<ndim_>   m_accessunit;
		DataSpec<ndim_>		m_dataorg;
		ViewOrg<ndim_>		m_vieworg;
		long long			m_nunits;
		long long			view_dim;
		int					argc_DistEnum = 0;
		int					argc_extents = 0;

	public:


	public:

		template<typename... Args>
		Pattern(Args&&... args)
		{
			static_assert(sizeof...(Args) >= ndim_,
				"Invalid number of constructor arguments.");

			check<0>(std::forward<Args>(args)...);
			int argc = sizeof...(Args);

			//Speficy default patterns for all dims
			//BLOCKED for the 1st, NONE for the rest
			if (argc_DistEnum == 0)
			{
				m_dist.data[0] = BLOCKED;
				argc_DistEnum = 1;
			}
			for (size_t i = argc_DistEnum; i < ndim_; i++)
			{
				m_dist.data[i] = NONE;
			}

			assert(argc_extents == ndim_);
			checkValidDistEnum();


			m_dataorg.construct();
			m_vieworg = ViewOrg<ndim_>(m_dataorg);
			view_dim = ndim_;

			checkTile();

			if (m_teamorg.nelem() == -1)
				m_teamorg = TeamSpec < ndim_ >();

			constructAccessBase();
		}



		Pattern(const DataSpec<ndim_> &dataorg, const DistSPec<ndim_> &dist = DistSPec<ndim_>(), const TeamSpec<ndim_> &teamorg = TeamSpec < ndim_ >()) :m_dataorg(dataorg), m_dist(dist), m_teamorg(teamorg)
		{
			// unnecessary check
			// assert(dist.ndim == teamorg.ndim && dist.ndim == dataorg.ndim);
			m_nunits = teamorg.nelem();
			m_vieworg = ViewOrg<ndim_>(m_dataorg);
			view_dim = ndim_;

			checkValidDistEnum();

			checkTile();
			constructAccessBase();
		}

		template<typename... values>
		long long atunit(values... Values)
		{
			assert(sizeof...(Values) == ndim_);
			long long rs = -1;
			long long inputindex[ndim_] = { Values... };
			long long index[ndim_] = { Values... };
			long long teamorgindex[ndim_];

			if (m_teamorg.ndim() == 1)
			{
				rs = 0;
				for (size_t i = 0; i < ndim_; i++)
				{
					index[i] = m_vieworg.begin[i];
					if (ndim_ - i <= view_dim)
						index[i] += inputindex[i - (ndim_ - view_dim)];

					long long cycle = m_teamorg.nelem() * m_dist.data[i].blocksz;
					switch (m_dist.data[i].type)
					{
					case DistEnum::disttype::BLOCKED:
						rs += index[i] / (m_dataorg.data[i] % m_teamorg.nelem() == 0 ? m_dataorg.data[i] / m_teamorg.nelem() : m_dataorg.data[i] / m_teamorg.nelem() + 1);
						break;
					case DistEnum::disttype::CYCLIC:
						rs += modulo(index[i], m_teamorg.nelem());
						break;
					case DistEnum::disttype::BLOCKCYCLIC:
						rs += (index[i] % cycle) / m_dist.data[i].blocksz;
						break;
					case DistEnum::disttype::TILE:
						rs += (index[i] % cycle) / m_dist.data[i].blocksz;
						break;
					case DistEnum::disttype::NONE:
						rs += 0;
						break;
					}
				}
				rs = rs%m_teamorg.nelem();
			}
			else
			{
				for (size_t i = 0; i < ndim_; i++)
				{
					index[i] = 0;
					index[i] += m_vieworg.begin[i];
					if (ndim_ - i <= view_dim)
						index[i] += inputindex[i - (ndim_ - view_dim)];

					long long cycle = m_teamorg.data[i] * m_dist.data[i].blocksz;
					assert(index[i] >= 0);
					switch (m_dist.data[i].type)
					{
					case DistEnum::disttype::BLOCKED:
						teamorgindex[i] = index[i] / (m_dataorg.data[i] % m_teamorg.data[i] == 0 ? m_dataorg.data[i] / m_teamorg.data[i] : m_dataorg.data[i] / m_teamorg.data[i] + 1);
						break;
					case DistEnum::disttype::CYCLIC:
						teamorgindex[i] = modulo(index[i], m_teamorg.data[i]);
						break;
					case DistEnum::disttype::BLOCKCYCLIC:
						teamorgindex[i] = (index[i] % cycle) / m_dist.data[i].blocksz;
						break;
					case DistEnum::disttype::TILE:
						teamorgindex[i] = (index[i] % cycle) / m_dist.data[i].blocksz;
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

		template<typename... values>
		long long at(values... Values) const
		{
			assert(sizeof...(Values) == view_dim);
			long long rs = -1;
			long long inputindex[ndim_] = { Values... };
			long long index[ndim_] = { Values... };
			long long teamorgindex[ndim_];
			long long cyclicfix[ndim_];
			long long DTeamfix = 0;
			long long blocksz[ndim_];

			for (size_t i = 0; i < ndim_; i++)
			{
				long long dimunit;
				if (ndim_ > 1 && m_teamorg.ndim() == 1)
				{
					dimunit = m_teamorg.nelem();
				}
				else
					dimunit = m_teamorg.data[i];

				index[i] = 0;
				index[i] += m_vieworg.begin[i];
				if (ndim_ - i <= view_dim)
					index[i] += inputindex[i - (ndim_ - view_dim)];

				long long cycle = dimunit * m_dist.data[i].blocksz;
				blocksz[i] = getCeil(m_dataorg.data[i], dimunit);
				teamorgindex[i] = index[i] % blocksz[i];
				cyclicfix[i] = 0;

				assert(index[i] >= 0);
				switch (m_dist.data[i].type)
				{
				case DistEnum::disttype::BLOCKED:
					teamorgindex[i] = index[i] % blocksz[i];
					if (i > 0)
					{
						if (m_dataorg.data[i] % dimunit != 0 && getCeil(index[i] + 1, blocksz[i]) == dimunit)
							cyclicfix[i - 1] = -1;
					}
					break;
				case DistEnum::disttype::CYCLIC:
					teamorgindex[i] = index[i] / dimunit;
					if (i > 0)
						cyclicfix[i - 1] = (index[i] % dimunit) < (m_dataorg.data[i] % dimunit) ? 1 : 0;
					break;
				case DistEnum::disttype::BLOCKCYCLIC:
					teamorgindex[i] = (index[i] / cycle) *  m_dist.data[i].blocksz + (index[i] % cycle) % m_dist.data[i].blocksz;
					if (i > 0)
					{
						if (m_dataorg.data[i] < cycle)
							cyclicfix[i - 1] = 0;
						else if ((index[i] / m_dist.data[i].blocksz) % dimunit < getFloor(m_dataorg.data[i] % cycle, m_dist.data[i].blocksz))
							cyclicfix[i - 1] = m_dist.data[i].blocksz;
						else if ((index[i] / m_dist.data[i].blocksz) % dimunit < getCeil(m_dataorg.data[i] % cycle, m_dist.data[i].blocksz))
							cyclicfix[i - 1] = m_dataorg.data[i] % m_dist.data[i].blocksz;
						else
							cyclicfix[i - 1] = 0;
					}
					break;
				case DistEnum::disttype::TILE:
					teamorgindex[i] = (index[i] / cycle) *  m_dist.data[i].blocksz + (index[i] % cycle) % m_dist.data[i].blocksz;
					if (i > 0)
					{
						if ((index[i] / m_dist.data[i].blocksz) % dimunit < getFloor(m_dataorg.data[i] % cycle, m_dist.data[i].blocksz))
							cyclicfix[i - 1] = m_dist.data[i].blocksz;
						else if ((index[i] / m_dist.data[i].blocksz) % dimunit < getCeil(m_dataorg.data[i] % cycle, m_dist.data[i].blocksz))
							cyclicfix[i - 1] = m_dataorg.data[i] % cycle;
						else
							cyclicfix[i - 1] = 0;
					}
					break;
				case DistEnum::disttype::NONE:
					teamorgindex[i] = index[i];
					break;
				}
			}

			if (m_dist.data[0].type == DistEnum::disttype::TILE)
			{
				rs = 0;
				long long incre[ndim_];
				incre[ndim_ - 1] = m_accessunit.nelem();
				for (size_t i = 1; i < ndim_; i++)
				{
					long long dim = ndim_ - i - 1;
					long long cycle = m_teamorg.data[dim] * m_dist.data[dim].blocksz;
					long long ntile = m_dataorg.data[i] / cycle + cyclicfix[dim] / m_accessunit.data[i];
					incre[dim] = incre[dim + 1] * ntile;
				}
				for (size_t i = 0; i < ndim_; i++)
				{
					long long tile_index = (teamorgindex[i]) / m_accessunit.data[i];
					long long tile_index_remain = (teamorgindex[i]) % m_accessunit.data[i];
					rs += tile_index_remain*m_accessunit.offset[i] + tile_index * incre[i];
				}
				return rs;
			}
			else
			{
				rs = m_accessunit.at(teamorgindex, cyclicfix);
			}

			return rs;
		}

		Pattern row(const long long index) const
		{
			Pattern<ndim_> rs = this;
			rs.view_dim = view_dim - 1;
			assert(ndim_ - rs.view_dim - 1 >= 0);
			rs.m_vieworg.begin[ndim_ - rs.view_dim - 1] = index;

			return rs;
		}
	};
}


#endif /* PATTERN_H_INCLUDED */
