// Candidate deactivator: on deactivate, unlinks goal–rule link, clears translation
// map, and records lineage in deactivated memory — in that dependency order.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/candidate_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_unset_candidate_translation_map.hpp"
#include "../../../core/hpp/interfaces/i_deactivated_candidate_memory.hpp"
#include "../../../core/hpp/interfaces/i_unlink_goal_candidate.hpp"

struct MockUnsetCandidateTranslationMap : public i_unset_candidate_translation_map {
    MOCK_METHOD(void, unset, (const resolution_lineage*), (override));
};

struct MockDeactivatedCandidateMemory : public i_deactivated_candidate_memory {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (const, override));
};

struct MockUnlinkGoalCandidate : public i_unlink_goal_candidate {
    MOCK_METHOD(void, unlink_goal_candidate, (const goal_lineage*, const rule*), (override));
};

struct CandidateDeactivatorTest : public ::testing::Test {
    MockUnsetCandidateTranslationMap unset_maps;
    MockDeactivatedCandidateMemory memory;
    MockUnlinkGoalCandidate unlink;
    candidate_deactivator deactivator{unset_maps, memory, unlink};

    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule idx{&head, {}};
    goal_lineage parent{nullptr, &goal_e};
    resolution_lineage rl{&parent, &idx};
};

TEST_F(CandidateDeactivatorTest, DeactivateUnlinksUnsetsAndRecords) {
    testing::InSequence seq;
    EXPECT_CALL(unlink, unlink_goal_candidate(&parent, &idx)).Times(1);
    EXPECT_CALL(unset_maps, unset(&rl)).Times(1);
    EXPECT_CALL(memory, insert(&rl)).Times(1);
    deactivator.deactivate(&rl);
}
