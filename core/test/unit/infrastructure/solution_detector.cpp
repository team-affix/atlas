#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/solution_detector.hpp"
#include "../../../core/hpp/interfaces/i_solution_detector.hpp"
#include "../../../core/hpp/interfaces/i_active_goals.hpp"

using ::testing::Return;

struct MockActiveGoals : public i_active_goals {
    MOCK_METHOD(void, insert, (const goal_lineage*), (override));
    MOCK_METHOD(void, erase, (const goal_lineage*), (override));
    MOCK_METHOD(bool, contains, (const goal_lineage*), (const, override));
    MOCK_METHOD(void, accept, (i_visitor<const goal_lineage*>&), (const, override));
    MOCK_METHOD(size_t, size, (), (const, override));
    MOCK_METHOD(bool, empty, (), (const, override));
};

TEST(SolutionDetectorTest, EmptyActiveGoalsMeansSolution) {
    MockActiveGoals ag;
    EXPECT_CALL(ag, empty()).WillOnce(Return(true));

    solution_detector detector{ag};
    i_solution_detector& sut{detector};
    EXPECT_TRUE(sut.detect());
}

TEST(SolutionDetectorTest, NonemptyActiveGoalsIsNotSolution) {
    MockActiveGoals ag;
    EXPECT_CALL(ag, empty()).WillOnce(Return(false));

    solution_detector detector{ag};
    i_solution_detector& sut{detector};
    EXPECT_FALSE(sut.detect());
}
