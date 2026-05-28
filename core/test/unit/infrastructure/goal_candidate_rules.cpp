// Goal–candidate rule id index: link/unlink per goal, erase removes a goal bucket, and get
// returns empty rule id set for unknown goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_candidate_rules.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<rule_id> collect_rule_ids(i_rule_id_set& rs) {
    std::vector<rule_id> out;
    auto sm = rs.iterate();
    while (!sm.done()) {
        if (auto r = sm.resume())
            out.push_back(*r);
    }
    return out;
}

}  // namespace

struct GoalCandidateRulesTest : public ::testing::Test {
    static constexpr rule_id kRule0 = 0;
    static constexpr rule_id kRule1 = 1;

    goal_candidate_rules index;
    expr goal_e{expr::var{0}};
    expr head0{expr::var{1}};
    rule r0{&head0, {}};
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, kRule0};
};

TEST_F(GoalCandidateRulesTest, UnknownGoalReturnsEmptySet) {
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, LinkAddsRuleToGoal) {
    index.link_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule0));
}

TEST_F(GoalCandidateRulesTest, UnlinkRemovesRule) {
    index.link_goal_candidate(&gl, kRule0);
    index.link_goal_candidate(&gl, kRule1);
    index.unlink_goal_candidate(&gl, kRule0);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), UnorderedElementsAre(kRule1));
}

TEST_F(GoalCandidateRulesTest, EraseRemovesGoalBucket) {
    index.link_goal_candidate(&gl, kRule0);
    index.erase(&gl);
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
}

TEST_F(GoalCandidateRulesTest, ConstGetReturnsEmptyForUnknownGoal) {
    const goal_candidate_rules& cref = index;
    EXPECT_EQ(cref.get(&gl).size(), 0u);
}

TEST_F(GoalCandidateRulesTest, ClearGoalCandidateRuleIdsEmptiesAllGoals) {
    goal_lineage gl_other{nullptr, 1};
    index.link_goal_candidate(&gl, kRule0);
    index.link_goal_candidate(&gl_other, kRule1);
    index.clear_goal_candidate_rule_ids();
    EXPECT_THAT(collect_rule_ids(index.get(&gl)), IsEmpty());
    EXPECT_THAT(collect_rule_ids(index.get(&gl_other)), IsEmpty());
}
