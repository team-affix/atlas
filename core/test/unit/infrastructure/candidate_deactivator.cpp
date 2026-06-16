// candidate_deactivator: on deactivate, unlinks goal-rule link and clears frame offset.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/candidate_deactivator.hpp"
#include "interfaces/i_unset_candidate_frame_offset.hpp"
#include "interfaces/i_unlink_goal_candidate.hpp"

struct MockUnsetCandidateFrameOffset : public i_unset_candidate_frame_offset {
    MOCK_METHOD(void, unset, (const resolution_lineage*), (override));
};

struct MockUnlinkGoalCandidate : public i_unlink_goal_candidate {
    MOCK_METHOD(void, unlink_goal_candidate, (const goal_lineage*, rule_id), (override));
};

struct CandidateDeactivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kGoal = 0;

    locator loc;
    MockUnsetCandidateFrameOffset unset_frame;
    MockUnlinkGoalCandidate unlink;
    candidate_deactivator deactivator;

    CandidateDeactivatorTest() : deactivator(init_deactivator()) {}

    candidate_deactivator init_deactivator() {
        loc.bind_as<i_unset_candidate_frame_offset>(unset_frame);
        loc.bind_as<i_unlink_goal_candidate>(unlink);
        return candidate_deactivator{loc};
    }

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
