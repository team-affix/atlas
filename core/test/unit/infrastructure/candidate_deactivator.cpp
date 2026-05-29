// Candidate deactivator: on deactivate, unlinks goal–rule link, clears translation
// map, and records lineage in deactivated memory.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/candidate_deactivator.hpp"
#include "interfaces/i_unset_candidate_translation_map.hpp"
#include "interfaces/i_deactivated_candidate_memory.hpp"
#include "interfaces/i_unlink_goal_candidate.hpp"

struct MockUnsetCandidateTranslationMap : public i_unset_candidate_translation_map {
    MOCK_METHOD(void, unset, (const resolution_lineage*), (override));
};

struct MockDeactivatedCandidateMemory : public i_deactivated_candidate_memory {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (const, override));
};

struct MockUnlinkGoalCandidate : public i_unlink_goal_candidate {
    MOCK_METHOD(void, unlink_goal_candidate, (const goal_lineage*, rule_id), (override));
};

struct CandidateDeactivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kGoal = 0;

    locator loc;
    MockUnsetCandidateTranslationMap unset_maps;
    MockDeactivatedCandidateMemory memory;
    MockUnlinkGoalCandidate unlink;
    candidate_deactivator deactivator;

    CandidateDeactivatorTest() : deactivator(init_deactivator()) {}

    candidate_deactivator init_deactivator() {
        loc.bind_as<i_unset_candidate_translation_map>(unset_maps);
        loc.bind_as<i_deactivated_candidate_memory>(memory);
        loc.bind_as<i_unlink_goal_candidate>(unlink);
        return candidate_deactivator{loc};
    }

    goal_lineage parent{nullptr, kGoal};
    resolution_lineage rl{&parent, kRule};
};

TEST_F(CandidateDeactivatorTest, DeactivateUnlinksUnsetsAndRecords) {
    bool unlinked = false;
    bool unset = false;
    bool recorded = false;
    EXPECT_CALL(unlink, unlink_goal_candidate(&parent, kRule))
        .WillOnce([&] { unlinked = true; });
    EXPECT_CALL(unset_maps, unset(&rl))
        .WillOnce([&] { unset = true; });
    EXPECT_CALL(memory, insert(&rl))
        .WillOnce([&] { recorded = true; });
    deactivator.deactivate(&rl);
    EXPECT_TRUE(unlinked);
    EXPECT_TRUE(unset);
    EXPECT_TRUE(recorded);
}

TEST_F(CandidateDeactivatorTest, DeactivateUsesResolutionParentAndRuleIndex) {
    static constexpr rule_id kAltRule = 4;
    static constexpr subgoal_id kAltGoal = 7;
    goal_lineage alt_parent{nullptr, kAltGoal};
    resolution_lineage alt_rl{&alt_parent, kAltRule};
    bool unlinked = false;
    bool unset = false;
    bool recorded = false;
    EXPECT_CALL(unlink, unlink_goal_candidate(&alt_parent, kAltRule))
        .WillOnce([&] { unlinked = true; });
    EXPECT_CALL(unset_maps, unset(&alt_rl))
        .WillOnce([&] { unset = true; });
    EXPECT_CALL(memory, insert(&alt_rl))
        .WillOnce([&] { recorded = true; });
    deactivator.deactivate(&alt_rl);
    EXPECT_TRUE(unlinked);
    EXPECT_TRUE(unset);
    EXPECT_TRUE(recorded);
}
