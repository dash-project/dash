#ifndef OMP_REDUCTION_H_INCLUDED
#define OMP_REDUCTION_H_INCLUDED

#include <limits>
#include <dash/Types.h>
#include <dash/Team.h>
#include <dash/Array.h>

namespace dash {
namespace omp {


namespace Op {

template<typename R>
class None {
public:
	static constexpr R Neutral = 0;
	static inline R op(R sum, R add) { return sum; }
};

template<typename R>
class Plus {
public:
	static constexpr R Neutral = 0;
	static inline R op(R sum, R add) { return sum + add; }
};

template<typename R>
class Minus {
public:
	static constexpr R Neutral = 0;
	static inline R op(R sum, R add) { return sum - add; }
};

template<typename R>
class Mult {
public:
	static constexpr R Neutral = 1;
	static inline R op(R sum, R add) { return sum * add; }
};

template<typename R>
class BitAnd {
public:
	static constexpr R Neutral = 1;
	static inline R op(R sum, R add) { return sum & add; }
};

template<typename R>
class BitOr {
public:
	static constexpr R Neutral = 0;
	static inline R op(R sum, R add) { return sum | add; }
};

template<typename R>
class BitXor {
public:
	static constexpr R Neutral = 0;
	static inline R op(R sum, R add) { return sum ^ add; }
};

template<typename R>
class LogicAnd {
public:
	static constexpr R Neutral = 1;
	static inline R op(R sum, R add) { return sum && add; }
};

template<typename R>
class LogicOr {
public:
	static constexpr R Neutral = 0;
	static inline R op(R sum, R add) { return sum || add; }
};

template<typename R>
class Min {
public:
	static constexpr R Neutral = std::numeric_limits<R>::max();
	static inline R op(R sum, R add) { return sum < add ? sum : add; }
};

template<typename R>
class Max {
public:
	static constexpr R Neutral = std::numeric_limits<R>::min();
	static inline R op(R sum, R add) { return sum > add ? sum : add; }
};

} // end namespace Op

/**
 * The Reduction class allows to reduce a variable (e.g. sum it up) that was
 * independently calculated by a team.
 *
 * \tparam ReductionType Type of the variable the reduction is executed on
 */
template<
	typename ReductionType, 
	class ReductionOp>
class Reduction {
private:
	ReductionType        _red;
	Team&                _team;
	Array<ReductionType> _results;

public:
	/**
	 * Create a new reduction. The reduction variable may have an initial
	 * value. After the execution of this function, the reduction variable
	 * is set to the neutral element of the reduction operation to ensure
	 * correct reduction.
	 *
	 * \param ReductionType& Reduction variable (before calculations)
	 * \param ReductionOp    Reduction operation
	 * \param Team&          Team that executes this reduction operation
	 */
	Reduction(ReductionType& red, Team& team = dash::Team::All()): 
		_results(team.size()+1, dash::BLOCKED),
		_team(team) {
		_red  = red;
		red = ReductionOp::Neutral;
	}

	/**
	 * Create a new reduction without paying attention to an initial value.
	 *
	 * \param ReductionType& Reduction variable (before calculations)
	 * \param ReductionOp    Reduction operation
	 * \param Team&          Team that executes this reduction operation
	 */
	Reduction(Team& team = dash::Team::All()): 
		_results(team.size()+1, dash::BLOCKED),
		_team(team) {
		_red  = ReductionOp::Neutral;
	}

	/**
	 * Execute the reduction after all team members have finished their
	 * private calculations on the reduction variable.
	 * After the execution of this function, all team members have the
	 * reduced value of the variable stored in \c red.
	 *
	 * \param ReductionType& Reduction variable (after calculations)
	 */
	void reduce(ReductionType& red) {
		// let every team member save its value to the array
		_results[_team.myid()] = red;
		_team.barrier();
		// the first team member ("master") executes the reduction
		if (_team.myid() == 0) {
			for (size_t i = 0; i < _team.size(); ++i) {
				_red = ReductionOp::op(_red, _results[i]);
			}
			// and stores it in the last field of the shared array
			_results[_team.size()] = _red;
		}
		_team.barrier();
		// after the reduction, all team members fetch the correct value
		red = _results[_team.size()];
	}
};

/**
 * Shorthand function to do a simple reduction.
 * Note that the initial value of \c red will be ignored!
 *
 * \tparam ReductionType Type of the variable the reduction is executed on
 *
 * \param ReductionType& Reduction variable (before calculations)
 * \param ReductionOp    Reduction operation
 * \param Team&          Team that executes this reduction operation 
 */
template<typename ReductionType, class ReductionOp>
void reduce(ReductionType& red, Team& team) {
	Reduction<ReductionType, ReductionOp> r(team);
	r.reduce(red);
}


} // end namespace omp
} // end namespace dash

#endif /* OMP_REDUCTION_H_INCLUDED */