#ifndef DASH__VIEW__UTILITY_H__INCLUDED
#define DASH__VIEW__UTILITY_H__INCLUDED

#include <type_traits>
#include <utility>

/*
 * Pipe utils adapted from implementation in range-v3,
 * published under Boost Software License Version 1.0
 * (c) Eric Niebler, Casey Carter
 *
 * See:
 *   https://github.com/ericniebler/range-v3/
 * Source:
 *   include/range/v3/utility/functional.hpp
 *
 */

namespace dash {

namespace internal {

  struct PipeableBase
  {};

  template<typename T>
  struct is_pipeable : std::is_base_of<PipeableBase, T>
  {};

  template<typename T>
  struct is_pipeable<T &> : is_pipeable<T>
  {};

  struct PipeableAccess
  {
    template<typename PipeableT>
    struct impl : PipeableT {
      using PipeableT::pipe;
    };

    template<typename PipeableT>
    struct impl<PipeableT &> : impl<PipeableT>
    {};
  };

  template<typename Derived>
  struct Pipeable : PipeableBase {
  private:
    friend PipeableAccess;
    template<typename Arg, typename Pipe>
    static auto pipe(Arg && arg, Pipe p)
    -> decltype(p(static_cast<Arg&&>(arg))) {
      p(static_cast<Arg&&>(arg));
    }
  };

  template<typename Bind>
  struct PipeableBinder : Bind, Pipeable< PipeableBinder<Bind> > {
    PipeableBinder(Bind bind)
    : Bind(std::move(bind))
    {}
  };

  template <typename PipeA, typename PipeB>
  struct ComposedPipe {
    PipeA pipe_a;
    PipeB pipe_b;
    template <typename Arg>
    auto operator()(Arg && arg) const
    -> decltype(static_cast<Arg &&>(arg) | pipe_a | pipe_b) {
      return static_cast<Arg &&>(arg) | pipe_a | pipe_b;
    }
  };

  struct make_pipeable_fn {
    template<typename Fun>
    PipeableBinder<Fun> operator()(Fun fun) const {
      return { std::move(fun) };
    }
  };

} // namespace internal

inline namespace {
  constexpr internal::make_pipeable_fn make_pipeable { };
}

/**
 * Compose pipes.
 *
 *   ... <pipeable> | <pipeable>
 */
template <
  typename PipeA,
  typename PipeB,
  typename std::enable_if<
    ( internal::is_pipeable<PipeA>::value &&
      internal::is_pipeable<PipeB>::value ),
    int
  >::type = 0 >
auto operator|(PipeA a, PipeB b)
-> decltype(make_pipeable(internal::ComposedPipe<PipeA,PipeB> { a, b })) {
  return make_pipeable(internal::ComposedPipe<PipeA,PipeB> { a, b });
}

/**
 * Evaluate a pipe.
 *
 *   arg | <pipeable>
 */
template <
  typename Arg,
  typename Pipe,
  typename std::enable_if<
    (!internal::is_pipeable<Arg>::value &&
      internal::is_pipeable<Pipe>::value ),
    int
  >::type = 0 >
auto operator|(Arg && arg, Pipe pipe)
-> decltype(pipe(arg)) {
  return pipe(arg);
}

} // namespace dash

#endif // DASH__VIEW__UTILITY_H__INCLUDED
