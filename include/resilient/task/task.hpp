#pragma once

#include <utility>
#include <tuple>
#include <type_traits>
#include <resilient/common/failable.hpp>
#include <resilient/common/utilities.hpp>
#include <resilient/common/foldinvoke.hpp>
#include <resilient/policies/pipeline.hpp>

namespace resilient {

class NoFailureCondition { };

template<typename Callable, typename FailureCondition>
class Task
{
public:
    template<typename NewFailureCondition>
    Task<Callable, NewFailureCondition> failsIf(NewFailureCondition&& condition) &&
    {
        static_assert(std::is_same<FailureCondition, NoFailureCondition>::value,
            "The Task already has a failure condition.");

        return Task<Callable, NewFailureCondition>(
            std::forward<Callable>(d_callable),
            std::forward<NewFailureCondition>(condition));
    }

    template<typename ...Args,
             typename = std::enable_if_t< !std::is_same<FailureCondition, void>::value > >
    decltype(auto) operator()(Args&&... args) &&
    {
        static_assert(!std::is_same<FailureCondition, NoFailureCondition>::value,
            "The Task does not have a failure condition.");

        // TODO Task job is to create a Failable from the function.
        // it means it should get the result of the call (or catch the exception)
        // and wrap it in a failable.
        // With the failable it should call the detector to see if it matches a condition.
        return make_failable<Failure>(std::forward<Callable>(d_callable)(std::forward<Args>(args)...));
    }

    Task(Callable&& callable, FailureCondition&& condition)
    : d_callable(std::forward<Callable>(callable))
    , d_failureDetector(std::forward<FailureCondition>(condition))
    { }

private:
    Callable d_callable;
    FailureCondition d_failureDetector;
};

template<typename Callable>
Task<Callable, NoFailureCondition> task(Callable&& callable)
{
    return Task<Callable, NoFailureCondition>(std::forward<Callable>(callable), NoFailureCondition());
}

}