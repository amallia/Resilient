#include "resilient/policy/pipeline.hpp"
#include "resilient/policy/circuitbreaker.hpp"
#include "resilient/policy/retry.hpp"
#include "resilient/policy/noop.hpp"
#include "resilient/common/failable.hpp"

#include <type_traits>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace resilient;

namespace {

struct FailureType {};

using FailableType = Failable<FailureType, int>;

class Callable
{
public:
    MOCK_METHOD0(call, FailableType());

    FailableType operator()()
    {
        return call();
    }
};


class Policies : public testing::Test
{
protected:
    testing::StrictMock<Callable> d_callable;
};

}

TEST_F(Policies, Noop)
{
    EXPECT_CALL(d_callable, call())
    .WillOnce(testing::Return(make_failable<FailureType>(0)));

    Noop noop;
    auto result = noop(d_callable);

    EXPECT_TRUE(result.isValue());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(Policies, RetryPolicySuccessAfterFewTries)
{
    testing::Sequence s;
    EXPECT_CALL(d_callable, call())
        .WillOnce(testing::Return(failure_for<FailableType>()))
        .WillOnce(testing::Return(failure_for<FailableType>()))
        .WillOnce(testing::Return(failure_for<FailableType>()))
        .WillOnce(testing::Return(make_failable<FailureType>(1)));

    RetryPolicy retry(5);

    auto result = retry(d_callable);
    EXPECT_TRUE(result.isValue());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(Policies, RetryPolicyNeverSucceeds)
{
    EXPECT_CALL(d_callable, call())
        .Times(3)
        .WillRepeatedly(testing::Return(failure_for<FailableType>()));

    RetryPolicy retry(3);

    auto result = retry(d_callable);
    EXPECT_TRUE(result.isFailure());
}