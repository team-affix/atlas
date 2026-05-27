#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/solution_detector.hpp"
#include "../../../core/hpp/interfaces/i_active_goals.hpp"

using ::testing::Return;

struct MockActiveGoals : public i_active_goals {
    MOCK_METHOD(void, insert, (const goal_lineage*), (override));
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
    MOCK_METHOD(bool, contains, (const goal_lineage*), (const, override));
    state_machine<const goal_lineage*> iterate() const override { co_return; }
    MOCK_METHOD(size_t, size, (), (const, override));
    MOCK_METHOD(bool, empty, (), (const, override));
};

struct SolutionDetectorTest : public ::testing::Test {
    MockActiveGoals ag;
    solution_detector detector{ag};
};

TEST_F(SolutionDetectorTest, EmptyActiveGoalsMeansSolution) {
    EXPECT_CALL(ag, empty()).WillOnce(Return(true));
    EXPECT_TRUE(detector.detect());
}

TEST_F(SolutionDetectorTest, NonemptyActiveGoalsIsNotSolution) {
    EXPECT_CALL(ag, empty()).WillOnce(Return(false));
    EXPECT_FALSE(detector.detect());
}
