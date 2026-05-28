// Candidate translation maps: per-resolution-lineage translation_map storage.
// get creates entries on demand; set moves maps in; unset removes without leaking.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/candidate_translation_maps.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

using ::testing::IsEmpty;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

struct CandidateTranslationMapsTest : public ::testing::Test {
    candidate_translation_maps maps;
    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    expr head_other{expr::var{3}};
    rule idx{&head, {}};
    rule other_idx{&head_other, {}};
    static constexpr subgoal_id kGoal = 0;
    static constexpr rule_id kRule = 0;
    static constexpr rule_id kOtherRule = 1;
    goal_lineage parent{nullptr, kGoal};
    resolution_lineage rl{&parent, kRule};
    resolution_lineage other_rl{&parent, kOtherRule};
    static constexpr uint32_t kSourceVar = 1;
    static constexpr uint32_t kTargetVar = 2;
};

TEST_F(CandidateTranslationMapsTest, GetCreatesEmptyMapByDefault) {
    translation_map& tm = maps.get(&rl);
    EXPECT_THAT(tm, IsEmpty());
}

TEST_F(CandidateTranslationMapsTest, SetStoresTranslationMap) {
    translation_map stored{{kSourceVar, kTargetVar}};
    maps.set(&rl, translation_map{stored});
    EXPECT_THAT(
        maps.get(&rl),
        UnorderedElementsAre(Pair(kSourceVar, kTargetVar)));
}

TEST_F(CandidateTranslationMapsTest, UnsetRemovesMap) {
    maps.set(&rl, translation_map{{kSourceVar, kTargetVar}});
    maps.unset(&rl);
    EXPECT_THAT(maps.get(&rl), IsEmpty());
}

TEST_F(CandidateTranslationMapsTest, DistinctLineagesHaveIndependentMaps) {
    maps.set(&rl, translation_map{{kSourceVar, kTargetVar}});
    EXPECT_THAT(maps.get(&other_rl), IsEmpty());
}
