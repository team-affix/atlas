// dbuct_goal_candidate_rules: insert/get/link/unlink/erase and frame undo via public get.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(const ra_rule_id_set& rs) {
    std::vector<rule_id> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

} // namespace

struct DbuctGoalCandidateRulesTest : public ::testing::Test {
    static constexpr rule_id kRule0 = 0;
    static constexpr rule_id kRule1 = 1;

    ra_rule_id_set_factory factory;
    dbuct_goal_candidate_rules index{factory};
    goal_lineage gl{nullptr, 0};
};

TEST_F(DbuctGoalCandidateRulesTest, GetOnUnknownGoalThrows) {
    EXPECT_THROW(index.get(&gl), std::out_of_range);
}

TEST_F(DbuctGoalCandidateRulesTest, InsertInitializesEmptySet) {
    index.push_frame();
    index.insert(&gl);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
}

TEST_F(DbuctGoalCandidateRulesTest, LinkAddsRuleToGoal) {
    index.push_frame();
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule0));
}

TEST_F(DbuctGoalCandidateRulesTest, UnlinkRemovesRule) {
    index.push_frame();
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    index.link_goal_candidate(&gl, kRule1);
    index.unlink_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule1));
}

TEST_F(DbuctGoalCandidateRulesTest, EraseRemovesBucket) {
    index.push_frame();
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    index.erase(&gl);
    EXPECT_THROW(index.get(&gl), std::out_of_range);
}

TEST_F(DbuctGoalCandidateRulesTest, PopFrameRestoresLinks) {
    index.push_frame();
    index.insert(&gl);
    index.link_goal_candidate(&gl, kRule0);
    index.push_frame();
    index.link_goal_candidate(&gl, kRule1);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule0, kRule1));
    index.pop_frame();
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule0));
}
