// solution_detector reports SAT when no active goals remain. Unit tests mock
// i_check_active_goals_empty and assert detect() mirrors the empty() predicate.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/solution_detector.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"

using ::testing::Return;

struct MockCheckActiveGoalsEmpty : public i_check_active_goals_empty {
    MOCK_METHOD(bool, empty, (), (const, override));
};

struct SolutionDetectorTest : public ::testing::Test {
    locator loc;
    MockCheckActiveGoalsEmpty check_active_goals_empty;
    solution_detector detector;

    SolutionDetectorTest()
        : detector(bind_and_make<solution_detector, i_check_active_goals_empty>(
              loc, check_active_goals_empty)) {}
};

TEST_F(SolutionDetectorTest, EmptyActiveGoalsMeansSolution) {
    EXPECT_CALL(check_active_goals_empty, empty()).WillOnce(Return(true));
    EXPECT_TRUE(detector.detect());
}

TEST_F(SolutionDetectorTest, NonemptyActiveGoalsIsNotSolution) {
    EXPECT_CALL(check_active_goals_empty, empty()).WillOnce(Return(false));
    EXPECT_FALSE(detector.detect());
}
