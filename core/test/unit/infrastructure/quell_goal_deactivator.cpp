// quell_goal_deactivator: erases depth and work, then delegates to srt_goal_deactivator.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_goal_deactivator.hpp"

namespace {

struct MockSrtGoalDeactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*));
};

struct MockEraseGoalDepth {
    MOCK_METHOD(void, erase, (const goal_lineage*));
};

struct MockEraseGoalWorkValue {
    MOCK_METHOD(void, erase, (const goal_lineage*));
};

} // namespace

using test_quell_goal_deactivator_t = quell_goal_deactivator<
    MockSrtGoalDeactivator, MockEraseGoalDepth, MockEraseGoalWorkValue>;

struct QuellGoalDeactivatorTest : public ::testing::Test {
    MockSrtGoalDeactivator mock_srt;
    MockEraseGoalDepth erase_goal_depth;
    MockEraseGoalWorkValue erase_goal_work_value;
    goal_lineage gl{nullptr, 0};
    test_quell_goal_deactivator_t deactivator{
        mock_srt, erase_goal_depth, erase_goal_work_value};
};

TEST_F(QuellGoalDeactivatorTest, ErasesDepthAndWorkThenDelegates) {
    testing::InSequence seq;
    EXPECT_CALL(erase_goal_depth, erase(&gl)).Times(1);
    EXPECT_CALL(erase_goal_work_value, erase(&gl)).Times(1);
    EXPECT_CALL(mock_srt, deactivate(&gl)).Times(1);
    deactivator.deactivate(&gl);
}
