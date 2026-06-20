// solution_detector reports SAT when no active goals remain. Unit tests mock
// check_active_goals_empty and assert detect() mirrors the empty() predicate.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/solution_detector.hpp"

using ::testing::Return;

struct MockCheckActiveGoalsEmpty {
    MOCK_METHOD(bool, empty, (), (const));
};

using TestSolutionDetector = solution_detector<MockCheckActiveGoalsEmpty>;

struct SolutionDetectorTest : public ::testing::Test {
    MockCheckActiveGoalsEmpty check_active_goals_empty;
    TestSolutionDetector detector{check_active_goals_empty};
};

TEST_F(SolutionDetectorTest, EmptyActiveGoalsMeansSolution) {
    EXPECT_CALL(check_active_goals_empty, empty()).WillOnce(Return(true));
    EXPECT_TRUE(detector.detect());
}

TEST_F(SolutionDetectorTest, NonemptyActiveGoalsIsNotSolution) {
    EXPECT_CALL(check_active_goals_empty, empty()).WillOnce(Return(false));
    EXPECT_FALSE(detector.detect());
}
