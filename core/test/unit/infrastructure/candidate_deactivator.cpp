// Candidate deactivator: on deactivate, unlinks goal–rule link, clears translation
// map, and records lineage in deactivated memory — in that dependency order.

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
    testing::InSequence seq;
    EXPECT_CALL(unlink, unlink_goal_candidate(&parent, kRule)).Times(1);
    EXPECT_CALL(unset_maps, unset(&rl)).Times(1);
    EXPECT_CALL(memory, insert(&rl)).Times(1);
    deactivator.deactivate(&rl);
}
