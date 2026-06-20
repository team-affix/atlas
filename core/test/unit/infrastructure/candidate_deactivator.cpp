// candidate_deactivator: on deactivate, unlinks goal-rule link and clears frame offset.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/candidate_deactivator.hpp"

struct MockUnsetCandidateFrameOffset {
    MOCK_METHOD(void, unset, (const resolution_lineage*));
};

struct MockUnlinkGoalCandidate {
    MOCK_METHOD(void, unlink_goal_candidate, (const goal_lineage*, rule_id));
};

using TestCandidateDeactivator = candidate_deactivator<MockUnsetCandidateFrameOffset, MockUnlinkGoalCandidate>;

struct CandidateDeactivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kGoal = 0;

    MockUnsetCandidateFrameOffset unset_frame;
    MockUnlinkGoalCandidate unlink;
    TestCandidateDeactivator deactivator{unset_frame, unlink};

    goal_lineage parent{nullptr, kGoal};
    resolution_lineage rl{&parent, kRule};
};

TEST_F(CandidateDeactivatorTest, DeactivateUnlinksAndUnsetsFrameOffset) {
    bool unlinked = false;
    bool unset = false;
    EXPECT_CALL(unlink, unlink_goal_candidate(&parent, kRule))
        .WillOnce([&] { unlinked = true; });
    EXPECT_CALL(unset_frame, unset(&rl))
        .WillOnce([&] { unset = true; });
    deactivator.deactivate(&rl);
    EXPECT_TRUE(unlinked);
    EXPECT_TRUE(unset);
}

TEST_F(CandidateDeactivatorTest, DeactivateUsesResolutionParentAndRuleIndex) {
    static constexpr rule_id kAltRule = 4;
    static constexpr subgoal_id kAltGoal = 7;
    goal_lineage alt_parent{nullptr, kAltGoal};
    resolution_lineage alt_rl{&alt_parent, kAltRule};

    EXPECT_CALL(unlink, unlink_goal_candidate(&alt_parent, kAltRule)).Times(1);
    EXPECT_CALL(unset_frame, unset(&alt_rl)).Times(1);
    deactivator.deactivate(&alt_rl);
}
