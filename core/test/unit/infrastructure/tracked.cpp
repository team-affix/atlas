// tracked wraps a value with trail-logged mutations. Unit tests mock i_trail and assert
// get() returns the initial value and mutate() logs a backtrackable_increment once.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/tracked.hpp"
#include "interfaces/i_trail.hpp"
#include "infrastructure/backtrackable_increment.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::StrictMock;

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
};

struct TrackedIntTest : public ::testing::Test {
protected:
    NiceMock<MockTrail> trail;
    tracked<int> v{trail, 10};
};

TEST_F(TrackedIntTest, GetReturnsInitialValue) {
    EXPECT_EQ(v.get(), 10);
}

TEST_F(TrackedIntTest, MutateChangesValue) {
    StrictMock<MockTrail> strict_trail;
    tracked<int> strict_v{strict_trail, 10};

    EXPECT_CALL(strict_trail, log(_)).Times(1);
    strict_v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(strict_v.get(), 11);
}
