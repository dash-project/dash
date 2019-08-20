#ifndef DASH_TASKS_INTERNAL_LAMBDATRAITS
#define DASH_TASKS_INTERNAL_LAMBDATRAITS

#include <tuple>
#include <type_traits>

namespace dash
{
namespace tasks
{
namespace internal
{

  /**
   * Implementation of is_detected taken from
   * https://rawgit.com/cplusplus/fundamentals-ts/v2/fundamentals-ts.html#meta.detect
   */
  template<class...>
  using void_t = void;

  struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;

    void operator=(nonesuch const&) = delete;
  };

  template <class Default, class AlwaysVoid,
        template<class...> class Op, class... Args>
  struct DETECTOR { // exposition only
    using value_t = std::false_type;
    using type = Default;
  };

  template <class Default, template<class...> class Op, class... Args>
  struct DETECTOR<Default, void_t<Op<Args...>>, Op, Args...> { // exposition only
    using value_t = std::true_type;
    using type = Op<Args...>;
  };

  template <template<class...> class Op, class... Args>
  using is_detected = typename DETECTOR<nonesuch, void, Op, Args...>::value_t;

  template<class FuncT, class... Args>
  using const_lvalue_callable_foo_t = decltype(std::declval<const FuncT&>().operator()(std::declval<Args>()...));

  /**
   * Check whether a function is callable on a const-qualified rhs.
   */
  template<typename FuncT, class... Args>
  using is_const_callable = is_detected<const_lvalue_callable_foo_t, FuncT, Args...>;

}
}
}

#endif // DASH_TASKS_INTERNAL_LAMBDATRAITS
