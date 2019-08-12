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

    namespace lambda_detail
    {
        template<class Ret, class Cls, class IsMutable, class... Args>
        struct types
        {
            using is_mutable = IsMutable;

            enum { arity = sizeof...(Args) };

            using return_type = Ret;

            template<size_t i>
            struct arg
            {
                typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
            };
        };
    }

    template<class Ld>
    struct lambda_type
        : lambda_type<decltype(&Ld::operator())>
    {};

    template<class Ret, class Cls, class... Args>
    struct lambda_type<Ret(Cls::*)(Args...)>
        : lambda_detail::types<Ret,Cls,std::true_type,Args...>
    {};

    template<class Ret, class Cls, class... Args>
    struct lambda_type<Ret(Cls::*)(Args...) const>
        : lambda_detail::types<Ret,Cls,std::false_type,Args...>
    {};

}
}
}

#endif // DASH_TASKS_INTERNAL_LAMBDATRAITS
